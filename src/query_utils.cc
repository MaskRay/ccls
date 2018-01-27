#include "query_utils.h"

#include "queue_manager.h"

#include <climits>

namespace {

// Computes roughly how long |range| is.
int ComputeRangeSize(const Range& range) {
  if (range.start.line != range.end.line)
    return INT_MAX;
  return range.end.column - range.start.column;
}

}  // namespace

optional<QueryLocation> GetDefinitionSpellingOfSymbol(QueryDatabase* db,
                                                      const QueryTypeId& id) {
  QueryType& type = db->types[id.id];
  if (type.def)
    return type.def->definition_spelling;
  return nullopt;
}

optional<QueryLocation> GetDefinitionSpellingOfSymbol(QueryDatabase* db,
                                                      const QueryFuncId& id) {
  QueryFunc& func = db->funcs[id.id];
  if (func.def)
    return func.def->definition_spelling;
  return nullopt;
}

optional<QueryLocation> GetDefinitionSpellingOfSymbol(QueryDatabase* db,
                                                      const QueryVarId& id) {
  QueryVar& var = db->vars[id.id];
  if (var.def)
    return var.def->definition_spelling;
  return nullopt;
}

optional<QueryLocation> GetDefinitionSpellingOfSymbol(QueryDatabase* db,
                                                      const SymbolIdx& symbol) {
  switch (symbol.kind) {
    case SymbolKind::Type: {
      QueryType& type = db->types[symbol.idx];
      if (type.def)
        return type.def->definition_spelling;
      break;
    }
    case SymbolKind::Func: {
      QueryFunc& func = db->funcs[symbol.idx];
      if (func.def)
        return func.def->definition_spelling;
      break;
    }
    case SymbolKind::Var: {
      QueryVar& var = db->vars[symbol.idx];
      if (var.def)
        return var.def->definition_spelling;
      break;
    }
    case SymbolKind::File:
    case SymbolKind::Invalid: {
      assert(false && "unexpected");
      break;
    }
  }
  return nullopt;
}

optional<QueryLocation> GetDefinitionExtentOfSymbol(QueryDatabase* db,
                                                    const SymbolIdx& symbol) {
  switch (symbol.kind) {
    case SymbolKind::Type: {
      QueryType& type = db->types[symbol.idx];
      if (type.def)
        return type.def->definition_extent;
      break;
    }
    case SymbolKind::Func: {
      QueryFunc& func = db->funcs[symbol.idx];
      if (func.def)
        return func.def->definition_extent;
      break;
    }
    case SymbolKind::Var: {
      QueryVar& var = db->vars[symbol.idx];
      if (var.def)
        return var.def->definition_extent;
      break;
    }
    case SymbolKind::File: {
      return QueryLocation(QueryFileId(symbol.idx),
                           Range(Position(1, 1), Position(1, 1)));
    }
    case SymbolKind::Invalid: {
      assert(false && "unexpected");
      break;
    }
  }
  return nullopt;
}

optional<QueryFileId> GetDeclarationFileForSymbol(QueryDatabase* db,
                                                  const SymbolIdx& symbol) {
  switch (symbol.kind) {
    case SymbolKind::Type: {
      QueryType& type = db->types[symbol.idx];
      if (type.def && type.def->definition_spelling)
        return type.def->definition_spelling->path;
      break;
    }
    case SymbolKind::Func: {
      QueryFunc& func = db->funcs[symbol.idx];
      if (!func.declarations.empty())
        return func.declarations[0].path;
      if (func.def && func.def->definition_spelling)
        return func.def->definition_spelling->path;
      break;
    }
    case SymbolKind::Var: {
      QueryVar& var = db->vars[symbol.idx];
      if (var.def && var.def->definition_spelling)
        return var.def->definition_spelling->path;
      break;
    }
    case SymbolKind::File: {
      return QueryFileId(symbol.idx);
    }
    case SymbolKind::Invalid: {
      assert(false && "unexpected");
      break;
    }
  }
  return nullopt;
}

