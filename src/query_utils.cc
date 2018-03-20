#include "query_utils.h"

#include "queue_manager.h"

#include <loguru.hpp>

#include <climits>
#include <unordered_set>

namespace {

// Computes roughly how long |range| is.
int ComputeRangeSize(const Range& range) {
  if (range.start.line != range.end.line)
    return INT_MAX;
  return range.end.column - range.start.column;
}

template <typename Q>
std::vector<Use> GetDeclarations(std::vector<Q>& entities,
                                 const std::vector<Id<Q>>& ids) {
  std::vector<Use> ret;
  ret.reserve(ids.size());
  for (auto id : ids) {
    Q& entity = entities[id.id];
    bool has_def = false;
    for (auto& def : entity.def)
      if (def.spell) {
        ret.push_back(*def.spell);
        has_def = true;
        break;
      }
    if (!has_def && entity.declarations.size())
      ret.push_back(entity.declarations[0]);
  }
  return ret;
}

}  // namespace

Maybe<Use> GetDefinitionSpell(QueryDatabase* db, SymbolIdx sym) {
  Maybe<Use> ret;
  EachEntityDef(db, sym, [&](const auto& def) { return !(ret = def.spell); });
  return ret;
}

Maybe<Use> GetDefinitionExtent(QueryDatabase* db, SymbolIdx sym) {
  // Used to jump to file.
  if (sym.kind == SymbolKind::File)
    return Use(Range(Position(0, 0), Position(0, 0)), sym.id, sym.kind,
               Role::None, QueryFileId(sym.id));
  Maybe<Use> ret;
  EachEntityDef(db, sym, [&](const auto& def) { return !(ret = def.extent); });
  return ret;
}

Maybe<QueryFileId> GetDeclarationFileForSymbol(QueryDatabase* db,
                                               SymbolIdx sym) {
  switch (sym.kind) {
    case SymbolKind::File:
      return QueryFileId(sym.id);
    case SymbolKind::Func: {
      QueryFunc& func = db->GetFunc(sym);
      if (!func.declarations.empty())
        return func.declarations[0].file;
      if (const auto* def = func.AnyDef())
        return def->file;
      break;
    }
    case SymbolKind::Type: {
      if (const auto* def = db->GetType(sym).AnyDef())
        return def->file;
      break;
    }
    case SymbolKind::Var: {
      if (const auto* def = db->GetVar(sym).AnyDef())
        return def->file;
      break;
    }
    case SymbolKind::Invalid: {
      assert(false && "unexpected");
      break;
    }
  }
  return nullopt;
}

std::vector<Use> GetDeclarations(QueryDatabase* db,
                                 const std::vector<QueryFuncId>& ids) {
  return GetDeclarations(db->funcs, ids);
}

std::vector<Use> GetDeclarations(QueryDatabase* db,
                                 const std::vector<QueryTypeId>& ids) {
  return GetDeclarations(db->types, ids);
}

std::vector<Use> GetDeclarations(QueryDatabase* db,
                                 const std::vector<QueryVarId>& ids) {
  return GetDeclarations(db->vars, ids);
}

std::vector<Use> GetNonDefDeclarations(QueryDatabase* db, SymbolIdx sym) {
  switch (sym.kind) {
    case SymbolKind::Func:
      return db->GetFunc(sym).declarations;
    case SymbolKind::Type:
      return db->GetType(sym).declarations;
    case SymbolKind::Var:
      return db->GetVar(sym).declarations;
    default:
      return {};
  }
}

std::vector<Use> GetUsesForAllBases(QueryDatabase* db, QueryFunc& root) {
  std::vector<Use> ret;
  std::vector<QueryFunc*> stack{&root};
  std::unordered_set<Usr> seen;
  seen.insert(root.usr);
  while (!stack.empty()) {
    QueryFunc& func = *stack.back();
    stack.pop_back();
    if (auto* def = func.AnyDef()) {
      EachDefinedEntity(db->funcs, def->bases, [&](QueryFunc& func1) {
        if (!seen.count(func1.usr)) {
          seen.insert(func1.usr);
          stack.push_back(&func1);
          AddRange(&ret, func1.uses);
        }
      });
    }
  }

  return ret;
}

