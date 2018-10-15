// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

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

struct In_TextDocumentCodeLens : public RequestMessage {
  MethodType GetMethodType() const override { return codeLens; }
  struct Params {
    lsTextDocumentIdentifier textDocument;
  } params;
};
MAKE_REFLECT_STRUCT(In_TextDocumentCodeLens::Params, textDocument);
MAKE_REFLECT_STRUCT(In_TextDocumentCodeLens, id, params);
REGISTER_IN_MESSAGE(In_TextDocumentCodeLens);

struct Handler_TextDocumentCodeLens
    : BaseMessageHandler<In_TextDocumentCodeLens> {
  MethodType GetMethodType() const override { return codeLens; }
  void Run(In_TextDocumentCodeLens *request) override {
    auto &params = request->params;
    std::vector<lsCodeLens> result;
    std::string path = params.textDocument.uri.GetPath();

    QueryFile *file;
    if (!FindFileOrFail(db, project, request->id, path, &file))
      return;
    WorkingFile *wfile = working_files->GetFileByFilename(file->def->path);

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
        Add("ref", {sym.usr, SymbolKind::Type, "uses"}, sym.range, type.uses.size(),
            true);
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

    pipeline::Reply(request->id, result);
  }
};
REGISTER_MESSAGE_HANDLER(Handler_TextDocumentCodeLens);

struct In_WorkspaceExecuteCommand : public RequestMessage {
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
      pipeline::Reply(request->id, result);
    }
  }
};
REGISTER_MESSAGE_HANDLER(Handler_WorkspaceExecuteCommand);
} // namespace
