#include "serializer.h"

#include "indexer.h"






void Serialize(Writer& writer, const char* key, Location location) {
  if (key) writer.Key(key);
  std::string s = location.ToString();
  writer.String(s.c_str());
}

void Serialize(Writer& writer, const char* key, optional<Location> location) {
  if (location)
    Serialize(writer, key, location.value());
}

void Serialize(Writer& writer, const char* key, const std::vector<Location>& locs) {
  if (locs.size() == 0)
    return;

  if (key) writer.Key(key);
  writer.StartArray();
  for (const Location& loc : locs)
    Serialize(writer, nullptr, loc);
  writer.EndArray();
}

template<typename T>
void Serialize(Writer& writer, const char* key, Id<T> id) {
  if (key) writer.Key(key);
  writer.Uint64(id.id);
}

template<typename T>
void Serialize(Writer& writer, const char* key, optional<Id<T>> id) {
  if (id) {
    Serialize(writer, key, id.value());
  }
}

template<typename T>
void Serialize(Writer& writer, const char* key, const std::vector<Id<T>>& ids) {
  if (ids.size() == 0)
    return;

  if (key) writer.Key(key);
  writer.StartArray();
  for (Id<T> id : ids)
    Serialize(writer, nullptr, id);
  writer.EndArray();
}

template<typename T>
void Serialize(Writer& writer, const char* key, Ref<T> ref) {
  if (key) writer.Key(key);
  std::string s = std::to_string(ref.id.id) + "@" + ref.loc.ToString();
  writer.String(s.c_str());
}

template<typename T>
void Serialize(Writer& writer, const char* key, const std::vector<Ref<T>>& refs) {
  if (refs.size() == 0)
    return;

  if (key) writer.Key(key);
  writer.StartArray();
  for (Ref<T> ref : refs)
    Serialize(writer, nullptr, ref);
  writer.EndArray();
}

void Serialize(Writer& writer, const char* key, const std::string& value) {
  if (value.size() == 0)
    return;

  if (key) writer.Key(key);
  writer.String(value.c_str());
}

void Serialize(Writer& writer, const char* key, uint64_t value) {
  if (key) writer.Key(key);
  writer.Uint64(value);
}

void Serialize(Writer& writer, IndexedFile* file) {
  auto it = file->id_cache.usr_to_type_id.find("");
  if (it != file->id_cache.usr_to_type_id.end()) {
    file->Resolve(it->second)->def.short_name = "<fundamental>";
    assert(file->Resolve(it->second)->uses.size() == 0);
  }

#define SERIALIZE(json_name, member_name) Serialize(writer, json_name, def.##member_name)

  writer.StartObject();

  // Types
  writer.Key("types");
  writer.StartArray();
  for (IndexedTypeDef& def : file->types) {
    if (def.is_bad_def)
      continue;

    writer.StartObject();
    SERIALIZE("id", id);
    SERIALIZE("usr", def.usr);
    SERIALIZE("short_name", def.short_name);
    SERIALIZE("qualified_name", def.qualified_name);
    SERIALIZE("definition", def.definition);
    SERIALIZE("alias_of", def.alias_of);
    SERIALIZE("parents", def.parents);
    SERIALIZE("derived", derived);
    SERIALIZE("types", def.types);
    SERIALIZE("funcs", def.funcs);
    SERIALIZE("vars", def.vars);
    SERIALIZE("uses", uses);
    writer.EndObject();
  }
  writer.EndArray();

  // Functions
  writer.Key("functions");
  writer.StartArray();
  for (IndexedFuncDef& def : file->funcs) {
    if (def.is_bad_def)
      continue;

    writer.StartObject();
    SERIALIZE("id", id);
    SERIALIZE("usr", def.usr);
    SERIALIZE("short_name", def.short_name);
    SERIALIZE("qualified_name", def.qualified_name);
    SERIALIZE("declarations", declarations);
    SERIALIZE("definition", def.definition);
    SERIALIZE("declaring_type", def.declaring_type);
    SERIALIZE("base", def.base);
    SERIALIZE("derived", derived);
    SERIALIZE("locals", def.locals);
    SERIALIZE("callers", callers);
    SERIALIZE("callees", def.callees);
    SERIALIZE("uses", uses);
    writer.EndObject();
  }
  writer.EndArray();

  // Variables
  writer.Key("variables");
  writer.StartArray();
  for (IndexedVarDef& def : file->vars) {
    if (def.is_bad_def)
      continue;

    writer.StartObject();
    SERIALIZE("id", id);
    SERIALIZE("usr", def.usr);
    SERIALIZE("short_name", def.short_name);
    SERIALIZE("qualified_name", def.qualified_name);
    SERIALIZE("declaration", def.declaration);
    SERIALIZE("definition", def.definition);
    SERIALIZE("variable_type", def.variable_type);
    SERIALIZE("declaring_type", def.declaring_type);
    SERIALIZE("uses", uses);
    writer.EndObject();
  }
  writer.EndArray();

  writer.EndObject();
#undef WRITE
}

