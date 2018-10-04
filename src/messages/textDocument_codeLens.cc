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

#include "message_handler.h"
#include "pipeline.hh"
#include "query_utils.h"
#include "serializers/json.h"

#include <llvm/Support/FormatVariadic.h>

#include <unordered_set>

using namespace ccls;

namespace {
const MethodType codeLens = "textDocument/codeLens",
                 executeCommand = "workspace/executeCommand";

struct lsCommand {
  std::string title;
  std::string command;
  std::vector<std::string> arguments;
};
MAKE_REFLECT_STRUCT(lsCommand, title, command, arguments);

struct lsCodeLens {
  lsRange range;
  std::optional<lsCommand> command;
};
MAKE_REFLECT_STRUCT(lsCodeLens, range, command);

struct Cmd_xref {
  Usr usr;
  SymbolKind kind;
  std::string field;
};
MAKE_REFLECT_STRUCT(Cmd_xref, usr, kind, field);

struct Out_xref : public lsOutMessage<Out_xref> {
  lsRequestId id;
  std::vector<lsLocation> result;
};
MAKE_REFLECT_STRUCT(Out_xref, jsonrpc, id, result);

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

struct In_TextDocumentCodeLens : public RequestInMessage {
  MethodType GetMethodType() const override { return codeLens; }
  struct Params {
    lsTextDocumentIdentifier textDocument;
  } params;
};
MAKE_REFLECT_STRUCT(In_TextDocumentCodeLens::Params, textDocument);
MAKE_REFLECT_STRUCT(In_TextDocumentCodeLens, id, params);
REGISTER_IN_MESSAGE(In_TextDocumentCodeLens);

struct Out_TextDocumentCodeLens
    : public lsOutMessage<Out_TextDocumentCodeLens> {
  lsRequestId id;
  std::vector<lsCodeLens> result;
};
MAKE_REFLECT_STRUCT(Out_TextDocumentCodeLens, jsonrpc, id, result);

struct Handler_TextDocumentCodeLens
    : BaseMessageHandler<In_TextDocumentCodeLens> {
  MethodType GetMethodType() const override { return codeLens; }
  void Run(In_TextDocumentCodeLens *request) override {
    auto &params = request->params;
    Out_TextDocumentCodeLens out;
    out.id = request->id;
    std::string path = params.textDocument.uri.GetPath();

    QueryFile *file;
    if (!FindFileOrFail(db, project, request->id, path, &file))
      return;
    WorkingFile *wfile = working_files->GetFileByFilename(file->def->path);

    auto Add = [&](const char *singular, Cmd_xref show, Use use, int num,
                   bool force_display = false) {
      if (!num && !force_display)
        return;
      std::optional<lsRange> range = GetLsRange(wfile, use.range);
      if (!range)
        return;
      lsCodeLens &code_lens = out.result.emplace_back();
      code_lens.range = *range;
      code_lens.command = lsCommand();
      code_lens.command->command = std::string(ccls_xref);
      bool plural = num > 1 && singular[strlen(singular) - 1] != 'd';
      code_lens.command->title =
          llvm::formatv("{0} {1}{2}", num, singular, plural ? "s" : "").str();
      code_lens.command->arguments.push_back(ToString(show));
    };

    auto ToSpell = [&](SymbolRef sym, int file_id) -> Use {
      Maybe<Use> def = GetDefinitionSpell(db, sym);
      if (def && def->file_id == file_id &&
          def->range.start.line == sym.range.start.line)
        return *def;
      return {{sym.range, sym.role}, file_id};
    };

    std::unordered_set<Range> seen;
    for (auto [sym, refcnt] : file->outline2refcnt) {
      if (refcnt <= 0 || !seen.insert(sym.range).second)
        continue;
      Use use = ToSpell(sym, file->id);
      switch (sym.kind) {
      case SymbolKind::Func: {
        QueryFunc &func = db->GetFunc(sym);
        const QueryFunc::Def *def = func.AnyDef();
        if (!def)
          continue;
        std::vector<Use> base_uses = GetUsesForAllBases(db, func);
        std::vector<Use> derived_uses = GetUsesForAllDerived(db, func);
        Add("ref", {sym.usr, SymbolKind::Func, "uses"}, use, func.uses.size(),
            base_uses.empty());
        if (base_uses.size())
          Add("b.ref", {sym.usr, SymbolKind::Func, "bases uses"}, use,
              base_uses.size());
        if (derived_uses.size())
          Add("d.ref", {sym.usr, SymbolKind::Func, "derived uses"}, use,
              derived_uses.size());
        if (base_uses.empty())
          Add("base", {sym.usr, SymbolKind::Func, "bases"}, use,
              def->bases.size());
        Add("derived", {sym.usr, SymbolKind::Func, "derived"}, use,
            func.derived.size());
        break;
      }
      case SymbolKind::Type: {
        QueryType &type = db->GetType(sym);
        Add("ref", {sym.usr, SymbolKind::Type, "uses"}, use, type.uses.size(),
            true);
        Add("derived", {sym.usr, SymbolKind::Type, "derived"}, use,
            type.derived.size());
        Add("var", {sym.usr, SymbolKind::Type, "instances"}, use,
            type.instances.size());
        break;
      }
      case SymbolKind::Var: {
        QueryVar &var = db->GetVar(sym);
        const QueryVar::Def *def = var.AnyDef();
        if (!def || (def->is_local() && !g_config->codeLens.localVariables))
          continue;
        Add("ref", {sym.usr, SymbolKind::Var, "uses"}, use, var.uses.size(),
            def->kind != lsSymbolKind::Macro);
        break;
      }
      case SymbolKind::File:
      case SymbolKind::Invalid:
        llvm_unreachable("");
      };
    }

    pipeline::WriteStdout(codeLens, out);
  }
};
REGISTER_MESSAGE_HANDLER(Handler_TextDocumentCodeLens);

struct In_WorkspaceExecuteCommand : public RequestInMessage {
  MethodType GetMethodType() const override { return executeCommand; }
  lsCommand params;
};
MAKE_REFLECT_STRUCT(In_WorkspaceExecuteCommand, id, params);
REGISTER_IN_MESSAGE(In_WorkspaceExecuteCommand);

struct Handler_WorkspaceExecuteCommand
    : BaseMessageHandler<In_WorkspaceExecuteCommand> {
  MethodType GetMethodType() const override { return executeCommand; }
  void Run(In_WorkspaceExecuteCommand *request) override {
    const auto &params = request->params;
    if (params.arguments.empty())
      return;
    rapidjson::Document reader;
    reader.Parse(params.arguments[0].c_str());
    JsonReader json_reader{&reader};
    if (params.command == ccls_xref) {
      Cmd_xref cmd;
      Reflect(json_reader, cmd);
      Out_xref out;
      out.id = request->id;
      auto Map = [&](auto &&uses) {
        for (auto &use : uses)
          if (auto loc = GetLsLocation(db, working_files, use))
            out.result.push_back(std::move(*loc));
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
      pipeline::WriteStdout(executeCommand, out);
    }
  }
};
REGISTER_MESSAGE_HANDLER(Handler_WorkspaceExecuteCommand);
} // namespace
