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

  Parameter = 25,
  Using,
};
MAKE_REFLECT_TYPE_PROXY(ClangSymbolKind,
                        std::underlying_type<ClangSymbolKind>::type);

