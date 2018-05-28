#include "query_utils.h"

#include "pipeline.hh"

#include <limits.h>
#include <unordered_set>

namespace {

// Computes roughly how long |range| is.
int ComputeRangeSize(const Range& range) {
  if (range.start.line != range.end.line)
    return INT_MAX;
  return range.end.column - range.start.column;
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
    return Use{{Range{{0, 0}, {0, 0}}, sym.usr, sym.kind, Role::None},
               int(sym.usr)};
  Maybe<Use> ret;
  EachEntityDef(db, sym, [&](const auto& def) { return !(ret = def.extent); });
  return ret;
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
      EachDefinedEntity(db->usr2func, def->bases, [&](QueryFunc& func1) {
        if (!seen.count(func1.usr)) {
          seen.insert(func1.usr);
          stack.push_back(&func1);
          ret.insert(ret.end(), func1.uses.begin(), func1.uses.end());
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
    EachDefinedEntity(db->usr2func, func.derived, [&](QueryFunc& func1) {
      if (!seen.count(func1.usr)) {
        seen.insert(func1.usr);
        stack.push_back(&func1);
        ret.insert(ret.end(), func1.uses.begin(), func1.uses.end());
      }
    });
  }

  return ret;
}

std::optional<lsPosition> GetLsPosition(WorkingFile* working_file,
                                   const Position& position) {
  if (!working_file)
    return lsPosition{position.line, position.column};

  int column = position.column;
  if (std::optional<int> start =
          working_file->GetBufferPosFromIndexPos(position.line, &column, false))
    return lsPosition{*start, column};
  return std::nullopt;
}

std::optional<lsRange> GetLsRange(WorkingFile* working_file, const Range& location) {
  if (!working_file) {
    return lsRange{lsPosition{location.start.line, location.start.column},
                   lsPosition{location.end.line, location.end.column}};
  }

  int start_column = location.start.column, end_column = location.end.column;
  std::optional<int> start = working_file->GetBufferPosFromIndexPos(
      location.start.line, &start_column, false);
  std::optional<int> end = working_file->GetBufferPosFromIndexPos(location.end.line,
                                                             &end_column, true);
  if (!start || !end)
    return std::nullopt;

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

  return lsRange{lsPosition{*start, start_column},
                 lsPosition{*end, end_column}};
}

lsDocumentUri GetLsDocumentUri(QueryDatabase* db,
                               int file_id,
                               std::string* path) {
  QueryFile& file = db->files[file_id];
  if (file.def) {
    *path = file.def->path;
    return lsDocumentUri::FromPath(*path);
  } else {
    *path = "";
    return lsDocumentUri::FromPath("");
  }
}

lsDocumentUri GetLsDocumentUri(QueryDatabase* db, int file_id) {
  QueryFile& file = db->files[file_id];
  if (file.def) {
    return lsDocumentUri::FromPath(file.def->path);
  } else {
    return lsDocumentUri::FromPath("");
  }
}

std::optional<lsLocation> GetLsLocation(QueryDatabase* db,
                                   WorkingFiles* working_files,
                                   Use use) {
  std::string path;
  lsDocumentUri uri = GetLsDocumentUri(db, use.file_id, &path);
  std::optional<lsRange> range =
      GetLsRange(working_files->GetFileByFilename(path), use.range);
  if (!range)
    return std::nullopt;
  return lsLocation{uri, *range};
}

std::optional<lsLocationEx> GetLsLocationEx(QueryDatabase* db,
                                       WorkingFiles* working_files,
                                       Use use,
                                       bool container) {
  std::optional<lsLocation> ls_loc = GetLsLocation(db, working_files, use);
  if (!ls_loc)
    return std::nullopt;
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
                                           const std::vector<Use>& uses) {
  std::vector<lsLocationEx> ret;
  for (Use use : uses)
    if (auto loc =
            GetLsLocationEx(db, working_files, use, g_config->xref.container))
      ret.push_back(*loc);
  std::sort(ret.begin(), ret.end());
  ret.erase(std::unique(ret.begin(), ret.end()), ret.end());
  if (ret.size() > g_config->xref.maxNum)
    ret.resize(g_config->xref.maxNum);
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
std::optional<lsSymbolInformation> GetSymbolInfo(QueryDatabase* db,
                                            WorkingFiles* working_files,
                                            SymbolIdx sym,
                                            bool detailed_name) {
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
        if (detailed_name)
          info.name = def.detailed_name;
        else
          info.name = def.Name(true);
        info.kind = def.kind;
        info.containerName = def.detailed_name;
        return false;
      });
      return info;
    }
  }

  return std::nullopt;
}

std::vector<SymbolRef> FindSymbolsAtLocation(WorkingFile* working_file,
                                             QueryFile* file,
                                             lsPosition& ls_pos) {
  std::vector<SymbolRef> symbols;
  if (working_file) {
    if (auto line = working_file->GetIndexPosFromBufferPos(
        ls_pos.line, &ls_pos.character, false)) {
      ls_pos.line = *line;
    } else {
      ls_pos.line = -1;
      return {};
    }
  }

  for (const SymbolRef& sym : file->def->all_symbols) {
    if (sym.range.Contains(ls_pos.line, ls_pos.character))
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
              return a.usr < b.usr;
            });

  return symbols;
}
