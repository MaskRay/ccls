#pragma once

#include "serializer.h"

#include <stdint.h>

// TODO Rename query.h:SymbolKind to another name
// clang/Index/IndexSymbol.h clang::index::SymbolKind
enum class ClangSymbolKind : uint8_t {
  Unknown,

  Module = 1,
  Namespace,
  NamespaceAlias,
  Macro,

  Enum = 5,
  Struct,
  Class,
  Protocol,
  Extension,
  Union,
  TypeAlias,

  Function = 12,
  Variable,
  Field,
  EnumConstant,

  InstanceMethod = 16,
  ClassMethod,
  StaticMethod,
  InstanceProperty,
  ClassProperty,
  StaticProperty,

  Constructor = 22,
  Destructor,
  ConversionFunction,

  // Used for both function and template parameters.
  // Clients can differentiate based on StorageClass.
  Parameter = 25,
  Using,
};
MAKE_REFLECT_TYPE_PROXY(ClangSymbolKind);

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
