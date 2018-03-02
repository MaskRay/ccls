#pragma once

#include "serializer.h"
#include "utils.h"

#include <string>

using lsRequestId = std::variant<std::monostate, int64_t, std::string>;

enum class IpcId : int {
  // Language server specific requests.
  CancelRequest = 0,
  Initialize,
  Initialized,
  Exit,
  Shutdown,

  CodeLensResolve,
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
  TextDocumentFormatting,
  TextDocumentRangeFormatting,
  TextDocumentOnTypeFormatting,
  TextDocumentReferences,
  TextDocumentDocumentSymbol,
  TextDocumentDocumentLink,
  TextDocumentCodeAction,
  TextDocumentCodeLens,
  WorkspaceDidChangeConfiguration,
  WorkspaceDidChangeWatchedFiles,
  WorkspaceSymbol,

  // Custom notifications
  // Comes from the client. Does various things, like update semantic
  // highlighting.
  CqueryTextDocumentDidView,
  CqueryPublishInactiveRegions,
  CqueryPublishSemanticHighlighting,

  // Custom messages
  CqueryFileInfo,
  CqueryFreshenIndex,
  // Messages used in tree views.
  CqueryCallHierarchy,
  CqueryInheritanceHierarchy,
  CqueryMemberHierarchy,
  // cquery cross reference extension.
  CqueryVars,     // Show all variables of a type.
  CqueryCallers,  // Show all callers of a function.
  CqueryBase,     // Show base types/method.
  CqueryDerived,  // Show all derived types/methods.
  CqueryRandom,   // Show random definition.

  // Messages for testing.
  CqueryIndexFile, // Index the given file contents.
  CqueryWait,      // Wait until all cquery threads are idle.

  // Internal implementation detail.
  Unknown,
};
MAKE_ENUM_HASHABLE(IpcId)
MAKE_REFLECT_TYPE_PROXY(IpcId)
const char* IpcIdToString(IpcId id);

struct BaseIpcMessage {
  const IpcId method_id;
  BaseIpcMessage(IpcId method_id);
  virtual ~BaseIpcMessage();

  virtual lsRequestId GetRequestId();

  template <typename T>
  T* As() {
    assert(method_id == T::kIpcId);
    return static_cast<T*>(this);
  }
};

template <typename T>
struct RequestMessage : public BaseIpcMessage {
  // number | string, actually no null
  lsRequestId id;
  RequestMessage() : BaseIpcMessage(T::kIpcId) {}

  lsRequestId GetRequestId() override { return id; }
};

// NotificationMessage does not have |id|.
template <typename T>
struct NotificationMessage : public BaseIpcMessage {
  NotificationMessage() : BaseIpcMessage(T::kIpcId) {}
};
