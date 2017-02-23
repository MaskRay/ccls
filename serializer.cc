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
void Serialize(Writer& writer, const char* key, LocalId<T> id) {
  if (key) writer.Key(key);
  writer.Uint64(id.local_id);
}

template<typename T>
void Serialize(Writer& writer, const char* key, optional<LocalId<T>> id) {
  if (id) {
    Serialize(writer, key, id.value());
  }
}

template<typename T>
void Serialize(Writer& writer, const char* key, const std::vector<LocalId<T>>& ids) {
  if (ids.size() == 0)
    return;

  if (key) writer.Key(key);
  writer.StartArray();
  for (LocalId<T> id : ids)
    Serialize(writer, nullptr, id);
  writer.EndArray();
}

template<typename T>
void Serialize(Writer& writer, const char* key, Ref<T> ref) {
  if (key) writer.Key(key);
  std::string s = std::to_string(ref.id.local_id) + "@" + ref.loc.ToString();
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
  auto it = file->usr_to_type_id.find("");
  if (it != file->usr_to_type_id.end()) {
    file->Resolve(it->second)->short_name = "<fundamental>";
    assert(file->Resolve(it->second)->uses.size() == 0);
  }

#define SERIALIZE(name) Serialize(writer, #name, def.name)

  writer.StartObject();

  // Types
  writer.Key("types");
  writer.StartArray();
  for (IndexedTypeDef& def : file->types) {
    if (def.is_system_def) continue;

    writer.StartObject();
    SERIALIZE(id);
    SERIALIZE(usr);
    SERIALIZE(short_name);
    SERIALIZE(qualified_name);
    SERIALIZE(definition);
    SERIALIZE(alias_of);
    SERIALIZE(parents);
    SERIALIZE(derived);
    SERIALIZE(types);
    SERIALIZE(funcs);
    SERIALIZE(vars);
    SERIALIZE(uses);
    writer.EndObject();
  }
  writer.EndArray();

  // Functions
  writer.Key("functions");
  writer.StartArray();
  for (IndexedFuncDef& def : file->funcs) {
    if (def.is_system_def) continue;

    writer.StartObject();
    SERIALIZE(id);
    SERIALIZE(usr);
    SERIALIZE(short_name);
    SERIALIZE(qualified_name);
    SERIALIZE(declaration);
    SERIALIZE(definition);
    SERIALIZE(declaring_type);
    SERIALIZE(base);
    SERIALIZE(derived);
    SERIALIZE(locals);
    SERIALIZE(callers);
    SERIALIZE(callees);
    SERIALIZE(uses);
    writer.EndObject();
  }
  writer.EndArray();

  // Variables
  writer.Key("variables");
  writer.StartArray();
  for (IndexedVarDef& def : file->vars) {
    if (def.is_system_def) continue;

    writer.StartObject();
    SERIALIZE(id);
    SERIALIZE(usr);
    SERIALIZE(short_name);
    SERIALIZE(qualified_name);
    SERIALIZE(declaration);
    SERIALIZE(definition);
    SERIALIZE(variable_type);
    SERIALIZE(declaring_type);
    SERIALIZE(uses);
    writer.EndObject();
  }
  writer.EndArray();

  writer.EndObject();
#undef WRITE
}
