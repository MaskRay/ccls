#pragma once
#include "lsp.h"

// The kind of a completion entry.
enum class lsCompletionItemKind {
  Text = 1,
  Method = 2,
  Function = 3,
  Constructor = 4,
  Field = 5,
  Variable = 6,
  Class = 7,
  Interface = 8,
  Module = 9,
  Property = 10,
  Unit = 11,
  Value = 12,
  Enum = 13,
  Keyword = 14,
  Snippet = 15,
  Color = 16,
  File = 17,
  Reference = 18,
  Folder = 19,
  EnumMember = 20,
  Constant = 21,
  Struct = 22,
  Event = 23,
  Operator = 24,
  TypeParameter = 25,
};
MAKE_REFLECT_TYPE_PROXY(lsCompletionItemKind);

// Defines whether the insert text in a completion item should be interpreted as
// plain text or a snippet.
enum class lsInsertTextFormat {
  // The primary text to be inserted is treated as a plain string.
  PlainText = 1,

  // The primary text to be inserted is treated as a snippet.
  //
  // A snippet can define tab stops and placeholders with `$1`, `$2`
  // and `${3:foo}`. `$0` defines the final tab stop, it defaults to
  // the end of the snippet. Placeholders with equal identifiers are linked,
  // that is typing in one will update others too.
  //
  // See also:
  // https://github.com/Microsoft/vscode/blob/master/src/vs/editor/contrib/snippet/common/snippet.md
  Snippet = 2
};
MAKE_REFLECT_TYPE_PROXY(lsInsertTextFormat);

struct lsCompletionItem {
  // A set of function parameters. Used internally for signature help. Not sent
  // to vscode.
  std::vector<std::string> parameters_;

  // The label of this completion item. By default
  // also the text that is inserted when selecting
  // this completion.
  std::string label;

  // The kind of this completion item. Based of the kind
  // an icon is chosen by the editor.
  lsCompletionItemKind kind = lsCompletionItemKind::Text;

  // A human-readable string with additional information
  // about this item, like type or symbol information.
  std::string detail;

  // A human-readable string that represents a doc-comment.
  optional<std::string> documentation;

  // Internal information to order candidates.
  int score_;
  unsigned priority_;

  // Use <> or "" by default as include path.
  bool use_angle_brackets_ = false;

  // A string that shoud be used when comparing this item
  // with other items. When `falsy` the label is used.
  std::string sortText;

  // A string that should be used when filtering a set of
  // completion items. When `falsy` the label is used.
  optional<std::string> filterText;

  // A string that should be inserted a document when selecting
  // this completion. When `falsy` the label is used.
  std::string insertText;

  // The format of the insert text. The format applies to both the `insertText`
  // property and the `newText` property of a provided `textEdit`.
  lsInsertTextFormat insertTextFormat = lsInsertTextFormat::PlainText;

  // An edit which is applied to a document when selecting this completion. When
  // an edit is provided the value of `insertText` is ignored.
  //
  // *Note:* The range of the edit must be a single line range and it must
  // contain the position at which completion has been requested.
  optional<lsTextEdit> textEdit;

  // An optional array of additional text edits that are applied when
  // selecting this completion. Edits must not overlap with the main edit
  // nor with themselves.
  // std::vector<TextEdit> additionalTextEdits;

  // An optional command that is executed *after* inserting this completion.
  // *Note* that additional modifications to the current document should be
  // described with the additionalTextEdits-property. Command command;

  // An data entry field that is preserved on a completion item between
  // a completion and a completion resolve request.
  // data ? : any

  // Use this helper to figure out what content the completion item will insert
  // into the document, as it could live in either |textEdit|, |insertText|, or
  // |label|.
  const std::string& InsertedContent() const {
    if (textEdit)
      return textEdit->newText;
    if (!insertText.empty())
      return insertText;
    return label;
  }
};
MAKE_REFLECT_STRUCT(lsCompletionItem,
                    label,
                    kind,
                    detail,
                    documentation,
                    sortText,
                    insertText,
                    filterText,
                    insertTextFormat,
                    textEdit);
