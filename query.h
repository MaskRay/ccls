#pragma once

// NOTE: If updating this enum, make sure to also update the parser and the
//       help text.
enum class Command {
  Callees,
  Callers,
  FindAllUsages,
  FindInterestingUsages,
  GotoReferenced,
  Hierarchy,
  Outline,
  Search
};

// NOTE: If updating this enum, make sure to also update the parser and the
//       help text.
enum class PreferredSymbolLocation {
  Declaration,
  Definition
};