// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#include "message_handler.hh"
#include "pipeline.hh"
#include "query_utils.h"
#include "serializers/json.h"

#include <llvm/Support/FormatVariadic.h>

#include <unordered_set>

namespace ccls {
namespace {
struct CodeAction {
  std::string title;
  const char *kind = "quickfix";
  lsWorkspaceEdit edit;
};
MAKE_REFLECT_STRUCT(CodeAction, title, kind, edit);
}
void MessageHandler::textDocument_codeAction(CodeActionParam &param,
                                             ReplyOnce &reply) {
  WorkingFile *wfile =
      working_files->GetFileByFilename(param.textDocument.uri.GetPath());
  if (!wfile) {
    return;
  }
  std::vector<CodeAction> result;
  std::vector<lsDiagnostic> diagnostics;
  working_files->DoAction([&]() { diagnostics = wfile->diagnostics_; });
  for (lsDiagnostic &diag : diagnostics)
    if (diag.fixits_.size()) {
      CodeAction &cmd = result.emplace_back();
      cmd.title = "FixIt: " + diag.message;
      auto &edit = cmd.edit.documentChanges.emplace_back();
      edit.textDocument.uri = param.textDocument.uri;
      edit.textDocument.version = wfile->version;
      edit.edits = diag.fixits_;
    }
  reply(result);
}

namespace {
struct Cmd_xref {
  Usr usr;
  SymbolKind kind;
  std::string field;
};
struct lsCommand {
  std::string title;
  std::string command;
  std::vector<std::string> arguments;
};
struct lsCodeLens {
  lsRange range;
  std::optional<lsCommand> command;
};
MAKE_REFLECT_STRUCT(Cmd_xref, usr, kind, field);
MAKE_REFLECT_STRUCT(lsCommand, title, command, arguments);
MAKE_REFLECT_STRUCT(lsCodeLens, range, command);

template <typename T>
std::string ToString(T &v) {
  rapidjson::StringBuffer output;
  rapidjson::Writer<rapidjson::StringBuffer> writer(output);
  JsonWriter json_writer(&writer);
  Reflect(json_writer, v);
  return output.GetString();
}

struct CommonCodeLensParams {
  std::vector<lsCodeLens> *result;
  DB *db;
  WorkingFile *wfile;
};
} // namespace

void MessageHandler::textDocument_codeLens(TextDocumentParam &param,
                                           ReplyOnce &reply) {
  std::vector<lsCodeLens> result;
  std::string path = param.textDocument.uri.GetPath();

  QueryFile *file = FindFile(reply, path);
  WorkingFile *wfile =
      file ? working_files->GetFileByFilename(file->def->path) : nullptr;
  if (!wfile) {
    return;
  }

  auto Add = [&](const char *singular, Cmd_xref show, Range range, int num,
                 bool force_display = false) {
    if (!num && !force_display)
      return;
    std::optional<lsRange> ls_range = GetLsRange(wfile, range);
    if (!ls_range)
      return;
    lsCodeLens &code_lens = result.emplace_back();
    code_lens.range = *ls_range;
    code_lens.command = lsCommand();
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
    case SymbolKind::Func: {
      QueryFunc &func = db->GetFunc(sym);
      const QueryFunc::Def *def = func.AnyDef();
      if (!def)
        continue;
      std::vector<Use> base_uses = GetUsesForAllBases(db, func);
      std::vector<Use> derived_uses = GetUsesForAllDerived(db, func);
      Add("ref", {sym.usr, SymbolKind::Func, "uses"}, sym.range,
          func.uses.size(), base_uses.empty());
      if (base_uses.size())
        Add("b.ref", {sym.usr, SymbolKind::Func, "bases uses"}, sym.range,
            base_uses.size());
      if (derived_uses.size())
        Add("d.ref", {sym.usr, SymbolKind::Func, "derived uses"}, sym.range,
            derived_uses.size());
      if (base_uses.empty())
        Add("base", {sym.usr, SymbolKind::Func, "bases"}, sym.range,
            def->bases.size());
      Add("derived", {sym.usr, SymbolKind::Func, "derived"}, sym.range,
          func.derived.size());
      break;
    }
    case SymbolKind::Type: {
      QueryType &type = db->GetType(sym);
      Add("ref", {sym.usr, SymbolKind::Type, "uses"}, sym.range,
          type.uses.size(), true);
      Add("derived", {sym.usr, SymbolKind::Type, "derived"}, sym.range,
          type.derived.size());
      Add("var", {sym.usr, SymbolKind::Type, "instances"}, sym.range,
          type.instances.size());
      break;
    }
    case SymbolKind::Var: {
      QueryVar &var = db->GetVar(sym);
      const QueryVar::Def *def = var.AnyDef();
      if (!def || (def->is_local() && !g_config->codeLens.localVariables))
        continue;
      Add("ref", {sym.usr, SymbolKind::Var, "uses"}, sym.range, var.uses.size(),
          def->kind != lsSymbolKind::Macro);
      break;
    }
    case SymbolKind::File:
    case SymbolKind::Invalid:
      llvm_unreachable("");
    };
  }

  reply(result);
}

void MessageHandler::workspace_executeCommand(Reader &reader,
                                              ReplyOnce &reply) {
  lsCommand param;
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
    std::vector<lsLocation> result;
    auto Map = [&](auto &&uses) {
      for (auto &use : uses)
        if (auto loc = GetLsLocation(db, working_files, use))
          result.push_back(std::move(*loc));
    };
    switch (cmd.kind) {
    case SymbolKind::Func: {
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
    case SymbolKind::Type: {
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
    case SymbolKind::Var: {
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