std::vector<Use> GetUsesForAllDerived(QueryDatabase* db, QueryFunc& root) {
  std::vector<Use> ret;
  std::vector<QueryFunc*> stack{&root};
  std::unordered_set<Usr> seen;
  seen.insert(root.usr);
  while (!stack.empty()) {
    QueryFunc& func = *stack.back();
    stack.pop_back();
    EachDefinedEntity(db->funcs, func.derived, [&](QueryFunc& func1) {
      if (!seen.count(func1.usr)) {
        seen.insert(func1.usr);
        stack.push_back(&func1);
        AddRange(&ret, func1.uses);
      }
    });
  }

  return ret;
}

optional<lsPosition> GetLsPosition(WorkingFile* working_file,
                                   const Position& position) {
  if (!working_file)
    return lsPosition(position.line, position.column);

  int column = position.column;
  if (optional<int> start =
          working_file->GetBufferPosFromIndexPos(position.line, &column, false))
    return lsPosition(*start, column);
  return nullopt;
}

optional<lsRange> GetLsRange(WorkingFile* working_file, const Range& location) {
  if (!working_file) {
    return lsRange(lsPosition(location.start.line, location.start.column),
                   lsPosition(location.end.line, location.end.column));
  }

  int start_column = location.start.column, end_column = location.end.column;
  optional<int> start = working_file->GetBufferPosFromIndexPos(
      location.start.line, &start_column, false);
  optional<int> end = working_file->GetBufferPosFromIndexPos(location.end.line,
                                                             &end_column, true);
  if (!start || !end)
    return nullopt;

  // If remapping end fails (end can never be < start), just guess that the
  // final location didn't move. This only screws up the highlighted code
  // region if we guess wrong, so not a big deal.
  //
  // Remapping fails often in C++ since there are a lot of "};" at the end of
  // class/struct definitions.
  if (*end < *start)
    *end = *start + (location.end.line - location.start.line);
  if (*start == *end && start_column > end_column)
    end_column = start_column;

  return lsRange(lsPosition(*start, start_column),
                 lsPosition(*end, end_column));
}

lsDocumentUri GetLsDocumentUri(QueryDatabase* db,
                               QueryFileId file_id,
                               std::string* path) {
  QueryFile& file = db->files[file_id.id];
  if (file.def) {
    *path = file.def->path;
    return lsDocumentUri::FromPath(*path);
  } else {
    *path = "";
    return lsDocumentUri::FromPath("");
  }
}

lsDocumentUri GetLsDocumentUri(QueryDatabase* db, QueryFileId file_id) {
  QueryFile& file = db->files[file_id.id];
  if (file.def) {
    return lsDocumentUri::FromPath(file.def->path);
  } else {
    return lsDocumentUri::FromPath("");
  }
}

optional<lsLocation> GetLsLocation(QueryDatabase* db,
                                   WorkingFiles* working_files,
                                   Use use) {
  std::string path;
  lsDocumentUri uri = GetLsDocumentUri(db, use.file, &path);
  optional<lsRange> range =
      GetLsRange(working_files->GetFileByFilename(path), use.range);
  if (!range)
    return nullopt;
  return lsLocation(uri, *range);
}

optional<lsLocationEx> GetLsLocationEx(QueryDatabase* db,
                                       WorkingFiles* working_files,
                                       Use use,
                                       bool container) {
  optional<lsLocation> ls_loc = GetLsLocation(db, working_files, use);
  if (!ls_loc)
    return nullopt;
  lsLocationEx ret;
  ret.lsLocation::operator=(*ls_loc);
  if (container) {
    ret.role = uint16_t(use.role);
    EachEntityDef(db, use, [&](const auto& def) {
      ret.containerName = std::string_view(def.detailed_name);
      return false;
    });
  }
  return ret;
}

