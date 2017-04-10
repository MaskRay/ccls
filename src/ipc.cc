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
  case IpcId::TextDocumentDidOpen:
    return "textDocument/didOpen";
  case IpcId::TextDocumentDidChange:
    return "textDocument/didChange";
  case IpcId::TextDocumentDidClose:
    return "textDocument/didClose";
  case IpcId::TextDocumentDidSave:
    return "textDocument/didSave";
  case IpcId::TextDocumentCompletion:
    return "textDocument/completion";
  case IpcId::TextDocumentDefinition:
    return "textDocument/definition";
  case IpcId::TextDocumentDocumentSymbol:
    return "textDocument/documentSymbol";
  case IpcId::TextDocumentCodeLens:
    return "textDocument/codeLens";
  case IpcId::CodeLensResolve:
    return "codeLens/resolve";
  case IpcId::WorkspaceSymbol:
    return "workspace/symbol";
  default:
    assert(false);
    exit(1);
  }
}

BaseIpcMessage::BaseIpcMessage(IpcId method_id)
  : method_id(method_id) {}
