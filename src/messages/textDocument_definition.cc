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

#include <ctype.h>
#include <limits.h>
#include <stdlib.h>

using namespace ccls;

namespace {
MethodType kMethodType = "textDocument/definition";

struct In_TextDocumentDefinition : public RequestInMessage {
  MethodType GetMethodType() const override { return kMethodType; }
  lsTextDocumentPositionParams params;
};
MAKE_REFLECT_STRUCT(In_TextDocumentDefinition, id, params);
REGISTER_IN_MESSAGE(In_TextDocumentDefinition);

std::vector<Use> GetNonDefDeclarationTargets(DB *db, SymbolRef sym) {
  switch (sym.kind) {
  case SymbolKind::Var: {
    std::vector<Use> ret = GetNonDefDeclarations(db, sym);
    // If there is no declaration, jump to its type.
    if (ret.empty()) {
      for (auto &def : db->GetVar(sym).def)
        if (def.type) {
          if (Maybe<DeclRef> use = GetDefinitionSpell(
                  db, SymbolIdx{def.type, SymbolKind::Type})) {
            ret.push_back(*use);
            break;
          }
        }
    }
    return ret;
  }
  default:
    return GetNonDefDeclarations(db, sym);
  }
}

struct Handler_TextDocumentDefinition
    : BaseMessageHandler<In_TextDocumentDefinition> {
  MethodType GetMethodType() const override { return kMethodType; }
  void Run(In_TextDocumentDefinition *request) override {
    auto &params = request->params;
    int file_id;
    QueryFile *file;
    if (!FindFileOrFail(db, project, request->id,
                        params.textDocument.uri.GetPath(), &file, &file_id))
      return;

    Out_LocationList out;
    out.id = request->id;

    Maybe<Use> on_def;
    WorkingFile *wfile = working_files->GetFileByFilename(file->def->path);
    lsPosition &ls_pos = params.position;

    for (SymbolRef sym : FindSymbolsAtLocation(wfile, file, ls_pos, true)) {
      // Special cases which are handled:
      //  - symbol has declaration but no definition (ie, pure virtual)
      //  - goto declaration while in definition of recursive type
      std::vector<Use> uses;
      EachEntityDef(db, sym, [&](const auto &def) {
        if (def.spell) {
          Use spell = *def.spell;
          if (spell.file_id == file_id &&
              spell.range.Contains(ls_pos.line, ls_pos.character)) {
            on_def = spell;
            uses.clear();
            return false;
          }
          uses.push_back(spell);
        }
        return true;
      });

      // |uses| is empty if on a declaration/definition, otherwise it includes
      // all declarations/definitions.
      if (uses.empty()) {
        for (Use use : GetNonDefDeclarationTargets(db, sym))
          if (!(use.file_id == file_id &&
                use.range.Contains(ls_pos.line, ls_pos.character)))
            uses.push_back(use);
        // There is no declaration but the cursor is on a definition.
        if (uses.empty() && on_def)
          uses.push_back(*on_def);
      }
      auto locs = GetLsLocations(db, working_files, uses);
      out.result.insert(out.result.end(), locs.begin(), locs.end());
    }

    if (out.result.size()) {
      std::sort(out.result.begin(), out.result.end());
      out.result.erase(std::unique(out.result.begin(), out.result.end()),
                       out.result.end());
    } else {
      Maybe<Range> range;
      // Check #include
      for (const IndexInclude &include : file->def->includes) {
        if (include.line == ls_pos.line) {
          out.result.push_back(
              lsLocation{lsDocumentUri::FromPath(include.resolved_path)});
          range = {{0, 0}, {0, 0}};
          break;
        }
      }
      // Find the best match of the identifier at point.
      if (!range) {
        lsPosition position = request->params.position;
        const std::string &buffer = wfile->buffer_content;
        std::string_view query = LexIdentifierAroundPos(position, buffer);
        std::string_view short_query = query;
        {
          auto pos = query.rfind(':');
          if (pos != std::string::npos)
            short_query = query.substr(pos + 1);
        }

        // For symbols whose short/detailed names contain |query| as a
        // substring, we use the tuple <length difference, negative position,
        // not in the same file, line distance> to find the best match.
        std::tuple<int, int, bool, int> best_score{INT_MAX, 0, true, 0};
        SymbolIdx best_sym;
        best_sym.kind = SymbolKind::Invalid;
        auto fn = [&](SymbolIdx sym) {
          std::string_view short_name = db->GetSymbolName(sym, false),
                           name = short_query.size() < query.size()
                                      ? db->GetSymbolName(sym, true)
                                      : short_name;
          if (short_name != short_query)
            return;
          if (Maybe<DeclRef> dr = GetDefinitionSpell(db, sym)) {
            std::tuple<int, int, bool, int> score{
                int(name.size() - short_query.size()), 0,
                dr->file_id != file_id,
                std::abs(dr->range.start.line - position.line)};
            // Update the score with qualified name if the qualified name
            // occurs in |name|.
            auto pos = name.rfind(query);
            if (pos != std::string::npos) {
              std::get<0>(score) = int(name.size() - query.size());
              std::get<1>(score) = -int(pos);
            }
            if (score < best_score) {
              best_score = score;
              best_sym = sym;
            }
          }
        };
        for (auto &func : db->funcs)
          fn({func.usr, SymbolKind::Func});
        for (auto &type : db->types)
          fn({type.usr, SymbolKind::Type});
        for (auto &var : db->vars)
          if (var.def.size() && !var.def[0].is_local())
            fn({var.usr, SymbolKind::Var});

        if (best_sym.kind != SymbolKind::Invalid) {
          Maybe<DeclRef> dr = GetDefinitionSpell(db, best_sym);
          assert(dr);
          if (auto loc = GetLsLocation(db, working_files, *dr))
            out.result.push_back(*loc);
        }
      }
    }

    pipeline::WriteStdout(kMethodType, out);
  }
};
REGISTER_MESSAGE_HANDLER(Handler_TextDocumentDefinition);
} // namespace
