#include "ipc.h"

#include <cassert>

const char* IpcIdToString(IpcId id) {
  switch (id) {
    case IpcId::CancelRequest:
      return "$/cancelRequest";
    case IpcId::Initialize:
      return "initialize";
    case IpcId::Initialized:
      return "initialized";
    case IpcId::Shutdown:
      return "shutdown";
    case IpcId::Exit:
      return "exit";
    case IpcId::TextDocumentDidOpen:
      return "textDocument/didOpen";
    case IpcId::TextDocumentDidChange:
      return "textDocument/didChange";
    case IpcId::TextDocumentDidClose:
      return "textDocument/didClose";
    case IpcId::TextDocumentDidSave:
      return "textDocument/didSave";
    case IpcId::TextDocumentPublishDiagnostics:
      return "textDocument/publishDiagnostics";
    case IpcId::TextDocumentRename:
      return "textDocument/rename";
    case IpcId::TextDocumentCompletion:
      return "textDocument/completion";
    case IpcId::TextDocumentSignatureHelp:
      return "textDocument/signatureHelp";
    case IpcId::TextDocumentDefinition:
      return "textDocument/definition";
    case IpcId::TextDocumentDocumentHighlight:
      return "textDocument/documentHighlight";
    case IpcId::TextDocumentHover:
      return "textDocument/hover";
    case IpcId::TextDocumentFormatting:
      return "textDocument/formatting";
    case IpcId::TextDocumentRangeFormatting:
      return "textDocument/rangeFormatting";
    case IpcId::TextDocumentOnTypeFormatting:
      return "textDocument/onTypeFormatting";
    case IpcId::TextDocumentReferences:
      return "textDocument/references";
    case IpcId::TextDocumentDocumentSymbol:
      return "textDocument/documentSymbol";
    case IpcId::TextDocumentDocumentLink:
      return "textDocument/documentLink";
    case IpcId::TextDocumentCodeAction:
      return "textDocument/codeAction";
    case IpcId::TextDocumentCodeLens:
      return "textDocument/codeLens";
    case IpcId::CodeLensResolve:
      return "codeLens/resolve";
    case IpcId::WorkspaceDidChangeWatchedFiles:
      return "workspace/didChangeWatchedFiles";
    case IpcId::WorkspaceSymbol:
      return "workspace/symbol";

    case IpcId::CqueryTextDocumentDidView:
      return "$cquery/textDocumentDidView";
    case IpcId::CqueryPublishInactiveRegions:
      return "$cquery/publishInactiveRegions";
    case IpcId::CqueryPublishSemanticHighlighting:
      return "$cquery/publishSemanticHighlighting";

    case IpcId::CqueryFileInfo:
      return "$cquery/fileInfo";
    case IpcId::CqueryFreshenIndex:
      return "$cquery/freshenIndex";
    case IpcId::CqueryInheritanceHierarchy:
      return "$cquery/inheritanceHierarchy";
    case IpcId::CqueryCallHierarchyInitial:
      return "$cquery/callHierarchyInitial";
    case IpcId::CqueryCallHierarchyExpand:
      return "$cquery/callHierarchyExpand";
    case IpcId::CqueryCallTreeInitial:
      return "$cquery/callTreeInitial";
    case IpcId::CqueryCallTreeExpand:
      return "$cquery/callTreeExpand";
    case IpcId::CqueryMemberHierarchyInitial:
      return "$cquery/memberHierarchyInitial";
    case IpcId::CqueryMemberHierarchyExpand:
      return "$cquery/memberHierarchyExpand";
    case IpcId::CqueryVars:
      return "$cquery/vars";
    case IpcId::CqueryCallers:
      return "$cquery/callers";
    case IpcId::CqueryBase:
      return "$cquery/base";
    case IpcId::CqueryDerived:
      return "$cquery/derived";
    case IpcId::CqueryRandom:
      return "$cquery/random";

    case IpcId::Unknown:
      return "$unknown";

    case IpcId::CqueryIndexFile:
      return "$cquery/indexFile";
    case IpcId::CqueryWait:
      return "$cquery/wait";
  }

  CQUERY_UNREACHABLE("missing IpcId string name");
}

BaseIpcMessage::BaseIpcMessage(IpcId method_id) : method_id(method_id) {}

BaseIpcMessage::~BaseIpcMessage() = default;

lsRequestId BaseIpcMessage::GetRequestId() {
  return std::monostate();
}
