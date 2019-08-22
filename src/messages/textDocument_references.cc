// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#include "message_handler.hh"
#include "query.hh"

#include <llvm/ADT/iterator_range.h>

#include <unordered_set>

using namespace llvm;

namespace ccls {
namespace {
struct ReferenceParam : public TextDocumentPositionParam {
  struct Context {
    // Include the declaration of the current symbol.
    bool includeDeclaration = false;
  } context;

  // ccls extension
  // If not empty, restrict to specified folders.
  std::vector<std::string> folders;
  // For Type, also return references of base types.
  bool base = true;
  // Exclude references with any |Role| bits set.
  Role excludeRole = Role::None;
  // Include references with all |Role| bits set.
  Role role = Role::None;
};
REFLECT_STRUCT(ReferenceParam::Context, includeDeclaration);
REFLECT_STRUCT(ReferenceParam, textDocument, position, context, folders, base,
               excludeRole, role);
} // namespace

void MessageHandler::textDocument_references(JsonReader &reader,
                                             ReplyOnce &reply) {
  ReferenceParam param;
  reflect(reader, param);
  auto [file, wf] = findOrFail(param.textDocument.uri.getPath(), reply);
  if (!wf)
    return;

  for (auto &folder : param.folders)
    ensureEndsInSlash(folder);
  std::vector<uint8_t> file_set = db->getFileSet(param.folders);
  std::vector<Location> result;

  std::unordered_set<Use> seen_uses;
  int line = param.position.line;

  for (SymbolRef sym : findSymbolsAtLocation(wf, file, param.position)) {
    // Found symbol. Return references.
    std::unordered_set<Usr> seen;
    seen.insert(sym.usr);
    std::vector<Usr> stack{sym.usr};
    if (sym.kind != Kind::Func)
      param.base = false;
    while (stack.size()) {
      sym.usr = stack.back();
      stack.pop_back();
      auto fn = [&](Use use, SymbolKind parent_kind) {
        if (file_set[use.file_id] &&
            Role(use.role & param.role) == param.role &&
            !(use.role & param.excludeRole) && seen_uses.insert(use).second)
          if (auto loc = getLsLocation(db, wfiles, use))
            result.push_back(*loc);
      };
      withEntity(db, sym, [&](const auto &entity) {
        SymbolKind parent_kind = SymbolKind::Unknown;
        for (auto &def : entity.def)
          if (def.spell) {
            parent_kind = getSymbolKind(db, sym);
            if (param.base)
              for (Usr usr : make_range(def.bases_begin(), def.bases_end()))
                if (!seen.count(usr)) {
                  seen.insert(usr);
                  stack.push_back(usr);
                }
            break;
          }
        for (Use use : entity.uses)
          fn(use, parent_kind);
        if (param.context.includeDeclaration) {
          for (auto &def : entity.def)
            if (def.spell)
              fn(*def.spell, parent_kind);
          for (Use use : entity.declarations)
            fn(use, parent_kind);
        }
      });
    }
    break;
  }

  if (result.empty()) {
    // |path| is the #include line. If the cursor is not on such line but line
    // = 0,
    // use the current filename.
    std::string path;
    if (line == 0 || line >= (int)wf->buffer_lines.size() - 1)
      path = file->def->path;
    for (const IndexInclude &include : file->def->includes)
      if (include.line == param.position.line) {
        path = include.resolved_path;
        break;
      }
    if (path.size())
      for (QueryFile &file1 : db->files)
        if (file1.def)
          for (const IndexInclude &include : file1.def->includes)
            if (include.resolved_path == path) {
              // Another file |file1| has the same include line.
              Location &loc = result.emplace_back();
              loc.uri = DocumentUri::fromPath(file1.def->path);
              loc.range.start.line = loc.range.end.line = include.line;
              break;
            }
  }

  if ((int)result.size() >= g_config->xref.maxNum)
    result.resize(g_config->xref.maxNum);
  reply(result);
}
} // namespace ccls
