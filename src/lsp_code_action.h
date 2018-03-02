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

// FIXME Don't use array in vscode-cquery
inline void Reflect(Writer& visitor, lsCodeLensCommandArguments& value) {
  visitor.StartArray(3);
  Reflect(visitor, value.uri);
  Reflect(visitor, value.position);
  Reflect(visitor, value.locations);
  visitor.EndArray();
}

inline void Reflect(Reader& visitor, lsCodeLensCommandArguments& value) {
  int i = 0;
  visitor.IterArray([&](Reader& visitor) {
    switch (i++) {
      case 0:
        Reflect(visitor, value.uri);
        break;
      case 1:
        Reflect(visitor, value.position);
        break;
      case 2:
        Reflect(visitor, value.locations);
        break;
    }
  });
}
