#include "serializer.h"

#include "indexer.h"


// int
void Reflect(Reader& visitor, int& value) {
  value = visitor.GetInt();
}
void Reflect(Writer& visitor, int& value) {
  visitor.Int(value);
}
// bool
void Reflect(Reader& visitor, bool& value) {
  value = visitor.GetBool();
}
void Reflect(Writer& visitor, bool& value) {
  visitor.Bool(value);
}
// std::string
void Reflect(Reader& visitor, std::string& value) {
  value = visitor.GetString();
}
void Reflect(Writer& visitor, std::string& value) {
  visitor.String(value.c_str(), value.size());
}


// ReflectMember
void ReflectMember(Writer& visitor, const char* name, std::string& value) {
  if (value.empty())
    return;
  visitor.Key(name);
  Reflect(visitor, value);
}


// Position
void Reflect(Reader& visitor, Position& value) {
  value = Position(visitor.GetString());
}
void Reflect(Writer& visitor, Position& value) {
  std::string output = value.ToString();
  visitor.String(output.c_str(), output.size());
}


// Range
void Reflect(Reader& visitor, Range& value) {
  value = Range(visitor.GetString());
}
void Reflect(Writer& visitor, Range& value) {
  std::string output = value.ToString();
  visitor.String(output.c_str(), output.size());
}

// Id<T>
template<typename T>
void Reflect(Reader& visitor, Id<T>& id) {
  id.id = visitor.GetUint64();
}
template<typename T>
void Reflect(Writer& visitor, Id<T>& value) {
  visitor.Uint64(value.id);
}


// Ref<IndexedFuncDef>
void Reflect(Reader& visitor, Ref<IndexedFuncDef>& value) {
  const char* str_value = visitor.GetString();
  uint64_t id = atoi(str_value);
  const char* loc_string = strchr(str_value, '@') + 1;

  value.id = Id<IndexedFuncDef>(id);
  value.loc = Range(loc_string);
}
void Reflect(Writer& visitor, Ref<IndexedFuncDef>& value) {
  std::string s = std::to_string(value.id.id) + "@" + value.loc.ToString();
  visitor.String(s.c_str());
}







// TODO: Move this to indexer.cpp
// TODO: Rename indexer.cpp to indexer.cc
// TODO: Do not serialize a USR if it has no usages/etc outside of USR info.

// IndexedTypeDef
bool ReflectMemberStart(Reader& reader, IndexedTypeDef& value) {
  //value.is_bad_def = false;
  return true;
}
bool ReflectMemberStart(Writer& writer, IndexedTypeDef& value) {
  if (!value.HasInterestingState())
    std::cerr << "bad";
  assert(value.HasInterestingState());

  //if (value.is_bad_def)
  //  return false;
  DefaultReflectMemberStart(writer);
  return true;
}
template<typename TVisitor>
void Reflect(TVisitor& visitor, IndexedTypeDef& value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER2("id", value.id);
  REFLECT_MEMBER2("usr", value.def.usr);
  REFLECT_MEMBER2("short_name", value.def.short_name);
  REFLECT_MEMBER2("qualified_name", value.def.qualified_name);
  REFLECT_MEMBER2("definition_spelling", value.def.definition_spelling);
  REFLECT_MEMBER2("definition_extent", value.def.definition_extent);
  REFLECT_MEMBER2("alias_of", value.def.alias_of);
  REFLECT_MEMBER2("parents", value.def.parents);
  REFLECT_MEMBER2("derived", value.derived);
  REFLECT_MEMBER2("types", value.def.types);
  REFLECT_MEMBER2("funcs", value.def.funcs);
  REFLECT_MEMBER2("vars", value.def.vars);
  REFLECT_MEMBER2("instantiations", value.instantiations);
  REFLECT_MEMBER2("uses", value.uses);
  REFLECT_MEMBER_END();
}


