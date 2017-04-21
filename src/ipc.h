#pragma once

#include "utils.h"
#include "serializer.h"

#include <string>

enum class IpcId : int {
  // Language server specific requests.
  CancelRequest = 0,
  Initialize,
  Initialized,
  TextDocumentDidOpen,
  TextDocumentDidChange,
  TextDocumentDidClose,
  TextDocumentDidSave,
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

  // Internal implementation detail.
  Quit,
  IsAlive,
  OpenProject,
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

struct Ipc_Quit : public IpcMessage<Ipc_Quit> {
  static constexpr IpcId kIpcId = IpcId::Quit;
};
MAKE_REFLECT_EMPTY_STRUCT(Ipc_Quit);

struct Ipc_IsAlive : public IpcMessage<Ipc_IsAlive> {
  static constexpr IpcId kIpcId = IpcId::IsAlive;
};
MAKE_REFLECT_EMPTY_STRUCT(Ipc_IsAlive);

struct Ipc_OpenProject : public IpcMessage<Ipc_OpenProject> {
  static constexpr IpcId kIpcId = IpcId::OpenProject;
  std::string project_path;
};
MAKE_REFLECT_STRUCT(Ipc_OpenProject, project_path);

struct Ipc_Cout : public IpcMessage<Ipc_Cout> {
  static constexpr IpcId kIpcId = IpcId::Cout;
  std::string content;
};
MAKE_REFLECT_STRUCT(Ipc_Cout, content);