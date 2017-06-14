// TODO: cleanup includes
#include "cache.h"
#include "clang_complete.h"
#include "file_consumer.h"
#include "match.h"
#include "include_complete.h"
#include "ipc_manager.h"
#include "indexer.h"
#include "query.h"
#include "language_server_api.h"
#include "options.h"
#include "project.h"
#include "platform.h"
#include "standard_includes.h"
#include "test.h"
#include "timer.h"
#include "threaded_queue.h"
#include "working_files.h"

#include <doctest/doctest.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/ostreamwrapper.h>

#include <climits>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <string>
#include <sstream>
#include <unordered_map>
#include <thread>
#include <vector>

// TODO: provide a feature like 'https://github.com/goldsborough/clang-expand',
// ie, a fully linear view of a function with inline function calls expanded.
// We can probably use vscode decorators to achieve it.

// TODO: implement ThreadPool type which monitors CPU usage / number of work items
// per second completed and scales up/down number of running threads.

namespace {

std::vector<std::string> kEmptyArgs;

// Expected client version. We show an error if this doesn't match.
const int kExpectedClientVersion = 2;














#if false
std::string FormatMicroseconds(long long microseconds) {
  long long milliseconds = microseconds / 1000;
  long long remaining = microseconds  - milliseconds;

  // Only show two digits after the dot.
  while (remaining >= 100)
    remaining /= 10;

  return std::to_string(milliseconds) + "." + std::to_string(remaining) + "ms";
}
#endif















// Cached completion information, so we can give fast completion results when
// the user erases a character. vscode will resend the completion request if
// that happens.
struct CodeCompleteCache {
  optional<std::string> cached_path;
  optional<lsPosition> cached_completion_position;
  NonElidedVector<lsCompletionItem> cached_results;

