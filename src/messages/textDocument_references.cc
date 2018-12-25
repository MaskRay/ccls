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
#include "query.hh"

#include <unordered_set>

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
  Reflect(reader, param);
  auto [file, wf] = FindOrFail(param.textDocument.uri.GetPath(), reply);
  if (!wf)
    return;

  for (auto &folder : param.folders)
    EnsureEndsInSlash(folder);
  std::vector<uint8_t> file_set = db->GetFileSet(param.folders);
  std::vector<Location> result;

  std::unordered_set<Use> seen_uses;
  int line = param.position.line;

  for (SymbolRef sym : FindSymbolsAtLocation(wf, file, param.position)) {
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
          if (auto loc = GetLsLocation(db, wfiles, use))
            result.push_back(*loc);
      };
      WithEntity(db, sym, [&](const auto &entity) {
        SymbolKind parent_kind = SymbolKind::Unknown;
        for (auto &def : entity.def)
          if (def.spell) {
            parent_kind = GetSymbolKind(db, sym);
            if (param.base)
              for (Usr usr : def.GetBases())
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
              loc.uri = DocumentUri::FromPath(file1.def->path);
              loc.range.start.line = loc.range.end.line = include.line;
              break;
            }
  }

  if ((int)result.size() >= g_config->xref.maxNum)
    result.resize(g_config->xref.maxNum);
  reply(result);
}
} // namespace ccls
