#include "query_utils.h"

#include "queue_manager.h"

#include <loguru.hpp>

#include <climits>
#include <queue>

namespace {

// Computes roughly how long |range| is.
int ComputeRangeSize(const Range& range) {
  if (range.start.line != range.end.line)
    return INT_MAX;
  return range.end.column - range.start.column;
}

}  // namespace

Maybe<Use> GetDefinitionSpellingOfSymbol(QueryDatabase* db,
                                         SymbolIdx sym) {
  switch (sym.kind) {
    case SymbolKind::File:
      break;
    case SymbolKind::Func: {
      for (auto& def : db->GetFunc(sym).def)
        if (def.spell)
          return def.spell;
      break;
    }
    case SymbolKind::Type: {
      for (auto& def : db->GetType(sym).def)
        if (def.spell)
          return def.spell;
      break;
    }
    case SymbolKind::Var: {
      for (auto& def : db->GetVar(sym).def)
        if (def.spell)
          return def.spell;
      break;
    }
    case SymbolKind::Invalid:
      assert(false && "unexpected");
      break;
  }
  return nullopt;
}

Maybe<Use> GetDefinitionExtentOfSymbol(QueryDatabase* db, SymbolIdx sym) {
  switch (sym.kind) {
    case SymbolKind::File:
      return Use(Range(Position(0, 0), Position(0, 0)), sym.id, sym.kind,
                 Role::None, QueryFileId(sym.id));
    case SymbolKind::Func: {
      for (auto& def : db->GetFunc(sym).def)
        if (def.extent)
          return def.extent;
      break;
    }
    case SymbolKind::Type: {
      for (auto& def : db->GetType(sym).def)
        if (def.extent)
          return def.extent;
      break;
    }
    case SymbolKind::Var: {
      for (auto& def : db->GetVar(sym).def)
        if (def.extent)
          return def.extent;
      break;
    }
    case SymbolKind::Invalid: {
      assert(false && "unexpected");
      break;
    }
  }
  return nullopt;
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

std::vector<Use> ToUses(QueryDatabase* db,
                        const std::vector<QueryFuncId>& ids) {
  std::vector<Use> ret;
  ret.reserve(ids.size());
  for (auto id : ids) {
    QueryFunc& func = db->funcs[id.id];
    bool has_def = false;
    for (auto& def : func.def)
      if (def.spell) {
        ret.push_back(*def.spell);
        has_def = true;
        break;
      }
    if (!has_def && func.declarations.size())
      ret.push_back(func.declarations[0]);
  }
  return ret;
}

std::vector<Use> ToUses(QueryDatabase* db,
                        const std::vector<QueryTypeId>& ids) {
  std::vector<Use> ret;
  ret.reserve(ids.size());
  for (auto id : ids) {
    QueryType& type = db->types[id.id];
    for (auto& def : type.def)
      if (def.spell) {
        ret.push_back(*def.spell);
        break;
      }
  }
  return ret;
}

std::vector<Use> ToUses(QueryDatabase* db, const std::vector<QueryVarId>& ids) {
  std::vector<Use> ret;
  ret.reserve(ids.size());
  for (auto id : ids) {
    QueryVar& var = db->vars[id.id];
    bool has_def = false;
    for (auto& def : var.def)
      if (def.spell) {
        ret.push_back(*def.spell);
        has_def = true;
        break;
      }
    if (!has_def && var.declarations.size())
      ret.push_back(var.declarations[0]);
  }
  return ret;
}

std::vector<Use> GetUsesOfSymbol(QueryDatabase* db,
                                 SymbolIdx sym,
                                 bool include_decl) {
  switch (sym.kind) {
    case SymbolKind::Type: {
      QueryType& type = db->GetType(sym);
      std::vector<Use> ret = type.uses;
      if (include_decl) {
        for (auto& def : type.def)
          if (def.spell)
            ret.push_back(*def.spell);
      }
      return ret;
    }
    case SymbolKind::Func: {
      QueryFunc& func = db->GetFunc(sym);
      std::vector<Use> ret = func.uses;
      if (include_decl) {
        for (auto& def : func.def)
          if (def.spell)
            ret.push_back(*def.spell);
        AddRange(&ret, func.declarations);
      }
      return ret;
    }
    case SymbolKind::Var: {
      QueryVar& var = db->GetVar(sym);
      std::vector<Use> ret = var.uses;
      if (include_decl) {
        for (auto& def : var.def)
          if (def.spell)
            ret.push_back(*def.spell);
        ret.insert(ret.end(), var.declarations.begin(), var.declarations.end());
      }
      return ret;
    }
    case SymbolKind::File:
    case SymbolKind::Invalid: {
      assert(false && "unexpected");
      return {};
    }
  }
}

std::vector<Use> GetDeclarationsOfSymbolForGotoDefinition(
    QueryDatabase* db,
    SymbolIdx sym) {
  switch (sym.kind) {
    case SymbolKind::Type: {
      // Returning the definition spelling of a type is a hack (and is why the
      // function has the postfix `ForGotoDefintion`, but it lets the user
      // jump to the start of a type if clicking goto-definition on the same
      // type from within the type definition.
      if (const auto* def = db->GetType(sym).AnyDef()) {
        Maybe<Use> spell = def->spell;
        if (spell)
          return {*spell};
      }
      break;
    }
    case SymbolKind::Func:
      return db->GetFunc(sym).declarations;
    case SymbolKind::Var:
      return db->GetVar(sym).declarations;
    default:
      break;
  }

  return {};
}

bool HasCallersOnSelfOrBaseOrDerived(QueryDatabase* db, QueryFunc& root) {
  // Check self.
  if (!root.uses.empty())
    return true;
  const QueryFunc::Def* def = root.AnyDef();

  // Check for base calls.
  std::queue<QueryFunc*> queue;
  EachWithGen<QueryFunc>(db->funcs, def->base, [&](QueryFunc& func) {
    queue.push(&func);
  });
  while (!queue.empty()) {
    QueryFunc& func = *queue.front();
    queue.pop();
    if (!func.uses.empty())
      return true;
    if (def)
      EachWithGen<QueryFunc>(db->funcs, def->base, [&](QueryFunc& func1) {
        queue.push(&func1);
      });
  }

  // Check for derived calls.
  EachWithGen<QueryFunc>(db->funcs, root.derived, [&](QueryFunc& func1) {
    queue.push(&func1);
  });
  while (!queue.empty()) {
    QueryFunc& func = *queue.front();
    queue.pop();
    if (!func.uses.empty())
      return true;
    EachWithGen<QueryFunc>(db->funcs, func.derived, [&](QueryFunc& func1) {
      queue.push(&func1);
    });
  }

  return false;
}

std::vector<Use> GetCallersForAllBaseFunctions(QueryDatabase* db,
                                               QueryFunc& root) {
  std::vector<Use> callers;
  const QueryFunc::Def* def = root.AnyDef();
  if (!def)
    return callers;

  std::queue<QueryFunc*> queue;
  EachWithGen<QueryFunc>(db->funcs, def->base, [&](QueryFunc& func1) {
    queue.push(&func1);
  });
  while (!queue.empty()) {
    QueryFunc& func = *queue.front();
    queue.pop();

    AddRange(&callers, func.uses);
    if (const QueryFunc::Def* def1 = func.AnyDef()) {
      EachWithGen<QueryFunc>(db->funcs, def1->base, [&](QueryFunc& func1) {
        queue.push(&func1);
      });
    }
  }

  return callers;
}

std::vector<Use> GetCallersForAllDerivedFunctions(QueryDatabase* db,
                                                  QueryFunc& root) {
  std::vector<Use> callers;

  std::queue<QueryFunc*> queue;
  EachWithGen<QueryFunc>(db->funcs, root.derived, [&](QueryFunc& func) {
    queue.push(&func);
  });

  while (!queue.empty()) {
    QueryFunc& func = *queue.front();
    queue.pop();

    EachWithGen<QueryFunc>(db->funcs, func.derived, [&](QueryFunc& func1) {
      queue.push(&func1);
    });
    AddRange(&callers, func.uses);
  }

  return callers;
}

optional<lsPosition> GetLsPosition(WorkingFile* working_file,
                                   const Position& position) {
  if (!working_file)
    return lsPosition(position.line, position.column);

  int column = position.column;
  optional<int> start =
      working_file->GetBufferPosFromIndexPos(position.line, &column, false);
  if (!start)
    return nullopt;

  return lsPosition(*start, column);
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
                                       bool extension) {
  optional<lsLocation> ls_loc = GetLsLocation(db, working_files, use);
  if (!ls_loc)
    return nullopt;
  lsLocationEx ret;
  ret.lsLocation::operator=(*ls_loc);
  if (extension)
    switch (use.kind) {
    default:
      break;
    case SymbolKind::Func: {
      const QueryFunc::Def* def = db->GetFunc(use).AnyDef();
      if (def)
        ret.containerName = std::string_view(def->detailed_name);
      break;
    }
    case SymbolKind::Type: {
      const QueryType::Def* def = db->GetType(use).AnyDef();
      if (def)
        ret.containerName = std::string_view(def->detailed_name);
      break;
    }
    case SymbolKind::Var: {
      const QueryVar::Def* def = db->GetVar(use).AnyDef();
      if (def)
        ret.containerName = std::string_view(def->detailed_name);
      break;
    }
    }
  return ret;
}

std::vector<lsLocation> GetLsLocations(
    QueryDatabase* db,
    WorkingFiles* working_files,
    const std::vector<Use>& uses) {
  std::vector<lsLocation> ret;
  for (Use use : uses) {
    optional<lsLocation> location =
        GetLsLocation(db, working_files, use);
    if (location)
      ret.push_back(*location);
  }
  std::sort(ret.begin(), ret.end());
  ret.erase(std::unique(ret.begin(), ret.end()), ret.end());
  return ret;
}

// Returns a symbol. The symbol will have *NOT* have a location assigned.
optional<lsSymbolInformation> GetSymbolInfo(QueryDatabase* db,
                                            WorkingFiles* working_files,
                                            SymbolIdx sym,
                                            bool use_short_name) {
  switch (sym.kind) {
    case SymbolKind::File: {
      QueryFile& file = db->GetFile(sym);
      if (!file.def)
        break;

      lsSymbolInformation info;
      info.name = file.def->path;
      info.kind = lsSymbolKind::File;
      return info;
    }
    case SymbolKind::Type: {
      const QueryType::Def* def = db->GetType(sym).AnyDef();
      if (!def)
        break;

      lsSymbolInformation info;
      if (use_short_name)
        info.name = def->ShortName();
      else
        info.name = def->detailed_name;
      info.kind = def->kind;
      if (def->detailed_name.c_str() != def->ShortName())
        info.containerName = def->detailed_name;
      return info;
    }
    case SymbolKind::Func: {
      const QueryFunc::Def* def = db->GetFunc(sym).AnyDef();
      if (!def)
        break;

      lsSymbolInformation info;
      if (use_short_name)
        info.name = def->ShortName();
      else
        info.name = def->detailed_name;
      info.kind = def->kind;
      info.containerName = def->detailed_name;
      return info;
    }
    case SymbolKind::Var: {
      const QueryVar::Def* def = db->GetVar(sym).AnyDef();
      if (!def)
        break;

      lsSymbolInformation info;
      if (use_short_name)
        info.name = def->ShortName();
      else
        info.name = def->detailed_name;
      info.kind = def->kind;
      info.containerName = def->detailed_name;
      return info;
    }
    case SymbolKind::Invalid:
      break;
  }

  return nullopt;
}

// TODO Sort only by range length, not |kind| or |idx|
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

void EmitDiagnostics(WorkingFiles* working_files,
                     std::string path,
                     std::vector<lsDiagnostic> diagnostics) {
  // Emit diagnostics.
  Out_TextDocumentPublishDiagnostics out;
  out.params.uri = lsDocumentUri::FromPath(path);
  out.params.diagnostics = diagnostics;
  QueueManager::WriteStdout(IpcId::TextDocumentPublishDiagnostics, out);

  // Cache diagnostics so we can show fixits.
  working_files->DoActionOnFile(path, [&](WorkingFile* working_file) {
    if (working_file)
      working_file->diagnostics_ = diagnostics;
  });
}