void Deserialize(rapidjson::GenericValue<rapidjson::UTF8<>>& document, const char* name, std::string& output) {
  auto it = document.FindMember(name);
  if (it != document.MemberEnd())
    output = document[name].GetString();
}

void Deserialize(rapidjson::GenericValue<rapidjson::UTF8<>>& document, const char* name, optional<Location>& output) {
  auto it = document.FindMember(name);
  if (it != document.MemberEnd())
    output = Location(it->value.GetString()); // TODO: Location parsing not implemented in Location type.
}

void Deserialize(rapidjson::GenericValue<rapidjson::UTF8<>>& document, const char* name, std::vector<Location>& output) {
  auto it = document.FindMember(name);
  if (it != document.MemberEnd()) {
    for (auto& array_value : it->value.GetArray())
      output.push_back(Location(array_value.GetString()));
  }
}

template<typename T>
void Deserialize(rapidjson::GenericValue<rapidjson::UTF8<>>& document, const char* name, optional<Id<T>>& output) {
  auto it = document.FindMember(name);
  if (it != document.MemberEnd())
    output = Id<T>(it->value.GetUint64());
}

template<typename T>
void Deserialize(rapidjson::GenericValue<rapidjson::UTF8<>>& document, const char* name, std::vector<Id<T>>& output) {
  auto it = document.FindMember(name);
  if (it != document.MemberEnd()) {
    for (auto& array_value : it->value.GetArray())
      output.push_back(Id<T>(array_value.GetUint64()));
  }
}

template<typename T>
void Deserialize(rapidjson::GenericValue<rapidjson::UTF8<>>& document, const char* name, std::vector<Ref<T>>& output) {
  auto it = document.FindMember(name);
  if (it != document.MemberEnd()) {
    for (auto& array_value : it->value.GetArray()) {
      const char* str_value = array_value.GetString();
      uint64_t id = atoi(str_value);
      const char* loc_string = strchr(str_value, '@') + 1;
      output.push_back(Ref<T>(Id<T>(id), Location(loc_string)));
    }
  }
}

void Deserialize(Reader& reader, IndexedFile* file) {
#define DESERIALIZE(json_name, member_name) Deserialize(entry, json_name, def.##member_name)

  auto& types = reader["types"].GetArray();
  for (auto& entry : types) {
    TypeId id(entry["id"].GetInt64());
    std::string usr = entry["usr"].GetString();

    IndexedTypeDef def(id, usr);
    def.is_bad_def = false;
    DESERIALIZE("short_name", def.short_name);
    DESERIALIZE("qualified_name", def.qualified_name);
    DESERIALIZE("definition", def.definition);
    DESERIALIZE("alias_of", def.alias_of);
    DESERIALIZE("parents", def.parents);
    DESERIALIZE("derived", derived);
    DESERIALIZE("types", def.types);
    DESERIALIZE("funcs", def.funcs);
    DESERIALIZE("vars", def.vars);
    DESERIALIZE("uses", uses);
    file->types.push_back(def);
  }

  auto& functions = reader["functions"].GetArray();
  for (auto& entry : functions) {
    FuncId id(entry["id"].GetInt64());
    std::string usr = entry["usr"].GetString();

    IndexedFuncDef def(id, usr);
    def.is_bad_def = false;
    DESERIALIZE("short_name", def.short_name);
    DESERIALIZE("qualified_name", def.qualified_name);
    DESERIALIZE("declarations", declarations);
    DESERIALIZE("definition", def.definition);
    DESERIALIZE("declaring_type", def.declaring_type);
    DESERIALIZE("base", def.base);
    DESERIALIZE("derived", derived);
    DESERIALIZE("locals", def.locals);
    DESERIALIZE("callers", callers);
    DESERIALIZE("callees", def.callees);
    DESERIALIZE("uses", uses);
    file->funcs.push_back(def);
  }

  auto& vars = reader["variables"].GetArray();
  for (auto& entry : vars) {
    VarId id(entry["id"].GetInt64());
    std::string usr = entry["usr"].GetString();

    IndexedVarDef def(id, usr);
    def.is_bad_def = false;
    DESERIALIZE("short_name", def.short_name);
    DESERIALIZE("qualified_name", def.qualified_name);
    DESERIALIZE("declaration", def.declaration);
    DESERIALIZE("definition", def.definition);
    DESERIALIZE("variable_type", def.variable_type);
    DESERIALIZE("declaring_type", def.declaring_type);
    DESERIALIZE("uses", uses);
    file->vars.push_back(def);
  }
#undef DESERIALIZE
}

std::string Serialize(IndexedFile* file) {
  rapidjson::StringBuffer output;
  rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(output);
  writer.SetFormatOptions(
    rapidjson::PrettyFormatOptions::kFormatSingleLineArray);
  writer.SetIndent(' ', 2);

  Serialize(writer, file);

  return output.GetString();
}

IndexedFile Deserialize(std::string path, std::string serialized) {
  rapidjson::Document document;
  document.Parse(serialized.c_str());

  IndexedFile file(path);
  Deserialize(document, &file);
  return file;
}
