// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <stdio.h>
#include <string>

namespace ccls {

struct QueryFunc;
struct QueryType;
struct QueryVar;
struct QueryFile;
struct DB;

class SouffleExporter {
public:
  SouffleExporter(const std::string &output_dir);
  ~SouffleExporter();

  void exportDB(const DB &db);

private:
  void exportFile(const QueryFile &file);
  void exportFunction(const QueryFunc &func, const DB &db);
  void exportType(const QueryType &type, const DB &db);
  void exportVariable(const QueryVar &var, const DB &db);

  FILE *function_file_;
  FILE *type_file_;
  FILE *variable_file_;
  FILE *calls_file_;
  FILE *inherits_file_;
  FILE *overrides_file_;
  FILE *member_func_file_;
  FILE *member_var_file_;
  FILE *has_type_file_;
  FILE *reference_file_;
  FILE *declaration_file_;
  FILE *definition_file_;
  FILE *includes_file_;
  FILE *func_derived_file_;
  FILE *type_derived_file_;
  FILE *type_instances_file_;
  FILE *symbol_in_file_file_;
};

} // namespace ccls
