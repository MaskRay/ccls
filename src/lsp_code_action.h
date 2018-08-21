// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "lsp.h"

// codeAction
struct CommandArgs {
  lsDocumentUri textDocumentUri;
  std::vector<lsTextEdit> edits;
};
MAKE_REFLECT_STRUCT_WRITER_AS_ARRAY(CommandArgs, textDocumentUri, edits);

// codeLens
struct lsCodeLensUserData {};
MAKE_REFLECT_EMPTY_STRUCT(lsCodeLensUserData);

struct lsCodeLensCommandArguments {
  lsDocumentUri uri;
  lsPosition position;
  std::vector<lsLocation> locations;
};
MAKE_REFLECT_STRUCT(lsCodeLensCommandArguments, uri, position, locations)
