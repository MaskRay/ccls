/* Copyright 2017-2018 ccls Authors

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#include "message_handler.hh"
#include "pipeline.hh"
#include "query.hh"

#include <llvm/Support/FormatVariadic.h>

#include <rapidjson/document.h>
#include <rapidjson/writer.h>

#include <unordered_set>

namespace ccls {
namespace {
struct CodeAction {
  std::string title;
  const char *kind = "quickfix";
  WorkspaceEdit edit;
};
REFLECT_STRUCT(CodeAction, title, kind, edit);
}
void MessageHandler::textDocument_codeAction(CodeActionParam &param,
                                             ReplyOnce &reply) {
  WorkingFile *wf = FindOrFail(param.textDocument.uri.GetPath(), reply).second;
  if (!wf)
    return;
  std::vector<CodeAction> result;
  std::vector<Diagnostic> diagnostics;
  wfiles->WithLock([&]() { diagnostics = wf->diagnostics; });
  for (Diagnostic &diag : diagnostics)
    if (diag.fixits_.size() &&
        (param.range.Intersects(diag.range) ||
         llvm::any_of(diag.fixits_, [&](const TextEdit &edit) {
           return param.range.Intersects(edit.range);
         }))) {
      CodeAction &cmd = result.emplace_back();
      cmd.title = "FixIt: " + diag.message;
      auto &edit = cmd.edit.documentChanges.emplace_back();
      edit.textDocument.uri = param.textDocument.uri;
      edit.textDocument.version = wf->version;
      edit.edits = diag.fixits_;
    }
  reply(result);
}

namespace {
struct Cmd_xref {
  Usr usr;
  Kind kind;
  std::string field;
};
struct Command {
  std::string title;
  std::string command;
  std::vector<std::string> arguments;
};
struct CodeLens {
  lsRange range;
  std::optional<Command> command;
};
REFLECT_STRUCT(Cmd_xref, usr, kind, field);
REFLECT_STRUCT(Command, title, command, arguments);
REFLECT_STRUCT(CodeLens, range, command);

template <typename T>
std::string ToString(T &v) {
  rapidjson::StringBuffer output;
  rapidjson::Writer<rapidjson::StringBuffer> writer(output);
  JsonWriter json_writer(&writer);
  Reflect(json_writer, v);
  return output.GetString();
}

struct CommonCodeLensParams {
  std::vector<CodeLens> *result;
  DB *db;
  WorkingFile *wfile;
};
} // namespace

void MessageHandler::textDocument_codeLens(TextDocumentParam &param,
                                           ReplyOnce &reply) {
  auto [file, wf] = FindOrFail(param.textDocument.uri.GetPath(), reply);
  if (!wf)
    return;

  std::vector<CodeLens> result;
  auto Add = [&, wf = wf](const char *singular, Cmd_xref show, Range range,
                          int num, bool force_display = false) {
    if (!num && !force_display)
      return;
    std::optional<lsRange> ls_range = GetLsRange(wf, range);
    if (!ls_range)
      return;
    CodeLens &code_lens = result.emplace_back();
    code_lens.range = *ls_range;
    code_lens.command = Command();
    code_lens.command->command = std::string(ccls_xref);
    bool plural = num > 1 && singular[strlen(singular) - 1] != 'd';
    code_lens.command->title =
        llvm::formatv("{0} {1}{2}", num, singular, plural ? "s" : "").str();
    code_lens.command->arguments.push_back(ToString(show));
  };

  std::unordered_set<Range> seen;
  for (auto [sym, refcnt] : file->symbol2refcnt) {
    if (refcnt <= 0 || !sym.extent.Valid() || !seen.insert(sym.range).second)
      continue;
    switch (sym.kind) {
    case Kind::Func: {
      QueryFunc &func = db->GetFunc(sym);
      const QueryFunc::Def *def = func.AnyDef();
      if (!def)
        continue;
      std::vector<Use> base_uses = GetUsesForAllBases(db, func);
      std::vector<Use> derived_uses = GetUsesForAllDerived(db, func);
      Add("ref", {sym.usr, Kind::Func, "uses"}, sym.range, func.uses.size(),
          base_uses.empty());
      if (base_uses.size())
        Add("b.ref", {sym.usr, Kind::Func, "bases uses"}, sym.range,
            base_uses.size());
      if (derived_uses.size())
        Add("d.ref", {sym.usr, Kind::Func, "derived uses"}, sym.range,
            derived_uses.size());
      if (base_uses.empty())
        Add("base", {sym.usr, Kind::Func, "bases"}, sym.range,
            def->bases.size());
      Add("derived", {sym.usr, Kind::Func, "derived"}, sym.range,
          func.derived.size());
      break;
    }
    case Kind::Type: {
      QueryType &type = db->GetType(sym);
      Add("ref", {sym.usr, Kind::Type, "uses"}, sym.range, type.uses.size(),
          true);
      Add("derived", {sym.usr, Kind::Type, "derived"}, sym.range,
          type.derived.size());
      Add("var", {sym.usr, Kind::Type, "instances"}, sym.range,
          type.instances.size());
      break;
    }
    case Kind::Var: {
      QueryVar &var = db->GetVar(sym);
      const QueryVar::Def *def = var.AnyDef();
      if (!def || (def->is_local() && !g_config->codeLens.localVariables))
        continue;
      Add("ref", {sym.usr, Kind::Var, "uses"}, sym.range, var.uses.size(),
          def->kind != SymbolKind::Macro);
      break;
    }
    case Kind::File:
    case Kind::Invalid:
      llvm_unreachable("");
    };
  }

  reply(result);
}

void MessageHandler::workspace_executeCommand(JsonReader &reader,
                                              ReplyOnce &reply) {
  Command param;
  Reflect(reader, param);
  if (param.arguments.empty()) {
    return;
  }
  rapidjson::Document reader1;
  reader1.Parse(param.arguments[0].c_str());
  JsonReader json_reader{&reader1};
  if (param.command == ccls_xref) {
    Cmd_xref cmd;
    Reflect(json_reader, cmd);
    std::vector<Location> result;
    auto Map = [&](auto &&uses) {
      for (auto &use : uses)
        if (auto loc = GetLsLocation(db, wfiles, use))
          result.push_back(std::move(*loc));
    };
    switch (cmd.kind) {
    case Kind::Func: {
      QueryFunc &func = db->Func(cmd.usr);
      if (cmd.field == "bases") {
        if (auto *def = func.AnyDef())
          Map(GetFuncDeclarations(db, def->bases));
      } else if (cmd.field == "bases uses") {
        Map(GetUsesForAllBases(db, func));
      } else if (cmd.field == "derived") {
        Map(GetFuncDeclarations(db, func.derived));
      } else if (cmd.field == "derived uses") {
        Map(GetUsesForAllDerived(db, func));
      } else if (cmd.field == "uses") {
        Map(func.uses);
      }
      break;
    }
    case Kind::Type: {
      QueryType &type = db->Type(cmd.usr);
      if (cmd.field == "derived") {
        Map(GetTypeDeclarations(db, type.derived));
      } else if (cmd.field == "instances") {
        Map(GetVarDeclarations(db, type.instances, 7));
      } else if (cmd.field == "uses") {
        Map(type.uses);
      }
      break;
    }
    case Kind::Var: {
      QueryVar &var = db->Var(cmd.usr);
      if (cmd.field == "uses")
        Map(var.uses);
      break;
    }
    default:
      break;
    }
    reply(result);
  }
}
} // namespace ccls
