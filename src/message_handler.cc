#include "message_handler.h"

#include "lex_utils.h"
#include "query_utils.h"

#include <loguru.hpp>

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

  LOG_S(INFO) << "Unable to find file \"" << absolute_path << "\"";

  if (id) {
    Out_Error out;
    out.id = *id;
    out.error.code = lsErrorCodes::InternalError;
    out.error.message = "Unable to find file " + absolute_path;
    IpcManager::WriteStdout(IpcId::Unknown, out);
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
  IpcManager::WriteStdout(IpcId::CqueryPublishInactiveRegions, out);
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
    // This switch statement also filters out symbols that are not highlighted.
    switch (sym.idx.kind) {
      case SymbolKind::Func: {
        QueryFunc* func = &db->funcs[sym.idx.idx];
        if (!func->def)
          continue;  // applies to for loop
        if (func->def->is_operator)
          continue;  // applies to for loop
        is_type_member = func->def->declaring_type.has_value();
        detailed_name = func->def->short_name;
        break;
      }
      case SymbolKind::Var: {
        QueryVar* var = &db->vars[sym.idx.idx];
        if (!var->def)
          continue;  // applies to for loop
        if (!var->def->is_local && !var->def->declaring_type)
          continue;  // applies to for loop
        is_type_member = var->def->declaring_type.has_value();
        detailed_name = var->def->short_name;
        break;
      }
      case SymbolKind::Type: {
        QueryType* type = &db->types[sym.idx.idx];
        if (!type->def)
          continue;  // applies to for loop
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
        symbol.type = map_symbol_kind_to_symbol_type(sym.idx.kind);
        symbol.isTypeMember = is_type_member;
        symbol.ranges.push_back(*loc);
        grouped_symbols[sym.idx] = symbol;
      }
    }
  }

  // Publish.
  Out_CqueryPublishSemanticHighlighting out;
  out.params.uri = lsDocumentUri::FromPath(working_file->filename);
  for (auto& entry : grouped_symbols)
    out.params.symbols.push_back(entry.second);
  IpcManager::WriteStdout(IpcId::CqueryPublishSemanticHighlighting, out);
}

void FilterCompletionResponse(Out_TextDocumentComplete* complete_response,
                              const std::string& complete_text) {
// Used to inject more completions.
#if false
  const size_t kNumIterations = 250;
  size_t size = complete_response->result.items.size();
  complete_response->result.items.reserve(size * (kNumIterations + 1));
  for (size_t iteration = 0; iteration < kNumIterations; ++iteration) {
    for (size_t i = 0; i < size; ++i) {
      auto item = complete_response->result.items[i];
      item.label += "#" + std::to_string(iteration);
      complete_response->result.items.push_back(item);
    }
  }
#endif

  const size_t kMaxResultSize = 100u;
  if (complete_response->result.items.size() > kMaxResultSize) {
    if (complete_text.empty()) {
      complete_response->result.items.resize(kMaxResultSize);
    } else {
      NonElidedVector<lsCompletionItem> filtered_result;
      filtered_result.reserve(kMaxResultSize);

      std::unordered_set<std::string> inserted;
      inserted.reserve(kMaxResultSize);

      // Find literal matches first.
      for (const lsCompletionItem& item : complete_response->result.items) {
        if (item.label.find(complete_text) != std::string::npos) {
          // Don't insert the same completion entry.
          if (!inserted.insert(item.InsertedContent()).second)
            continue;

          filtered_result.push_back(item);
          if (filtered_result.size() >= kMaxResultSize)
            break;
        }
      }

      // Find fuzzy matches if we haven't found all of the literal matches.
      if (filtered_result.size() < kMaxResultSize) {
        for (const lsCompletionItem& item : complete_response->result.items) {
          if (SubstringMatch(complete_text, item.label)) {
            // Don't insert the same completion entry.
            if (!inserted.insert(item.InsertedContent()).second)
              continue;

            filtered_result.push_back(item);
            if (filtered_result.size() >= kMaxResultSize)
              break;
          }
        }
      }

      complete_response->result.items = filtered_result;
    }

    // Assuming the client does not support out-of-order completion (ie, ao
    // matches against oa), our filtering is guaranteed to contain any
    // potential matches, so the completion is only incomplete if we have the
    // max number of emitted matches.
    if (complete_response->result.items.size() >= kMaxResultSize) {
      LOG_S(INFO) << "Marking completion results as incomplete";
      complete_response->result.isIncomplete = true;
    }
  }
}
