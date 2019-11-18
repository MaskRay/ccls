// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#include "message_handler.hh"
#include "pipeline.hh"
#include "query.hh"

#include <llvm/Support/FormatVariadic.h>

#include <rapidjson/document.h>
#include <rapidjson/writer.h>

#include <unordered_set>

namespace ccls {
namespace {
struct Command {
  std::string title;
  std::string command;
  std::vector<std::string> arguments;
};
struct CodeAction {
  std::string title;
  std::string kind;
  WorkspaceEdit edit;
  Command command;
};
struct ReferenceCommand {
  TextDocumentIdentifier textDocument;
  Position position;
  bool callee;
  std::string direction;
  bool derived;
  int kind;
};
REFLECT_STRUCT(Command, title, command, arguments);
REFLECT_STRUCT(CodeAction, title, kind, edit, command);
REFLECT_STRUCT(ReferenceCommand, textDocument, position,
               callee, direction, derived, kind);

template <typename T> std::string toString(T &v) {
  rapidjson::StringBuffer output;
  rapidjson::Writer<rapidjson::StringBuffer> writer(output);
  JsonWriter json_writer(&writer);
  reflect(json_writer, v);
  return output.GetString();
}
} // namespace

template <typename T> bool vec_has(const std::vector<T> &vec, const T &key) {
  return std::find(std::begin(vec), std::end(vec), key) != std::end(vec);
}

bool should_send_action(std::vector<std::string> available_kinds,
                        std::vector<std::string> requested_kinds,
                        std::string kind) {
  if (!requested_kinds.empty() && !vec_has(requested_kinds, kind)) {
    return false;
  }
  if (!available_kinds.empty() && !vec_has(available_kinds, kind)) {
    return false;
  }
  return true;
}

void MessageHandler::textDocument_codeAction(CodeActionParam &param,
                                             ReplyOnce &reply) {
  WorkingFile *wf = findOrFail(param.textDocument.uri.getPath(), reply).second;
  if (!wf)
    return;
  auto available_kinds = g_config->client.codeActionKind;
  std::vector<std::string> requested_kinds = param.context.only;
  std::vector<CodeAction> result;

  if (should_send_action(available_kinds, requested_kinds, "quickfix")) {
    std::vector<Diagnostic> diagnostics;
    wfiles->withLock([&]() { diagnostics = wf->diagnostics; });
    for (Diagnostic &diag : diagnostics)
      if (diag.fixits_.size() &&
          (param.range.intersects(diag.range) ||
           llvm::any_of(diag.fixits_, [&](const TextEdit &edit) {
             return param.range.intersects(edit.range);
           }))) {
        CodeAction &cmd = result.emplace_back();
        cmd.title = "FixIt: " + diag.message;
        cmd.kind = "quickfix";
        auto &edit = cmd.edit.documentChanges.emplace_back();
        edit.textDocument.uri = param.textDocument.uri;
        edit.textDocument.version = wf->version;
        edit.edits = diag.fixits_;
      }
  }

  if (should_send_action(available_kinds, requested_kinds, "reference")) {
    auto add = [&, param = param] (
          const char *title, const char *command_name,
          const bool callee=false, const char *dir="",
          const bool derived=false, int kind=0) {
      CodeAction &cmd = result.emplace_back();
      ReferenceCommand rcmd;
      rcmd.textDocument = param.textDocument;
      rcmd.position = param.range.start;
      rcmd.callee = callee;
      rcmd.direction = dir;
      rcmd.derived = derived;
      rcmd.kind = kind;
      cmd.title = title;
      cmd.kind = "reference";
      cmd.command.title = title;
      cmd.command.command = command_name;
      cmd.command.arguments.push_back(toString(rcmd));
    };

    add("call",   "$ccls/call");
    add("callee", "$ccls/call", true);
    add("navigate-up",    "$ccls/navigate", false, "U");
    add("navigate-down",  "$ccls/navigate", false, "D");
    add("navigate-right", "$ccls/navigate", false, "R");
    add("navigate-left",  "$ccls/navigate", false, "L");
    add("inheritance",         "$ccls/inheritance");
    add("inheritance-derived", "$ccls/inheritance", false, "", true);
    add("member-var",  "$ccls/member", false, "", false, 4);
    add("member-fun",  "$ccls/member", false, "", false, 3);
    add("member-type", "$ccls/member", false, "", false, 2);
    add("vars", "$ccls/vars");
  }

  reply(result);
}

namespace {
struct Cmd_xref {
  Usr usr;
  Kind kind;
  std::string field;
};
struct CodeLens {
  lsRange range;
  std::optional<Command> command;
};
REFLECT_STRUCT(Cmd_xref, usr, kind, field);
REFLECT_STRUCT(CodeLens, range, command);

struct CommonCodeLensParams {
  std::vector<CodeLens> *result;
  DB *db;
  WorkingFile *wfile;
};
} // namespace

void MessageHandler::textDocument_codeLens(TextDocumentParam &param,
                                           ReplyOnce &reply) {
  auto [file, wf] = findOrFail(param.textDocument.uri.getPath(), reply);
  if (!wf)
    return;

  std::vector<CodeLens> result;
  auto add = [&, wf = wf](const char *singular, Cmd_xref show, Range range,
                          int num, bool force_display = false) {
    if (!num && !force_display)
      return;
    std::optional<lsRange> ls_range = getLsRange(wf, range);
    if (!ls_range)
      return;
    CodeLens &code_lens = result.emplace_back();
    code_lens.range = *ls_range;
    code_lens.command = Command();
    code_lens.command->command = std::string(ccls_xref);
    bool plural = num > 1 && singular[strlen(singular) - 1] != 'd';
    code_lens.command->title =
        llvm::formatv("{0} {1}{2}", num, singular, plural ? "s" : "").str();
    code_lens.command->arguments.push_back(toString(show));
  };

  std::unordered_set<Range> seen;
  for (auto [sym, refcnt] : file->symbol2refcnt) {
    if (refcnt <= 0 || !sym.extent.valid() || !seen.insert(sym.range).second)
      continue;
    switch (sym.kind) {
    case Kind::Func: {
      QueryFunc &func = db->getFunc(sym);
      const QueryFunc::Def *def = func.anyDef();
      if (!def)
        continue;
      std::vector<Use> base_uses = getUsesForAllBases(db, func);
      std::vector<Use> derived_uses = getUsesForAllDerived(db, func);
      add("ref", {sym.usr, Kind::Func, "uses"}, sym.range, func.uses.size(),
          base_uses.empty());
      if (base_uses.size())
        add("b.ref", {sym.usr, Kind::Func, "bases uses"}, sym.range,
            base_uses.size());
      if (derived_uses.size())
        add("d.ref", {sym.usr, Kind::Func, "derived uses"}, sym.range,
            derived_uses.size());
      if (base_uses.empty())
        add("base", {sym.usr, Kind::Func, "bases"}, sym.range,
            def->bases.size());
      add("derived", {sym.usr, Kind::Func, "derived"}, sym.range,
          func.derived.size());
      break;
    }
    case Kind::Type: {
      QueryType &type = db->getType(sym);
      add("ref", {sym.usr, Kind::Type, "uses"}, sym.range, type.uses.size(),
          true);
      add("derived", {sym.usr, Kind::Type, "derived"}, sym.range,
          type.derived.size());
      add("var", {sym.usr, Kind::Type, "instances"}, sym.range,
          type.instances.size());
      break;
    }
    case Kind::Var: {
      QueryVar &var = db->getVar(sym);
      const QueryVar::Def *def = var.anyDef();
      if (!def || (def->is_local() && !g_config->codeLens.localVariables))
        continue;
      add("ref", {sym.usr, Kind::Var, "uses"}, sym.range, var.uses.size(),
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
  reflect(reader, param);
  if (param.arguments.empty()) {
    return;
  }
  rapidjson::Document reader1;
  reader1.Parse(param.arguments[0].c_str());
  JsonReader json_reader{&reader1};
  if (param.command == ccls_xref) {
    Cmd_xref cmd;
    reflect(json_reader, cmd);
    std::vector<Location> result;
    auto map = [&](auto &&uses) {
      for (auto &use : uses)
        if (auto loc = getLsLocation(db, wfiles, use))
          result.push_back(std::move(*loc));
    };
    switch (cmd.kind) {
    case Kind::Func: {
      QueryFunc &func = db->getFunc(cmd.usr);
      if (cmd.field == "bases") {
        if (auto *def = func.anyDef())
          map(getFuncDeclarations(db, def->bases));
      } else if (cmd.field == "bases uses") {
        map(getUsesForAllBases(db, func));
      } else if (cmd.field == "derived") {
        map(getFuncDeclarations(db, func.derived));
      } else if (cmd.field == "derived uses") {
        map(getUsesForAllDerived(db, func));
      } else if (cmd.field == "uses") {
        map(func.uses);
      }
      break;
    }
    case Kind::Type: {
      QueryType &type = db->getType(cmd.usr);
      if (cmd.field == "derived") {
        map(getTypeDeclarations(db, type.derived));
      } else if (cmd.field == "instances") {
        map(getVarDeclarations(db, type.instances, 7));
      } else if (cmd.field == "uses") {
        map(type.uses);
      }
      break;
    }
    case Kind::Var: {
      QueryVar &var = db->getVar(cmd.usr);
      if (cmd.field == "uses")
        map(var.uses);
      break;
    }
    default:
      break;
    }
    reply(result);
  } else if (param.command == "$ccls/call") {
    ccls_call(json_reader, reply);
  } else if (param.command == "$ccls/navigate") {
    ccls_navigate(json_reader, reply);
  } else if (param.command == "$ccls/inheritance") {
    ccls_inheritance(json_reader, reply);
  } else if (param.command == "$ccls/member") {
    ccls_member(json_reader, reply);
  } else if (param.command == "$ccls/vars") {
    ccls_vars(json_reader, reply);
  }
}
} // namespace ccls
