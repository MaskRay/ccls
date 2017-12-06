#include "lex_utils.h"
#include "message_handler.h"

#include <loguru.hpp>

struct TextDocumentDocumentLinkHandler
    : BaseMessageHandler<Ipc_TextDocumentDocumentLink> {
  void Run(Ipc_TextDocumentDocumentLink* request) override {
    Out_TextDocumentDocumentLink out;
    out.id = request->id;

    if (config->showDocumentLinksOnIncludes) {
      QueryFile* file;
      if (!FindFileOrFail(db, request->id,
                          request->params.textDocument.uri.GetPath(), &file)) {
        return;
      }

      WorkingFile* working_file = working_files->GetFileByFilename(
          request->params.textDocument.uri.GetPath());
      if (!working_file) {
        LOG_S(WARNING) << "Unable to find working file "
                       << request->params.textDocument.uri.GetPath();
        return;
      }
      for (const IndexInclude& include : file->def->includes) {
        optional<int> buffer_line;
        optional<std::string> buffer_line_content =
            working_file->GetBufferLineContentFromIndexLine(include.line,
                                                            &buffer_line);
        if (!buffer_line || !buffer_line_content)
          continue;

        // Subtract 1 from line because querydb stores 1-based lines but
        // vscode expects 0-based lines.
        optional<lsRange> between_quotes =
            ExtractQuotedRange(*buffer_line - 1, *buffer_line_content);
        if (!between_quotes)
          continue;

        lsDocumentLink link;
        link.target = lsDocumentUri::FromPath(include.resolved_path);
        link.range = *between_quotes;
        out.result.push_back(link);
      }
    }

    IpcManager::WriteStdout(IpcId::TextDocumentDocumentLink, out);
  }
};
REGISTER_MESSAGE_HANDLER(TextDocumentDocumentLinkHandler);
