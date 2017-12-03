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
  Cout,

  // Index the given file contents. Used in tests.
  CqueryIndexFile,
  // Make querydb wait for the indexer to be idle. Used in tests.
  CqueryQueryDbWaitForIdleIndexer,
  // Exit after all messages have been read/processes. Used in tests.
  CqueryExitWhenIdle
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

struct Ipc_Cout : public IpcMessage<Ipc_Cout> {
  static constexpr IpcId kIpcId = IpcId::Cout;
  std::string content;
  IpcId original_ipc_id;
};
MAKE_REFLECT_STRUCT(Ipc_Cout, content);

struct Ipc_CqueryIndexFile : public IpcMessage<Ipc_CqueryIndexFile> {
  static constexpr IpcId kIpcId = IpcId::CqueryIndexFile;

  struct Params {
    std::string path;
    std::vector<std::string> args;
    bool is_interactive = false;
    std::string contents;
  };
  Params params;
};
MAKE_REFLECT_STRUCT(Ipc_CqueryIndexFile::Params,
                    path,
                    args,
                    is_interactive,
                    contents);
MAKE_REFLECT_STRUCT(Ipc_CqueryIndexFile, params);

struct Ipc_CqueryQueryDbWaitForIdleIndexer
    : public IpcMessage<Ipc_CqueryQueryDbWaitForIdleIndexer> {
  static constexpr IpcId kIpcId = IpcId::CqueryQueryDbWaitForIdleIndexer;
};
MAKE_REFLECT_EMPTY_STRUCT(Ipc_CqueryQueryDbWaitForIdleIndexer);

struct Ipc_CqueryExitWhenIdle : public IpcMessage<Ipc_CqueryExitWhenIdle> {
  static constexpr IpcId kIpcId = IpcId::CqueryExitWhenIdle;
};
MAKE_REFLECT_EMPTY_STRUCT(Ipc_CqueryExitWhenIdle);