// IndexedFuncDef
bool ReflectMemberStart(Reader& reader, IndexedFuncDef& value) {
  //value.is_bad_def = false;
  return true;
}
bool ReflectMemberStart(Writer& writer, IndexedFuncDef& value) {
  if (!value.HasInterestingState())
    std::cerr << "bad";
  assert(value.HasInterestingState());
  
  //if (value.is_bad_def)
  //  return false;
  DefaultReflectMemberStart(writer);
  return true;
}
template<typename TVisitor>
void Reflect(TVisitor& visitor, IndexedFuncDef& value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER2("id", value.id);
  REFLECT_MEMBER2("usr", value.def.usr);
  REFLECT_MEMBER2("short_name", value.def.short_name);
  REFLECT_MEMBER2("qualified_name", value.def.qualified_name);
  REFLECT_MEMBER2("declarations", value.declarations);
  REFLECT_MEMBER2("definition_spelling", value.def.definition_spelling);
  REFLECT_MEMBER2("definition_extent", value.def.definition_extent);
  REFLECT_MEMBER2("declaring_type", value.def.declaring_type);
  REFLECT_MEMBER2("base", value.def.base);
  REFLECT_MEMBER2("derived", value.derived);
  REFLECT_MEMBER2("locals", value.def.locals);
  REFLECT_MEMBER2("callers", value.callers);
  REFLECT_MEMBER2("callees", value.def.callees);
  REFLECT_MEMBER2("uses", value.uses);
  REFLECT_MEMBER_END();
}


// IndexedVarDef
bool ReflectMemberStart(Reader& reader, IndexedVarDef& value) {
  //value.is_bad_def = false;
  return true;
}
bool ReflectMemberStart(Writer& writer, IndexedVarDef& value) {
  if (!value.HasInterestingState())
    std::cerr << "bad";
  assert(value.HasInterestingState());

  //if (value.is_bad_def)
  //  return false;
  DefaultReflectMemberStart(writer);
  return true;
}
template<typename TVisitor>
void Reflect(TVisitor& visitor, IndexedVarDef& value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER2("id", value.id);
  REFLECT_MEMBER2("usr", value.def.usr);
  REFLECT_MEMBER2("short_name", value.def.short_name);
  REFLECT_MEMBER2("qualified_name", value.def.qualified_name);
  REFLECT_MEMBER2("declaration", value.def.declaration);
  REFLECT_MEMBER2("definition_spelling", value.def.definition_spelling);
  REFLECT_MEMBER2("definition_extent", value.def.definition_extent);
  REFLECT_MEMBER2("variable_type", value.def.variable_type);
  REFLECT_MEMBER2("declaring_type", value.def.declaring_type);
  REFLECT_MEMBER2("uses", value.uses);
  REFLECT_MEMBER_END();
}


// IndexedFile
bool ReflectMemberStart(Writer& visitor, IndexedFile& value) {
  auto it = value.id_cache.usr_to_type_id.find("");
  if (it != value.id_cache.usr_to_type_id.end()) {
    value.Resolve(it->second)->def.short_name = "<fundamental>";
    assert(value.Resolve(it->second)->uses.size() == 0);
  }

  DefaultReflectMemberStart(visitor);
  return true;
}
template<typename TVisitor>
void Reflect(TVisitor& visitor, IndexedFile& value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER(types);
  REFLECT_MEMBER(funcs);
  REFLECT_MEMBER(vars);
  REFLECT_MEMBER_END();
}







std::string Serialize(IndexedFile& file) {
  rapidjson::StringBuffer output;
  rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(output);
  //Writer writer(output);
  writer.SetFormatOptions(
    rapidjson::PrettyFormatOptions::kFormatSingleLineArray);
  writer.SetIndent(' ', 2);

  Reflect(writer, file);

  return output.GetString();
}

optional<IndexedFile> Deserialize(std::string path, std::string serialized) {
  rapidjson::Document reader;
  reader.Parse(serialized.c_str());
  if (reader.HasParseError())
    return nullopt;

  IndexedFile file(path);
  Reflect(reader, file);
  
  // Restore non-serialized state.
  file.path = path;
  file.id_cache.primary_file = file.path;
  for (const auto& type : file.types) {
    file.id_cache.type_id_to_usr[type.id] = type.def.usr;
    file.id_cache.usr_to_type_id[type.def.usr] = type.id;
  }
  for (const auto& func : file.funcs) {
    file.id_cache.func_id_to_usr[func.id] = func.def.usr;
    file.id_cache.usr_to_func_id[func.def.usr] = func.id;
  }
  for (const auto& var : file.vars) {
    file.id_cache.var_id_to_usr[var.id] = var.def.usr;
    file.id_cache.usr_to_var_id[var.def.usr] = var.id;
  }

  return file;
}
