#include "serializer.h"

#include "serializers/json.h"
#include "serializers/msgpack.h"

#include "indexer.h"

#include <doctest/doctest.h>
#include <loguru.hpp>

#include <stdexcept>

bool gTestOutputMode = false;

//// Elementary types

void Reflect(Reader& visitor, uint8_t& value) {
  if (!visitor.IsInt())
    throw std::invalid_argument("uint8_t");
  value = (uint8_t)visitor.GetInt();
}
void Reflect(Writer& visitor, uint8_t& value) {
  visitor.Int(value);
}

void Reflect(Reader& visitor, short& value) {
  if (!visitor.IsInt())
    throw std::invalid_argument("short");
  value = (short)visitor.GetInt();
}
void Reflect(Writer& visitor, short& value) {
  visitor.Int(value);
}

void Reflect(Reader& visitor, unsigned short& value) {
  if (!visitor.IsInt())
    throw std::invalid_argument("unsigned short");
  value = (unsigned short)visitor.GetInt();
}
void Reflect(Writer& visitor, unsigned short& value) {
  visitor.Int(value);
}

void Reflect(Reader& visitor, int& value) {
  if (!visitor.IsInt())
    throw std::invalid_argument("int");
  value = visitor.GetInt();
}
void Reflect(Writer& visitor, int& value) {
  visitor.Int(value);
}

void Reflect(Reader& visitor, unsigned& value) {
  if (!visitor.IsUint64())
    throw std::invalid_argument("unsigned");
  value = visitor.GetUint32();
}
void Reflect(Writer& visitor, unsigned& value) {
  visitor.Uint32(value);
}

void Reflect(Reader& visitor, long& value) {
  if (!visitor.IsInt64())
    throw std::invalid_argument("long");
  value = long(visitor.GetInt64());
}
void Reflect(Writer& visitor, long& value) {
  visitor.Int64(value);
}

void Reflect(Reader& visitor, unsigned long& value) {
  if (!visitor.IsUint64())
    throw std::invalid_argument("unsigned long");
  value = (unsigned long)visitor.GetUint64();
}
void Reflect(Writer& visitor, unsigned long& value) {
  visitor.Uint64(value);
}

void Reflect(Reader& visitor, long long& value) {
  if (!visitor.IsInt64())
    throw std::invalid_argument("long long");
  value = visitor.GetInt64();
}
void Reflect(Writer& visitor, long long& value) {
  visitor.Int64(value);
}

void Reflect(Reader& visitor, unsigned long long& value) {
  if (!visitor.IsUint64())
    throw std::invalid_argument("unsigned long long");
  value = visitor.GetUint64();
}
void Reflect(Writer& visitor, unsigned long long& value) {
  visitor.Uint64(value);
}

void Reflect(Reader& visitor, double& value) {
  if (!visitor.IsDouble())
    throw std::invalid_argument("double");
  value = visitor.GetDouble();
}
void Reflect(Writer& visitor, double& value) {
  visitor.Double(value);
}

void Reflect(Reader& visitor, bool& value) {
  if (!visitor.IsBool())
    throw std::invalid_argument("bool");
  value = visitor.GetBool();
}
void Reflect(Writer& visitor, bool& value) {
  visitor.Bool(value);
}

void Reflect(Reader& visitor, std::string& value) {
  if (!visitor.IsString())
    throw std::invalid_argument("std::string");
  value = visitor.GetString();
}
void Reflect(Writer& visitor, std::string& value) {
  visitor.String(value.c_str(), (rapidjson::SizeType)value.size());
}

void Reflect(Reader&, std::string_view&) {
  assert(0);
}
void Reflect(Writer& visitor, std::string_view& data) {
  if (data.empty())
    visitor.String("");
  else
    visitor.String(&data[0], (rapidjson::SizeType)data.size());
}

void Reflect(Reader& visitor, NtString& value) {
  if (!visitor.IsString())
    throw std::invalid_argument("std::string");
  value = visitor.GetString();
}
void Reflect(Writer& visitor, NtString& value) {
  const char* s = value.c_str();
  visitor.String(s ? s : "");
}

// TODO: Move this to indexer.cc
void Reflect(Reader& visitor, IndexInclude& value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER(line);
  REFLECT_MEMBER(resolved_path);
  REFLECT_MEMBER_END();
}
void Reflect(Writer& visitor, IndexInclude& value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER(line);
  if (gTestOutputMode) {
    std::string basename = GetBaseName(value.resolved_path);
    if (!StartsWith(value.resolved_path, "&"))
      basename = "&" + basename;
    REFLECT_MEMBER2("resolved_path", basename);
  } else {
    REFLECT_MEMBER(resolved_path);
  }
  REFLECT_MEMBER_END();
}

template <typename Def>
void ReflectHoverAndComments(Reader& visitor, Def& def) {
  ReflectMember(visitor, "hover", def.hover);
  ReflectMember(visitor, "comments", def.comments);
}

