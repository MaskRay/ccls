#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>

#include "indexer.h"

struct IndexedFile;
using Writer = rapidjson::PrettyWriter<rapidjson::StringBuffer>;
using Reader = rapidjson::Document;


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

void Serialize(Writer& writer, const char* key, Location location);
void Serialize(Writer& writer, const char* key, optional<Location> location);
void Serialize(Writer& writer, const char* key, const std::vector<Location>& locs);
void Serialize(Writer& writer, const char* key, const std::string& value);
void Serialize(Writer& writer, const char* key, const std::vector<std::string>& value);
void Serialize(Writer& writer, const char* key, uint64_t value);
void Serialize(Writer& writer, IndexedFile* file);



template<typename T>
void Deserialize(const rapidjson::GenericValue<rapidjson::UTF8<>>& document, const char* name, optional<Id<T>>& output) {
  auto it = document.FindMember(name);
  if (it != document.MemberEnd())
    output = Id<T>(it->value.GetUint64());
}

template<typename T>
void Deserialize(const rapidjson::GenericValue<rapidjson::UTF8<>>& document, const char* name, std::vector<Id<T>>& output) {
  auto it = document.FindMember(name);
  if (it != document.MemberEnd()) {
    for (auto& array_value : it->value.GetArray())
      output.push_back(Id<T>(array_value.GetUint64()));
  }
}

template<typename T>
void Deserialize(const rapidjson::GenericValue<rapidjson::UTF8<>>& document, const char* name, std::vector<Ref<T>>& output) {
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
void Deserialize(const rapidjson::GenericValue<rapidjson::UTF8<>>& document, const char* name, std::string& output);
void Deserialize(const rapidjson::GenericValue<rapidjson::UTF8<>>& document, const char* name, std::vector<std::string>& output);
void Deserialize(const rapidjson::GenericValue<rapidjson::UTF8<>>& document, const char* name, optional<Location>& output);
void Deserialize(const rapidjson::GenericValue<rapidjson::UTF8<>>& document, const char* name, std::vector<Location>& output);
void Deserialize(const Reader& reader, IndexedFile* file);

std::string Serialize(IndexedFile* file);
IndexedFile Deserialize(std::string path, std::string serialized);