std::vector<lsLocationEx> GetLsLocationExs(QueryDatabase* db,
                                           WorkingFiles* working_files,
                                           const std::vector<Use>& uses,
                                           bool container,
                                           int limit) {
  std::vector<lsLocationEx> ret;
  for (Use use : uses)
    if (auto loc = GetLsLocationEx(db, working_files, use, container))
      ret.push_back(*loc);
  std::sort(ret.begin(), ret.end());
  ret.erase(std::unique(ret.begin(), ret.end()), ret.end());
  if (ret.size() > limit)
    ret.resize(limit);
  return ret;
}

lsSymbolKind GetSymbolKind(QueryDatabase* db, SymbolIdx sym) {
  lsSymbolKind ret;
  if (sym.kind == SymbolKind::File)
    ret = lsSymbolKind::File;
  else {
    ret = lsSymbolKind::Unknown;
    WithEntity(db, sym, [&](const auto& entity) {
      for (auto& def : entity.def) {
        ret = def.kind;
        break;
      }
    });
  }
  return ret;
}

// Returns a symbol. The symbol will have *NOT* have a location assigned.
optional<lsSymbolInformation> GetSymbolInfo(QueryDatabase* db,
                                            WorkingFiles* working_files,
                                            SymbolIdx sym,
                                            bool use_short_name) {
  switch (sym.kind) {
    case SymbolKind::Invalid:
      break;
    case SymbolKind::File: {
      QueryFile& file = db->GetFile(sym);
      if (!file.def)
        break;

      lsSymbolInformation info;
      info.name = file.def->path;
      info.kind = lsSymbolKind::File;
      return info;
    }
    default: {
      lsSymbolInformation info;
      EachEntityDef(db, sym, [&](const auto& def) {
        if (use_short_name)
          info.name = def.ShortName();
        else
          info.name = def.detailed_name;
        info.kind = def.kind;
        info.containerName = def.detailed_name;
        return false;
      });
      return info;
    }
  }

  return nullopt;
}

std::vector<SymbolRef> FindSymbolsAtLocation(WorkingFile* working_file,
                                             QueryFile* file,
                                             lsPosition position) {
  std::vector<SymbolRef> symbols;
  symbols.reserve(1);

  int target_line = position.line;
  int target_column = position.character;
  if (working_file) {
    optional<int> index_line = working_file->GetIndexPosFromBufferPos(
        target_line, &target_column, false);
    if (index_line)
      target_line = *index_line;
  }

  for (const SymbolRef& sym : file->def->all_symbols) {
    if (sym.range.Contains(target_line, target_column))
      symbols.push_back(sym);
  }

  // Order shorter ranges first, since they are more detailed/precise. This is
  // important for macros which generate code so that we can resolving the
  // macro argument takes priority over the entire macro body.
  //
  // Order SymbolKind::Var before SymbolKind::Type. Macro calls are treated as
  // Var currently. If a macro expands to tokens led by a SymbolKind::Type, the
  // macro and the Type have the same range. We want to find the macro
  // definition instead of the Type definition.
  //
  // Then order functions before other types, which makes goto definition work
  // better on constructors.
  std::sort(symbols.begin(), symbols.end(),
            [](const SymbolRef& a, const SymbolRef& b) {
              int t = ComputeRangeSize(a.range) - ComputeRangeSize(b.range);
              if (t)
                return t < 0;
              t = (a.role & Role::Definition) - (b.role & Role::Definition);
              if (t)
                return t > 0;
              // operator> orders Var/Func before Type.
              t = static_cast<int>(a.kind) - static_cast<int>(b.kind);
              if (t)
                return t > 0;
              return a.id < b.id;
            });

  return symbols;
}
