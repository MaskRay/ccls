#include "message_handler.h"

#include "lex_utils.h"
#include "project.h"
#include "query_utils.h"
#include "queue_manager.h"
#include "semantic_highlight_symbol_cache.h"

#include <loguru.hpp>

#include <algorithm>

namespace {
struct ScanLineEvent {
  lsPosition pos;
  lsPosition end_pos;  // Second key when there is a tie for insertion events.
  int id;
  Out_CqueryPublishSemanticHighlighting::Symbol* symbol;
  bool operator<(const ScanLineEvent& other) const {
    // See the comments below when insertion/deletion events are inserted.
    return !(pos == other.pos) ? pos < other.pos : other.end_pos < end_pos;
  }
};
}  // namespace

MessageHandler::MessageHandler() {
  // Dynamically allocate |message_handlers|, otherwise there will be static
  // initialization order races.
  if (!message_handlers)
    message_handlers = new std::vector<MessageHandler*>();
  message_handlers->push_back(this);
}

// static
std::vector<MessageHandler*>* MessageHandler::message_handlers = nullptr;

bool FindFileOrFail(QueryDatabase* db,
                    const Project* project,
                    optional<lsRequestId> id,
                    const std::string& absolute_path,
                    QueryFile** out_query_file,
                    QueryFileId* out_file_id) {
  *out_query_file = nullptr;

  auto it = db->usr_to_file.find(LowerPathIfCaseInsensitive(absolute_path));
  if (it != db->usr_to_file.end()) {
    QueryFile& file = db->files[it->second.id];
    if (file.def) {
      *out_query_file = &file;
      if (out_file_id)
        *out_file_id = QueryFileId(it->second.id);
      return true;
    }
  }

  if (out_file_id)
    *out_file_id = QueryFileId((size_t)-1);

  bool indexing = project->absolute_path_to_entry_index_.find(absolute_path) !=
                  project->absolute_path_to_entry_index_.end();
  if (indexing)
    LOG_S(INFO) << "\"" << absolute_path << "\" is being indexed.";
  else
    LOG_S(INFO) << "Unable to find file \"" << absolute_path << "\"";
  /*
  LOG_S(INFO) << "Files (size=" << db->usr_to_file.size() << "): "
              << StringJoinMap(db->usr_to_file,
                               [](const std::pair<Usr, QueryFileId>& entry) {
                                 return entry.first;
                               });
  */

  if (id) {
    Out_Error out;
    out.id = *id;
    if (indexing) {
      out.error.code = lsErrorCodes::ServerNotInitialized;
      out.error.message = absolute_path + " is being indexed.";
    } else {
      out.error.code = lsErrorCodes::InternalError;
      out.error.message = "Unable to find file " + absolute_path;
    }
    QueueManager::WriteStdout(IpcId::Unknown, out);
  }

  return false;
}

void EmitInactiveLines(WorkingFile* working_file,
                       const std::vector<Range>& inactive_regions) {
  Out_CquerySetInactiveRegion out;
  out.params.uri = lsDocumentUri::FromPath(working_file->filename);
  for (Range skipped : inactive_regions) {
    optional<lsRange> ls_skipped = GetLsRange(working_file, skipped);
    if (ls_skipped)
      out.params.inactiveRegions.push_back(*ls_skipped);
  }
  QueueManager::WriteStdout(IpcId::CqueryPublishInactiveRegions, out);
}

