#include "include_complete.h"
#include "lex_utils.h"
#include "lsp_code_action.h"
#include "message_handler.h"
#include "query_utils.h"
#include "queue_manager.h"

#include <doctest/doctest.h>

#include <climits>
#include <loguru.hpp>

namespace {
MethodType kMethodType = "textDocument/codeAction";

optional<int> FindIncludeLine(const std::vector<std::string>& lines,
                              const std::string& full_include_line) {
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
    std::string text = Trim(lines[line]);
    if (!StartsWith(text, "#include")) {
      last_line_compare = 1;
      continue;
    }

    last_include_line = line;

    int current_line_compare = full_include_line.compare(text);
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

optional<QueryFileId> GetImplementationFile(QueryDatabase* db,
                                            QueryFileId file_id,
                                            QueryFile* file) {
  for (SymbolRef sym : file->def->outline) {
    switch (sym.kind) {
      case SymbolKind::Func: {
        if (const auto* def = db->GetFunc(sym).AnyDef()) {
          // Note: we ignore the definition if it is in the same file (ie,
          // possibly a header).
          if (def->extent) {
            QueryFileId t = def->extent->file;
            if (t != file_id)
              return t;
          }
        }
        break;
      }
      case SymbolKind::Var: {
        const QueryVar::Def* def = db->GetVar(sym).AnyDef();
        // Note: we ignore the definition if it is in the same file (ie,
        // possibly a header).
        if (def && def->extent) {
          QueryFileId t = def->extent->file;
          if (t != file_id)
            return t;
        }
        break;
      }
      default:
        break;
    }
  }

  // No associated definition, scan the project for a file in the same
  // directory with the same base-name.
  NormalizedPath original_path(file->def->path);
  std::string target_path = original_path.path;
  size_t last = target_path.find_last_of('.');
  if (last != std::string::npos) {
    target_path = target_path.substr(0, last);
  }

  LOG_S(INFO) << "!! Looking for impl file that starts with " << target_path;

  for (auto& entry : db->usr_to_file) {
    const NormalizedPath& path = entry.first;

    // Do not consider header files for implementation files.
    // TODO: make file extensions configurable.
    if (EndsWith(path.path, ".h") || EndsWith(path.path, ".hpp"))
      continue;

    if (StartsWith(path.path, target_path) && path != original_path) {
      return entry.second;
    }
  }

  return nullopt;
}

void EnsureImplFile(QueryDatabase* db,
                    QueryFileId file_id,
                    optional<lsDocumentUri>& impl_uri,
                    optional<QueryFileId>& impl_file_id) {
  if (!impl_uri.has_value()) {
    QueryFile& file = db->files[file_id.id];
    assert(file.def);

    impl_file_id = GetImplementationFile(db, file_id, &file);
    if (!impl_file_id.has_value())
      impl_file_id = file_id;

    QueryFile& impl_file = db->files[impl_file_id->id];
    if (impl_file.def)
      impl_uri = lsDocumentUri::FromPath(impl_file.def->path);
    else
      impl_uri = lsDocumentUri::FromPath(file.def->path);
  }
}

optional<lsTextEdit> BuildAutoImplementForFunction(QueryDatabase* db,
                                                   WorkingFiles* working_files,
                                                   WorkingFile* working_file,
                                                   int default_line,
                                                   QueryFileId decl_file_id,
                                                   QueryFileId impl_file_id,
                                                   QueryFunc& func) {
  const QueryFunc::Def* def = func.AnyDef();
  assert(def);
  for (Use decl : func.declarations) {
    if (decl.file != decl_file_id)
      continue;

    optional<lsRange> ls_decl = GetLsRange(working_file, decl.range);
    if (!ls_decl)
      continue;

    optional<std::string> type_name;
    optional<lsPosition> same_file_insert_end;
    if (def->declaring_type) {
      QueryType& declaring_type = db->types[def->declaring_type->id];
      if (const auto* def1 = declaring_type.AnyDef()) {
        type_name = std::string(def1->ShortName());
        optional<lsRange> ls_type_extent =
            GetLsRange(working_file, def1->extent->range);
        if (ls_type_extent) {
          same_file_insert_end = ls_type_extent->end;
          same_file_insert_end->character += 1;  // move past semicolon.
        }
      }
    }

    std::string insert_text;
    int newlines_after_name = 0;
    LexFunctionDeclaration(working_file->buffer_content, ls_decl->start,
                           type_name, &insert_text, &newlines_after_name);

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
    } else {
      lsPosition best_pos;
      best_pos.line = default_line;
      int best_dist = INT_MAX;

      QueryFile& file = db->files[impl_file_id.id];
      assert(file.def);
      for (SymbolRef sym : file.def->outline) {
        switch (sym.kind) {
          case SymbolKind::Func: {
            QueryFunc& sym_func = db->GetFunc(sym);
            const QueryFunc::Def* def1 = sym_func.AnyDef();
            if (!def1 || !def1->extent)
              break;

            for (Use func_decl : sym_func.declarations) {
              if (func_decl.file == decl_file_id) {
                int dist = func_decl.range.start.line - decl.range.start.line;
                if (abs(dist) < abs(best_dist)) {
                  optional<lsLocation> def_loc =
                      GetLsLocation(db, working_files, *def1->extent);
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
            break;
          }
          case SymbolKind::Invalid:
          case SymbolKind::File:
          case SymbolKind::Type:
            LOG_S(WARNING) << "Unexpected SymbolKind "
                           << static_cast<int>(sym.kind);
            break;
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

struct In_TextDocumentCodeAction : public RequestInMessage {
  MethodType GetMethodType() const override { return kMethodType; }

  // Contains additional diagnostic information about the context in which
  // a code action is run.
  struct lsCodeActionContext {
    // An array of diagnostics.
    std::vector<lsDiagnostic> diagnostics;
  };
  // Params for the CodeActionRequest
  struct lsCodeActionParams {
    // The document in which the command was invoked.
    lsTextDocumentIdentifier textDocument;
    // The range for which the command was invoked.
    lsRange range;
    // Context carrying additional information.
    lsCodeActionContext context;
  };
  lsCodeActionParams params;
};
MAKE_REFLECT_STRUCT(In_TextDocumentCodeAction::lsCodeActionContext,
                    diagnostics);
MAKE_REFLECT_STRUCT(In_TextDocumentCodeAction::lsCodeActionParams,
                    textDocument,
                    range,
                    context);
MAKE_REFLECT_STRUCT(In_TextDocumentCodeAction, id, params);
REGISTER_IN_MESSAGE(In_TextDocumentCodeAction);

struct Out_TextDocumentCodeAction
    : public lsOutMessage<Out_TextDocumentCodeAction> {
  using Command = lsCommand<CommandArgs>;

  lsRequestId id;
  std::vector<Command> result;
};
MAKE_REFLECT_STRUCT(Out_TextDocumentCodeAction, jsonrpc, id, result);

struct Handler_TextDocumentCodeAction
    : BaseMessageHandler<In_TextDocumentCodeAction> {
  MethodType GetMethodType() const override { return kMethodType; }

  void Run(In_TextDocumentCodeAction* request) override {
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

    QueryFileId file_id;
    QueryFile* file;
    if (!FindFileOrFail(db, project, request->id,
                        request->params.textDocument.uri.GetPath(), &file,
                        &file_id)) {
      return;
    }

    WorkingFile* working_file = working_files->GetFileByFilename(
        request->params.textDocument.uri.GetPath());
    if (!working_file) {
      // TODO: send error response.
      LOG_S(WARNING)
          << "[error] textDocument/codeAction could not find working file";
      return;
    }

    Out_TextDocumentCodeAction out;
    out.id = request->id;

    // TODO: auto-insert namespace?

    int default_line = (int)working_file->buffer_lines.size();

    // Make sure to call EnsureImplFile before using these. We lazy load
    // them because computing the values could involve an entire project
    // scan.
    optional<lsDocumentUri> impl_uri;
    optional<QueryFileId> impl_file_id;

    std::vector<SymbolRef> syms =
        FindSymbolsAtLocation(working_file, file, request->params.range.start);
    for (SymbolRef sym : syms) {
      switch (sym.kind) {
        case SymbolKind::Type: {
          QueryType& type = db->GetType(sym);
          const QueryType::Def* def = type.AnyDef();
          if (!def)
            break;

          int num_edits = 0;

          // Get implementation file.
          Out_TextDocumentCodeAction::Command command;

          EachDefinedEntity(db->funcs, def->funcs, [&](QueryFunc& func_def) {
            const QueryFunc::Def* def1 = func_def.AnyDef();
            if (def1->extent)
              return;
            EnsureImplFile(db, file_id, impl_uri /*out*/, impl_file_id /*out*/);
            optional<lsTextEdit> edit = BuildAutoImplementForFunction(
                db, working_files, working_file, default_line, file_id,
                *impl_file_id, func_def);
            if (!edit)
              return;

            ++num_edits;

            // Merge edits together if they are on the same line.
            // TODO: be smarter about newline merging? ie, don't end up
            //       with foo()\n\n\n\nfoo(), we want foo()\n\nfoo()\n\n
            //
            if (!command.arguments.edits.empty() &&
                command.arguments.edits[command.arguments.edits.size() - 1]
                        .range.end.line == edit->range.start.line) {
              command.arguments.edits[command.arguments.edits.size() - 1]
                  .newText += edit->newText;
            } else {
              command.arguments.edits.push_back(*edit);
            }
          });
          if (command.arguments.edits.empty())
            break;

          // If we're inserting at the end of the document, put a newline
          // before the insertion.
          if (command.arguments.edits[0].range.start.line >= default_line)
            command.arguments.edits[0].newText.insert(0, "\n");

          command.arguments.textDocumentUri = *impl_uri;
          command.title = "Auto-Implement " + std::to_string(num_edits) +
                          " methods on " + std::string(def->ShortName());
          command.command = "cquery._autoImplement";
          out.result.push_back(command);
          break;
        }

        case SymbolKind::Func: {
          QueryFunc& func = db->GetFunc(sym);
          const QueryFunc::Def* def = func.AnyDef();
          if (!def || def->extent)
            break;

          EnsureImplFile(db, file_id, impl_uri /*out*/, impl_file_id /*out*/);

          // Get implementation file.
          Out_TextDocumentCodeAction::Command command;
          command.title = "Auto-Implement " + std::string(def->ShortName());
          command.command = "cquery._autoImplement";
          command.arguments.textDocumentUri = *impl_uri;
          optional<lsTextEdit> edit = BuildAutoImplementForFunction(
              db, working_files, working_file, default_line, file_id,
              *impl_file_id, func);
          if (!edit)
            break;

          // If we're inserting at the end of the document, put a newline
          // before the insertion.
          if (edit->range.start.line >= default_line)
            edit->newText.insert(0, "\n");
          command.arguments.edits.push_back(*edit);
          out.result.push_back(command);
          break;
        }
        default:
          break;
      }

      // Only show one auto-impl section.
      if (!out.result.empty())
        break;
    }

    std::vector<lsDiagnostic> diagnostics;
    working_files->DoAction(
        [&]() { diagnostics = working_file->diagnostics_; });
    for (lsDiagnostic& diag : diagnostics) {
      if (diag.range.start.line != request->params.range.start.line)
        continue;

      // For error diagnostics, provide an action to resolve an include.
      // TODO: find a way to index diagnostic contents so line numbers
      // don't get mismatched when actively editing a file.
      std::string_view include_query = LexIdentifierAroundPos(
          diag.range.start, working_file->buffer_content);
      if (diag.severity == lsDiagnosticSeverity::Error &&
          !include_query.empty()) {
        const size_t kMaxResults = 20;

        std::unordered_set<std::string> include_absolute_paths;

        // Find include candidate strings.
        for (size_t i = 0; i < db->symbols.size(); ++i) {
          if (include_absolute_paths.size() > kMaxResults)
            break;
          if (db->GetSymbolDetailedName(i).find(include_query) ==
              std::string::npos)
            continue;

          Maybe<QueryFileId> decl_file_id =
              GetDeclarationFileForSymbol(db, db->symbols[i]);
          if (!decl_file_id)
            continue;

          QueryFile& decl_file = db->files[decl_file_id->id];
          if (!decl_file.def)
            continue;

          include_absolute_paths.insert(decl_file.def->path);
        }

        // Build include strings.
        std::unordered_set<std::string> include_insert_strings;
        include_insert_strings.reserve(include_absolute_paths.size());

        for (const std::string& path : include_absolute_paths) {
          optional<lsCompletionItem> item =
              include_complete->FindCompletionItemForAbsolutePath(path);
          if (!item)
            continue;
          if (item->textEdit)
            include_insert_strings.insert(item->textEdit->newText);
          else if (!item->insertText.empty())
            include_insert_strings.insert(item->insertText);
          else {
            // FIXME https://github.com/cquery-project/cquery/issues/463
            LOG_S(WARNING) << "unable to determine insert string for include "
                              "completion item";
          }
        }

        // Build code action.
        if (!include_insert_strings.empty()) {
          Out_TextDocumentCodeAction::Command command;

          // Build edits.
          for (const std::string& include_insert_string :
               include_insert_strings) {
            lsTextEdit edit;
            optional<int> include_line = FindIncludeLine(
                working_file->buffer_lines, include_insert_string);
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
            command.title = "Pick one of " +
                            std::to_string(command.arguments.edits.size()) +
                            " includes to insert";
          command.command = "cquery._insertInclude";
          command.arguments.textDocumentUri = request->params.textDocument.uri;
          out.result.push_back(command);
        }
      }

      // clang does not provide accurate enough column reporting for
      // diagnostics to do good column filtering, so report all
      // diagnostics on the line.
      if (!diag.fixits_.empty()) {
        Out_TextDocumentCodeAction::Command command;
        command.title = "FixIt: " + diag.message;
        command.command = "cquery._applyFixIt";
        command.arguments.textDocumentUri = request->params.textDocument.uri;
        command.arguments.edits = diag.fixits_;
        out.result.push_back(command);
      }
    }

    QueueManager::WriteStdout(kMethodType, out);
  }
};
REGISTER_MESSAGE_HANDLER(Handler_TextDocumentCodeAction);

TEST_SUITE("FindIncludeLine") {
  TEST_CASE("in document") {
    std::vector<std::string> lines = {
        "#include <bbb>",  // 0
        "#include <ddd>"   // 1
    };

    REQUIRE(FindIncludeLine(lines, "#include <bbb>") == nullopt);
  }

  TEST_CASE("insert before") {
    std::vector<std::string> lines = {
        "#include <bbb>",  // 0
        "#include <ddd>"   // 1
    };

    REQUIRE(FindIncludeLine(lines, "#include <aaa>") == 0);
  }

  TEST_CASE("insert middle") {
    std::vector<std::string> lines = {
        "#include <bbb>",  // 0
        "#include <ddd>"   // 1
    };

    REQUIRE(FindIncludeLine(lines, "#include <ccc>") == 1);
  }

  TEST_CASE("insert after") {
    std::vector<std::string> lines = {
        "#include <bbb>",  // 0
        "#include <ddd>",  // 1
        "",                // 2
    };

    REQUIRE(FindIncludeLine(lines, "#include <eee>") == 2);
  }

  TEST_CASE("ignore header") {
    std::vector<std::string> lines = {
        "// FOOBAR",       // 0
        "// FOOBAR",       // 1
        "// FOOBAR",       // 2
        "// FOOBAR",       // 3
        "",                // 4
        "#include <bbb>",  // 5
        "#include <ddd>",  // 6
        "",                // 7
    };

    REQUIRE(FindIncludeLine(lines, "#include <a>") == 5);
    REQUIRE(FindIncludeLine(lines, "#include <c>") == 6);
    REQUIRE(FindIncludeLine(lines, "#include <e>") == 7);
  }
}
}  // namespace
