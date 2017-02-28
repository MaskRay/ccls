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

#define SERIALIZE(name, value) Serialize(writer, name, def.value)

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

void Deserialize(std::string& output, rapidjson::GenericValue<rapidjson::UTF8<>>& value) {
  output = value.GetString();
}

void Deserialize(optional<Location>& output, rapidjson::GenericValue<rapidjson::UTF8<>>& value) {
  if (!value.IsNull())
    output = Location(value.GetString()); // TODO: Location parsing not implemented in Location type.
}

void Deserialize(Reader& reader, IndexedFile* file) {
  auto& types = reader["types"].GetArray();
  for (auto& type : types) {
    TypeId id = TypeId(type["id"].GetInt64());
    std::string usr = type["usr"].GetString();

    IndexedTypeDef def(id, usr);
    Deserialize(def.def.short_name, type["short_name"]);
    Deserialize(def.def.qualified_name, type["qualified_name"]);
    Deserialize(def.def.definition, type["definition"]); // TODO: What happens if entry is not present?
    //SERIALIZE("short_name", def.short_name);
    //SERIALIZE("qualified_name", def.qualified_name);
    //SERIALIZE("definition", def.definition);
    //SERIALIZE("alias_of", def.alias_of);
    //SERIALIZE("parents", def.parents);
    //SERIALIZE("derived", derived);
    //SERIALIZE("types", def.types);
    //SERIALIZE("funcs", def.funcs);
    //SERIALIZE("vars", def.vars);
    //SERIALIZE("uses", uses);

    file->types.push_back(def);
  }
}