void EmitSemanticHighlighting(QueryDatabase* db,
                              SemanticHighlightSymbolCache* semantic_cache,
                              WorkingFile* working_file,
                              QueryFile* file) {
  assert(file->def);
  auto map_symbol_kind_to_symbol_type = [](SymbolKind kind) {
    switch (kind) {
      case SymbolKind::Type:
        return Out_CqueryPublishSemanticHighlighting::SymbolType::Type;
      case SymbolKind::Func:
        return Out_CqueryPublishSemanticHighlighting::SymbolType::Function;
      case SymbolKind::Var:
        return Out_CqueryPublishSemanticHighlighting::SymbolType::Variable;
      default:
        assert(false);
        return Out_CqueryPublishSemanticHighlighting::SymbolType::Variable;
    }
  };

  auto semantic_cache_for_file =
      semantic_cache->GetCacheForFile(file->def->path);

  // Group symbols together.
  std::unordered_map<SymbolIdx, Out_CqueryPublishSemanticHighlighting::Symbol>
      grouped_symbols;
  for (SymbolRef sym : file->def->all_symbols) {
    std::string detailed_name;
    bool is_type_member = false;
    ClangSymbolKind kind = ClangSymbolKind::Unknown;
    ClangStorageClass storage = ClangStorageClass::SC_Invalid;
    // This switch statement also filters out symbols that are not highlighted.
    switch (sym.idx.kind) {
      case SymbolKind::Func: {
        QueryFunc* func = &db->funcs[sym.idx.idx];
        if (!func->def)
          continue;  // applies to for loop
        if (func->def->is_operator)
          continue;  // applies to for loop
        kind = func->def->kind;
        is_type_member = func->def->declaring_type.has_value();
        detailed_name = func->def->short_name;

        // TODO We use cursor extent for lambda definition. Without the region
        // shrinking hack, the contained keywords and primitive types will be
        // highlighted undesiredly.
        auto concise_name = detailed_name.substr(0, detailed_name.find('<'));
        if (0 <= sym.loc.range.start.line &&
            sym.loc.range.start.line < working_file->index_lines.size()) {
          const std::string& line =
              working_file->index_lines[sym.loc.range.start.line];
          sym.loc.range.end.line = sym.loc.range.start.line;
          int col = sym.loc.range.start.column;
          if (line.compare(col, concise_name.size(), concise_name) == 0)
            sym.loc.range.end.column =
                sym.loc.range.start.column + concise_name.size();
          else
            sym.loc.range.end.column = sym.loc.range.start.column;
        }
        break;
      }
      case SymbolKind::Var: {
        QueryVar* var = &db->vars[sym.idx.idx];
        if (!var->def)
          continue;  // applies to for loop
        kind = var->def->kind;
        storage = var->def->storage;
        is_type_member = var->def->declaring_type.has_value();
        detailed_name = var->def->short_name;
        break;
      }
      case SymbolKind::Type: {
        QueryType* type = &db->types[sym.idx.idx];
        if (!type->def)
          continue;  // applies to for loop
        kind = type->def->kind;
        detailed_name = type->def->detailed_name;
        break;
      }
      default:
        continue;  // applies to for loop
    }

    optional<lsRange> loc = GetLsRange(working_file, sym.loc.range);
    if (loc) {
      auto it = grouped_symbols.find(sym.idx);
      if (it != grouped_symbols.end()) {
        it->second.ranges.push_back(*loc);
      } else {
        Out_CqueryPublishSemanticHighlighting::Symbol symbol;
        symbol.stableId =
            semantic_cache_for_file->GetStableId(sym.idx.kind, detailed_name);
        symbol.kind = kind;
        symbol.storage = storage;
        symbol.type = map_symbol_kind_to_symbol_type(sym.idx.kind);
        symbol.isTypeMember = is_type_member;
        symbol.ranges.push_back(*loc);
        grouped_symbols[sym.idx] = symbol;
      }
    }
  }

  // Make ranges non-overlapping using a scan line algorithm.
  std::vector<ScanLineEvent> events;
  int id = 0;
  for (auto& entry : grouped_symbols) {
    Out_CqueryPublishSemanticHighlighting::Symbol& symbol = entry.second;
    for (auto& loc : symbol.ranges) {
      // For ranges sharing the same start point, the one with leftmost end
      // point comes first.
      events.push_back({loc.start, loc.end, id, &symbol});
      // For ranges sharing the same end point, their relative order does not
      // matter, therefore we arbitrarily assign loc.end to them. We use
      // negative id to indicate a deletion event.
      events.push_back({loc.end, loc.end, ~id, &symbol});
      id++;
    }
    symbol.ranges.clear();
  }
  std::sort(events.begin(), events.end());

  std::vector<uint8_t> deleted(id, 0);
  int top = 0;
  for (size_t i = 0; i < events.size(); i++) {
    while (top && deleted[events[top - 1].id])
      top--;
    // Order [a, b0) after [a, b1) if b0 < b1. The range comes later overrides
    // the ealier. The order of [a0, b) [a1, b) does not matter.
    // The order of [a, b) [b, c) does not as long as we do not emit empty
    // ranges.
    // Attribute range [events[i-1].pos, events[i].pos) to events[top-1].symbol
    // .
    if (top && !(events[i - 1].pos == events[i].pos))
      events[top - 1].symbol->ranges.emplace_back(events[i - 1].pos,
                                                  events[i].pos);
    if (events[i].id >= 0)
      events[top++] = events[i];
    else
      deleted[~events[i].id] = 1;
  }

  // Publish.
  Out_CqueryPublishSemanticHighlighting out;
  out.params.uri = lsDocumentUri::FromPath(working_file->filename);
  for (auto& entry : grouped_symbols)
    if (entry.second.ranges.size())
      out.params.symbols.push_back(entry.second);
  QueueManager::WriteStdout(IpcId::CqueryPublishSemanticHighlighting, out);
}

bool ShouldIgnoreFileForIndexing(const std::string& path) {
  return StartsWith(path, "git:");
}
