// General
CASE(Initialize, "initialize")
CASE(Shutdown, "shutdown")

CASE(CodeLensResolve, "codeLens/resolve")
CASE(TextDocumentCodeAction, "textDocument/codeAction")
CASE(TextDocumentCodeLens, "textDocument/codeLens")
CASE(TextDocumentCompletion, "textDocument/completion")
CASE(TextDocumentDefinition, "textDocument/definition")
CASE(TextDocumentDidChange, "textDocument/didChange")
CASE(TextDocumentDidClose, "textDocument/didClose")
CASE(TextDocumentDidOpen, "textDocument/didOpen")
CASE(TextDocumentDidSave, "textDocument/didSave")
CASE(TextDocumentDocumentHighlight, "textDocument/documentHighlight")
CASE(TextDocumentDocumentLink, "textDocument/documentLink")
CASE(TextDocumentDocumentSymbol, "textDocument/documentSymbol")
CASE(TextDocumentFormatting, "textDocument/formatting")
CASE(TextDocumentHover, "textDocument/hover")
CASE(TextDocumentOnTypeFormatting, "textDocument/onTypeFormatting")
CASE(TextDocumentPublishDiagnostics, "textDocument/publishDiagnostics")
CASE(TextDocumentRangeFormatting, "textDocument/rangeFormatting")
CASE(TextDocumentReferences, "textDocument/references")
CASE(TextDocumentRename, "textDocument/rename")
CASE(TextDocumentSignatureHelp, "textDocument/signatureHelp")
CASE(TextDocumentTypeDefinition, "textDocument/typeDefinition")
CASE(WorkspaceDidChangeConfiguration, "workspace/didChangeConfiguration")
CASE(WorkspaceDidChangeWatchedFiles, "workspace/didChangeWatchedFiles")
CASE(WorkspaceSymbol, "workspace/symbol")

// Notification extensions
CASE(CqueryTextDocumentDidView, "$cquery/textDocumentDidView")
CASE(CqueryPublishInactiveRegions, "$cquery/publishInactiveRegions")
CASE(CqueryPublishSemanticHighlighting, "$cquery/publishSemanticHighlighting")

// Request extensions
CASE(CqueryFileInfo, "$cquery/fileInfo")
CASE(CqueryFreshenIndex, "$cquery/freshenIndex")

CASE(CqueryCallHierarchy, "$cquery/callHierarchy")
CASE(CqueryInheritanceHierarchy, "$cquery/inheritanceHierarchy")
CASE(CqueryMemberHierarchy, "$cquery/memberHierarchy")

// Cross reference extensions.
// Show all variables of a type.
CASE(CqueryVars, "$cquery/vars")
// Show all callers of a function.
CASE(CqueryCallers, "$cquery/callers")
// Show base types/method.
CASE(CqueryBase, "$cquery/base")
// Show all derived types/methods.
CASE(CqueryDerived, "$cquery/derived")
// Show random definition.
CASE(CqueryRandom, "$cquery/random")

// Messages for testing.
// Index the given file contents.
CASE(CqueryIndexFile, "$cquery/indexFile")
// Wait until all cquery threads are idle.
CASE(CqueryWait, "$cquery/wait")
