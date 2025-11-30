// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#include "souffle_exporter.hh"

#include "log.hh"
#include "query.hh"

#include <llvm/Support/FileSystem.h>
#include <llvm/Support/FormatVariadic.h>

namespace ccls {

namespace {
template <typename... Ts> void writeFmt(FILE *f, const char *fmt, Ts &&...vals) {
  auto str = llvm::formatv(fmt, std::forward<Ts>(vals)...).str();
  fputs(str.c_str(), f);
}

std::string escapeTSV(const std::string &str) {
  std::string result;
  result.reserve(str.size());
  for (auto c : str) {
    switch (c) {
    case '\t':
      result += "\\t";
      break;
    case '\n':
      result += "\\n";
      break;
    case '\r':
      result += "\\r";
      break;
    case '\\':
      result += "\\\\";
      break;
    default:
      result += c;
      break;
    }
  }
  return result;
}
std::string getEscapeFilePath(const DB &db, int file_id) {
  if (file_id < 0 || file_id >= (int)db.files.size())
    return "";
  const auto &file = db.files[file_id];
  if (!file.def)
    return "";
  return escapeTSV(file.def->path);
}
const char *kind2Str(Kind kind) {
  switch (kind) {
  case Kind::Invalid:
    return "Invalid";
  case Kind::File:
    return "File";
  case Kind::Type:
    return "Type";
  case Kind::Func:
    return "Func";
  case Kind::Var:
    return "Var";
  default:
    return "ERROR";
  }
}
std::string role2Str(Role role) {
  if (role == Role::None)
    return "None";
  if (role == Role::All)
    return "All";

  std::string result;
  auto add = [&](Role r, const char *name) {
    if (uint16_t(role) & uint16_t(r)) {
      if (!result.empty())
        result += "|";
      result += name;
    }
  };

  add(Role::Declaration, "Declaration");
  add(Role::Definition, "Definition");
  add(Role::Reference, "Reference");
  add(Role::Read, "Read");
  add(Role::Write, "Write");
  add(Role::Call, "Call");
  add(Role::Dynamic, "Dynamic");
  add(Role::Address, "Address");
  add(Role::Implicit, "Implicit");

  return result.empty() ? "None" : result;
}
const char *symbolKind2Str(SymbolKind kind) {
  switch (kind) {
  case SymbolKind::Unknown:
    return "Unknown";
  case SymbolKind::File:
    return "File";
  case SymbolKind::Module:
    return "Module";
  case SymbolKind::Namespace:
    return "Namespace";
  case SymbolKind::Package:
    return "Package";
  case SymbolKind::Class:
    return "Class";
  case SymbolKind::Method:
    return "Method";
  case SymbolKind::Property:
    return "Property";
  case SymbolKind::Field:
    return "Field";
  case SymbolKind::Constructor:
    return "Constructor";
  case SymbolKind::Enum:
    return "Enum";
  case SymbolKind::Interface:
    return "Interface";
  case SymbolKind::Function:
    return "Function";
  case SymbolKind::Variable:
    return "Variable";
  case SymbolKind::Constant:
    return "Constant";
  case SymbolKind::String:
    return "String";
  case SymbolKind::Number:
    return "Number";
  case SymbolKind::Boolean:
    return "Boolean";
  case SymbolKind::Array:
    return "Array";
  case SymbolKind::Object:
    return "Object";
  case SymbolKind::Key:
    return "Key";
  case SymbolKind::Null:
    return "Null";
  case SymbolKind::EnumMember:
    return "EnumMember";
  case SymbolKind::Struct:
    return "Struct";
  case SymbolKind::Event:
    return "Event";
  case SymbolKind::Operator:
    return "Operator";
  case SymbolKind::TypeParameter:
    return "TypeParameter";
  case SymbolKind::FirstNonStandard:
    return "FirstNonStandard";
  case SymbolKind::TypeAlias:
    return "TypeAlias";
  case SymbolKind::Parameter:
    return "Parameter";
  case SymbolKind::StaticMethod:
    return "StaticMethod";
  case SymbolKind::Macro:
    return "Macro";
  default:
    return "ERROR";
  }
}
} // namespace

SouffleExporter::SouffleExporter(const std::string &output_dir)
    : function_file_(nullptr), type_file_(nullptr), variable_file_(nullptr), calls_file_(nullptr),
      inherits_file_(nullptr), overrides_file_(nullptr), member_func_file_(nullptr), member_var_file_(nullptr),
      has_type_file_(nullptr), reference_file_(nullptr), declaration_file_(nullptr), definition_file_(nullptr),
      includes_file_(nullptr), func_derived_file_(nullptr), type_derived_file_(nullptr), type_instances_file_(nullptr),
      symbol_in_file_file_(nullptr) {
  auto ec = llvm::sys::fs::create_directories(output_dir);
  if (ec) {
    LOG_S(ERROR) << "Failed to create output directory " << output_dir << ": " << ec.message();
  } else {
    auto open_file = [&output_dir](const char *name) {
      std::string path = output_dir + "/" + name;
      FILE *f = fopen(path.c_str(), "w");
      if (!f) {
        LOG_S(ERROR) << "Failed to open " << path << " for writing. errno: " << errno;
      }
      return f;
    };

    function_file_ = open_file("function.facts");
    type_file_ = open_file("type.facts");
    variable_file_ = open_file("variable.facts");
    calls_file_ = open_file("calls.facts");
    inherits_file_ = open_file("inherits.facts");
    overrides_file_ = open_file("overrides.facts");
    member_func_file_ = open_file("member_func.facts");
    member_var_file_ = open_file("member_var.facts");
    has_type_file_ = open_file("has_type.facts");
    reference_file_ = open_file("reference.facts");
    declaration_file_ = open_file("declaration.facts");
    definition_file_ = open_file("definition.facts");
    includes_file_ = open_file("includes.facts");
    func_derived_file_ = open_file("func_derived.facts");
    type_derived_file_ = open_file("type_derived.facts");
    type_instances_file_ = open_file("type_instances.facts");
    symbol_in_file_file_ = open_file("symbol_in_file.facts");
  }
}

SouffleExporter::~SouffleExporter() {
  auto close_file = [](FILE *&f) {
    if (f) {
      fclose(f);
      f = nullptr;
    }
  };

  close_file(function_file_);
  close_file(type_file_);
  close_file(variable_file_);
  close_file(calls_file_);
  close_file(inherits_file_);
  close_file(overrides_file_);
  close_file(member_func_file_);
  close_file(member_var_file_);
  close_file(has_type_file_);
  close_file(reference_file_);
  close_file(declaration_file_);
  close_file(definition_file_);
  close_file(includes_file_);
  close_file(func_derived_file_);
  close_file(type_derived_file_);
  close_file(type_instances_file_);
  close_file(symbol_in_file_file_);
}

void SouffleExporter::exportDB(const DB &db) {
  int total_symbols = 0;
  for (auto &file : db.files) {
    total_symbols += file.symbol2refcnt.size();
  }
  LOG_S(INFO) << "Exporting " << total_symbols << " symbols, " << db.funcs.size() << " functions, " << db.types.size()
              << " types, " << db.vars.size() << " variables, " << db.files.size() << " files";

  for (auto &file : db.files) {
    exportFile(file);
  }

  for (auto &func : db.funcs) {
    exportFunction(func, db);
  }

  for (auto &type : db.types) {
    exportType(type, db);
  }

  for (auto &var : db.vars) {
    exportVariable(var, db);
  }
}

void SouffleExporter::exportFile(const QueryFile &file) {
  if (!file.def) {
    return;
  }

  auto &file_path = file.def->path;

  if (includes_file_) {
    for (auto &include : file.def->includes) {
      writeFmt(includes_file_, "{0}\t{1}\n", escapeTSV(file_path), escapeTSV(include.resolved_path));
    }
  }

  if (symbol_in_file_file_) {
    for (auto [sym, refcnt] : file.symbol2refcnt) {
      if (refcnt > 0) {
        writeFmt(symbol_in_file_file_, "{0}\t{1}\t{2}\t{3}\t{4}\t{5}\n", escapeTSV(file_path), sym.range.start.line + 1,
                 sym.range.start.column, sym.usr, kind2Str(sym.kind), role2Str(sym.role));
      }
    }
  }
}

void SouffleExporter::exportFunction(const QueryFunc &func, const DB &db) {
  if (!function_file_ || !definition_file_ || !calls_file_ || !overrides_file_ || !declaration_file_ ||
      !func_derived_file_ || !reference_file_) {
    return;
  }

  auto def = func.anyDef();

  if (def) {
    if (def->spell) {
      auto def_file = getEscapeFilePath(db, def->spell->file_id);
      auto def_line = def->spell->range.start.line + 1;

      if (!def_file.empty()) {
        writeFmt(function_file_, "{0}\t{1}\t{2}\t{3}\t{4}\n", func.usr, escapeTSV(def->detailed_name), def_file,
                 def_line, symbolKind2Str(def->kind));
      }

      if (def->spell->role & Role::Definition) {
        writeFmt(definition_file_, "{0}\t{1}\t{2}\n", func.usr, def_file, def_line);
      }
    }

    for (auto &callee : def->callees) {
      if (callee.kind == Kind::Func) {
        writeFmt(calls_file_, "{0}\t{1}\n", func.usr, callee.usr);
      }
    }

    for (auto &base_usr : def->bases) {
      writeFmt(overrides_file_, "{0}\t{1}\n", func.usr, base_usr);
    }
  }

  for (auto &decl : func.declarations) {
    std::string decl_file = getEscapeFilePath(db, decl.file_id);
    int decl_line = decl.range.start.line + 1;
    if (!decl_file.empty()) {
      writeFmt(declaration_file_, "{0}\t{1}\t{2}\n", func.usr, decl_file, decl_line);
    }
  }

  for (auto &derived_usr : func.derived) {
    writeFmt(func_derived_file_, "{0}\t{1}\n", func.usr, derived_usr);
  }

  for (auto &use : func.uses) {
    std::string use_file = getEscapeFilePath(db, use.file_id);
    int use_line = use.range.start.line + 1;
    if (!use_file.empty()) {
      writeFmt(reference_file_, "{0}\t{1}\t{2}\t{3}\n", func.usr, use_file, use_line, role2Str(use.role));
    }
  }
}

void SouffleExporter::exportType(const QueryType &type, const DB &db) {
  if (!type_file_ || !definition_file_ || !inherits_file_ || !member_func_file_ || !member_var_file_ ||
      !declaration_file_ || !type_derived_file_ || !type_instances_file_ || !reference_file_) {
    return;
  }

  auto def = type.anyDef();

  if (def) {
    if (def->spell) {
      auto def_file = getEscapeFilePath(db, def->spell->file_id);
      auto def_line = def->spell->range.start.line + 1;

      if (!def_file.empty()) {
        writeFmt(type_file_, "{0}\t{1}\t{2}\t{3}\t{4}\n", type.usr, escapeTSV(def->detailed_name), def_file, def_line,
                 symbolKind2Str(def->kind));
      }

      if (def->spell->role & Role::Definition) {
        writeFmt(definition_file_, "{0}\t{1}\t{2}\n", type.usr, def_file, def_line);
      }
    }

    for (auto &base_usr : def->bases) {
      writeFmt(inherits_file_, "{0}\t{1}\n", type.usr, base_usr);
    }

    for (auto &func_usr : def->funcs) {
      writeFmt(member_func_file_, "{0}\t{1}\n", type.usr, func_usr);
    }

    for (auto &[var_usr, offset] : def->vars) {
      writeFmt(member_var_file_, "{0}\t{1}\n", type.usr, var_usr);
    }
  }

  for (auto &decl : type.declarations) {
    auto decl_file = getEscapeFilePath(db, decl.file_id);
    auto decl_line = decl.range.start.line + 1;
    if (!decl_file.empty()) {
      writeFmt(declaration_file_, "{0}\t{1}\t{2}\n", type.usr, decl_file, decl_line);
    }
  }

  for (auto &derived_usr : type.derived) {
    writeFmt(type_derived_file_, "{0}\t{1}\n", type.usr, derived_usr);
  }

  for (auto &instance_usr : type.instances) {
    writeFmt(type_instances_file_, "{0}\t{1}\n", type.usr, instance_usr);
  }

  for (auto &use : type.uses) {
    auto use_file = getEscapeFilePath(db, use.file_id);
    auto use_line = use.range.start.line + 1;
    if (!use_file.empty()) {
      writeFmt(reference_file_, "{0}\t{1}\t{2}\t{3}\n", type.usr, use_file, use_line, role2Str(use.role));
    }
  }
}

void SouffleExporter::exportVariable(const QueryVar &var, const DB &db) {
  if (!variable_file_ || !definition_file_ || !has_type_file_ || !declaration_file_ || !reference_file_) {
    return;
  }

  auto def = var.anyDef();

  if (def && def->spell) {
    auto def_file = getEscapeFilePath(db, def->spell->file_id);
    auto def_line = def->spell->range.start.line + 1;

    if (!def_file.empty()) {
      writeFmt(variable_file_, "{0}\t{1}\t{2}\t{3}\n", var.usr, escapeTSV(def->detailed_name), def_file, def_line);
    }

    if (def->spell->role & Role::Definition) {
      writeFmt(definition_file_, "{0}\t{1}\t{2}\n", var.usr, def_file, def_line);
    }

    if (def->type && has_type_file_) {
      writeFmt(has_type_file_, "{0}\t{1}\n", var.usr, def->type);
    }
  }

  for (auto &decl : var.declarations) {
    auto decl_file = getEscapeFilePath(db, decl.file_id);
    auto decl_line = decl.range.start.line + 1;
    if (!decl_file.empty()) {
      writeFmt(declaration_file_, "{0}\t{1}\t{2}\n", var.usr, decl_file, decl_line);
    }
  }

  for (auto &use : var.uses) {
    auto use_file = getEscapeFilePath(db, use.file_id);
    auto use_line = use.range.start.line + 1;
    if (!use_file.empty()) {
      writeFmt(reference_file_, "{0}\t{1}\t{2}\t{3}\n", var.usr, use_file, use_line, role2Str(use.role));
    }
  }
}

} // namespace ccls
