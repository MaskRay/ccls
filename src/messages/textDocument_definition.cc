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

#include <ctype.h>
#include <limits.h>
#include <stdlib.h>

namespace ccls {
namespace {
std::vector<DeclRef> GetNonDefDeclarationTargets(DB *db, SymbolRef sym) {
  switch (sym.kind) {
  case Kind::Var: {
    std::vector<DeclRef> ret = GetNonDefDeclarations(db, sym);
    // If there is no declaration, jump to its type.
    if (ret.empty()) {
      for (auto &def : db->GetVar(sym).def)
        if (def.type) {
          if (Maybe<DeclRef> use =
                  GetDefinitionSpell(db, SymbolIdx{def.type, Kind::Type})) {
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
} // namespace

void MessageHandler::textDocument_declaration(TextDocumentPositionParam &param,
                                              ReplyOnce &reply) {
  int file_id;
  auto [file, wf] = FindOrFail(param.textDocument.uri.GetPath(), reply, &file_id);
  if (!wf)
    return;

  std::vector<LocationLink> result;
  Position &ls_pos = param.position;
  for (SymbolRef sym : FindSymbolsAtLocation(wf, file, param.position))
    for (DeclRef dr : GetNonDefDeclarations(db, sym))
      if (!(dr.file_id == file_id &&
            dr.range.Contains(ls_pos.line, ls_pos.character)))
        if (auto loc = GetLocationLink(db, wfiles, dr))
          result.push_back(loc);
  reply.ReplyLocationLink(result);
}

void MessageHandler::textDocument_definition(TextDocumentPositionParam &param,
                                             ReplyOnce &reply) {
  int file_id;
  auto [file, wf] = FindOrFail(param.textDocument.uri.GetPath(), reply, &file_id);
  if (!wf)
    return;

  std::vector<LocationLink> result;
  Maybe<DeclRef> on_def;
  Position &ls_pos = param.position;

  for (SymbolRef sym : FindSymbolsAtLocation(wf, file, ls_pos, true)) {
    // Special cases which are handled:
    //  - symbol has declaration but no definition (ie, pure virtual)
    //  - goto declaration while in definition of recursive type
    std::vector<DeclRef> drs;
    EachEntityDef(db, sym, [&](const auto &def) {
      if (def.spell) {
        DeclRef spell = *def.spell;
        if (spell.file_id == file_id &&
            spell.range.Contains(ls_pos.line, ls_pos.character)) {
          on_def = spell;
          drs.clear();
          return false;
        }
        drs.push_back(spell);
      }
      return true;
    });

    // |uses| is empty if on a declaration/definition, otherwise it includes
    // all declarations/definitions.
    if (drs.empty()) {
      for (DeclRef dr : GetNonDefDeclarationTargets(db, sym))
        if (!(dr.file_id == file_id &&
              dr.range.Contains(ls_pos.line, ls_pos.character)))
          drs.push_back(dr);
      // There is no declaration but the cursor is on a definition.
      if (drs.empty() && on_def)
        drs.push_back(*on_def);
    }
    for (DeclRef dr : drs)
      if (auto loc = GetLocationLink(db, wfiles, dr))
        result.push_back(loc);
  }

  if (result.empty()) {
    Maybe<Range> range;
    // Check #include
    for (const IndexInclude &include : file->def->includes) {
      if (include.line == ls_pos.line) {
        result.push_back(
            {DocumentUri::FromPath(include.resolved_path).raw_uri});
        range = {{0, 0}, {0, 0}};
        break;
      }
    }
    // Find the best match of the identifier at point.
    if (!range) {
      Position position = param.position;
      const std::string &buffer = wf->buffer_content;
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
      best_sym.kind = Kind::Invalid;
      auto fn = [&](SymbolIdx sym) {
        std::string_view short_name = db->GetSymbolName(sym, false),
                         name = short_query.size() < query.size()
                                    ? db->GetSymbolName(sym, true)
                                    : short_name;
        if (short_name != short_query)
          return;
        if (Maybe<DeclRef> dr = GetDefinitionSpell(db, sym)) {
          std::tuple<int, int, bool, int> score{
              int(name.size() - short_query.size()), 0, dr->file_id != file_id,
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
        fn({func.usr, Kind::Func});
      for (auto &type : db->types)
        fn({type.usr, Kind::Type});
      for (auto &var : db->vars)
        if (var.def.size() && !var.def[0].is_local())
          fn({var.usr, Kind::Var});

      if (best_sym.kind != Kind::Invalid) {
        Maybe<DeclRef> dr = GetDefinitionSpell(db, best_sym);
        assert(dr);
        if (auto loc = GetLocationLink(db, wfiles, *dr))
          result.push_back(loc);
      }
    }
  }

  reply.ReplyLocationLink(result);
}

void MessageHandler::textDocument_typeDefinition(
    TextDocumentPositionParam &param, ReplyOnce &reply) {
  auto [file, wf] = FindOrFail(param.textDocument.uri.GetPath(), reply);
  if (!file)
    return;

  std::vector<LocationLink> result;
  auto Add = [&](const QueryType &type) {
    for (const auto &def : type.def)
      if (def.spell)
        if (auto loc = GetLocationLink(db, wfiles, *def.spell))
          result.push_back(loc);
    if (result.empty())
      for (const DeclRef &dr : type.declarations)
        if (auto loc = GetLocationLink(db, wfiles, dr))
          result.push_back(loc);
  };
  for (SymbolRef sym : FindSymbolsAtLocation(wf, file, param.position)) {
    switch (sym.kind) {
    case Kind::Var: {
      const QueryVar::Def *def = db->GetVar(sym).AnyDef();
      if (def && def->type)
        Add(db->Type(def->type));
      break;
    }
    case Kind::Type: {
      for (auto &def : db->GetType(sym).def)
        if (def.alias_of) {
          Add(db->Type(def.alias_of));
          break;
        }
      break;
    }
    default:
      break;
    }
  }

  reply.ReplyLocationLink(result);
}
} // namespace ccls
