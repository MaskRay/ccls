#pragma once

#include "serializer.h"

// The order matters. In FindSymbolsAtLocation, we want Var/Func ordered in
// front of others.
enum class SymbolKind : uint8_t { Invalid, File, Type, Func, Var };
MAKE_REFLECT_TYPE_PROXY(SymbolKind);
MAKE_ENUM_HASHABLE(SymbolKind);

// clang/Basic/Specifiers.h clang::StorageClass
enum class StorageClass : uint8_t {
  // In |CX_StorageClass| but not in |clang::StorageClass|
  // e.g. non-type template parameters
  Invalid,

  // These are legal on both functions and variables.
  // e.g. global functions/variables, local variables
  None,
  Extern,
  Static,
  // e.g. |__private_extern__ int a;|
  PrivateExtern,

  // These are only legal on variables.
  // e.g. explicit |auto int a;|
  Auto,
  Register
};
MAKE_REFLECT_TYPE_PROXY(StorageClass);

enum class Role : uint16_t {
  None = 0,
  Declaration = 1 << 0,
  Definition = 1 << 1,
  Reference = 1 << 2,
  Read = 1 << 3,
  Write = 1 << 4,
  Call = 1 << 5,
  Dynamic = 1 << 6,
  Address = 1 << 7,
  Implicit = 1 << 8,
};
MAKE_REFLECT_TYPE_PROXY(Role);
MAKE_ENUM_HASHABLE(Role);

inline uint16_t operator&(Role lhs, Role rhs) {
  return uint16_t(lhs) & uint16_t(rhs);
}

inline Role operator|(Role lhs, Role rhs) {
  return Role(uint16_t(lhs) | uint16_t(rhs));
}

// A document highlight kind.
enum class lsDocumentHighlightKind {
  // A textual occurrence.
  Text = 1,
  // Read-access of a symbol, like reading a variable.
  Read = 2,
  // Write-access of a symbol, like writing to a variable.
  Write = 3
};
MAKE_REFLECT_TYPE_PROXY(lsDocumentHighlightKind);

// A document highlight is a range inside a text document which deserves
// special attention. Usually a document highlight is visualized by changing
// the background color of its range.
struct lsDocumentHighlight {
  // The range this highlight applies to.
  lsRange range;

  // The highlight kind, default is DocumentHighlightKind.Text.
  lsDocumentHighlightKind kind = lsDocumentHighlightKind::Text;

  // cquery extension
  Role role = Role::None;
};
MAKE_REFLECT_STRUCT(lsDocumentHighlight, range, kind, role);

enum class lsSymbolKind : uint8_t {
  Unknown = 0,

  File = 1,
  Module = 2,
  Namespace = 3,
  Package = 4,
  Class = 5,
  Method = 6,
  Property = 7,
  Field = 8,
  Constructor = 9,
  Enum = 10,
  Interface = 11,
  Function = 12,
  Variable = 13,
  Constant = 14,
  String = 15,
  Number = 16,
  Boolean = 17,
  Array = 18,
  Object = 19,
  Key = 20,
  Null = 21,
  EnumMember = 22,
  Struct = 23,
  Event = 24,
  Operator = 25,
  TypeParameter = 26,

  // cquery extensions
  // See also https://github.com/Microsoft/language-server-protocol/issues/344 for new SymbolKind
  // clang/Index/IndexSymbol.h clang::index::SymbolKind
  Parameter = 13,
  Macro = 255,
};
MAKE_REFLECT_TYPE_PROXY(lsSymbolKind);

struct lsSymbolInformation {
  std::string_view name;
  lsSymbolKind kind;
  lsLocation location;
  std::string_view containerName;
};
MAKE_REFLECT_STRUCT(lsSymbolInformation, name, kind, location, containerName);