template <typename Def>
void ReflectHoverAndComments(Writer& visitor, Def& def) {
  // Don't emit empty hover and comments in JSON test mode.
  if (!gTestOutputMode || !def.hover.empty())
    ReflectMember(visitor, "hover", def.hover);
  if (!gTestOutputMode || !def.comments.empty())
    ReflectMember(visitor, "comments", def.comments);
}

template <typename Def>
void ReflectShortName(Reader& visitor, Def& def) {
  if (gTestOutputMode) {
    std::string short_name;
    ReflectMember(visitor, "short_name", short_name);
    def.short_name_offset = def.detailed_name.find(short_name);
    assert(def.short_name_offset != std::string::npos);
    def.short_name_size = short_name.size();
  } else {
    ReflectMember(visitor, "short_name_offset", def.short_name_offset);
    ReflectMember(visitor, "short_name_size", def.short_name_size);
  }
}

template <typename Def>
void ReflectShortName(Writer& visitor, Def& def) {
  if (gTestOutputMode) {
    std::string short_name(
        def.detailed_name.substr(def.short_name_offset, def.short_name_size));
    ReflectMember(visitor, "short_name", short_name);
  } else {
    ReflectMember(visitor, "short_name_offset", def.short_name_offset);
    ReflectMember(visitor, "short_name_size", def.short_name_size);
  }
}

template <typename TVisitor>
void Reflect(TVisitor& visitor, IndexType& value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER2("id", value.id);
  REFLECT_MEMBER2("usr", value.usr);
  REFLECT_MEMBER2("detailed_name", value.def.detailed_name);
  ReflectShortName(visitor, value.def);
  REFLECT_MEMBER2("kind", value.def.kind);
  ReflectHoverAndComments(visitor, value.def);
  REFLECT_MEMBER2("declarations", value.declarations);
  REFLECT_MEMBER2("spell", value.def.spell);
  REFLECT_MEMBER2("extent", value.def.extent);
  REFLECT_MEMBER2("alias_of", value.def.alias_of);
  REFLECT_MEMBER2("bases", value.def.bases);
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
  REFLECT_MEMBER2("usr", value.usr);
  REFLECT_MEMBER2("detailed_name", value.def.detailed_name);
  ReflectShortName(visitor, value.def);
  REFLECT_MEMBER2("kind", value.def.kind);
  REFLECT_MEMBER2("storage", value.def.storage);
  ReflectHoverAndComments(visitor, value.def);
  REFLECT_MEMBER2("declarations", value.declarations);
  REFLECT_MEMBER2("spell", value.def.spell);
  REFLECT_MEMBER2("extent", value.def.extent);
  REFLECT_MEMBER2("declaring_type", value.def.declaring_type);
  REFLECT_MEMBER2("bases", value.def.bases);
  REFLECT_MEMBER2("derived", value.derived);
  REFLECT_MEMBER2("vars", value.def.vars);
  REFLECT_MEMBER2("uses", value.uses);
  REFLECT_MEMBER2("callees", value.def.callees);
  REFLECT_MEMBER_END();
}

template <typename TVisitor>
void Reflect(TVisitor& visitor, IndexVar& value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER2("id", value.id);
  REFLECT_MEMBER2("usr", value.usr);
  REFLECT_MEMBER2("detailed_name", value.def.detailed_name);
  ReflectShortName(visitor, value.def);
  ReflectHoverAndComments(visitor, value.def);
  REFLECT_MEMBER2("declarations", value.declarations);
  REFLECT_MEMBER2("spell", value.def.spell);
  REFLECT_MEMBER2("extent", value.def.extent);
  REFLECT_MEMBER2("type", value.def.type);
  REFLECT_MEMBER2("uses", value.uses);
  REFLECT_MEMBER2("kind", value.def.kind);
  REFLECT_MEMBER2("storage", value.def.storage);
  REFLECT_MEMBER_END();
}

// IndexFile
bool ReflectMemberStart(Writer& visitor, IndexFile& value) {
  // FIXME
  auto it = value.id_cache.usr_to_type_id.find(HashUsr(""));
  if (it != value.id_cache.usr_to_type_id.end()) {
    value.Resolve(it->second)->def.detailed_name = "<fundamental>";
    assert(value.Resolve(it->second)->uses.size() == 0);
  }

  DefaultReflectMemberStart(visitor);
  return true;
}
template <typename TVisitor>
void Reflect(TVisitor& visitor, IndexFile& value) {
  REFLECT_MEMBER_START();
  if (!gTestOutputMode) {
    REFLECT_MEMBER(last_modification_time);
    REFLECT_MEMBER(language);
    REFLECT_MEMBER(import_file);
    REFLECT_MEMBER(args);
  }
  REFLECT_MEMBER(includes);
  if (!gTestOutputMode)
    REFLECT_MEMBER(dependencies);
  REFLECT_MEMBER(skipped_by_preprocessor);
  REFLECT_MEMBER(types);
  REFLECT_MEMBER(funcs);
  REFLECT_MEMBER(vars);
  REFLECT_MEMBER_END();
}

void Reflect(Reader& visitor, std::monostate&) {
  visitor.GetNull();
}

void Reflect(Writer& visitor, std::monostate&) {
  visitor.Null();
}

