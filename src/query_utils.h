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

#pragma once

#include "query.h"
#include "working_files.h"

#include <optional>

Maybe<Use> GetDefinitionSpell(DB *db, SymbolIdx sym);
Maybe<Use> GetDefinitionExtent(DB *db, SymbolIdx sym);

// Get defining declaration (if exists) or an arbitrary declaration (otherwise)
// for each id.
std::vector<Use> GetFuncDeclarations(DB *, const std::vector<Usr> &);
std::vector<Use> GetTypeDeclarations(DB *, const std::vector<Usr> &);
std::vector<Use> GetVarDeclarations(DB *, const std::vector<Usr> &, unsigned);

// Get non-defining declarations.
std::vector<Use> GetNonDefDeclarations(DB *db, SymbolIdx sym);

std::vector<Use> GetUsesForAllBases(DB *db, QueryFunc &root);
std::vector<Use> GetUsesForAllDerived(DB *db, QueryFunc &root);
std::optional<lsRange> GetLsRange(WorkingFile *working_file,
                                  const Range &location);
lsDocumentUri GetLsDocumentUri(DB *db, int file_id, std::string *path);
lsDocumentUri GetLsDocumentUri(DB *db, int file_id);

std::optional<lsLocation> GetLsLocation(DB *db, WorkingFiles *wfiles, Use use);
std::optional<lsLocation> GetLsLocation(DB *db, WorkingFiles *wfiles,
                                        SymbolRef sym, int file_id);
std::vector<lsLocation> GetLsLocations(DB *db, WorkingFiles *wfiles,
                                           const std::vector<Use> &uses);
// Returns a symbol. The symbol will *NOT* have a location assigned.
std::optional<lsSymbolInformation> GetSymbolInfo(DB *db, SymbolIdx sym,
                                                 bool detailed);

std::vector<SymbolRef> FindSymbolsAtLocation(WorkingFile *working_file,
                                             QueryFile *file,
                                             lsPosition &ls_pos,
                                             bool smallest = false);

template <typename Fn> void WithEntity(DB *db, SymbolIdx sym, Fn &&fn) {
  switch (sym.kind) {
  case SymbolKind::Invalid:
  case SymbolKind::File:
    break;
  case SymbolKind::Func:
    fn(db->GetFunc(sym));
    break;
  case SymbolKind::Type:
    fn(db->GetType(sym));
    break;
  case SymbolKind::Var:
    fn(db->GetVar(sym));
    break;
  }
}

template <typename Fn> void EachEntityDef(DB *db, SymbolIdx sym, Fn &&fn) {
  WithEntity(db, sym, [&](const auto &entity) {
    for (auto &def : entity.def)
      if (!fn(def))
        break;
  });
}

template <typename Fn>
void EachOccurrence(DB *db, SymbolIdx sym, bool include_decl, Fn &&fn) {
  WithEntity(db, sym, [&](const auto &entity) {
    for (Use use : entity.uses)
      fn(use);
    if (include_decl) {
      for (auto &def : entity.def)
        if (def.spell)
          fn(*def.spell);
      for (Use use : entity.declarations)
        fn(use);
    }
  });
}

lsSymbolKind GetSymbolKind(DB *db, SymbolIdx sym);

template <typename Fn>
void EachDefinedFunc(DB *db, const std::vector<Usr> &usrs, Fn &&fn) {
  for (Usr usr : usrs) {
    auto &obj = db->Func(usr);
    if (!obj.def.empty())
      fn(obj);
  }
}