std::vector<QueryLocation> ToQueryLocation(
    QueryDatabase* db,
    const std::vector<QueryFuncRef>& refs) {
  std::vector<QueryLocation> locs;
  locs.reserve(refs.size());
  for (const QueryFuncRef& ref : refs)
    locs.push_back(ref.loc);
  return locs;
}
std::vector<QueryLocation> ToQueryLocation(
    QueryDatabase* db,
    const std::vector<QueryTypeId>& ids) {
  std::vector<QueryLocation> locs;
  locs.reserve(ids.size());
  for (const QueryTypeId& id : ids) {
    optional<QueryLocation> loc = GetDefinitionSpellingOfSymbol(db, id);
    if (loc)
      locs.push_back(loc.value());
  }
  return locs;
}
std::vector<QueryLocation> ToQueryLocation(
    QueryDatabase* db,
    const std::vector<QueryFuncId>& ids) {
  std::vector<QueryLocation> locs;
  locs.reserve(ids.size());
  for (const QueryFuncId& id : ids) {
    optional<QueryLocation> loc = GetDefinitionSpellingOfSymbol(db, id);
    if (loc)
      locs.push_back(loc.value());
  }
  return locs;
}
std::vector<QueryLocation> ToQueryLocation(QueryDatabase* db,
                                           const std::vector<QueryVarId>& ids) {
  std::vector<QueryLocation> locs;
  locs.reserve(ids.size());
  for (const QueryVarId& id : ids) {
    optional<QueryLocation> loc = GetDefinitionSpellingOfSymbol(db, id);
    if (loc)
      locs.push_back(loc.value());
  }
  return locs;
}

std::vector<QueryLocation> GetUsesOfSymbol(QueryDatabase* db,
                                           const SymbolIdx& symbol,
                                           bool include_decl) {
  switch (symbol.kind) {
    case SymbolKind::Type: {
      QueryType& type = db->types[symbol.idx];
      return type.uses;
    }
    case SymbolKind::Func: {
      // TODO: the vector allocation could be avoided.
      QueryFunc& func = db->funcs[symbol.idx];
      std::vector<QueryLocation> result = ToQueryLocation(db, func.callers);
      if (include_decl) {
        AddRange(&result, func.declarations);
        if (func.def && func.def->definition_spelling)
          result.push_back(*func.def->definition_spelling);
      }
      return result;
    }
    case SymbolKind::Var: {
      QueryVar& var = db->vars[symbol.idx];
      std::vector<QueryLocation> ret = var.uses;
      if (include_decl && var.def && var.def->definition_spelling)
        ret.push_back(*var.def->definition_spelling);
      return ret;
    }
    case SymbolKind::File:
    case SymbolKind::Invalid: {
      assert(false && "unexpected");
      break;
    }
  }
  return {};
}

std::vector<QueryLocation> GetDeclarationsOfSymbolForGotoDefinition(
    QueryDatabase* db,
    const SymbolIdx& symbol) {
  switch (symbol.kind) {
    case SymbolKind::Type: {
      // Returning the definition spelling of a type is a hack (and is why the
      // function has the postfix `ForGotoDefintion`, but it lets the user
      // jump to the start of a type if clicking goto-definition on the same
      // type from within the type definition.
      QueryType& type = db->types[symbol.idx];
      if (type.def) {
        optional<QueryLocation> declaration = type.def->definition_spelling;
        if (declaration)
          return {*declaration};
      }
      break;
    }
    case SymbolKind::Func: {
      QueryFunc& func = db->funcs[symbol.idx];
      return func.declarations;
    }
    case SymbolKind::Var: {
      QueryVar& var = db->vars[symbol.idx];
      if (var.def) {
        optional<QueryLocation> declaration = var.def->declaration;
        if (declaration)
          return {*declaration};
      }
      break;
    }
    default:
      break;
  }

  return {};
}

bool HasCallersOnSelfOrBaseOrDerived(QueryDatabase* db, QueryFunc& root) {
  // Check self.
  if (!root.callers.empty())
    return true;

  // Check for base calls.
  std::queue<QueryFuncId> queue;
  PushRange(&queue, root.def->base);
  while (!queue.empty()) {
    QueryFunc& func = db->funcs[queue.front().id];
    queue.pop();
    if (!func.callers.empty())
      return true;
    if (func.def)
      PushRange(&queue, func.def->base);
  }

  // Check for derived calls.
  PushRange(&queue, root.derived);
  while (!queue.empty()) {
    QueryFunc& func = db->funcs[queue.front().id];
    queue.pop();
    if (!func.callers.empty())
      return true;
    PushRange(&queue, func.derived);
  }

  return false;
}

std::vector<QueryFuncRef> GetCallersForAllBaseFunctions(QueryDatabase* db,
                                                        QueryFunc& root) {
  std::vector<QueryFuncRef> callers;

  std::queue<QueryFuncId> queue;
  PushRange(&queue, root.def->base);
  while (!queue.empty()) {
    QueryFunc& func = db->funcs[queue.front().id];
    queue.pop();

    AddRange(&callers, func.callers);
    if (func.def)
      PushRange(&queue, func.def->base);
  }

  return callers;
}

std::vector<QueryFuncRef> GetCallersForAllDerivedFunctions(QueryDatabase* db,
                                                           QueryFunc& root) {
  std::vector<QueryFuncRef> callers;

  std::queue<QueryFuncId> queue;
  PushRange(&queue, root.derived);

  while (!queue.empty()) {
    QueryFunc& func = db->funcs[queue.front().id];
    queue.pop();

    PushRange(&queue, func.derived);
    AddRange(&callers, func.callers);
  }

  return callers;
}

