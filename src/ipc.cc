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
  case IpcId::WorkspaceSymbol:
    return "workspace/symbol";

  case IpcId::CqueryPublishInactiveRegions:
    return "$cquery/publishInactiveRegions";

  case IpcId::CqueryFreshenIndex:
    return "$cquery/freshenIndex";
  case IpcId::CqueryTypeHierarchyTree:
    return "$cquery/typeHierarchyTree";
  case IpcId::CqueryCallTreeInitial:
    return "$cquery/callTreeInitial";
  case IpcId::CqueryCallTreeExpand:
    return "$cquery/callTreeExpand";
  case IpcId::CqueryVars:
    return "$cquery/vars";
  case IpcId::CqueryCallers:
    return "$cquery/callers";
  case IpcId::CqueryBase:
    return "$cquery/base";
  case IpcId::CqueryDerived:
    return "$cquery/derived";

  case IpcId::Cout:
    return "$cout";

  case IpcId::CqueryIndexFile:
    return "$cquery/indexFile";
  case IpcId::CqueryQueryDbWaitForIdleIndexer:
    return "$cquery/queryDbWaitForIdleIndexer";
  case IpcId::CqueryExitWhenIdle:
    return "$cquery/exitWhenIdle";

  default:
    assert(false && "missing IpcId string name");
    exit(1);
  }
}

BaseIpcMessage::BaseIpcMessage(IpcId method_id)
  : method_id(method_id) {}

BaseIpcMessage::~BaseIpcMessage() = default;