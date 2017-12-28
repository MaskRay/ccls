#pragma once

#include "serializer.h"
#include "utils.h"

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
  TextDocumentSignatureHelp,
  TextDocumentDefinition,
  TextDocumentDocumentHighlight,
  TextDocumentHover,
  TextDocumentReferences,
  TextDocumentDocumentSymbol,
  TextDocumentDocumentLink,
  TextDocumentCodeAction,
  TextDocumentCodeLens,
  CodeLensResolve,
  WorkspaceSymbol,

  // Custom notifications
  // Comes from the client. Does various things, like update semantic
  // highlighting.
  CqueryTextDocumentDidView,
  CqueryPublishInactiveRegions,
  CqueryPublishSemanticHighlighting,

  // Custom messages
  CqueryFreshenIndex,
  // Messages used in tree views.
  CqueryTypeHierarchyTree,
  CqueryCallTreeInitial,
  CqueryCallTreeExpand,
  // These are like DocumentReferences but show different types of data.
  CqueryVars,     // Show all variables of a type.
  CqueryCallers,  // Show all callers of a function.
  CqueryBase,     // Show base types/method.
  CqueryDerived,  // Show all derived types/methods.

  // Internal implementation detail.
  Unknown,

  // Index the given file contents. Used in tests.
  CqueryIndexFile,
  // Wait until all cquery threads are idle. Used in tests.
  CqueryWait,
};
MAKE_ENUM_HASHABLE(IpcId)
MAKE_REFLECT_TYPE_PROXY(IpcId, int)
const char* IpcIdToString(IpcId id);

struct BaseIpcMessage {
  const IpcId method_id;
  BaseIpcMessage(IpcId method_id);
  virtual ~BaseIpcMessage();

  template <typename T>
  T* As() {
    assert(method_id == T::kIpcId);
    return static_cast<T*>(this);
  }
};

template <typename T>
struct IpcMessage : public BaseIpcMessage {
  IpcMessage() : BaseIpcMessage(T::kIpcId) {}
};