void Reflect(Reader& visitor, SerializeFormat& value) {
  std::string fmt = visitor.GetString();
  value = fmt[0] == 'm' ? SerializeFormat::MessagePack : SerializeFormat::Json;
}

void Reflect(Writer& visitor, SerializeFormat& value) {
  switch (value) {
    case SerializeFormat::Json:
      visitor.String("json");
      break;
    case SerializeFormat::MessagePack:
      visitor.String("msgpack");
      break;
  }
}

std::string Serialize(SerializeFormat format, IndexFile& file) {
  switch (format) {
    case SerializeFormat::Json: {
      rapidjson::StringBuffer output;
      rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(output);
      writer.SetFormatOptions(
          rapidjson::PrettyFormatOptions::kFormatSingleLineArray);
      writer.SetIndent(' ', 2);
      JsonWriter json_writer(&writer);
      if (!gTestOutputMode) {
        std::string version = std::to_string(IndexFile::kMajorVersion);
        for (char c : version)
          output.Put(c);
        output.Put('\n');
      }
      Reflect(json_writer, file);
      return output.GetString();
    }
    case SerializeFormat::MessagePack: {
      msgpack::sbuffer buf;
      msgpack::packer<msgpack::sbuffer> pk(&buf);
      MessagePackWriter msgpack_writer(&pk);
      uint64_t magic = IndexFile::kMajorVersion;
      int version = IndexFile::kMinorVersion;
      Reflect(msgpack_writer, magic);
      Reflect(msgpack_writer, version);
      Reflect(msgpack_writer, file);
      return std::string(buf.data(), buf.size());
    }
  }
  return "";
}

std::unique_ptr<IndexFile> Deserialize(
    SerializeFormat format,
    const std::string& path,
    const std::string& serialized_index_content,
    const std::string& file_content,
    optional<int> expected_version) {
  if (serialized_index_content.empty())
    return nullptr;

  std::unique_ptr<IndexFile> file;
  switch (format) {
    case SerializeFormat::Json: {
      rapidjson::Document reader;
      if (gTestOutputMode || !expected_version) {
        reader.Parse(serialized_index_content.c_str());
      } else {
        const char* p = strchr(serialized_index_content.c_str(), '\n');
        if (!p)
          return nullptr;
        if (atoi(serialized_index_content.c_str()) != *expected_version)
          return nullptr;
        reader.Parse(p + 1);
      }
      if (reader.HasParseError())
        return nullptr;

      file = std::make_unique<IndexFile>(path, file_content);
      JsonReader json_reader{&reader};
      try {
        Reflect(json_reader, *file);
      } catch (std::invalid_argument& e) {
        LOG_S(INFO) << "'" << path << "': failed to deserialize "
                    << json_reader.GetPath() << "." << e.what();
        return nullptr;
      }
      break;
    }

    case SerializeFormat::MessagePack: {
      try {
        int major, minor;
        if (serialized_index_content.size() < 8)
          throw std::invalid_argument("Invalid");
        msgpack::unpacker upk;
        upk.reserve_buffer(serialized_index_content.size());
        memcpy(upk.buffer(), serialized_index_content.data(),
               serialized_index_content.size());
        upk.buffer_consumed(serialized_index_content.size());
        file = std::make_unique<IndexFile>(path, file_content);
        MessagePackReader reader(&upk);
        Reflect(reader, major);
        Reflect(reader, minor);
        if (major != IndexFile::kMajorVersion ||
            minor != IndexFile::kMinorVersion)
          throw std::invalid_argument("Invalid version");
        Reflect(reader, *file);
      } catch (std::invalid_argument& e) {
        LOG_S(INFO) << "Failed to deserialize msgpack '" << path
                    << "': " << e.what();
        return nullptr;
      }
      break;
    }
  }

  // Restore non-serialized state.
  file->path = path;
  file->id_cache.primary_file = file->path;
  for (const auto& type : file->types) {
    file->id_cache.type_id_to_usr[type.id] = type.usr;
    file->id_cache.usr_to_type_id[type.usr] = type.id;
  }
  for (const auto& func : file->funcs) {
    file->id_cache.func_id_to_usr[func.id] = func.usr;
    file->id_cache.usr_to_func_id[func.usr] = func.id;
  }
  for (const auto& var : file->vars) {
    file->id_cache.var_id_to_usr[var.id] = var.usr;
    file->id_cache.usr_to_var_id[var.usr] = var.id;
  }

  return file;
}

void SetTestOutputMode() {
  gTestOutputMode = true;
}

TEST_SUITE("Serializer utils") {
  TEST_CASE("GetBaseName") {
    REQUIRE(GetBaseName("foo.cc") == "foo.cc");
    REQUIRE(GetBaseName("foo/foo.cc") == "foo.cc");
    REQUIRE(GetBaseName("/foo.cc") == "foo.cc");
    REQUIRE(GetBaseName("///foo.cc") == "foo.cc");
    REQUIRE(GetBaseName("bar/") == "bar/");
    REQUIRE(GetBaseName("foobar/bar/") ==
            "foobar/bar/");  // TODO: Should be bar, but good enough.
  }
}
