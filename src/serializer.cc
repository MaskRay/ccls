#include "serializer.h"

#include "indexer.h"

namespace {
bool gTestOutputMode = false;
}  // namespace

// int16_t
void Reflect(Reader& visitor, int16_t& value) {
  value = (int16_t)visitor.GetInt();
}
void Reflect(Writer& visitor, int16_t& value) {
  visitor.Int(value);
}
// int32_t
void Reflect(Reader& visitor, int32_t& value) {
  value = visitor.GetInt();
}
void Reflect(Writer& visitor, int32_t& value) {
  visitor.Int(value);
}
// int64_t
void Reflect(Reader& visitor, int64_t& value) {
  value = visitor.GetInt64();
}
void Reflect(Writer& visitor, int64_t& value) {
  visitor.Int64(value);
}
// uint64_t
void Reflect(Reader& visitor, uint64_t& value) {
  value = visitor.GetUint64();
}
void Reflect(Writer& visitor, uint64_t& value) {
  visitor.Uint64(value);
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
  visitor.String(value.c_str(), (rapidjson::SizeType)value.size());
}

// ReflectMember
void ReflectMember(Writer& visitor, const char* name, std::string& value) {
  if (value.empty())
    return;
  visitor.Key(name);
  Reflect(visitor, value);
}

// TODO: Move this to indexer.cc

template <typename TVisitor>
void Reflect(TVisitor& visitor, IndexType& value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER2("id", value.id);
  REFLECT_MEMBER2("usr", value.def.usr);
  REFLECT_MEMBER2("short_name", value.def.short_name);
  REFLECT_MEMBER2("detailed_name", value.def.detailed_name);
  REFLECT_MEMBER2("definition_spelling", value.def.definition_spelling);
  REFLECT_MEMBER2("definition_extent", value.def.definition_extent);
  REFLECT_MEMBER2("alias_of", value.def.alias_of);
  REFLECT_MEMBER2("parents", value.def.parents);
  REFLECT_MEMBER2("derived", value.derived);
  REFLECT_MEMBER2("types", value.def.types);
  REFLECT_MEMBER2("funcs", value.def.funcs);
  REFLECT_MEMBER2("vars", value.def.vars);
  REFLECT_MEMBER2("instances", value.instances);
  REFLECT_MEMBER2("uses", value.uses);
  REFLECT_MEMBER_END();
}

template <typename TVisitor>
void Reflect(TVisitor& visitor, IndexFunc& value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER2("id", value.id);
  REFLECT_MEMBER2("usr", value.def.usr);
  REFLECT_MEMBER2("short_name", value.def.short_name);
  REFLECT_MEMBER2("detailed_name", value.def.detailed_name);
  REFLECT_MEMBER2("is_constructor", value.is_constructor);
  REFLECT_MEMBER2("parameter_type_descriptions", value.parameter_type_descriptions);
  REFLECT_MEMBER2("declarations", value.declarations);
  REFLECT_MEMBER2("definition_spelling", value.def.definition_spelling);
  REFLECT_MEMBER2("definition_extent", value.def.definition_extent);
  REFLECT_MEMBER2("declaring_type", value.def.declaring_type);
  REFLECT_MEMBER2("base", value.def.base);
  REFLECT_MEMBER2("derived", value.derived);
  REFLECT_MEMBER2("locals", value.def.locals);
  REFLECT_MEMBER2("callers", value.callers);
  REFLECT_MEMBER2("callees", value.def.callees);
  REFLECT_MEMBER_END();
}

template <typename TVisitor>
void Reflect(TVisitor& visitor, IndexVar& value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER2("id", value.id);
  REFLECT_MEMBER2("usr", value.def.usr);
  REFLECT_MEMBER2("short_name", value.def.short_name);
  REFLECT_MEMBER2("detailed_name", value.def.detailed_name);
  REFLECT_MEMBER2("declaration", value.def.declaration);
  REFLECT_MEMBER2("definition_spelling", value.def.definition_spelling);
  REFLECT_MEMBER2("definition_extent", value.def.definition_extent);
  REFLECT_MEMBER2("variable_type", value.def.variable_type);
  REFLECT_MEMBER2("declaring_type", value.def.declaring_type);
  REFLECT_MEMBER2("is_local", value.def.is_local);
  REFLECT_MEMBER2("is_macro", value.def.is_macro);
  REFLECT_MEMBER2("uses", value.uses);
  REFLECT_MEMBER_END();
}

// IndexFile
bool ReflectMemberStart(Writer& visitor, IndexFile& value) {
  auto it = value.id_cache.usr_to_type_id.find("");
  if (it != value.id_cache.usr_to_type_id.end()) {
    value.Resolve(it->second)->def.short_name = "<fundamental>";
    assert(value.Resolve(it->second)->uses.size() == 0);
  }

  value.version = IndexFile::kCurrentVersion;
  DefaultReflectMemberStart(visitor);
  return true;
}
template <typename TVisitor>
void Reflect(TVisitor& visitor, IndexFile& value) {
  REFLECT_MEMBER_START();
  if (!gTestOutputMode) {
    REFLECT_MEMBER(version);
    REFLECT_MEMBER(last_modification_time);
    REFLECT_MEMBER(import_file);
    REFLECT_MEMBER(args);
  }
  REFLECT_MEMBER(includes);
  REFLECT_MEMBER(dependencies);
  REFLECT_MEMBER(skipped_by_preprocessor);
  REFLECT_MEMBER(types);
  REFLECT_MEMBER(funcs);
  REFLECT_MEMBER(vars);
  REFLECT_MEMBER_END();
}

std::string Serialize(IndexFile& file) {
  rapidjson::StringBuffer output;
  rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(output);
  // Writer writer(output);
  writer.SetFormatOptions(
      rapidjson::PrettyFormatOptions::kFormatSingleLineArray);
  writer.SetIndent(' ', 2);

  Reflect(writer, file);

  return output.GetString();
}

std::unique_ptr<IndexFile> Deserialize(std::string path,
                                       std::string serialized,
                                       optional<int> expected_version) {
  rapidjson::Document reader;
  reader.Parse(serialized.c_str());
  if (reader.HasParseError())
    return nullptr;

  // Do not deserialize a document with a bad version. Doing so could cause a
  // crash because the file format may have changed.
  if (expected_version) {
    auto actual_version = reader.FindMember("version");
    if (actual_version == reader.MemberEnd() ||
        actual_version->value.GetInt() != expected_version) {
      return nullptr;
    }
  }

  auto file = MakeUnique<IndexFile>(path);
  Reflect(reader, *file);

  // Restore non-serialized state.
  file->path = path;
  file->id_cache.primary_file = file->path;
  for (const auto& type : file->types) {
    file->id_cache.type_id_to_usr[type.id] = type.def.usr;
    file->id_cache.usr_to_type_id[type.def.usr] = type.id;
  }
  for (const auto& func : file->funcs) {
    file->id_cache.func_id_to_usr[func.id] = func.def.usr;
    file->id_cache.usr_to_func_id[func.def.usr] = func.id;
  }
  for (const auto& var : file->vars) {
    file->id_cache.var_id_to_usr[var.id] = var.def.usr;
    file->id_cache.usr_to_var_id[var.def.usr] = var.id;
  }

  return file;
}

void SetTestOutputMode() {
  gTestOutputMode = true;
}