  bool IsCacheValid(lsTextDocumentPositionParams position) const {
    return cached_path == position.textDocument.uri.GetPath() &&
            cached_completion_position == position.position;
  }
};






















// This function returns true if e2e timing should be displayed for the given IpcId.
bool ShouldDisplayIpcTiming(IpcId id) {
  switch (id) {
  case IpcId::TextDocumentPublishDiagnostics:
  case IpcId::CqueryPublishInactiveRegions:
    return false;
  default:
    return true;
  }
}






bool ShouldRunIncludeCompletion(const std::string& line) {
  size_t start = 0;
  while (start < line.size() && isspace(line[start]))
    ++start;
  return start < line.size() && line[start] == '#';
}

// TODO: eliminate |line_number| param.
optional<lsRange> ExtractQuotedRange(int line_number, const std::string& line) {
  // Find starting and ending quote.
  int start = 0;
  while (start < line.size()) {
    char c = line[start];
    ++start;
    if (c == '"' || c == '<')
      break;
  }
  if (start == line.size())
    return nullopt;

  int end = (int)line.size();
  while (end > 0) {
    char c = line[end];
    if (c == '"' || c == '>')
      break;
    --end;
  }

  if (start >= end)
    return nullopt;

  return lsRange(lsPosition(line_number, start), lsPosition(line_number, end));
}



























void PushBack(NonElidedVector<lsLocation>* result, optional<lsLocation> location) {
  if (location)
    result->push_back(*location);
}

bool FindFileOrFail(QueryDatabase* db, lsRequestId id, const std::string& absolute_path, QueryFile** out_query_file, QueryFileId* out_file_id = nullptr) {
  auto it = db->usr_to_file.find(LowerPathIfCaseInsensitive(absolute_path));
  if (it != db->usr_to_file.end()) {
    optional<QueryFile>& file = db->files[it->second.id];
    if (file) {
      *out_query_file = &file.value();
      if (out_file_id)
        *out_file_id = QueryFileId(it->second.id);
      return true;
    }
  }

  if (out_file_id)
    *out_file_id = QueryFileId((size_t)-1);

  std::cerr << "Unable to find file " << absolute_path << std::endl;

  Out_Error out;
  out.id = id;
  out.error.code = lsErrorCodes::InternalError;
  out.error.message = "Unable to find file " + absolute_path;
  IpcManager::instance()->SendOutMessageToClient(IpcId::Cout, out);

  return false;
}

QueryFile* FindFile(QueryDatabase* db, const std::string& absolute_path) {
  auto it = db->usr_to_file.find(LowerPathIfCaseInsensitive(absolute_path));
  if (it != db->usr_to_file.end()) {
    optional<QueryFile>& file = db->files[it->second.id];
    if (file)
      return &file.value();
  }
  return nullptr;
}

optional<QueryLocation> GetDefinitionSpellingOfSymbol(QueryDatabase* db, const QueryTypeId& id) {
  optional<QueryType>& type = db->types[id.id];
  if (type)
    return type->def.definition_spelling;
  return nullopt;
}

optional<QueryLocation> GetDefinitionSpellingOfSymbol(QueryDatabase* db, const QueryFuncId& id) {
  optional<QueryFunc>& func = db->funcs[id.id];
  if (func)
    return func->def.definition_spelling;
  return nullopt;
}

optional<QueryLocation> GetDefinitionSpellingOfSymbol(QueryDatabase* db, const QueryVarId& id) {
  optional<QueryVar>& var = db->vars[id.id];
  if (var)
    return var->def.definition_spelling;
  return nullopt;
}

optional<QueryLocation> GetDefinitionSpellingOfSymbol(QueryDatabase* db, const SymbolIdx& symbol) {
  switch (symbol.kind) {
    case SymbolKind::Type: {
      optional<QueryType>& type = db->types[symbol.idx];
      if (type)
        return type->def.definition_spelling;
      break;
    }
    case SymbolKind::Func: {
      optional<QueryFunc>& func = db->funcs[symbol.idx];
      if (func)
        return func->def.definition_spelling;
      break;
    }
    case SymbolKind::Var: {
      optional<QueryVar>& var = db->vars[symbol.idx];
      if (var)
        return var->def.definition_spelling;
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

optional<QueryLocation> GetDefinitionExtentOfSymbol(QueryDatabase* db, const SymbolIdx& symbol) {
  switch (symbol.kind) {
    case SymbolKind::Type: {
      optional<QueryType>& type = db->types[symbol.idx];
      if (type)
        return type->def.definition_extent;
      break;
    }
    case SymbolKind::Func: {
      optional<QueryFunc>& func = db->funcs[symbol.idx];
      if (func)
        return func->def.definition_extent;
      break;
    }
    case SymbolKind::Var: {
      optional<QueryVar>& var = db->vars[symbol.idx];
      if (var)
        return var->def.definition_extent;
      break;
    }
    case SymbolKind::File: {
      return QueryLocation(QueryFileId(symbol.idx), Range(Position(1, 1), Position(1, 1)));
    }
    case SymbolKind::Invalid: {
      assert(false && "unexpected");
      break;
    }
  }
  return nullopt;
}

std::string GetHoverForSymbol(QueryDatabase* db, const SymbolIdx& symbol) {
  switch (symbol.kind) {
    case SymbolKind::Type: {
      optional<QueryType>& type = db->types[symbol.idx];
      if (type)
        return type->def.detailed_name;
      break;
    }
    case SymbolKind::Func: {
      optional<QueryFunc>& func = db->funcs[symbol.idx];
      if (func)
        return func->def.detailed_name;
      break;
    }
    case SymbolKind::Var: {
      optional<QueryVar>& var = db->vars[symbol.idx];
      if (var)
        return var->def.detailed_name;
      break;
    }
    case SymbolKind::File:
    case SymbolKind::Invalid: {
      assert(false && "unexpected");
      break;
    }
  }
  return "";
}

optional<QueryFileId> GetDeclarationFileForSymbol(QueryDatabase* db, const SymbolIdx& symbol) {
  switch (symbol.kind) {
    case SymbolKind::Type: {
      optional<QueryType>& type = db->types[symbol.idx];
      if (type && type->def.definition_spelling)
        return type->def.definition_spelling->path;
      break;
    }
    case SymbolKind::Func: {
      optional<QueryFunc>& func = db->funcs[symbol.idx];
      if (func) {
        if (!func->declarations.empty())
          return func->declarations[0].path;
        if (func->def.definition_spelling)
          return func->def.definition_spelling->path;
      }
      break;
    }
    case SymbolKind::Var: {
      optional<QueryVar>& var = db->vars[symbol.idx];
      if (var && var->def.definition_spelling)
        return var->def.definition_spelling->path;
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

std::vector<QueryLocation> ToQueryLocation(QueryDatabase* db, const std::vector<QueryFuncRef>& refs) {
  std::vector<QueryLocation> locs;
  locs.reserve(refs.size());
  for (const QueryFuncRef& ref : refs)
    locs.push_back(ref.loc);
  return locs;
}
std::vector<QueryLocation> ToQueryLocation(QueryDatabase* db, const std::vector<QueryTypeId>& ids) {
  std::vector<QueryLocation> locs;
  locs.reserve(ids.size());
  for (const QueryTypeId& id : ids) {
    optional<QueryLocation> loc = GetDefinitionSpellingOfSymbol(db, id);
    if (loc)
      locs.push_back(loc.value());
  }
  return locs;
}
std::vector<QueryLocation> ToQueryLocation(QueryDatabase* db, const std::vector<QueryFuncId>& ids) {
  std::vector<QueryLocation> locs;
  locs.reserve(ids.size());
  for (const QueryFuncId& id : ids) {
    optional<QueryLocation> loc = GetDefinitionSpellingOfSymbol(db, id);
    if (loc)
      locs.push_back(loc.value());
  }
  return locs;
}
std::vector<QueryLocation> ToQueryLocation(QueryDatabase* db, const std::vector<QueryVarId>& ids) {
  std::vector<QueryLocation> locs;
  locs.reserve(ids.size());
  for (const QueryVarId& id : ids) {
    optional<QueryLocation> loc = GetDefinitionSpellingOfSymbol(db, id);
    if (loc)
      locs.push_back(loc.value());
  }
  return locs;
}



std::vector<QueryLocation> GetUsesOfSymbol(QueryDatabase* db, const SymbolIdx& symbol) {
  switch (symbol.kind) {
    case SymbolKind::Type: {
      optional<QueryType>& type = db->types[symbol.idx];
      if (type)
        return type->uses;
      break;
    }
    case SymbolKind::Func: {
      // TODO: the vector allocation could be avoided.
      optional<QueryFunc>& func = db->funcs[symbol.idx];
      if (func) {
        std::vector<QueryLocation> result = ToQueryLocation(db, func->callers);
        AddRange(&result, func->declarations);
        if (func->def.definition_spelling)
          result.push_back(*func->def.definition_spelling);
        return result;
      }
      break;
    }
    case SymbolKind::Var: {
      optional<QueryVar>& var = db->vars[symbol.idx];
      if (var)
        return var->uses;
      break;
    }
    case SymbolKind::File:
    case SymbolKind::Invalid: {
      assert(false && "unexpected");
      break;
    }
  }
  return {};
}

std::vector<QueryLocation> GetDeclarationsOfSymbolForGotoDefinition(QueryDatabase* db, const SymbolIdx& symbol) {
  switch (symbol.kind) {
    case SymbolKind::Type: {
      // Returning the definition spelling of a type is a hack (and is why the
      // function has the postfix `ForGotoDefintion`, but it lets the user
      // jump to the start of a type if clicking goto-definition on the same
      // type from within the type definition.
      optional<QueryType>& type = db->types[symbol.idx];
      if (type) {
        optional<QueryLocation> declaration = type->def.definition_spelling;
        if (declaration)
          return { *declaration };
      }
      break;
    }
    case SymbolKind::Func: {
      optional<QueryFunc>& func = db->funcs[symbol.idx];
      if (func)
        return func->declarations;
      break;
    }
    case SymbolKind::Var: {
      optional<QueryVar>& var = db->vars[symbol.idx];
      if (var) {
        optional<QueryLocation> declaration = var->def.declaration;
        if (declaration)
          return { *declaration };
      }
      break;
    }
    default:
      break;
  }

  return {};
}

optional<QueryLocation> GetBaseDefinitionOrDeclarationSpelling(QueryDatabase* db, QueryFunc& func) {
  if (!func.def.base)
    return nullopt;
  optional<QueryFunc>& base = db->funcs[func.def.base->id];
  if (!base)
    return nullopt;

  auto def = base->def.definition_spelling;
  if (!def && !base->declarations.empty())
    def = base->declarations[0];
  return def;
}

std::vector<QueryFuncRef> GetCallersForAllBaseFunctions(QueryDatabase* db, QueryFunc& root) {
  std::vector<QueryFuncRef> callers;

  optional<QueryFuncId> func_id = root.def.base;
  while (func_id) {
    optional<QueryFunc>& func = db->funcs[func_id->id];
    if (!func)
      break;

    AddRange(&callers, func->callers);
    func_id = func->def.base;
  }

  return callers;
}

std::vector<QueryFuncRef> GetCallersForAllDerivedFunctions(QueryDatabase* db, QueryFunc& root) {
  std::vector<QueryFuncRef> callers;

  std::queue<QueryFuncId> queue;
  PushRange(&queue, root.derived);

  while (!queue.empty()) {
    optional<QueryFunc>& func = db->funcs[queue.front().id];
    queue.pop();
    if (!func)
      continue;

    PushRange(&queue, func->derived);
    AddRange(&callers, func->callers);
  }

  return callers;
}

optional<lsPosition> GetLsPosition(WorkingFile* working_file, const Position& position) {
  if (!working_file)
    return lsPosition(position.line - 1, position.column - 1);

  optional<int> start = working_file->GetBufferLineFromIndexLine(position.line);
  if (!start)
    return nullopt;

  return lsPosition(*start - 1, position.column - 1);
}

optional<lsRange> GetLsRange(WorkingFile* working_file, const Range& location) {
  if (!working_file) {
    return lsRange(
      lsPosition(location.start.line - 1, location.start.column - 1),
      lsPosition(location.end.line - 1, location.end.column - 1));
  }

  optional<int> start = working_file->GetBufferLineFromIndexLine(location.start.line);
  optional<int> end = working_file->GetBufferLineFromIndexLine(location.end.line);
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

  return lsRange(
    lsPosition(*start - 1, location.start.column - 1),
    lsPosition(*end - 1, location.end.column - 1));
}

lsDocumentUri GetLsDocumentUri(QueryDatabase* db, QueryFileId file_id, std::string* path) {
  optional<QueryFile>& file = db->files[file_id.id];
  if (file) {
    *path = file->def.path;
    return lsDocumentUri::FromPath(*path);
  }
  else {
    *path = "";
    return lsDocumentUri::FromPath("");
  }
}

lsDocumentUri GetLsDocumentUri(QueryDatabase* db, QueryFileId file_id) {
  optional<QueryFile>& file = db->files[file_id.id];
  if (file) {
    return lsDocumentUri::FromPath(file->def.path);
  }
  else {
    return lsDocumentUri::FromPath("");
  }
}

optional<lsLocation> GetLsLocation(QueryDatabase* db, WorkingFiles* working_files, const QueryLocation& location) {
  std::string path;
  lsDocumentUri uri = GetLsDocumentUri(db, location.path, &path);
  optional<lsRange> range = GetLsRange(working_files->GetFileByFilename(path), location.range);
  if (!range)
    return nullopt;
  return lsLocation(uri, *range);
}

NonElidedVector<lsLocation> GetLsLocations(QueryDatabase* db, WorkingFiles* working_files, const std::vector<QueryLocation>& locations) {
  std::unordered_set<lsLocation> unique_locations;
  for (const QueryLocation& query_location : locations) {
    optional<lsLocation> location = GetLsLocation(db, working_files, query_location);
    if (!location)
      continue;
    unique_locations.insert(*location);
  }

  NonElidedVector<lsLocation> result;
  result.reserve(unique_locations.size());
  result.assign(unique_locations.begin(), unique_locations.end());
  return result;
}

// Returns a symbol. The symbol will have *NOT* have a location assigned.
optional<lsSymbolInformation> GetSymbolInfo(QueryDatabase* db, WorkingFiles* working_files, SymbolIdx symbol) {
    switch (symbol.kind) {
    case SymbolKind::File: {
      optional<QueryFile>& file = db->files[symbol.idx];
      if (!file)
        return nullopt;

      lsSymbolInformation info;
      info.name = file->def.path;
      info.kind = lsSymbolKind::File;
      return info;
    }
    case SymbolKind::Type: {
      optional<QueryType>& type = db->types[symbol.idx];
      if (!type)
        return nullopt;

      lsSymbolInformation info;
      info.name = type->def.short_name;
      if (type->def.detailed_name != type->def.short_name)
        info.containerName = type->def.detailed_name;
      info.kind = lsSymbolKind::Class;
      return info;
    }
    case SymbolKind::Func: {
      optional<QueryFunc>& func = db->funcs[symbol.idx];
      if (!func)
        return nullopt;

      lsSymbolInformation info;
      info.name = func->def.short_name;
      info.containerName = func->def.detailed_name;
      info.kind = lsSymbolKind::Function;

      if (func->def.declaring_type.has_value()) {
        optional<QueryType>& container = db->types[func->def.declaring_type->id];
        if (container)
          info.kind = lsSymbolKind::Method;
      }

      return info;
    }
    case SymbolKind::Var: {
      optional<QueryVar>& var = db->vars[symbol.idx];
      if (!var)
        return nullopt;

      lsSymbolInformation info;
      info.name += var->def.short_name;
      info.containerName = var->def.detailed_name;
      info.kind = lsSymbolKind::Variable;
      return info;
    }
    case SymbolKind::Invalid: {
      return nullopt;
    }
  };

  return nullopt;
}

struct CommonCodeLensParams {
  std::vector<TCodeLens>* result;
  QueryDatabase* db;
  WorkingFiles* working_files;
  WorkingFile* working_file;
};

void AddCodeLens(
    const char* singular,
    const char* plural,
    CommonCodeLensParams* common,
    QueryLocation loc,
    const std::vector<QueryLocation>& uses,
    optional<QueryLocation> excluded,
    bool force_display) {
  TCodeLens code_lens;
  optional<lsRange> range = GetLsRange(common->working_file, loc.range);
  if (!range)
    return;
  code_lens.range = *range;
  code_lens.command = lsCommand<lsCodeLensCommandArguments>();
  code_lens.command->command = "cquery.showReferences";
  code_lens.command->arguments.uri = GetLsDocumentUri(common->db, loc.path);
  code_lens.command->arguments.position = code_lens.range.start;

  // Add unique uses.
  std::unordered_set<lsLocation> unique_uses;
  for (const QueryLocation& use : uses) {
    if (excluded == use)
      continue;
    optional<lsLocation> location = GetLsLocation(common->db, common->working_files, use);
    if (!location)
      continue;
    unique_uses.insert(*location);
  }
  code_lens.command->arguments.locations.assign(unique_uses.begin(),
    unique_uses.end());

  // User visible label
  size_t num_usages = unique_uses.size();
  code_lens.command->title = std::to_string(num_usages) + " ";
  if (num_usages == 1)
    code_lens.command->title += singular;
  else
    code_lens.command->title += plural;

  if (force_display || unique_uses.size() > 0)
    common->result->push_back(code_lens);
}

lsWorkspaceEdit BuildWorkspaceEdit(QueryDatabase* db, WorkingFiles* working_files, const std::vector<QueryLocation>& locations, const std::string& new_text) {
  std::unordered_map<QueryFileId, lsTextDocumentEdit> path_to_edit;

  for (auto& location : locations) {
    optional<lsLocation> ls_location = GetLsLocation(db, working_files, location);
    if (!ls_location)
      continue;

    if (path_to_edit.find(location.path) == path_to_edit.end()) {
      path_to_edit[location.path] = lsTextDocumentEdit();

      optional<QueryFile>& file = db->files[location.path.id];
      if (!file)
        continue;

      const std::string& path = file->def.path;
      path_to_edit[location.path].textDocument.uri = lsDocumentUri::FromPath(path);

      WorkingFile* working_file = working_files->GetFileByFilename(path);
      if (working_file)
        path_to_edit[location.path].textDocument.version = working_file->version;
    }

    lsTextEdit edit;
    edit.range = ls_location->range;
    edit.newText = new_text;

    // vscode complains if we submit overlapping text edits.
    auto& edits = path_to_edit[location.path].edits;
    if (std::find(edits.begin(), edits.end(), edit) == edits.end())
      edits.push_back(edit);
  }


  lsWorkspaceEdit edit;
  for (const auto& changes : path_to_edit)
    edit.documentChanges.push_back(changes.second);
  return edit;
}

std::vector<SymbolRef> FindSymbolsAtLocation(WorkingFile* working_file, QueryFile* file, lsPosition position) {
  std::vector<SymbolRef> symbols;
  symbols.reserve(1);

  int target_line = position.line + 1;
  int target_column = position.character + 1;
  if (working_file) {
    optional<int> index_line = working_file->GetIndexLineFromBufferLine(target_line);
    if (index_line)
      target_line = *index_line;
  }

  for (const SymbolRef& ref : file->def.all_symbols) {
    if (ref.loc.range.Contains(target_line, target_column))
      symbols.push_back(ref);
  }

  // Order function symbols first. This makes goto definition work better when
  // used on a constructor.
  std::sort(symbols.begin(), symbols.end(), [](const SymbolRef& a, const SymbolRef& b) {
    if (a.idx.kind != b.idx.kind && a.idx.kind == SymbolKind::Func)
      return 1;
    return 0;
  });

  return symbols;
}

NonElidedVector<Out_CqueryTypeHierarchyTree::TypeEntry> BuildParentTypeHierarchy(QueryDatabase* db, WorkingFiles* working_files, QueryTypeId root) {
  optional<QueryType>& root_type = db->types[root.id];
  if (!root_type)
    return {};

  NonElidedVector<Out_CqueryTypeHierarchyTree::TypeEntry> parent_entries;
  parent_entries.reserve(root_type->def.parents.size());

  for (QueryTypeId parent_id : root_type->def.parents) {
    optional<QueryType>& parent_type = db->types[parent_id.id];
    if (!parent_type)
      continue;

    Out_CqueryTypeHierarchyTree::TypeEntry parent_entry;
    parent_entry.name = parent_type->def.detailed_name;
    if (parent_type->def.definition_spelling)
      parent_entry.location = GetLsLocation(db, working_files, *parent_type->def.definition_spelling);
    parent_entry.children = BuildParentTypeHierarchy(db, working_files, parent_id);

    parent_entries.push_back(parent_entry);
  }

  return parent_entries;
}


optional<Out_CqueryTypeHierarchyTree::TypeEntry> BuildTypeHierarchy(QueryDatabase* db, WorkingFiles* working_files, QueryTypeId root_id) {
  optional<QueryType>& root_type = db->types[root_id.id];
  if (!root_type)
    return nullopt;

  Out_CqueryTypeHierarchyTree::TypeEntry entry;

  // Name and location.
  entry.name = root_type->def.detailed_name;
  if (root_type->def.definition_spelling)
    entry.location = GetLsLocation(db, working_files, *root_type->def.definition_spelling);

  entry.children.reserve(root_type->derived.size());

  // Base types.
  Out_CqueryTypeHierarchyTree::TypeEntry base;
  base.name = "[[Base]]";
  base.location = entry.location;
  base.children = BuildParentTypeHierarchy(db, working_files, root_id);
  if (!base.children.empty())
    entry.children.push_back(base);

  // Add derived.
  for (QueryTypeId derived : root_type->derived) {
    auto derived_entry = BuildTypeHierarchy(db, working_files, derived);
    if (derived_entry)
      entry.children.push_back(*derived_entry);
  }

  return entry;
}

NonElidedVector<Out_CqueryCallTree::CallEntry> BuildInitialCallTree(QueryDatabase* db, WorkingFiles* working_files, QueryFuncId root) {
  optional<QueryFunc>& root_func = db->funcs[root.id];
  if (!root_func)
    return {};
  if (!root_func->def.definition_spelling)
    return {};
  optional<lsLocation> def_loc = GetLsLocation(db, working_files, *root_func->def.definition_spelling);
  if (!def_loc)
    return {};

  Out_CqueryCallTree::CallEntry entry;
  entry.name = root_func->def.short_name;
  entry.usr = root_func->def.usr;
  entry.location = *def_loc;
  entry.hasCallers = !root_func->callers.empty();
  NonElidedVector<Out_CqueryCallTree::CallEntry> result;
  result.push_back(entry);
  return result;
}

NonElidedVector<Out_CqueryCallTree::CallEntry> BuildExpandCallTree(QueryDatabase* db, WorkingFiles* working_files, QueryFuncId root) {
  optional<QueryFunc>& root_func = db->funcs[root.id];
  if (!root_func)
    return {};

  NonElidedVector<Out_CqueryCallTree::CallEntry> result;
  result.reserve(root_func->callers.size());
  for (QueryFuncRef caller : root_func->callers) {
    optional<QueryFunc>& call_func = db->funcs[caller.id.id];
    if (!call_func)
      continue;
    optional<lsLocation> call_location = GetLsLocation(db, working_files, caller.loc);
    if (!call_location)
      continue;

    Out_CqueryCallTree::CallEntry call_entry;
    call_entry.name = call_func->def.short_name;
    call_entry.usr = call_func->def.usr;
    call_entry.location = *call_location;
    call_entry.hasCallers = !call_func->callers.empty();
    result.push_back(call_entry);
  }

  return result;
}

void PublishInactiveLines(WorkingFile* working_file, const std::vector<Range>& inactive) {
  Out_CquerySetInactiveRegion out;
  out.params.uri = lsDocumentUri::FromPath(working_file->filename);
  for (Range skipped : inactive) {
    optional<lsRange> ls_skipped = GetLsRange(working_file, skipped);
    if (ls_skipped)
      out.params.inactiveRegions.push_back(*ls_skipped);
  }
  IpcManager::instance()->SendOutMessageToClient(IpcId::CqueryPublishInactiveRegions, out);
}


void LexFunctionDeclaration(const std::string& buffer_content, lsPosition declaration_spelling, optional<std::string> type_name, std::string* insert_text, int* newlines_after_name) {
  int name_start = GetOffsetForPosition(declaration_spelling, buffer_content);

  bool parse_return_type = true;
  // We need to check if we have a return type (ctors and dtors do not).
  if (type_name) {
    int name_end = name_start;
    while (name_end < buffer_content.size()) {
      char c = buffer_content[name_end];
      if (isspace(c) || c == '(')
        break;
      ++name_end;
    }

    std::string func_name = buffer_content.substr(name_start, name_end - name_start);
    if (func_name == *type_name || func_name == ("~" + *type_name))
      parse_return_type = false;
  }

  // We need to fetch the return type. This can get complex, ie,
  //
  //  std::vector <int> foo();
  //
  int return_start = name_start;
  if (parse_return_type) {
    int paren_balance = 0;
    int angle_balance = 0;
    bool expect_token = true;
    while (return_start > 0) {
      char c = buffer_content[return_start - 1];
      if (paren_balance == 0 && angle_balance == 0) {
        if (isspace(c) && !expect_token) {
          break;
        }
        if (!isspace(c))
          expect_token = false;
      }

      if (c == ')')
        ++paren_balance;
      if (c == '(') {
        --paren_balance;
        expect_token = true;
      }

      if (c == '>')
        ++angle_balance;
      if (c == '<') {
        --angle_balance;
        expect_token = true;
      }

      return_start -= 1;
    }
  }

  // We need to fetch the arguments. Just scan for the next ';'.
  *newlines_after_name = 0;
  int end = name_start;
  while (end < buffer_content.size()) {
    char c = buffer_content[end];
    if (c == ';')
      break;
    if (c == '\n')
      *newlines_after_name += 1;
    ++end;
  }

  std::string result;
  result += buffer_content.substr(return_start, name_start - return_start);
  if (type_name && !type_name->empty())
    result += *type_name + "::";
  result += buffer_content.substr(name_start, end - name_start);
  TrimEnd(result);
  result += " {\n}";
  *insert_text = result;
}

std::string LexWordAroundPos(lsPosition position, const std::string& content) {
  int index = GetOffsetForPosition(position, content);

  int start = index;
  int end = index;

  while (start > 0) {
    char c = content[start - 1];
    if (isalnum(c) || c == '_') {
      --start;
    }
    else {
      break;
    }
  }

  while ((end + 1) < content.size()) {
    char c = content[end + 1];
    if (isalnum(c) || c == '_') {
      ++end;
    }
    else {
      break;
    }
  }

  return content.substr(start, end - start + 1);
}

optional<int> FindIncludeLine(const std::vector<std::string>& lines, const std::string& full_include_line) {
  //
  // This returns an include line. For example,
  //
  //    #include <a>  // 0
  //    #include <c>  // 1
  //
  // Given #include <b>, this will return '1', which means that the
  // #include <b> text should be inserted at the start of line 1. Inserting
  // at the start of a line allows insertion at both the top and bottom of the
  // document.
  //
  // If the include line is already in the document this returns nullopt.
  //

  optional<int> last_include_line;
  optional<int> best_include_line;

  //  1 => include line is gt content (ie, it should go after)
  // -1 => include line is lt content (ie, it should go before)
  int last_line_compare = 1;

  for (int line = 0; line < (int)lines.size(); ++line) {
    if (!StartsWith(lines[line], "#include")) {
      last_line_compare = 1;
      continue;
    }

    last_include_line = line;

    int current_line_compare = full_include_line.compare(lines[line]);
    if (current_line_compare == 0)
      return nullopt;

    if (last_line_compare == 1 && current_line_compare == -1)
      best_include_line = line;
    last_line_compare = current_line_compare;
  }

  if (best_include_line)
    return *best_include_line;
  // If |best_include_line| didn't match that means we likely didn't find an
  // include which was lt the new one, so put it at the end of the last include
  // list.
  if (last_include_line)
    return *last_include_line + 1;
  // No includes, use top of document.
  return 0;
}

optional<QueryFileId> GetImplementationFile(QueryDatabase* db, QueryFile* file) {
  for (SymbolRef sym : file->def.outline) {
    switch (sym.idx.kind) {
      case SymbolKind::Func: {
        optional<QueryFunc>& func = db->funcs[sym.idx.idx];
        if (func && func->def.definition_extent)
          return func->def.definition_extent->path;
        break;
      }
      case SymbolKind::Var: {
        optional<QueryVar>& var = db->vars[sym.idx.idx];
        if (var && var->def.definition_extent)
          return db->vars[sym.idx.idx]->def.definition_extent->path;
        break;
      }
      default:
        break;
    }
  }

  // No associated definition, scan the project for a file in the same
  // directory with the same base-name.
  std::string original_path = LowerPathIfCaseInsensitive(file->def.path);
  std::string target_path = original_path;
  size_t last = target_path.find_last_of('.');
  if (last != std::string::npos) {
    target_path = target_path.substr(0, last);
  }

  for (auto& entry : db->usr_to_file) {
    Usr path = entry.first;

    // Do not consider header files for implementation files.
    // TODO: make file extensions configurable.
    if (EndsWith(path, ".h") || EndsWith(path, ".hpp"))
      continue;

    if (StartsWith(path, target_path) && path != original_path) {
      return entry.second;
    }
  }

  return nullopt;
}

void EnsureImplFile(QueryDatabase* db, QueryFileId file_id, optional<lsDocumentUri>& impl_uri, optional<QueryFileId>& impl_file_id) {
  if (!impl_uri.has_value()) {
    optional<QueryFile>& file = db->files[file_id.id];
    assert(file);

    impl_file_id = GetImplementationFile(db, &file.value());
    if (!impl_file_id.has_value())
      impl_file_id = file_id;

    optional<QueryFile>& impl_file = db->files[impl_file_id->id];
    if (impl_file)
      impl_uri = lsDocumentUri::FromPath(impl_file->def.path);
    else
      impl_uri = lsDocumentUri::FromPath(file->def.path);
  }
}

optional<lsTextEdit> BuildAutoImplementForFunction(QueryDatabase* db, WorkingFiles* working_files, WorkingFile* working_file, int default_line, QueryFileId decl_file_id, QueryFileId impl_file_id, QueryFunc& func) {
  for (const QueryLocation& decl : func.declarations) {
    if (decl.path != decl_file_id)
      continue;

    optional<lsRange> ls_decl = GetLsRange(working_file, decl.range);
    if (!ls_decl)
      continue;

    optional<std::string> type_name;
    optional<lsPosition> same_file_insert_end;
    if (func.def.declaring_type) {
      optional<QueryType>& declaring_type = db->types[func.def.declaring_type->id];
      if (declaring_type) {
        type_name = declaring_type->def.short_name;
        optional<lsRange> ls_type_def_extent = GetLsRange(working_file, declaring_type->def.definition_extent->range);
        if (ls_type_def_extent) {
          same_file_insert_end = ls_type_def_extent->end;
          same_file_insert_end->character += 1; // move past semicolon.
        }
      }
    }

    std::string insert_text;
    int newlines_after_name = 0;
    LexFunctionDeclaration(working_file->buffer_content, ls_decl->start, type_name, &insert_text, &newlines_after_name);

    if (!same_file_insert_end) {
      same_file_insert_end = ls_decl->end;
      same_file_insert_end->line += newlines_after_name;
      same_file_insert_end->character = 1000;
    }

    lsTextEdit edit;

    if (decl_file_id == impl_file_id) {
      edit.range.start = *same_file_insert_end;
      edit.range.end = *same_file_insert_end;
      edit.newText = "\n\n" + insert_text;
    }
    else {
      lsPosition best_pos;
      best_pos.line = default_line;
      int best_dist = INT_MAX;

      optional<QueryFile>& file = db->files[impl_file_id.id];
      assert(file);
      for (SymbolRef sym : file->def.outline) {
        switch (sym.idx.kind) {
          case SymbolKind::Func: {
            optional<QueryFunc>& sym_func = db->funcs[sym.idx.idx];
            if (!sym_func || !sym_func->def.definition_extent)
              break;

            for (QueryLocation& func_decl : sym_func->declarations) {
              if (func_decl.path == decl_file_id) {
                int dist = func_decl.range.start.line - decl.range.start.line;
                if (abs(dist) < abs(best_dist)) {
                  optional<lsLocation> def_loc = GetLsLocation(db, working_files, *sym_func->def.definition_extent);
                  if (!def_loc)
                    continue;

                  best_dist = dist;

                  if (dist > 0)
                    best_pos = def_loc->range.start;
                  else
                    best_pos = def_loc->range.end;
                }
              }
            }

            break;
          }
          case SymbolKind::Var: {
            // TODO: handle vars.

            //optional<QueryVar>& var = db->vars[sym.idx.idx];
            //if (!var || !var->def.definition_extent)
            //  continue;

            break;
          }
        }
      }


      edit.range.start = best_pos;
      edit.range.end = best_pos;
      if (best_dist < 0)
        edit.newText = "\n\n" + insert_text;
      else
        edit.newText = insert_text + "\n\n";
    }

    return edit;
  }

  return nullopt;
}

void EmitDiagnostics(WorkingFiles* working_files, std::string path, NonElidedVector<lsDiagnostic> diagnostics) {
  // Emit diagnostics.
  Out_TextDocumentPublishDiagnostics diagnostic_response;
  diagnostic_response.params.uri = lsDocumentUri::FromPath(path);
  diagnostic_response.params.diagnostics = diagnostics;
  IpcManager::instance()->SendOutMessageToClient(IpcId::TextDocumentPublishDiagnostics, diagnostic_response);

  // Cache diagnostics so we can show fixits.
  working_files->DoActionOnFile(path, [&](WorkingFile* working_file) {
    if (working_file)
      working_file->diagnostics_ = diagnostics;
  });
}






























struct Index_DoIndex {
  enum class Type {
    // ImportOnly is used internally for loading dependency caches. The main cc
    // file is loaded with ImportThenParse, which will call ImportOnly on all
    // of the dependencies. The main cc will then be parsed, which will include
    // updates to all dependencies.

    ImportThenParse,
    Parse,
    Freshen,
  };

  Index_DoIndex(Type type, const Project::Entry& entry, optional<std::string> content, bool is_interactive)
    : type(type), entry(entry), content(content), is_interactive(is_interactive) {}

  // Type of index operation.
  Type type;
  // Project entry for file path and file arguments.
  Project::Entry entry;
  // File contents that should be indexed.
  optional<std::string> content;
  // If this index request is in response to an interactive user session, for
  // example, the user saving a file they are actively editing. We report
  // additional information for interactive indexes such as the IndexUpdate
  // delta as well as the diagnostics.
  bool is_interactive;
};

struct Index_DoIdMap {
  std::unique_ptr<IndexFile> previous;
  std::unique_ptr<IndexFile> current;
  optional<std::string> indexed_content;
  PerformanceImportFile perf;
  bool is_interactive;

  explicit Index_DoIdMap(std::unique_ptr<IndexFile> current,
                         optional<std::string> indexed_content,
                         PerformanceImportFile perf,
                         bool is_interactive)
    : current(std::move(current)),
      indexed_content(indexed_content),
      perf(perf),
      is_interactive(is_interactive) {}

  explicit Index_DoIdMap(std::unique_ptr<IndexFile> previous,
                         std::unique_ptr<IndexFile> current,
                         optional<std::string> indexed_content,
                         PerformanceImportFile perf,
                         bool is_interactive)
    : previous(std::move(previous)),
      current(std::move(current)),
      indexed_content(indexed_content),
      perf(perf),
      is_interactive(is_interactive) {}
};

struct Index_OnIdMapped {
  std::unique_ptr<IndexFile> previous_index;
  std::unique_ptr<IndexFile> current_index;
  std::unique_ptr<IdMap> previous_id_map;
  std::unique_ptr<IdMap> current_id_map;
  optional<std::string> indexed_content;
  PerformanceImportFile perf;
  bool is_interactive;

  Index_OnIdMapped(const optional<std::string>& indexed_content,
                   PerformanceImportFile perf,
                   bool is_interactive)
    : indexed_content(indexed_content),
      perf(perf),
      is_interactive(is_interactive) {}
};

struct Index_OnIndexed {
  IndexUpdate update;
  // Map is file path to file content.
  std::unordered_map<std::string, std::string> indexed_content;
  PerformanceImportFile perf;

  explicit Index_OnIndexed(
      IndexUpdate& update,
      const optional<std::string>& indexed_content,
      PerformanceImportFile perf)
    : update(update), perf(perf) {
    if (indexed_content) {
      assert(update.files_def_update.size() == 1);
      this->indexed_content[update.files_def_update[0].path] = *indexed_content;
    }
  }
};

using Index_DoIndexQueue = ThreadedQueue<Index_DoIndex>;
using Index_DoIdMapQueue = ThreadedQueue<Index_DoIdMap>;
using Index_OnIdMappedQueue = ThreadedQueue<Index_OnIdMapped>;
using Index_OnIndexedQueue = ThreadedQueue<Index_OnIndexed>;

void RegisterMessageTypes() {
  MessageRegistry::instance()->Register<Ipc_CancelRequest>();
  MessageRegistry::instance()->Register<Ipc_InitializeRequest>();
  MessageRegistry::instance()->Register<Ipc_InitializedNotification>();
  MessageRegistry::instance()->Register<Ipc_Exit>();
  MessageRegistry::instance()->Register<Ipc_TextDocumentDidOpen>();
  MessageRegistry::instance()->Register<Ipc_TextDocumentDidChange>();
  MessageRegistry::instance()->Register<Ipc_TextDocumentDidClose>();
  MessageRegistry::instance()->Register<Ipc_TextDocumentDidSave>();
  MessageRegistry::instance()->Register<Ipc_TextDocumentRename>();
  MessageRegistry::instance()->Register<Ipc_TextDocumentComplete>();
  MessageRegistry::instance()->Register<Ipc_TextDocumentSignatureHelp>();
  MessageRegistry::instance()->Register<Ipc_TextDocumentDefinition>();
  MessageRegistry::instance()->Register<Ipc_TextDocumentDocumentHighlight>();
  MessageRegistry::instance()->Register<Ipc_TextDocumentHover>();
  MessageRegistry::instance()->Register<Ipc_TextDocumentReferences>();
  MessageRegistry::instance()->Register<Ipc_TextDocumentDocumentSymbol>();
  MessageRegistry::instance()->Register<Ipc_TextDocumentDocumentLink>();
  MessageRegistry::instance()->Register<Ipc_TextDocumentCodeAction>();
  MessageRegistry::instance()->Register<Ipc_TextDocumentCodeLens>();
  MessageRegistry::instance()->Register<Ipc_CodeLensResolve>();
  MessageRegistry::instance()->Register<Ipc_WorkspaceSymbol>();
  MessageRegistry::instance()->Register<Ipc_CqueryFreshenIndex>();
  MessageRegistry::instance()->Register<Ipc_CqueryTypeHierarchyTree>();
  MessageRegistry::instance()->Register<Ipc_CqueryCallTreeInitial>();
  MessageRegistry::instance()->Register<Ipc_CqueryCallTreeExpand>();
  MessageRegistry::instance()->Register<Ipc_CqueryVars>();
  MessageRegistry::instance()->Register<Ipc_CqueryCallers>();
  MessageRegistry::instance()->Register<Ipc_CqueryBase>();
  MessageRegistry::instance()->Register<Ipc_CqueryDerived>();
}















}  // namespace


























































































bool ImportCachedIndex(Config* config,
                       FileConsumer::SharedState* file_consumer_shared,
                       Index_DoIdMapQueue* queue_do_id_map,
                       const std::string& tu_path,
                       const optional<std::string>& indexed_content) {
  // TODO: only load cache if command line arguments are the same.
  PerformanceImportFile tu_perf;
  Timer time;

  std::unique_ptr<IndexFile> tu_cache = LoadCachedIndex(config, tu_path);
  tu_perf.index_load_cached = time.ElapsedMicrosecondsAndReset();
  if (!tu_cache)
    return true;

  bool needs_reparse = false;

  // Import all dependencies.
  for (auto& dependency_path : tu_cache->dependencies) {
    //std::cerr << "- Got dependency " << dependency_path << std::endl;
    PerformanceImportFile perf;
    time.Reset();
    std::unique_ptr<IndexFile> cache = LoadCachedIndex(config, dependency_path);
    perf.index_load_cached = time.ElapsedMicrosecondsAndReset();
    if (cache && GetLastModificationTime(cache->path) == cache->last_modification_time)
      file_consumer_shared->Mark(cache->path);
    else
      needs_reparse = true;
    if (cache)
      queue_do_id_map->Enqueue(Index_DoIdMap(std::move(cache), nullopt, perf, false /*is_interactive*/));
  }

  // Import primary file.
  if (GetLastModificationTime(tu_path) == tu_cache->last_modification_time)
    file_consumer_shared->Mark(tu_path);
  else
    needs_reparse = true;
  queue_do_id_map->Enqueue(Index_DoIdMap(std::move(tu_cache), indexed_content, tu_perf, false /*is_interactive*/));

  return needs_reparse;
}

void ParseFile(Config* config,
               WorkingFiles* working_files,
               FileConsumer::SharedState* file_consumer_shared,
               clang::Index* index,
               Index_DoIdMapQueue* queue_do_id_map,
               const Project::Entry& entry,
               const optional<std::string>& indexed_content,
               bool is_interactive) {

  std::unique_ptr<IndexFile> cache_for_args = LoadCachedIndex(config, entry.filename);

  std::string tu_path = cache_for_args ? cache_for_args->import_file : entry.filename;
  const std::vector<std::string>& tu_args = entry.args;

  PerformanceImportFile perf;

  std::vector<std::unique_ptr<IndexFile>> indexes = Parse(
    config, file_consumer_shared,
    tu_path, tu_args,
    entry.filename, indexed_content,
    &perf, index);

  for (std::unique_ptr<IndexFile>& new_index : indexes) {
    Timer time;

    // Load the cached index.
    std::unique_ptr<IndexFile> cached_index;
    if (cache_for_args && new_index->path == cache_for_args->path)
      cached_index = std::move(cache_for_args);
    else
      cached_index = LoadCachedIndex(config, new_index->path);
    // TODO: Enable this assert when we are no longer forcibly indexing the primary file.
    //assert(!cached_index || GetLastModificationTime(new_index->path) != cached_index->last_modification_time);

    // Note: we are reusing the parent perf.
    perf.index_load_cached = time.ElapsedMicrosecondsAndReset();

    // Publish lines skipped by the preprocessor if this is an interactive
    // index.
    if (is_interactive) {
      WorkingFile* working_file = working_files->GetFileByFilename(new_index->path);
      if (working_file) {
        // Publish source ranges disabled by preprocessor.
        // TODO: We shouldn't be updating actual indexed content here, but we
        // need to use the latest indexed content for the remapping.
        // TODO: We should also remap diagnostics.
        if (indexed_content)
          working_file->SetIndexContent(*indexed_content);
        PublishInactiveLines(working_file, new_index->skipped_by_preprocessor);
      }
    }

    // Any any existing dependencies to |new_index| that were there before,
    // because we will not reparse them if they haven't changed.
    // TODO: indexer should always include dependencies. This doesn't let us remove old dependencies.
    if (cached_index) {
      for (auto& dep : cached_index->dependencies) {
        if (std::find(new_index->dependencies.begin(), new_index->dependencies.end(), dep) == new_index->dependencies.end())
          new_index->dependencies.push_back(dep);
      }
    }

    // Forward file content, but only for the primary file.
    optional<std::string> content;
    if (new_index->path == entry.filename)
      content = indexed_content;

    // Cache the newly indexed file. This replaces the existing cache.
    // TODO: Run this as another import pipeline stage.
    time.Reset();
    WriteToCache(config, new_index->path, *new_index, content);
    perf.index_save_to_disk = time.ElapsedMicrosecondsAndReset();

    // Dispatch IdMap creation request, which will happen on querydb thread.
    Index_DoIdMap response(std::move(cached_index), std::move(new_index), content, perf, is_interactive);
    queue_do_id_map->Enqueue(std::move(response));
  }

}

bool ResetStaleFiles(Config* config,
                     FileConsumer::SharedState* file_consumer_shared,
                     const std::string& tu_path) {
  Timer time;
  std::unique_ptr<IndexFile> tu_cache = LoadCachedIndex(config, tu_path);

  if (!tu_cache) {
    std::cerr << "[indexer] Unable to load existing index from file when freshening (dependences will not be freshened)" << std::endl;
    file_consumer_shared->Mark(tu_path);
    return true;
  }

  bool needs_reparse = false;

  // Check dependencies
  for (auto& dependency_path : tu_cache->dependencies) {
    std::unique_ptr<IndexFile> cache = LoadCachedIndex(config, dependency_path);
    if (GetLastModificationTime(cache->path) != cache->last_modification_time) {
      needs_reparse = true;
      file_consumer_shared->Reset(cache->path);
    }
  }

  // Check primary file
  if (GetLastModificationTime(tu_path) != tu_cache->last_modification_time) {
    needs_reparse = true;
    file_consumer_shared->Mark(tu_path);
  }

  return needs_reparse;
}

bool IndexMain_DoIndex(Config* config,
                       FileConsumer::SharedState* file_consumer_shared,
                       Project* project,
                       WorkingFiles* working_files,
                       clang::Index* index,
                       Index_DoIndexQueue* queue_do_index,
                       Index_DoIdMapQueue* queue_do_id_map) {
  optional<Index_DoIndex> index_request = queue_do_index->TryDequeue();
  if (!index_request)
    return false;

  Timer time;

  switch (index_request->type) {
    case Index_DoIndex::Type::ImportThenParse: {
      // This assumes index_request->path is a cc or translation unit file (ie,
      // it is in compile_commands.json).

      bool needs_reparse = ImportCachedIndex(config, file_consumer_shared, queue_do_id_map, index_request->entry.filename, index_request->content);

      // If the file has been updated, we need to reparse it.
      if (needs_reparse) {
        // Instead of parsing the file immediately, we push the request to the
        // back of the queue so we will finish all of the Import requests
        // before starting to run actual index jobs. This gives the user a
        // partially-correct index potentially much sooner.
        index_request->type = Index_DoIndex::Type::Parse;
        queue_do_index->Enqueue(std::move(*index_request));
      }
      break;
    }

    case Index_DoIndex::Type::Parse: {
      // index_request->path can be a cc/tu or a dependency path.
      file_consumer_shared->Reset(index_request->entry.filename);
      ParseFile(config, working_files, file_consumer_shared, index, queue_do_id_map, index_request->entry, index_request->content, index_request->is_interactive);
      break;
    }

    case Index_DoIndex::Type::Freshen: {
      // This assumes index_request->path is a cc or translation unit file (ie,
      // it is in compile_commands.json).

      bool needs_reparse = ResetStaleFiles(config, file_consumer_shared, index_request->entry.filename);
      if (needs_reparse)
        ParseFile(config, working_files, file_consumer_shared, index, queue_do_id_map, index_request->entry, index_request->content, index_request->is_interactive);
      break;
    }
  }

  return true;
}

bool IndexMain_DoCreateIndexUpdate(
    Index_OnIdMappedQueue* queue_on_id_mapped,
    Index_OnIndexedQueue* queue_on_indexed) {
  optional<Index_OnIdMapped> response = queue_on_id_mapped->TryDequeue();
  if (!response)
    return false;

  Timer time;
  // TODO/FIXME: Running call tree on IndexUpdate::CreateDelta crashes cquery.
  IndexUpdate update = IndexUpdate::CreateDelta(response->previous_id_map.get(), response->current_id_map.get(),
    response->previous_index.get(), response->current_index.get());
  response->perf.index_make_delta = time.ElapsedMicrosecondsAndReset();

#if false
#define PRINT_SECTION(name) \
  if (response->perf.name) {\
    total += response->perf.name; \
    long long milliseconds = response->perf.name / 1000; \
    long long remaining = response->perf.name - milliseconds; \
    output << " " << #name << ": " << FormatMicroseconds(response->perf.name); \
  }
  std::stringstream output;
  long long total = 0;
  output << "[perf]";
  PRINT_SECTION(index_parse);
  PRINT_SECTION(index_build);
  PRINT_SECTION(index_save_to_disk);
  PRINT_SECTION(index_load_cached);
  PRINT_SECTION(querydb_id_map);
  PRINT_SECTION(index_make_delta);
  output << "\n       total: " << FormatMicroseconds(total);
  output << " path: " << response->current_index->path;
  output << std::endl;
  std::cerr << output.rdbuf();
#undef PRINT_SECTION

  if (response->is_interactive)
    std::cerr << "Applying IndexUpdate" << std::endl << update.ToString() << std::endl;
#endif

  Index_OnIndexed reply(update, response->indexed_content, response->perf);
  queue_on_indexed->Enqueue(std::move(reply));

  return true;
}

bool IndexMergeIndexUpdates(Index_OnIndexedQueue* queue_on_indexed) {
  optional<Index_OnIndexed> root = queue_on_indexed->TryDequeue();
  if (!root)
    return false;

  bool did_merge = false;
  while (true) {
    optional<Index_OnIndexed> to_join = queue_on_indexed->TryDequeue();
    if (!to_join) {
      queue_on_indexed->Enqueue(std::move(*root));
      return did_merge;
    }

    did_merge = true;
    //Timer time;
    root->update.Merge(to_join->update);
    for (auto&& entry : to_join->indexed_content)
      root->indexed_content.emplace(entry);
    //time.ResetAndPrint("[indexer] Joining two querydb updates");
  }
}

void IndexMain(
    Config* config,
    FileConsumer::SharedState* file_consumer_shared,
    Project* project,
    WorkingFiles* working_files,
    MultiQueueWaiter* waiter,
    Index_DoIndexQueue* queue_do_index,
    Index_DoIdMapQueue* queue_do_id_map,
    Index_OnIdMappedQueue* queue_on_id_mapped,
    Index_OnIndexedQueue* queue_on_indexed) {

  SetCurrentThreadName("indexer");
  // TODO: dispose of index after it is not used for a while.
  clang::Index index(1, 0);

  while (true) {
    // TODO: process all off IndexMain_DoIndex before calling IndexMain_DoCreateIndexUpdate for
    //       better icache behavior. We need to have some threads spinning on both though
    //       otherwise memory usage will get bad.

    // We need to make sure to run both IndexMain_DoIndex and
    // IndexMain_DoCreateIndexUpdate so we don't starve querydb from doing any
    // work. Running both also lets the user query the partially constructed
    // index.
    bool did_index = IndexMain_DoIndex(config, file_consumer_shared, project, working_files, &index, queue_do_index, queue_do_id_map);
    bool did_create_update = IndexMain_DoCreateIndexUpdate(queue_on_id_mapped, queue_on_indexed);
    bool did_merge = false;

    // Nothing to index and no index updates to create, so join some already
    // created index updates to reduce work on querydb thread.
    if (!did_index && !did_create_update)
      did_merge = IndexMergeIndexUpdates(queue_on_indexed);

    // We didn't do any work, so wait for a notification.
    if (!did_index && !did_create_update && !did_merge)
      waiter->Wait({
        queue_do_index,
        queue_on_id_mapped,
        queue_on_indexed
      });
  }
}


















































bool QueryDbMainLoop(
    Config* config,
    QueryDatabase* db,
    MultiQueueWaiter* waiter,
    Index_DoIndexQueue* queue_do_index,
    Index_DoIdMapQueue* queue_do_id_map,
    Index_OnIdMappedQueue* queue_on_id_mapped,
    Index_OnIndexedQueue* queue_on_indexed,
    Project* project,
    FileConsumer::SharedState* file_consumer_shared,
    WorkingFiles* working_files,
    ClangCompleteManager* clang_complete,
    IncludeComplete* include_complete,
    CodeCompleteCache* global_code_complete_cache,
    CodeCompleteCache* non_global_code_complete_cache,
    CodeCompleteCache* signature_cache) {
  IpcManager* ipc = IpcManager::instance();

  bool did_work = false;

  std::vector<std::unique_ptr<BaseIpcMessage>> messages = ipc->GetMessages(IpcManager::Destination::Server);
  for (auto& message : messages) {
    did_work = true;
    //std::cerr << "[querydb] Processing message " << IpcIdToString(message->method_id) << std::endl;

    switch (message->method_id) {
      case IpcId::Initialize: {
        auto request = static_cast<Ipc_InitializeRequest*>(message.get());

        // Log initialization parameters.
        rapidjson::StringBuffer output;
        Writer writer(output);
        Reflect(writer, request->params.initializationOptions);
        std::cerr << output.GetString() << std::endl;

        if (request->params.rootUri) {
          std::string project_path = request->params.rootUri->GetPath();
          std::cerr << "[querydb] Initialize in directory " << project_path
            << " with uri " << request->params.rootUri->raw_uri
            << std::endl;

          if (!request->params.initializationOptions) {
            std::cerr << "Initialization parameters (particularily cacheDirectory) are required" << std::endl;
            exit(1);
          }

          *config = *request->params.initializationOptions;

          // Check client version.
          if (config->clientVersion != kExpectedClientVersion) {
            Out_ShowLogMessage out;
            out.display_type = Out_ShowLogMessage::DisplayType::Show;
            out.params.type = lsMessageType::Error;
            out.params.message = "cquery client (v" + std::to_string(config->clientVersion) + ") and server (v" + std::to_string(kExpectedClientVersion) + ") version mismatch. Please update ";
            if (config->clientVersion > kExpectedClientVersion)
              out.params.message += "the cquery binary.";
            else
              out.params.message += "your extension client (VSIX file).";
            out.Write(std::cout);
          }

          // Make sure cache directory is valid.
          if (config->cacheDirectory.empty()) {
            std::cerr << "[fatal] No cache directory" << std::endl;
            exit(1);
          }
          config->cacheDirectory = NormalizePath(config->cacheDirectory);
          EnsureEndsInSlash(config->cacheDirectory);
          MakeDirectoryRecursive(config->cacheDirectory);

          // Set project root.
          config->projectRoot = NormalizePath(request->params.rootUri->GetPath());
          EnsureEndsInSlash(config->projectRoot);

          // Start indexer threads.
          int indexer_count = std::max<int>(std::thread::hardware_concurrency(), 2) - 1;
          if (config->indexerCount > 0)
            indexer_count = config->indexerCount;
          std::cerr << "[querydb] Starting " << indexer_count << " indexers" << std::endl;
          for (int i = 0; i < indexer_count; ++i) {
            new std::thread([&]() {
              IndexMain(config, file_consumer_shared, project, working_files, waiter, queue_do_index, queue_do_id_map, queue_on_id_mapped, queue_on_indexed);
            });
          }

          Timer time;

          // Open up / load the project.
          project->Load(config->extraClangArguments, project_path);
          time.ResetAndPrint("[perf] Loaded compilation entries (" + std::to_string(project->entries.size()) + " files)");

          // Start scanning include directories before dispatching project files, because that takes a long time.
          include_complete->Rescan();

          time.Reset();
          project->ForAllFilteredFiles(config, [&](int i, const Project::Entry& entry) {
            //std::cerr << "[" << i << "/" << (project->entries.size() - 1)
            //  << "] Dispatching index request for file " << entry.filename
            //  << std::endl;
            queue_do_index->Enqueue(Index_DoIndex(Index_DoIndex::Type::ImportThenParse, entry, nullopt, false /*is_interactive*/));
          });
          time.ResetAndPrint("[perf] Dispatched initial index requests");
        }

        // TODO: query request->params.capabilities.textDocument and support only things
        // the client supports.

        auto response = Out_InitializeResponse();
        response.id = request->id;

        //response.result.capabilities.textDocumentSync = lsTextDocumentSyncOptions();
        //response.result.capabilities.textDocumentSync->openClose = true;
        //response.result.capabilities.textDocumentSync->change = lsTextDocumentSyncKind::Full;
        //response.result.capabilities.textDocumentSync->willSave = true;
        //response.result.capabilities.textDocumentSync->willSaveWaitUntil = true;
        response.result.capabilities.textDocumentSync = lsTextDocumentSyncKind::Incremental;

        response.result.capabilities.renameProvider = true;

        response.result.capabilities.completionProvider = lsCompletionOptions();
        response.result.capabilities.completionProvider->resolveProvider = false;
        // vscode doesn't support trigger character sequences, so we use ':' for '::' and '>' for '->'.
        // See https://github.com/Microsoft/language-server-protocol/issues/138.
        response.result.capabilities.completionProvider->triggerCharacters = { ".", ":", ">", "#" };

        response.result.capabilities.signatureHelpProvider = lsSignatureHelpOptions();
        // NOTE: If updating signature help tokens make sure to also update
        // WorkingFile::FindClosestCallNameInBuffer.
        response.result.capabilities.signatureHelpProvider->triggerCharacters = { "(", "," };

        response.result.capabilities.codeLensProvider = lsCodeLensOptions();
        response.result.capabilities.codeLensProvider->resolveProvider = false;

        response.result.capabilities.definitionProvider = true;
        response.result.capabilities.documentHighlightProvider = true;
        response.result.capabilities.hoverProvider = true;
        response.result.capabilities.referencesProvider = true;

        response.result.capabilities.codeActionProvider = true;

        response.result.capabilities.documentSymbolProvider = true;
        response.result.capabilities.workspaceSymbolProvider = true;

        response.result.capabilities.documentLinkProvider = lsDocumentLinkOptions();
        response.result.capabilities.documentLinkProvider->resolveProvider = false;

        ipc->SendOutMessageToClient(IpcId::Initialize, response);
        break;
      }

      case IpcId::Exit: {
        exit(0);
        break;
      }

      case IpcId::CqueryFreshenIndex: {
        std::cerr << "Freshening " << project->entries.size() << " files" << std::endl;
        project->ForAllFilteredFiles(config, [&](int i, const Project::Entry& entry) {
          std::cerr << "[" << i << "/" << (project->entries.size() - 1)
            << "] Dispatching index request for file " << entry.filename
            << std::endl;
          queue_do_index->Enqueue(Index_DoIndex(Index_DoIndex::Type::Freshen, entry, nullopt, false /*is_interactive*/));
        });
        break;
      }

      case IpcId::CqueryTypeHierarchyTree: {
        auto msg = static_cast<Ipc_CqueryTypeHierarchyTree*>(message.get());

        QueryFile* file;
        if (!FindFileOrFail(db, msg->id, msg->params.textDocument.uri.GetPath(), &file))
          break;

        WorkingFile* working_file = working_files->GetFileByFilename(file->def.path);

        Out_CqueryTypeHierarchyTree response;
        response.id = msg->id;

        for (const SymbolRef& ref : FindSymbolsAtLocation(working_file, file, msg->params.position)) {
          if (ref.idx.kind == SymbolKind::Type) {
            response.result = BuildTypeHierarchy(db, working_files, QueryTypeId(ref.idx.idx));
            break;
          }
        }

        ipc->SendOutMessageToClient(IpcId::CqueryTypeHierarchyTree, response);
        break;
      }

      case IpcId::CqueryCallTreeInitial: {
        auto msg = static_cast<Ipc_CqueryCallTreeInitial*>(message.get());

        QueryFile* file;
        if (!FindFileOrFail(db, msg->id, msg->params.textDocument.uri.GetPath(), &file))
          break;

        WorkingFile* working_file = working_files->GetFileByFilename(file->def.path);

        Out_CqueryCallTree response;
        response.id = msg->id;

        for (const SymbolRef& ref : FindSymbolsAtLocation(working_file, file, msg->params.position)) {
          if (ref.idx.kind == SymbolKind::Func) {
            response.result = BuildInitialCallTree(db, working_files, QueryFuncId(ref.idx.idx));
            break;
          }
        }

        ipc->SendOutMessageToClient(IpcId::CqueryCallTreeInitial, response);
        break;
      }

      case IpcId::CqueryCallTreeExpand: {
        auto msg = static_cast<Ipc_CqueryCallTreeExpand*>(message.get());

        Out_CqueryCallTree response;
        response.id = msg->id;

        auto func_id = db->usr_to_func.find(msg->params.usr);
        if (func_id != db->usr_to_func.end())
          response.result = BuildExpandCallTree(db, working_files, func_id->second);

        ipc->SendOutMessageToClient(IpcId::CqueryCallTreeExpand, response);
        break;
      }

      case IpcId::CqueryVars: {
        auto msg = static_cast<Ipc_CqueryVars*>(message.get());

        QueryFile* file;
        if (!FindFileOrFail(db, msg->id, msg->params.textDocument.uri.GetPath(), &file))
          break;

        WorkingFile* working_file = working_files->GetFileByFilename(file->def.path);

        Out_LocationList response;
        response.id = msg->id;
        for (const SymbolRef& ref : FindSymbolsAtLocation(working_file, file, msg->params.position)) {
          if (ref.idx.kind == SymbolKind::Type) {
            optional<QueryType>& type = db->types[ref.idx.idx];
            if (!type) continue;
            std::vector<QueryLocation> locations = ToQueryLocation(db, type->instances);
            response.result = GetLsLocations(db, working_files, locations);
          }
        }
        ipc->SendOutMessageToClient(IpcId::CqueryVars, response);
        break;
      }

      case IpcId::CqueryCallers: {
        auto msg = static_cast<Ipc_CqueryCallers*>(message.get());

        QueryFile* file;
        if (!FindFileOrFail(db, msg->id, msg->params.textDocument.uri.GetPath(), &file))
          break;

        WorkingFile* working_file = working_files->GetFileByFilename(file->def.path);

        Out_LocationList response;
        response.id = msg->id;
        for (const SymbolRef& ref : FindSymbolsAtLocation(working_file, file, msg->params.position)) {
          if (ref.idx.kind == SymbolKind::Func) {
            optional<QueryFunc>& func = db->funcs[ref.idx.idx];
            if (!func) continue;
            std::vector<QueryLocation> locations = ToQueryLocation(db, func->callers);
            response.result = GetLsLocations(db, working_files, locations);
          }
        }
        ipc->SendOutMessageToClient(IpcId::CqueryCallers, response);
        break;
      }

      case IpcId::CqueryBase: {
        auto msg = static_cast<Ipc_CqueryBase*>(message.get());

        QueryFile* file;
        if (!FindFileOrFail(db, msg->id, msg->params.textDocument.uri.GetPath(), &file))
          break;

        WorkingFile* working_file = working_files->GetFileByFilename(file->def.path);

        Out_LocationList response;
        response.id = msg->id;
        for (const SymbolRef& ref : FindSymbolsAtLocation(working_file, file, msg->params.position)) {
          if (ref.idx.kind == SymbolKind::Type) {
            optional<QueryType>& type = db->types[ref.idx.idx];
            if (!type) continue;
            std::vector<QueryLocation> locations = ToQueryLocation(db, type->def.parents);
            response.result = GetLsLocations(db, working_files, locations);
          }
          else if (ref.idx.kind == SymbolKind::Func) {
            optional<QueryFunc>& func = db->funcs[ref.idx.idx];
            if (!func) continue;
            optional<QueryLocation> location = GetBaseDefinitionOrDeclarationSpelling(db, *func);
            if (!location) continue;
            optional<lsLocation> ls_loc = GetLsLocation(db, working_files, *location);
            if (!ls_loc) continue;
            response.result.push_back(*ls_loc);
          }
        }
        ipc->SendOutMessageToClient(IpcId::CqueryBase, response);
        break;
      }

      case IpcId::CqueryDerived: {
        auto msg = static_cast<Ipc_CqueryDerived*>(message.get());

        QueryFile* file;
        if (!FindFileOrFail(db, msg->id, msg->params.textDocument.uri.GetPath(), &file))
          break;

        WorkingFile* working_file = working_files->GetFileByFilename(file->def.path);

        Out_LocationList response;
        response.id = msg->id;
        for (const SymbolRef& ref : FindSymbolsAtLocation(working_file, file, msg->params.position)) {
          if (ref.idx.kind == SymbolKind::Type) {
            optional<QueryType>& type = db->types[ref.idx.idx];
            if (!type) continue;
            std::vector<QueryLocation> locations = ToQueryLocation(db, type->derived);
            response.result = GetLsLocations(db, working_files, locations);
          }
          else if (ref.idx.kind == SymbolKind::Func) {
            optional<QueryFunc>& func = db->funcs[ref.idx.idx];
            if (!func) continue;
            std::vector<QueryLocation> locations = ToQueryLocation(db, func->derived);
            response.result = GetLsLocations(db, working_files, locations);
          }
        }
        ipc->SendOutMessageToClient(IpcId::CqueryDerived, response);
        break;
      }










      case IpcId::TextDocumentDidOpen: {
        // NOTE: This function blocks code lens. If it starts taking a long time
        // we will need to find a way to unblock the code lens request.

        Timer time;
        auto msg = static_cast<Ipc_TextDocumentDidOpen*>(message.get());
        std::string path = msg->params.textDocument.uri.GetPath();
        WorkingFile* working_file = working_files->OnOpen(msg->params);
        optional<std::string> cached_file_contents = LoadCachedFileContents(config, path);
        if (cached_file_contents)
          working_file->SetIndexContent(*cached_file_contents);
        else
          working_file->SetIndexContent(working_file->buffer_content);

        std::unique_ptr<IndexFile> cache = LoadCachedIndex(config, path);
        if (cache && !cache->skipped_by_preprocessor.empty())
          PublishInactiveLines(working_file, cache->skipped_by_preprocessor);

        time.ResetAndPrint("[querydb] Loading cached index file for DidOpen (blocks CodeLens)");

        include_complete->AddFile(working_file->filename);
        clang_complete->NotifyView(path);

        break;
      }

      case IpcId::TextDocumentDidChange: {
        auto msg = static_cast<Ipc_TextDocumentDidChange*>(message.get());
        std::string path = msg->params.textDocument.uri.GetPath();
        working_files->OnChange(msg->params);
        clang_complete->NotifyEdit(path);
        break;
      }

      case IpcId::TextDocumentDidClose: {
        auto msg = static_cast<Ipc_TextDocumentDidClose*>(message.get());

        // Clear any diagnostics for the file.
        Out_TextDocumentPublishDiagnostics diag;
        diag.params.uri = msg->params.textDocument.uri;
        IpcManager::instance()->SendOutMessageToClient(IpcId::TextDocumentPublishDiagnostics, diag);

        // Remove internal state.
        working_files->OnClose(msg->params);

        break;
      }

      case IpcId::TextDocumentDidSave: {
        auto msg = static_cast<Ipc_TextDocumentDidSave*>(message.get());

        std::string path = msg->params.textDocument.uri.GetPath();
        // Send out an index request, and copy the current buffer state so we
        // can update the cached index contents when the index is done.
        //
        // We also do not index if there is already an index request.
        //
        // TODO: Cancel outgoing index request. Might be tricky to make
        //       efficient since we have to cancel.
        //    - we could have an |atomic<int> active_cancellations| variable
        //      that all of the indexers check before accepting an index. if
        //      zero we don't slow down fast-path. if non-zero we acquire
        //      mutex and check to see if we should skip the current request.
        //      if so, ignore that index response.
        WorkingFile* working_file = working_files->GetFileByFilename(path);
        if (working_file) {
          // Only do a delta update (Type::Parse) if we've already imported the
          // file. If the user saves a file not loaded by the project we don't
          // want the initial import to be a delta-update.
          Index_DoIndex::Type index_type = Index_DoIndex::Type::Parse;
          QueryFile* file = FindFile(db, path);
          if (!file)
            index_type = Index_DoIndex::Type::ImportThenParse;

          queue_do_index->PriorityEnqueue(Index_DoIndex(index_type, project->FindCompilationEntryForFile(path), working_file->buffer_content, true /*is_interactive*/));
        }

        clang_complete->NotifySave(path);

        break;
      }

      case IpcId::TextDocumentRename: {
        auto msg = static_cast<Ipc_TextDocumentRename*>(message.get());

        QueryFileId file_id;
        QueryFile* file;
        if (!FindFileOrFail(db, msg->id, msg->params.textDocument.uri.GetPath(), &file, &file_id))
          break;

        WorkingFile* working_file = working_files->GetFileByFilename(file->def.path);

        Out_TextDocumentRename response;
        response.id = msg->id;

        for (const SymbolRef& ref : FindSymbolsAtLocation(working_file, file, msg->params.position)) {
          // Found symbol. Return references to rename.
          std::vector<QueryLocation> uses = GetUsesOfSymbol(db, ref.idx);
          response.result = BuildWorkspaceEdit(db, working_files, uses, msg->params.newName);
          break;
        }

        ipc->SendOutMessageToClient(IpcId::TextDocumentRename, response);
        break;
      }

      case IpcId::TextDocumentCompletion: {
        auto msg = static_cast<Ipc_TextDocumentComplete*>(message.get());

        std::string path = msg->params.textDocument.uri.GetPath();
        WorkingFile* file = working_files->GetFileByFilename(path);

        // It shouldn't be possible, but sometimes vscode will send queries out
        // of order, ie, we get completion request before buffer content update.
        std::string buffer_line;
        if (msg->params.position.line >= 0 && msg->params.position.line < file->all_buffer_lines.size())
          buffer_line = file->all_buffer_lines[msg->params.position.line];

        if (ShouldRunIncludeCompletion(buffer_line)) {
          Out_TextDocumentComplete complete_response;
          complete_response.id = msg->id;
          complete_response.result.isIncomplete = false;

          {
            std::unique_lock<std::mutex> lock(include_complete->completion_items_mutex, std::defer_lock);
            if (include_complete->is_scanning)
              lock.lock();
            complete_response.result.items.assign(
              include_complete->completion_items.begin(),
              include_complete->completion_items.end());
            if (lock)
              lock.unlock();

            // Update textEdit params.
            for (lsCompletionItem& item : complete_response.result.items) {
              item.textEdit->range.start.line = msg->params.position.line;
              item.textEdit->range.start.character = 0;
              item.textEdit->range.end.line = msg->params.position.line;
              item.textEdit->range.end.character = (int)buffer_line.size();
            }
          }

          std::cerr << "[complete] Returning " << complete_response.result.items.size() << " include completions" << std::endl;
          ipc->SendOutMessageToClient(IpcId::TextDocumentCompletion, complete_response);
        }
        else {
          bool is_global_completion = false;
          if (file)
            msg->params.position = file->FindStableCompletionSource(msg->params.position, &is_global_completion);

          ClangCompleteManager::OnComplete callback = std::bind(
            [working_files, global_code_complete_cache, non_global_code_complete_cache, is_global_completion]
            (Ipc_TextDocumentComplete* msg, NonElidedVector<lsCompletionItem> results) {

            Out_TextDocumentComplete complete_response;
            complete_response.id = msg->id;
            complete_response.result.isIncomplete = false;
            complete_response.result.items = results;

            // Emit completion results.
            IpcManager::instance()->SendOutMessageToClient(IpcId::TextDocumentCompletion, complete_response);

            // Cache completion results.
            std::string path = msg->params.textDocument.uri.GetPath();
            if (is_global_completion) {
              global_code_complete_cache->cached_path = path;
              global_code_complete_cache->cached_results = results;
            }
            else {
              non_global_code_complete_cache->cached_path = path;
              non_global_code_complete_cache->cached_completion_position = msg->params.position;
              non_global_code_complete_cache->cached_results = results;
            }

            delete msg;
          }, static_cast<Ipc_TextDocumentComplete*>(message.release()), std::placeholders::_1);

          if (is_global_completion && global_code_complete_cache->cached_path == path && !global_code_complete_cache->cached_results.empty()) {
            std::cerr << "[complete] Early-returning cached global completion results at " << msg->params.position.ToString() << std::endl;

            ClangCompleteManager::OnComplete freshen_global =
              [global_code_complete_cache]
              (NonElidedVector<lsCompletionItem> results) {

              std::cerr << "[complete] Updated global completion cache" << std::endl;
              // note: path is updated in the normal completion handler.
              global_code_complete_cache->cached_results = results;
            };
            clang_complete->CodeComplete(msg->params, std::move(freshen_global));

            // Note: callback will delete the message (ie, |params|) so we need to run completion_manager->CodeComplete before |callback|.
            callback(global_code_complete_cache->cached_results);
          }
          else if (non_global_code_complete_cache->IsCacheValid(msg->params)) {
            std::cerr << "[complete] Using cached completion results at " << msg->params.position.ToString() << std::endl;
            callback(non_global_code_complete_cache->cached_results);
          }
          else {
            clang_complete->CodeComplete(msg->params, std::move(callback));
          }
        }

        break;
      }

      case IpcId::TextDocumentSignatureHelp: {
        auto msg = static_cast<Ipc_TextDocumentSignatureHelp*>(message.get());
        lsTextDocumentPositionParams& params = msg->params;
        WorkingFile* file = working_files->GetFileByFilename(params.textDocument.uri.GetPath());
        std::string search;
        int active_param = 0;
        if (file) {
          lsPosition completion_position;
          search = file->FindClosestCallNameInBuffer(params.position, &active_param, &completion_position);
          params.position = completion_position;
        }
        //std::cerr << "[completion] Returning signatures for " << search << std::endl;
        if (search.empty())
          break;

        ClangCompleteManager::OnComplete callback = std::bind([signature_cache](BaseIpcMessage* message, std::string search, int active_param, const NonElidedVector<lsCompletionItem>& results) {
          auto msg = static_cast<Ipc_TextDocumentSignatureHelp*>(message);
          auto ipc = IpcManager::instance();

          Out_TextDocumentSignatureHelp response;
          response.id = msg->id;

          for (auto& result : results) {
            if (result.label != search)
              continue;

            lsSignatureInformation signature;
            signature.label = result.detail;
            for (auto& parameter : result.parameters_) {
              lsParameterInformation ls_param;
              ls_param.label = parameter;
              signature.parameters.push_back(ls_param);
            }
            response.result.signatures.push_back(signature);
          }

          // Guess the signature the user wants based on available parameter
          // count.
          response.result.activeSignature = 0;
          for (size_t i = 0; i < response.result.signatures.size(); ++i) {
            if (active_param < response.result.signatures.size()) {
              response.result.activeSignature = (int)i;
              break;
            }
          }

          // Set signature to what we parsed from the working file.
          response.result.activeParameter = active_param;

          Timer timer;
          ipc->SendOutMessageToClient(IpcId::TextDocumentSignatureHelp, response);
          timer.ResetAndPrint("[complete] Writing signature help results");

          signature_cache->cached_path = msg->params.textDocument.uri.GetPath();
          signature_cache->cached_completion_position = msg->params.position;
          signature_cache->cached_results = results;

          delete message;
        }, message.release(), search, active_param, std::placeholders::_1);

        if (signature_cache->IsCacheValid(params)) {
          std::cerr << "[complete] Using cached completion results at " << params.position.ToString() << std::endl;
          callback(signature_cache->cached_results);
        }
        else {
          clang_complete->CodeComplete(params, std::move(callback));
        }

        break;
      }

      case IpcId::TextDocumentDefinition: {
        auto msg = static_cast<Ipc_TextDocumentDefinition*>(message.get());

        QueryFileId file_id;
        QueryFile* file;
        if (!FindFileOrFail(db, msg->id, msg->params.textDocument.uri.GetPath(), &file, &file_id))
          break;

        WorkingFile* working_file = working_files->GetFileByFilename(file->def.path);

        Out_TextDocumentDefinition response;
        response.id = msg->id;

        int target_line = msg->params.position.line + 1;
        int target_column = msg->params.position.character + 1;

        for (const SymbolRef& ref : FindSymbolsAtLocation(working_file, file, msg->params.position)) {
          // Found symbol. Return definition.

          // Special cases which are handled:
          //  - symbol has declaration but no definition (ie, pure virtual)
          //  - start at spelling but end at extent for better mouse tooltip
          //  - goto declaration while in definition of recursive type

          optional<QueryLocation> def_loc = GetDefinitionSpellingOfSymbol(db, ref.idx);

          // We use spelling start and extent end because this causes vscode to
          // highlight the entire definition when previewing / hoving with the
          // mouse.
          optional<QueryLocation> def_extent = GetDefinitionExtentOfSymbol(db, ref.idx);
          if (def_loc && def_extent)
            def_loc->range.end = def_extent->range.end;

          // If the cursor is currently at or in the definition we should goto
          // the declaration if possible. We also want to use declarations if
          // we're pointing to, ie, a pure virtual function which has no
          // definition.
          if (!def_loc || (def_loc->path == file_id &&
                            def_loc->range.Contains(target_line, target_column))) {
            // Goto declaration.

            std::vector<QueryLocation> declarations = GetDeclarationsOfSymbolForGotoDefinition(db, ref.idx);
            for (auto declaration : declarations) {
              optional<lsLocation> ls_declaration = GetLsLocation(db, working_files, declaration);
              if (ls_declaration)
                response.result.push_back(*ls_declaration);
            }
            // We found some declarations. Break so we don't add the definition location.
            if (!response.result.empty())
              break;
          }

          if (def_loc)
            PushBack(&response.result, GetLsLocation(db, working_files, *def_loc));

          if (!response.result.empty())
            break;
        }

        // No symbols - check for includes.
        if (response.result.empty()) {
          for (const IndexInclude& include : file->def.includes) {
            if (include.line == target_line) {
              lsLocation result;
              result.uri = lsDocumentUri::FromPath(include.resolved_path);
              response.result.push_back(result);
              break;
            }
          }
        }

        ipc->SendOutMessageToClient(IpcId::TextDocumentDefinition, response);
        break;
      }

      case IpcId::TextDocumentDocumentHighlight: {
        auto msg = static_cast<Ipc_TextDocumentDocumentHighlight*>(message.get());

        QueryFileId file_id;
        QueryFile* file;
        if (!FindFileOrFail(db, msg->id, msg->params.textDocument.uri.GetPath(), &file, &file_id))
          break;

        WorkingFile* working_file = working_files->GetFileByFilename(file->def.path);

        Out_TextDocumentDocumentHighlight response;
        response.id = msg->id;

        for (const SymbolRef& ref : FindSymbolsAtLocation(working_file, file, msg->params.position)) {
          // Found symbol. Return references to highlight.
          std::vector<QueryLocation> uses = GetUsesOfSymbol(db, ref.idx);
          response.result.reserve(uses.size());
          for (const QueryLocation& use : uses) {
            if (use.path != file_id)
              continue;

            optional<lsLocation> ls_location = GetLsLocation(db, working_files, use);
            if (!ls_location)
              continue;

            lsDocumentHighlight highlight;
            highlight.kind = lsDocumentHighlightKind::Text;
            highlight.range = ls_location->range;
            response.result.push_back(highlight);
          }
          break;
        }

        ipc->SendOutMessageToClient(IpcId::TextDocumentDocumentHighlight, response);
        break;
      }

      case IpcId::TextDocumentHover: {
        auto msg = static_cast<Ipc_TextDocumentHover*>(message.get());

        QueryFile* file;
        if (!FindFileOrFail(db, msg->id, msg->params.textDocument.uri.GetPath(), &file))
          break;

        WorkingFile* working_file = working_files->GetFileByFilename(file->def.path);

        Out_TextDocumentHover response;
        response.id = msg->id;

        for (const SymbolRef& ref : FindSymbolsAtLocation(working_file, file, msg->params.position)) {
          // Found symbol. Return hover.
          optional<lsRange> ls_range = GetLsRange(working_files->GetFileByFilename(file->def.path), ref.loc.range);
          if (!ls_range)
            continue;

          response.result.contents = GetHoverForSymbol(db, ref.idx);
          response.result.range = *ls_range;
          break;
        }

        ipc->SendOutMessageToClient(IpcId::TextDocumentHover, response);
        break;
      }

      case IpcId::TextDocumentReferences: {
        auto msg = static_cast<Ipc_TextDocumentReferences*>(message.get());

        QueryFile* file;
        if (!FindFileOrFail(db, msg->id, msg->params.textDocument.uri.GetPath(), &file))
          break;

        WorkingFile* working_file = working_files->GetFileByFilename(file->def.path);

        Out_TextDocumentReferences response;
        response.id = msg->id;

        for (const SymbolRef& ref : FindSymbolsAtLocation(working_file, file, msg->params.position)) {
          optional<QueryLocation> excluded_declaration;
          if (!msg->params.context.includeDeclaration) {
            std::cerr << "Excluding declaration in references" << std::endl;
            excluded_declaration = GetDefinitionSpellingOfSymbol(db, ref.idx);
          }

          // Found symbol. Return references.
          std::vector<QueryLocation> uses = GetUsesOfSymbol(db, ref.idx);
          response.result.reserve(uses.size());
          for (const QueryLocation& use : uses) {
            if (excluded_declaration.has_value() && use == *excluded_declaration)
              continue;

            optional<lsLocation> ls_location = GetLsLocation(db, working_files, use);
            if (ls_location)
              response.result.push_back(*ls_location);
          }
          break;
        }

        ipc->SendOutMessageToClient(IpcId::TextDocumentReferences, response);
        break;
      }

      case IpcId::TextDocumentDocumentSymbol: {
        auto msg = static_cast<Ipc_TextDocumentDocumentSymbol*>(message.get());

        Out_TextDocumentDocumentSymbol response;
        response.id = msg->id;

        QueryFile* file;
        if (!FindFileOrFail(db, msg->id, msg->params.textDocument.uri.GetPath(), &file))
          break;


        for (SymbolRef ref : file->def.outline) {
          optional<lsSymbolInformation> info = GetSymbolInfo(db, working_files, ref.idx);
          if (!info)
            continue;

          optional<lsLocation> location = GetLsLocation(db, working_files, ref.loc);
          if (!location)
            continue;
          info->location = *location;
          response.result.push_back(*info);
        }

        ipc->SendOutMessageToClient(IpcId::TextDocumentDocumentSymbol, response);
        break;
      }

      case IpcId::TextDocumentDocumentLink: {
        auto msg = static_cast<Ipc_TextDocumentDocumentLink*>(message.get());

        Out_TextDocumentDocumentLink response;
        response.id = msg->id;

        if (config->showDocumentLinksOnIncludes) {
          QueryFile* file;
          if (!FindFileOrFail(db, msg->id, msg->params.textDocument.uri.GetPath(), &file))
            break;

          WorkingFile* working_file = working_files->GetFileByFilename(msg->params.textDocument.uri.GetPath());
          if (!working_file) {
            std::cerr << "Unable to find working file " << msg->params.textDocument.uri.GetPath() << std::endl;
            break;
          }
          for (const IndexInclude& include : file->def.includes) {
            optional<int> buffer_line;
            optional<std::string> buffer_line_content = working_file->GetBufferLineContentFromIndexLine(include.line, &buffer_line);
            if (!buffer_line || !buffer_line_content)
              continue;

            // Subtract 1 from line because querydb stores 1-based lines but
            // vscode expects 0-based lines.
            optional<lsRange> between_quotes = ExtractQuotedRange(*buffer_line - 1, *buffer_line_content);
            if (!between_quotes)
              continue;

            lsDocumentLink link;
            link.target = lsDocumentUri::FromPath(include.resolved_path);
            link.range = *between_quotes;
            response.result.push_back(link);
          }
        }

        ipc->SendOutMessageToClient(IpcId::TextDocumentDocumentLink, response);
        break;
      }

      case IpcId::TextDocumentCodeAction: {
        // NOTE: This code snippet will generate some FixIts for testing:
        //
        //    struct origin { int x, int y };
        //    void foo() {
        //      point origin = {
        //        x: 0.0,
        //        y: 0.0
        //      };
        //    }
        //
        auto msg = static_cast<Ipc_TextDocumentCodeAction*>(message.get());

        QueryFileId file_id;
        QueryFile* file;
        if (!FindFileOrFail(db, msg->id, msg->params.textDocument.uri.GetPath(), &file, &file_id))
          break;

        WorkingFile* working_file = working_files->GetFileByFilename(msg->params.textDocument.uri.GetPath());
        if (!working_file) {
          // TODO: send error response.
          std::cerr << "[error] textDocument/codeAction could not find working file" << std::endl;
          break;
        }

        Out_TextDocumentCodeAction response;
        response.id = msg->id;

        // TODO: auto-insert namespace?

        int default_line = (int)working_file->all_buffer_lines.size();

        // Make sure to call EnsureImplFile before using these. We lazy load
        // them because computing the values could involve an entire project
        // scan.
        optional<lsDocumentUri> impl_uri;
        optional<QueryFileId> impl_file_id;

        std::vector<SymbolRef> syms = FindSymbolsAtLocation(working_file, file, msg->params.range.start);
        for (SymbolRef sym : syms) {
          switch (sym.idx.kind) {
            case SymbolKind::Type: {
              optional<QueryType>& type = db->types[sym.idx.idx];
              if (!type)
                break;

              int num_edits = 0;

              // Get implementation file.
              Out_TextDocumentCodeAction::Command command;

              for (QueryFuncId func_id : type->def.funcs) {
                optional<QueryFunc>& func_def = db->funcs[func_id.id];
                if (!func_def || func_def->def.definition_extent)
                  continue;

                EnsureImplFile(db, file_id, impl_uri /*out*/, impl_file_id /*out*/);
                optional<lsTextEdit> edit = BuildAutoImplementForFunction(db, working_files, working_file, default_line, file_id, *impl_file_id, *func_def);
                if (!edit)
                  continue;

                ++num_edits;

                // Merge edits together if they are on the same line.
                // TODO: be smarter about newline merging? ie, don't end up
                //       with foo()\n\n\n\nfoo(), we want foo()\n\nfoo()\n\n
                //
                if (!command.arguments.edits.empty() &&
                    command.arguments.edits[command.arguments.edits.size() - 1].range.end.line == edit->range.start.line) {
                  command.arguments.edits[command.arguments.edits.size() - 1].newText += edit->newText;
                }
                else {
                  command.arguments.edits.push_back(*edit);
                }
              }
              if (command.arguments.edits.empty())
                break;

              // If we're inserting at the end of the document, put a newline before the insertion.
              if (command.arguments.edits[0].range.start.line >= default_line)
                command.arguments.edits[0].newText.insert(0, "\n");

              command.arguments.textDocumentUri = *impl_uri;
              command.title = "Auto-Implement " + std::to_string(num_edits) + " methods on " + type->def.short_name;
              command.command = "cquery._autoImplement";
              response.result.push_back(command);
              break;
            }

            case SymbolKind::Func: {
              optional<QueryFunc>& func = db->funcs[sym.idx.idx];
              if (!func || func->def.definition_extent)
                break;

              EnsureImplFile(db, file_id, impl_uri /*out*/, impl_file_id /*out*/);

              // Get implementation file.
              Out_TextDocumentCodeAction::Command command;
              command.title = "Auto-Implement " + func->def.short_name;
              command.command = "cquery._autoImplement";
              command.arguments.textDocumentUri = *impl_uri;
              optional<lsTextEdit> edit = BuildAutoImplementForFunction(db, working_files, working_file, default_line, file_id, *impl_file_id, *func);
              if (!edit)
                break;

              // If we're inserting at the end of the document, put a newline before the insertion.
              if (edit->range.start.line >= default_line)
                edit->newText.insert(0, "\n");
              command.arguments.edits.push_back(*edit);
              response.result.push_back(command);
              break;
            }
            default:
              break;
          }

          // Only show one auto-impl section.
          if (!response.result.empty())
            break;
        }

        std::vector<lsDiagnostic> diagnostics;
        working_files->DoAction([&]() {
          diagnostics = working_file->diagnostics_;
        });
        for (lsDiagnostic& diag : diagnostics) {
          if (diag.range.start.line != msg->params.range.start.line)
            continue;

          // For error diagnostics, provide an action to resolve an include.
          // TODO: find a way to index diagnostic contents so line numbers
          // don't get mismatched when actively editing a file.
          std::string include_query = LexWordAroundPos(diag.range.start, working_file->buffer_content);
          if (diag.severity == lsDiagnosticSeverity::Error && !include_query.empty()) {
            const size_t kMaxResults = 20;


            std::unordered_set<std::string> include_absolute_paths;

            // Find include candidate strings.
            for (int i = 0; i < db->detailed_names.size(); ++i) {
              if (include_absolute_paths.size() > kMaxResults)
                break;
              if (db->detailed_names[i].find(include_query) == std::string::npos)
                continue;

              optional<QueryFileId> decl_file_id = GetDeclarationFileForSymbol(db, db->symbols[i]);
              if (!decl_file_id)
                continue;

              optional<QueryFile>& decl_file = db->files[decl_file_id->id];
              if (!decl_file)
                continue;

              include_absolute_paths.insert(decl_file->def.path);
            }

            // Build include strings.
            std::unordered_set<std::string> include_insert_strings;
            include_insert_strings.reserve(include_absolute_paths.size());

            for (const std::string& path : include_absolute_paths) {
              optional<lsCompletionItem> item = include_complete->FindCompletionItemForAbsolutePath(path);
              if (!item)
                continue;
              if (item->textEdit)
                include_insert_strings.insert(item->textEdit->newText);
              else if (!item->insertText.empty())
                include_insert_strings.insert(item->insertText);
              else
                assert(false && "unable to determine insert string for include completion item");
            }

            // Build code action.
            if (!include_insert_strings.empty()) {
              Out_TextDocumentCodeAction::Command command;

              // Build edits.
              for (const std::string& include_insert_string : include_insert_strings) {
                lsTextEdit edit;
                optional<int> include_line = FindIncludeLine(working_file->all_buffer_lines, include_insert_string);
                if (!include_line)
                  continue;

                edit.range.start.line = *include_line;
                edit.range.end.line = *include_line;
                edit.newText = include_insert_string + "\n";
                command.arguments.edits.push_back(edit);
              }

              // Setup metadata and send to client.
              if (include_insert_strings.size() == 1)
                command.title = "Insert " + *include_insert_strings.begin();
              else
                command.title = "Pick one of " + std::to_string(include_insert_strings.size()) + " includes to insert";
              command.command = "cquery._insertInclude";
              command.arguments.textDocumentUri = msg->params.textDocument.uri;
              response.result.push_back(command);
            }
          }

          // clang does not provide accurate enough column reporting for
          // diagnostics to do good column filtering, so report all
          // diagnostics on the line.
          if (!diag.fixits_.empty()) {
            Out_TextDocumentCodeAction::Command command;
            command.title = "FixIt: " + diag.message;
            command.command = "cquery._applyFixIt";
            command.arguments.textDocumentUri = msg->params.textDocument.uri;
            command.arguments.edits = diag.fixits_;
            response.result.push_back(command);
          }
        }

        ipc->SendOutMessageToClient(IpcId::TextDocumentCodeAction, response);
        break;
      }

      case IpcId::TextDocumentCodeLens: {
        auto msg = static_cast<Ipc_TextDocumentCodeLens*>(message.get());

        Out_TextDocumentCodeLens response;
        response.id = msg->id;

        lsDocumentUri file_as_uri = msg->params.textDocument.uri;
        std::string path = file_as_uri.GetPath();

        clang_complete->NotifyView(path);

        QueryFile* file;
        if (!FindFileOrFail(db, msg->id, msg->params.textDocument.uri.GetPath(), &file))
          break;

        CommonCodeLensParams common;
        common.result = &response.result;
        common.db = db;
        common.working_files = working_files;
        common.working_file = working_files->GetFileByFilename(file->def.path);

        for (SymbolRef ref : file->def.outline) {
          // NOTE: We OffsetColumn so that the code lens always show up in a
          // predictable order. Otherwise, the client may randomize it.

          SymbolIdx symbol = ref.idx;
          switch (symbol.kind) {
          case SymbolKind::Type: {
            optional<QueryType>& type = db->types[symbol.idx];
            if (!type)
              continue;
            AddCodeLens("ref", "refs", &common, ref.loc.OffsetStartColumn(0), type->uses, type->def.definition_spelling, true /*force_display*/);
            AddCodeLens("derived", "derived", &common, ref.loc.OffsetStartColumn(1), ToQueryLocation(db, type->derived), nullopt, false /*force_display*/);
            AddCodeLens("var", "vars", &common, ref.loc.OffsetStartColumn(2), ToQueryLocation(db, type->instances), nullopt, false /*force_display*/);
            break;
          }
          case SymbolKind::Func: {
            optional<QueryFunc>& func = db->funcs[symbol.idx];
            if (!func)
              continue;

            int16_t offset = 0;

            std::vector<QueryFuncRef> base_callers = GetCallersForAllBaseFunctions(db, *func);
            std::vector<QueryFuncRef> derived_callers = GetCallersForAllDerivedFunctions(db, *func);
            if (base_callers.empty() && derived_callers.empty()) {
              AddCodeLens("call", "calls", &common, ref.loc.OffsetStartColumn(offset++), ToQueryLocation(db, func->callers), nullopt, true /*force_display*/);
            }
            else {
              AddCodeLens("direct call", "direct calls", &common, ref.loc.OffsetStartColumn(offset++), ToQueryLocation(db, func->callers), nullopt, false /*force_display*/);
              if (!base_callers.empty())
                AddCodeLens("base call", "base calls", &common, ref.loc.OffsetStartColumn(offset++), ToQueryLocation(db, base_callers), nullopt, false /*force_display*/);
              if (!derived_callers.empty())
                AddCodeLens("derived call", "derived calls", &common, ref.loc.OffsetStartColumn(offset++), ToQueryLocation(db, derived_callers), nullopt, false /*force_display*/);
            }

            AddCodeLens("derived", "derived", &common, ref.loc.OffsetStartColumn(offset++), ToQueryLocation(db, func->derived), nullopt, false /*force_display*/);

            // "Base"
            optional<QueryLocation> base_loc = GetBaseDefinitionOrDeclarationSpelling(db, *func);
            if (base_loc) {
              optional<lsLocation> ls_base = GetLsLocation(db, working_files, *base_loc);
              if (ls_base) {
                optional<lsRange> range = GetLsRange(common.working_file, ref.loc.range);
                if (range) {
                  TCodeLens code_lens;
                  code_lens.range = *range;
                  code_lens.range.start.character += offset++;
                  code_lens.command = lsCommand<lsCodeLensCommandArguments>();
                  code_lens.command->title = "Base";
                  code_lens.command->command = "cquery.goto";
                  code_lens.command->arguments.uri = ls_base->uri;
                  code_lens.command->arguments.position = ls_base->range.start;
                  response.result.push_back(code_lens);
                }
              }
            }

            break;
          }
          case SymbolKind::Var: {
            optional<QueryVar>& var = db->vars[symbol.idx];
            if (!var)
              continue;

            if (var->def.is_local && !config->codeLensOnLocalVariables)
              continue;

            AddCodeLens("ref", "refs", &common, ref.loc.OffsetStartColumn(0), var->uses, var->def.definition_spelling, true /*force_display*/);
            break;
          }
          case SymbolKind::File:
          case SymbolKind::Invalid: {
            assert(false && "unexpected");
            break;
          }
          };
        }

        ipc->SendOutMessageToClient(IpcId::TextDocumentCodeLens, response);
        break;
      }

      case IpcId::WorkspaceSymbol: {
        // TODO: implement fuzzy search, see https://github.com/junegunn/fzf/blob/master/src/matcher.go for inspiration
        auto msg = static_cast<Ipc_WorkspaceSymbol*>(message.get());

        Out_WorkspaceSymbol response;
        response.id = msg->id;


        std::cerr << "[querydb] Considering " << db->detailed_names.size()
          << " candidates for query " << msg->params.query << std::endl;

        std::string query = msg->params.query;
        for (int i = 0; i < db->detailed_names.size(); ++i) {
          if (response.result.size() >= config->maxWorkspaceSearchResults) {
            std::cerr << "[querydb] Query exceeded maximum number of responses (" << config->maxWorkspaceSearchResults << "), output may not contain all results" << std::endl;
            break;
          }

          if (db->detailed_names[i].find(query) != std::string::npos) {
            optional<lsSymbolInformation> info = GetSymbolInfo(db, working_files, db->symbols[i]);
            if (!info)
              continue;

            optional<QueryLocation> location = GetDefinitionExtentOfSymbol(db, db->symbols[i]);
            if (!location) {
              auto decls = GetDeclarationsOfSymbolForGotoDefinition(db, db->symbols[i]);
              if (decls.empty())
                continue;
              location = decls[0];
            }

            optional<lsLocation> ls_location = GetLsLocation(db, working_files, *location);
            if (!ls_location)
              continue;
            info->location = *ls_location;
            response.result.push_back(*info);
          }
        }

        std::cerr << "[querydb] Found " << response.result.size() << " results for query " << query << std::endl;
        ipc->SendOutMessageToClient(IpcId::WorkspaceSymbol, response);
        break;
      }

      default: {
        std::cerr << "[querydb] Unhandled IPC message " << IpcIdToString(message->method_id) << std::endl;
        exit(1);
      }
    }
  }

  // TODO: consider rate-limiting and checking for IPC messages so we don't block
  // requests / we can serve partial requests.


  while (true) {
    optional<Index_DoIdMap> request = queue_do_id_map->TryDequeue();
    if (!request)
      break;

    did_work = true;

    Index_OnIdMapped response(request->indexed_content, request->perf, request->is_interactive);
    Timer time;

    if (request->previous) {
      response.previous_id_map = MakeUnique<IdMap>(db, request->previous->id_cache);
      response.previous_index = std::move(request->previous);
    }

    assert(request->current);
    response.current_id_map = MakeUnique<IdMap>(db, request->current->id_cache);
    response.current_index = std::move(request->current);
    response.perf.querydb_id_map = time.ElapsedMicrosecondsAndReset();

    queue_on_id_mapped->Enqueue(std::move(response));
  }

  while (true) {
    optional<Index_OnIndexed> response = queue_on_indexed->TryDequeue();
    if (!response)
      break;

    did_work = true;

    Timer time;

    for (auto& updated_file : response->update.files_def_update) {
      // TODO: We're reading a file on querydb thread. This is slow!! If it is a
      // problem in practice we need to create a file reader queue, dispatch the
      // read to it, get a response, and apply the new index then.
      WorkingFile* working_file = working_files->GetFileByFilename(updated_file.path);
      if (working_file) {
        auto it = response->indexed_content.find(updated_file.path);
        if (it != response->indexed_content.end()) {
          working_file->SetIndexContent(it->second);
          time.ResetAndPrint("[querydb] Update WorkingFile index contents (via in-memory buffer) for " + updated_file.path);
        }
        else {
          optional<std::string> cached_file_contents = LoadCachedFileContents(config, updated_file.path);
          if (cached_file_contents)
            working_file->SetIndexContent(*cached_file_contents);
          else
            working_file->SetIndexContent(working_file->buffer_content);
          time.ResetAndPrint("[querydb] Update WorkingFile index contents (via disk load) for " + updated_file.path);
        }
      }
    }

    db->ApplyIndexUpdate(&response->update);
    //time.ResetAndPrint("[querydb] Applying index update");
  }

  return did_work;
}

void QueryDbMain(Config* config, MultiQueueWaiter* waiter) {
  // Create queues.
  Index_DoIndexQueue queue_do_index(waiter);
  Index_DoIdMapQueue queue_do_id_map(waiter);
  Index_OnIdMappedQueue queue_on_id_mapped(waiter);
  Index_OnIndexedQueue queue_on_indexed(waiter);

  Project project;
  WorkingFiles working_files;
  ClangCompleteManager clang_complete(
      config, &project, &working_files,
      std::bind(&EmitDiagnostics, &working_files, std::placeholders::_1, std::placeholders::_2));
  IncludeComplete include_complete(config, &project);
  CodeCompleteCache global_code_complete_cache;
  CodeCompleteCache non_global_code_complete_cache;
  CodeCompleteCache signature_cache;
  FileConsumer::SharedState file_consumer_shared;

  // Run query db main loop.
  SetCurrentThreadName("querydb");
  QueryDatabase db;
  while (true) {
    bool did_work = QueryDbMainLoop(
        config, &db, waiter, &queue_do_index, &queue_do_id_map, &queue_on_id_mapped, &queue_on_indexed,
        &project, &file_consumer_shared, &working_files,
        &clang_complete, &include_complete, &global_code_complete_cache, &non_global_code_complete_cache, &signature_cache);
    if (!did_work) {
      IpcManager* ipc = IpcManager::instance();
      waiter->Wait({
        ipc->threaded_queue_for_server_.get(),
        &queue_do_id_map,
        &queue_on_indexed
      });
    }
  }
}































































// TODO: global lock on stderr output.

// Separate thread whose only job is to read from stdin and
// dispatch read commands to the actual indexer program. This
// cannot be done on the main thread because reading from std::cin
// blocks.
//
// |ipc| is connected to a server.
void LanguageServerStdinLoop(Config* config, std::unordered_map<IpcId, Timer>* request_times) {
  IpcManager* ipc = IpcManager::instance();

  SetCurrentThreadName("stdin");
  while (true) {
    std::unique_ptr<BaseIpcMessage> message = MessageRegistry::instance()->ReadMessageFromStdin();

    // Message parsing can fail if we don't recognize the method.
    if (!message)
      continue;

    (*request_times)[message->method_id] = Timer();

    //std::cerr << "[stdin] Got message " << IpcIdToString(message->method_id) << std::endl;
    switch (message->method_id) {
    case IpcId::Initialized: {
      // TODO: don't send output until we get this notification
      break;
    }

    case IpcId::CancelRequest: {
      // TODO: support cancellation
      break;
    }

    case IpcId::Initialize:
    case IpcId::Exit:
    case IpcId::TextDocumentDidOpen:
    case IpcId::TextDocumentDidChange:
    case IpcId::TextDocumentDidClose:
    case IpcId::TextDocumentDidSave:
    case IpcId::TextDocumentRename:
    case IpcId::TextDocumentCompletion:
    case IpcId::TextDocumentSignatureHelp:
    case IpcId::TextDocumentDefinition:
    case IpcId::TextDocumentDocumentHighlight:
    case IpcId::TextDocumentHover:
    case IpcId::TextDocumentReferences:
    case IpcId::TextDocumentDocumentSymbol:
    case IpcId::TextDocumentDocumentLink:
    case IpcId::TextDocumentCodeAction:
    case IpcId::TextDocumentCodeLens:
    case IpcId::WorkspaceSymbol:
    case IpcId::CqueryFreshenIndex:
    case IpcId::CqueryTypeHierarchyTree:
    case IpcId::CqueryCallTreeInitial:
    case IpcId::CqueryCallTreeExpand:
    case IpcId::CqueryVars:
    case IpcId::CqueryCallers:
    case IpcId::CqueryBase:
    case IpcId::CqueryDerived: {
      ipc->SendMessage(IpcManager::Destination::Server, std::move(message));
      break;
    }

    default: {
      std::cerr << "[stdin] Unhandled IPC message " << IpcIdToString(message->method_id) << std::endl;
      exit(1);
    }
    }
  }
}














































void StdoutMain(std::unordered_map<IpcId, Timer>* request_times, MultiQueueWaiter* waiter) {
  SetCurrentThreadName("stdout");
  IpcManager* ipc = IpcManager::instance();

  while (true) {
    std::vector<std::unique_ptr<BaseIpcMessage>> messages = ipc->GetMessages(IpcManager::Destination::Client);
    if (messages.empty()) {
      waiter->Wait({ipc->threaded_queue_for_client_.get()});
      continue;
    }

    for (auto& message : messages) {
      //std::cerr << "[stdout] Processing message " << IpcIdToString(message->method_id) << std::endl;

      switch (message->method_id) {
        case IpcId::Cout: {
          auto msg = static_cast<Ipc_Cout*>(message.get());

          if (ShouldDisplayIpcTiming(message->method_id)) {
            Timer time = (*request_times)[msg->original_ipc_id];
            time.ResetAndPrint("[e2e] Running " + std::string(IpcIdToString(msg->original_ipc_id)));
          }

          std::cout << msg->content;
          std::cout.flush();
          break;
        }

        default: {
          std::cerr << "[stdout] Unhandled IPC message " << IpcIdToString(message->method_id) << std::endl;
          exit(1);
        }
      }
    }
  }
}

void LanguageServerMain(Config* config, MultiQueueWaiter* waiter) {
  std::unordered_map<IpcId, Timer> request_times;

  // Start stdin reader. Reading from stdin is a blocking operation so this
  // needs a dedicated thread.
  new std::thread([&]() {
    LanguageServerStdinLoop(config, &request_times);
  });

  // Start querydb thread. querydb will start indexer threads as needed.
  new std::thread([&]() {
    QueryDbMain(config, waiter);
  });

  // We run a dedicated thread for writing to stdout because there can be an
  // unknown number of delays when output information.
  StdoutMain(&request_times, waiter);
}



















































int main(int argc, char** argv) {
  MultiQueueWaiter waiter;
  IpcManager::CreateInstance(&waiter);

  //bool loop = true;
  //while (loop)
  //  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  //std::this_thread::sleep_for(std::chrono::seconds(10));

  PlatformInit();
  IndexInit();

  RegisterMessageTypes();

  std::unordered_map<std::string, std::string> options =
    ParseOptions(argc, argv);

  if (HasOption(options, "--test")) {
    doctest::Context context;
    context.applyCommandLine(argc, argv);
    int res = context.run();
    if (context.shouldExit())
      return res;

    for (int i = 0; i < 1; ++i)
      RunTests();

    /*
    for (int i = 0; i < 1; ++i) {
      std::this_thread::sleep_for(std::chrono::seconds(5));
      std::cerr << "[POST] Memory usage: " << GetProcessMemoryUsedInMb() << "mb" << std::endl;
    }
    */

    std::cerr << std::endl << "[Enter] to exit" << std::endl;
    std::cin.get();
    return 0;
  }
  else if (HasOption(options, "--language-server")) {
    //std::cerr << "Running language server" << std::endl;
    Config config;
    LanguageServerMain(&config, &waiter);
    return 0;
  }
  else {
    std::cout << R"help(cquery help:

  cquery is a low-latency C++ language server.

  General:
    --help        Print this help information.
    --language-server
                  Run as a language server. This implements the language
                  server spec over STDIN and STDOUT.
    --test        Run tests. Does nothing if test support is not compiled in.

  Configuration:
    When opening up a directory, cquery will look for a compile_commands.json
    file emitted by your preferred build system. If not present, cquery will
    use a recursive directory listing instead. Command line flags can be
    provided by adding a "clang_args" file in the top-level directory. Each
    line in that file is a separate argument.

    There are also a number of configuration options available when
    initializing the language server - your editor should have tooling to
    describe those options. See |Config| in this source code for a detailed
    list of all currently supported options.
)help";
    return 0;
  }
}



TEST_SUITE("LexFunctionDeclaration");

TEST_CASE("simple") {
  std::string buffer_content = " void Foo(); ";
  lsPosition declaration = CharPos(buffer_content, 'F');
  std::string insert_text;
  int newlines_after_name = 0;

  LexFunctionDeclaration(buffer_content, declaration, nullopt, &insert_text, &newlines_after_name);
  REQUIRE(insert_text == "void Foo() {\n}");
  REQUIRE(newlines_after_name == 0);

  LexFunctionDeclaration(buffer_content, declaration, std::string("Type"), &insert_text, &newlines_after_name);
  REQUIRE(insert_text == "void Type::Foo() {\n}");
  REQUIRE(newlines_after_name == 0);
}

TEST_CASE("ctor") {
  std::string buffer_content = " Foo(); ";
  lsPosition declaration = CharPos(buffer_content, 'F');
  std::string insert_text;
  int newlines_after_name = 0;

  LexFunctionDeclaration(buffer_content, declaration, std::string("Foo"), &insert_text, &newlines_after_name);
  REQUIRE(insert_text == "Foo::Foo() {\n}");
  REQUIRE(newlines_after_name == 0);
}

TEST_CASE("dtor") {
  std::string buffer_content = " ~Foo(); ";
  lsPosition declaration = CharPos(buffer_content, '~');
  std::string insert_text;
  int newlines_after_name = 0;

  LexFunctionDeclaration(buffer_content, declaration, std::string("Foo"), &insert_text, &newlines_after_name);
  REQUIRE(insert_text == "Foo::~Foo() {\n}");
  REQUIRE(newlines_after_name == 0);
}

TEST_CASE("complex return type") {
  std::string buffer_content = " std::vector<int> Foo(); ";
  lsPosition declaration = CharPos(buffer_content, 'F');
  std::string insert_text;
  int newlines_after_name = 0;

  LexFunctionDeclaration(buffer_content, declaration, nullopt, &insert_text, &newlines_after_name);
  REQUIRE(insert_text == "std::vector<int> Foo() {\n}");
  REQUIRE(newlines_after_name == 0);

  LexFunctionDeclaration(buffer_content, declaration, std::string("Type"), &insert_text, &newlines_after_name);
  REQUIRE(insert_text == "std::vector<int> Type::Foo() {\n}");
  REQUIRE(newlines_after_name == 0);
}

TEST_CASE("extra complex return type") {
  std::string buffer_content = " std::function < int() > \n Foo(); ";
  lsPosition declaration = CharPos(buffer_content, 'F');
  std::string insert_text;
  int newlines_after_name = 0;

  LexFunctionDeclaration(buffer_content, declaration, nullopt, &insert_text, &newlines_after_name);
  REQUIRE(insert_text == "std::function < int() > \n Foo() {\n}");
  REQUIRE(newlines_after_name == 0);

  LexFunctionDeclaration(buffer_content, declaration, std::string("Type"), &insert_text, &newlines_after_name);
  REQUIRE(insert_text == "std::function < int() > \n Type::Foo() {\n}");
  REQUIRE(newlines_after_name == 0);
}

TEST_CASE("parameters") {
  std::string buffer_content = "void Foo(int a,\n\n    int b); ";
  lsPosition declaration = CharPos(buffer_content, 'F');
  std::string insert_text;
  int newlines_after_name = 0;

  LexFunctionDeclaration(buffer_content, declaration, nullopt, &insert_text, &newlines_after_name);
  REQUIRE(insert_text == "void Foo(int a,\n\n    int b) {\n}");
  REQUIRE(newlines_after_name == 2);

  LexFunctionDeclaration(buffer_content, declaration, std::string("Type"), &insert_text, &newlines_after_name);
  REQUIRE(insert_text == "void Type::Foo(int a,\n\n    int b) {\n}");
  REQUIRE(newlines_after_name == 2);
}

TEST_SUITE_END();



TEST_SUITE("LexWordAroundPos");

TEST_CASE("edges") {
  std::string content = "Foobar";
  REQUIRE(LexWordAroundPos(CharPos(content, 'F'), content) == "Foobar");
  REQUIRE(LexWordAroundPos(CharPos(content, 'o'), content) == "Foobar");
  REQUIRE(LexWordAroundPos(CharPos(content, 'b'), content) == "Foobar");
  REQUIRE(LexWordAroundPos(CharPos(content, 'a'), content) == "Foobar");
  REQUIRE(LexWordAroundPos(CharPos(content, 'r'), content) == "Foobar");
}

TEST_CASE("simple") {
  std::string content = "  Foobar  ";
  REQUIRE(LexWordAroundPos(CharPos(content, 'F'), content) == "Foobar");
  REQUIRE(LexWordAroundPos(CharPos(content, 'o'), content) == "Foobar");
  REQUIRE(LexWordAroundPos(CharPos(content, 'b'), content) == "Foobar");
  REQUIRE(LexWordAroundPos(CharPos(content, 'a'), content) == "Foobar");
  REQUIRE(LexWordAroundPos(CharPos(content, 'r'), content) == "Foobar");
}

TEST_CASE("underscores and numbers") {
  std::string content = "  _my_t5ype7  ";
  REQUIRE(LexWordAroundPos(CharPos(content, '_'), content) == "_my_t5ype7");
  REQUIRE(LexWordAroundPos(CharPos(content, '5'), content) == "_my_t5ype7");
  REQUIRE(LexWordAroundPos(CharPos(content, 'e'), content) == "_my_t5ype7");
  REQUIRE(LexWordAroundPos(CharPos(content, '7'), content) == "_my_t5ype7");
}

TEST_CASE("dot, dash, colon are skipped") {
  std::string content = "1. 2- 3:";
  REQUIRE(LexWordAroundPos(CharPos(content, '1'), content) == "1");
  REQUIRE(LexWordAroundPos(CharPos(content, '2'), content) == "2");
  REQUIRE(LexWordAroundPos(CharPos(content, '3'), content) == "3");
}

TEST_SUITE_END();



TEST_SUITE("FindIncludeLine");

TEST_CASE("in document") {
  std::vector<std::string> lines = {
    "#include <bbb>",   // 0
    "#include <ddd>"    // 1
  };

  REQUIRE(FindIncludeLine(lines, "#include <bbb>") == nullopt);
}

TEST_CASE("insert before") {
  std::vector<std::string> lines = {
    "#include <bbb>",   // 0
    "#include <ddd>"    // 1
  };

  REQUIRE(FindIncludeLine(lines, "#include <aaa>") == 0);
}

TEST_CASE("insert middle") {
  std::vector<std::string> lines = {
    "#include <bbb>",   // 0
    "#include <ddd>"    // 1
  };

  REQUIRE(FindIncludeLine(lines, "#include <ccc>") == 1);
}

TEST_CASE("insert after") {
  std::vector<std::string> lines = {
    "#include <bbb>",   // 0
    "#include <ddd>",   // 1
    "",                 // 2
  };

  REQUIRE(FindIncludeLine(lines, "#include <eee>") == 2);
}

TEST_CASE("ignore header") {
  std::vector<std::string> lines = {
    "// FOOBAR",        // 0
    "// FOOBAR",        // 1
    "// FOOBAR",        // 2
    "// FOOBAR",        // 3
    "",                 // 4
    "#include <bbb>",   // 5
    "#include <ddd>",   // 6
    "",                 // 7
  };

  REQUIRE(FindIncludeLine(lines, "#include <a>") == 5);
  REQUIRE(FindIncludeLine(lines, "#include <c>") == 6);
  REQUIRE(FindIncludeLine(lines, "#include <e>") == 7);
}

TEST_SUITE_END();