optional<lsPosition> GetLsPosition(WorkingFile* working_file,
                                   const Position& position) {
  if (!working_file)
    return lsPosition(position.line, position.column);

  int column = position.column;
  optional<int> start = working_file->GetBufferPosFromIndexPos(position.line, &column, false);
  if (!start)
    return nullopt;

  return lsPosition(*start, column);
}

optional<lsRange> GetLsRange(WorkingFile* working_file, const Range& location) {
  if (!working_file) {
    return lsRange(
        lsPosition(location.start.line, location.start.column),
        lsPosition(location.end.line, location.end.column));
  }

  int start_column = location.start.column, end_column = location.end.column;
  optional<int> start =
      working_file->GetBufferPosFromIndexPos(location.start.line, &start_column, false);
  optional<int> end =
      working_file->GetBufferPosFromIndexPos(location.end.line, &end_column, true);
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
                                   const QueryLocation& location) {
  std::string path;
  lsDocumentUri uri = GetLsDocumentUri(db, location.path, &path);
  optional<lsRange> range =
      GetLsRange(working_files->GetFileByFilename(path), location.range);
  if (!range)
    return nullopt;
  return lsLocation(uri, *range);
}

std::vector<lsLocation> GetLsLocations(
    QueryDatabase* db,
    WorkingFiles* working_files,
    const std::vector<QueryLocation>& locations) {
  std::unordered_set<lsLocation> unique_locations;
  for (const QueryLocation& query_location : locations) {
    optional<lsLocation> location =
        GetLsLocation(db, working_files, query_location);
    if (!location)
      continue;
    unique_locations.insert(*location);
  }

  std::vector<lsLocation> result;
  result.reserve(unique_locations.size());
  result.assign(unique_locations.begin(), unique_locations.end());
  return result;
}

// Returns a symbol. The symbol will have *NOT* have a location assigned.
optional<lsSymbolInformation> GetSymbolInfo(QueryDatabase* db,
                                            WorkingFiles* working_files,
                                            SymbolIdx symbol,
                                            bool use_short_name) {
  switch (symbol.kind) {
    case SymbolKind::File: {
      QueryFile& file = db->files[symbol.idx];
      if (!file.def)
        return nullopt;

      lsSymbolInformation info;
      info.name = file.def->path;
      info.kind = lsSymbolKind::File;
      return info;
    }
    case SymbolKind::Type: {
      QueryType& type = db->types[symbol.idx];
      if (!type.def)
        return nullopt;

      lsSymbolInformation info;
      info.name =
          use_short_name ? type.def->short_name : type.def->detailed_name;
      if (type.def->detailed_name != type.def->short_name)
        info.containerName = type.def->detailed_name;
      // TODO ClangSymbolKind -> lsSymbolKind
      switch (type.def->kind) {
      default:
        info.kind = lsSymbolKind::Class;
        break;
      case ClangSymbolKind::Namespace:
        info.kind = lsSymbolKind::Namespace;
        break;
      }
      return info;
    }
    case SymbolKind::Func: {
      QueryFunc& func = db->funcs[symbol.idx];
      if (!func.def)
        return nullopt;

      lsSymbolInformation info;
      info.name =
          use_short_name ? func.def->short_name : func.def->detailed_name;
      info.containerName = func.def->detailed_name;
      info.kind = lsSymbolKind::Function;

      if (func.def->declaring_type.has_value()) {
        QueryType& container = db->types[func.def->declaring_type->id];
        if (container.def)
          info.kind = lsSymbolKind::Method;
      }

      return info;
    }
    case SymbolKind::Var: {
      QueryVar& var = db->vars[symbol.idx];
      if (!var.def)
        return nullopt;

      lsSymbolInformation info;
      info.name = use_short_name ? var.def->short_name : var.def->detailed_name;
      info.containerName = var.def->detailed_name;
      info.kind = lsSymbolKind::Variable;
      return info;
    }
    case SymbolKind::Invalid: {
      return nullopt;
    }
  };

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

  for (const SymbolRef& ref : file->def->all_symbols) {
    if (ref.loc.range.Contains(target_line, target_column))
      symbols.push_back(ref);
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
  std::sort(symbols.begin(), symbols.end(), [](const SymbolRef& a,
                                               const SymbolRef& b) {
    int a_size = ComputeRangeSize(a.loc.range);
    int b_size = ComputeRangeSize(b.loc.range);

    if (a_size != b_size)
      return a_size < b_size;
    // operator> orders Var/Func before Type.
    int t = static_cast<int>(a.idx.kind) - static_cast<int>(b.idx.kind);
    if (t)
      return t > 0;
    return a.idx.idx < b.idx.idx;
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
