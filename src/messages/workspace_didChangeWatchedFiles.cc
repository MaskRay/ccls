// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#include "clang_complete.hh"
#include "message_handler.h"
#include "pipeline.hh"
#include "project.h"
#include "working_files.h"
using namespace ccls;

namespace {
MethodType kMethodType = "workspace/didChangeWatchedFiles";

enum class lsFileChangeType {
  Created = 1,
  Changed = 2,
  Deleted = 3,
};
MAKE_REFLECT_TYPE_PROXY(lsFileChangeType);

struct lsFileEvent {
  lsDocumentUri uri;
  lsFileChangeType type;
};
MAKE_REFLECT_STRUCT(lsFileEvent, uri, type);

struct lsDidChangeWatchedFilesParams {
  std::vector<lsFileEvent> changes;
};
MAKE_REFLECT_STRUCT(lsDidChangeWatchedFilesParams, changes);

struct In_WorkspaceDidChangeWatchedFiles : public NotificationInMessage {
  MethodType GetMethodType() const override { return kMethodType; }
  lsDidChangeWatchedFilesParams params;
};
MAKE_REFLECT_STRUCT(In_WorkspaceDidChangeWatchedFiles, params);
REGISTER_IN_MESSAGE(In_WorkspaceDidChangeWatchedFiles);

struct Handler_WorkspaceDidChangeWatchedFiles
    : BaseMessageHandler<In_WorkspaceDidChangeWatchedFiles> {
  MethodType GetMethodType() const override { return kMethodType; }
  void Run(In_WorkspaceDidChangeWatchedFiles *request) override {
    for (lsFileEvent &event : request->params.changes) {
      std::string path = event.uri.GetPath();
      IndexMode mode = working_files->GetFileByFilename(path)
                           ? IndexMode::Normal
                           : IndexMode::NonInteractive;
      switch (event.type) {
      case lsFileChangeType::Created:
      case lsFileChangeType::Changed: {
        pipeline::Index(path, {}, mode);
        if (mode == IndexMode::Normal)
          clang_complete->NotifySave(path);
        else
          clang_complete->FlushSession(path);
        break;
      }
      case lsFileChangeType::Deleted:
        pipeline::Index(path, {}, mode);
        clang_complete->FlushSession(path);
        break;
      }
    }
  }
};
REGISTER_MESSAGE_HANDLER(Handler_WorkspaceDidChangeWatchedFiles);
} // namespace
