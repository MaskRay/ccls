#pragma once

#include "utils.h"
#include "serializer.h"

#include <string>

enum class IpcId : int {
  // Language server specific requests.
  CancelRequest = 0,
  Initialize,
  Initialized,
  Exit,
  TextDocumentDidOpen,
  TextDocumentDidChange,
  TextDocumentDidClose,
  TextDocumentDidSave,
  TextDocumentPublishDiagnostics,
  TextDocumentRename,
  TextDocumentCompletion,
  TextDocumentDefinition,
  TextDocumentDocumentHighlight,
  TextDocumentHover,
  TextDocumentReferences,
  TextDocumentDocumentSymbol,
  TextDocumentCodeLens,
  CodeLensResolve,
  WorkspaceSymbol,

  // Custom messages
  CqueryFreshenIndex,

  // These are like DocumentReferences but show different types of data.
  CqueryVars,       // Show all variables of a type.
  CqueryCallers,    // Show all callers of a function.
  CqueryBase,       // Show base types/method.
  CqueryDerived,    // Show all derived types/methods.

  // Internal implementation detail.
  Cout
};
MAKE_ENUM_HASHABLE(IpcId)
MAKE_REFLECT_TYPE_PROXY(IpcId, int)
const char* IpcIdToString(IpcId id);

struct BaseIpcMessage {
  const IpcId method_id;
  BaseIpcMessage(IpcId method_id);
};

template <typename T>
struct IpcMessage : public BaseIpcMessage {
  IpcMessage() : BaseIpcMessage(T::kIpcId) {}
};

struct Ipc_Cout : public IpcMessage<Ipc_Cout> {
  static constexpr IpcId kIpcId = IpcId::Cout;
  std::string content;
  IpcId original_ipc_id;
};
MAKE_REFLECT_STRUCT(Ipc_Cout, content);