#include "lex_utils.h"
#include "message_handler.h"
#include "pipeline.hh"
#include "query_utils.h"
using namespace ccls;

#include <ctype.h>
#include <limits.h>
#include <cstdlib>

namespace {
MethodType kMethodType = "textDocument/definition";

struct In_TextDocumentDefinition : public RequestInMessage {
  MethodType GetMethodType() const override { return kMethodType; }
  lsTextDocumentPositionParams params;
};
MAKE_REFLECT_STRUCT(In_TextDocumentDefinition, id, params);
REGISTER_IN_MESSAGE(In_TextDocumentDefinition);

struct Out_TextDocumentDefinition
    : public lsOutMessage<Out_TextDocumentDefinition> {
  lsRequestId id;
  std::vector<lsLocationEx> result;
};
MAKE_REFLECT_STRUCT(Out_TextDocumentDefinition, jsonrpc, id, result);

std::vector<Use> GetNonDefDeclarationTargets(DB* db, SymbolRef sym) {
  switch (sym.kind) {
    case SymbolKind::Var: {
      std::vector<Use> ret = GetNonDefDeclarations(db, sym);
      // If there is no declaration, jump the its type.
      if (ret.empty()) {
        for (auto& def : db->GetVar(sym).def)
          if (def.type) {
            if (Maybe<Use> use = GetDefinitionSpell(
                    db, SymbolIdx{def.type, SymbolKind::Type})) {
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

struct Handler_TextDocumentDefinition
    : BaseMessageHandler<In_TextDocumentDefinition> {
  MethodType GetMethodType() const override { return kMethodType; }
  void Run(In_TextDocumentDefinition* request) override {
    auto& params = request->params;
    int file_id;
    QueryFile* file;
    if (!FindFileOrFail(db, project, request->id,
                        params.textDocument.uri.GetPath(), &file, &file_id))
      return;

    Out_TextDocumentDefinition out;
    out.id = request->id;

    Maybe<Use> on_def;
    bool has_symbol = false;
    WorkingFile* wfile =
        working_files->GetFileByFilename(file->def->path);
    lsPosition& ls_pos = params.position;

    for (SymbolRef sym : FindSymbolsAtLocation(wfile, file, ls_pos)) {
      // Found symbol. Return definition.
      has_symbol = true;

      // Special cases which are handled:
      //  - symbol has declaration but no definition (ie, pure virtual)
      //  - start at spelling but end at extent for better mouse tooltip
      //  - goto declaration while in definition of recursive type
      std::vector<Use> uses;
      EachEntityDef(db, sym, [&](const auto& def) {
        if (def.spell && def.extent) {
          Use spell = *def.spell;
          // If on a definition, clear |uses| to find declarations below.
          if (spell.file_id == file_id &&
              spell.range.Contains(ls_pos.line, ls_pos.character)) {
            on_def = spell;
            uses.clear();
            return false;
          }
          // We use spelling start and extent end because this causes vscode
          // to highlight the entire definition when previewing / hoving with
          // the mouse.
          spell.range.end = def.extent->range.end;
          uses.push_back(spell);
        }
        return true;
      });

      if (uses.empty()) {
        // The symbol has no definition or the cursor is on a definition.
        uses = GetNonDefDeclarationTargets(db, sym);
        // There is no declaration but the cursor is on a definition.
        if (uses.empty() && on_def)
          uses.push_back(*on_def);
      }
      auto locs = GetLsLocationExs(db, working_files, uses);
      out.result.insert(out.result.end(), locs.begin(), locs.end());
      if (!out.result.empty())
        break;
    }

    // No symbols - check for includes.
    if (out.result.empty()) {
      for (const IndexInclude& include : file->def->includes) {
        if (include.line == ls_pos.line) {
          lsLocationEx result;
          result.uri = lsDocumentUri::FromPath(include.resolved_path);
          out.result.push_back(result);
          has_symbol = true;
          break;
        }
      }
      // Find the best match of the identifier at point.
      if (!has_symbol) {
        lsPosition position = request->params.position;
        const std::string& buffer = wfile->buffer_content;
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
        best_sym.kind = SymbolKind::Invalid;
        auto fn = [&](SymbolIdx sym) {
          std::string_view short_name = db->GetSymbolName(sym, false),
                           name = short_query.size() < query.size()
                                      ? db->GetSymbolName(sym, true)
                                      : short_name;
          if (short_name != short_query)
            return;
          if (Maybe<Use> use = GetDefinitionSpell(db, sym)) {
            std::tuple<int, int, bool, int> score{
                int(name.size() - short_query.size()), 0,
                use->file_id != file_id,
                std::abs(use->range.start.line - position.line)};
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
        for (auto& func : db->funcs)
          fn({func.usr, SymbolKind::Func});
        for (auto& type : db->types)
          fn({type.usr, SymbolKind::Type});
        for (auto& var : db->vars)
          if (var.def.size() && !var.def[0].is_local())
            fn({var.usr, SymbolKind::Var});

        if (best_sym.kind != SymbolKind::Invalid) {
          Maybe<Use> use = GetDefinitionSpell(db, best_sym);
          assert(use);
          if (auto ls_loc = GetLsLocationEx(db, working_files, *use,
                                            g_config->xref.container))
            out.result.push_back(*ls_loc);
        }
      }
    }

    pipeline::WriteStdout(kMethodType, out);
  }
};
REGISTER_MESSAGE_HANDLER(Handler_TextDocumentDefinition);
}  // namespace
