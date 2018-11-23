// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#include "serializer.hh"

#include "filesystem.hh"
#include "indexer.hh"
#include "log.hh"
#include "message_handler.hh"
#include "serializers/binary.hh"
#include "serializers/json.hh"

#include <llvm/ADT/CachedHashString.h>
#include <llvm/ADT/DenseSet.h>

#include <mutex>
#include <stdexcept>

using namespace llvm;

bool gTestOutputMode = false;

namespace ccls {
Reader::~Reader() {}
BinaryReader::~BinaryReader() {}
JsonReader::~JsonReader() {}

Writer::~Writer() {}
BinaryWriter::~BinaryWriter() {}
JsonWriter::~JsonWriter() {}

void Reflect(Reader &vis, uint8_t &v) { v = vis.GetUInt8(); }
void Reflect(Writer &vis, uint8_t &v) { vis.UInt8(v); }

void Reflect(Reader &vis, short &v) {
  if (!vis.IsInt())
    throw std::invalid_argument("short");
  v = (short)vis.GetInt();
}
void Reflect(Writer &vis, short &v) { vis.Int(v); }

void Reflect(Reader &vis, unsigned short &v) {
  if (!vis.IsInt())
    throw std::invalid_argument("unsigned short");
  v = (unsigned short)vis.GetInt();
}
void Reflect(Writer &vis, unsigned short &v) { vis.Int(v); }

void Reflect(Reader &vis, int &v) {
  if (!vis.IsInt())
    throw std::invalid_argument("int");
  v = vis.GetInt();
}
void Reflect(Writer &vis, int &v) { vis.Int(v); }

void Reflect(Reader &vis, unsigned &v) {
  if (!vis.IsUInt64())
    throw std::invalid_argument("unsigned");
  v = vis.GetUInt32();
}
void Reflect(Writer &vis, unsigned &v) { vis.UInt32(v); }

void Reflect(Reader &vis, long &v) {
  if (!vis.IsInt64())
    throw std::invalid_argument("long");
  v = long(vis.GetInt64());
}
void Reflect(Writer &vis, long &v) { vis.Int64(v); }

void Reflect(Reader &vis, unsigned long &v) {
  if (!vis.IsUInt64())
    throw std::invalid_argument("unsigned long");
  v = (unsigned long)vis.GetUInt64();
}
void Reflect(Writer &vis, unsigned long &v) { vis.UInt64(v); }

void Reflect(Reader &vis, long long &v) {
  if (!vis.IsInt64())
    throw std::invalid_argument("long long");
  v = vis.GetInt64();
}
void Reflect(Writer &vis, long long &v) { vis.Int64(v); }

void Reflect(Reader &vis, unsigned long long &v) {
  if (!vis.IsUInt64())
    throw std::invalid_argument("unsigned long long");
  v = vis.GetUInt64();
}
void Reflect(Writer &vis, unsigned long long &v) { vis.UInt64(v); }

void Reflect(Reader &vis, double &v) {
  if (!vis.IsDouble())
    throw std::invalid_argument("double");
  v = vis.GetDouble();
}
void Reflect(Writer &vis, double &v) { vis.Double(v); }

void Reflect(Reader &vis, bool &v) {
  if (!vis.IsBool())
    throw std::invalid_argument("bool");
  v = vis.GetBool();
}
void Reflect(Writer &vis, bool &v) { vis.Bool(v); }

void Reflect(Reader &vis, std::string &v) {
  if (!vis.IsString())
    throw std::invalid_argument("std::string");
  v = vis.GetString();
}
void Reflect(Writer &vis, std::string &v) {
  vis.String(v.c_str(), (rapidjson::SizeType)v.size());
}

void Reflect(Reader &, std::string_view &) { assert(0); }
void Reflect(Writer &vis, std::string_view &data) {
  if (data.empty())
    vis.String("");
  else
    vis.String(&data[0], (rapidjson::SizeType)data.size());
}

void Reflect(Reader &vis, const char *&v) {
  const char *str = vis.GetString();
  v = Intern(str);
}
void Reflect(Writer &vis, const char *&v) { vis.String(v); }

void Reflect(Reader &vis, JsonNull &v) {
  assert(vis.Format() == SerializeFormat::Json);
  vis.GetNull();
}

void Reflect(Writer &vis, JsonNull &v) { vis.Null(); }

// std::unordered_map
template <typename V>
void Reflect(Reader &vis, std::unordered_map<Usr, V> &map) {
  vis.IterArray([&](Reader &entry) {
    V val;
    Reflect(entry, val);
    auto usr = val.usr;
    map[usr] = std::move(val);
  });
}
template <typename V>
void Reflect(Writer &vis, std::unordered_map<Usr, V> &map) {
  std::vector<std::pair<uint64_t, V>> xs(map.begin(), map.end());
  std::sort(xs.begin(), xs.end(),
            [](const auto &a, const auto &b) { return a.first < b.first; });
  vis.StartArray(xs.size());
  for (auto &it : xs)
    Reflect(vis, it.second);
  vis.EndArray();
}

// Used by IndexFile::dependencies.
void Reflect(Reader &vis, DenseMap<CachedHashStringRef, int64_t> &v) {
  std::string name;
  if (vis.Format() == SerializeFormat::Json) {
    auto &vis1 = static_cast<JsonReader&>(vis);
    for (auto it = vis1.m().MemberBegin(); it != vis1.m().MemberEnd(); ++it)
      v[InternH(it->name.GetString())] = it->value.GetInt64();
  } else {
    vis.IterArray([&](Reader &entry) {
      Reflect(entry, name);
      Reflect(entry, v[InternH(name)]);
    });
  }
}
void Reflect(Writer &vis, DenseMap<CachedHashStringRef, int64_t> &v) {
  if (vis.Format() == SerializeFormat::Json) {
    auto &vis1 = static_cast<JsonWriter&>(vis);
    vis.StartObject();
    for (auto &it : v) {
      vis1.m().Key(it.first.val().data()); // llvm 8 -> data()
      vis1.m().Int64(it.second);
    }
    vis.EndObject();
  } else {
    vis.StartArray(v.size());
    for (auto &it : v) {
      std::string key = it.first.val().str();
      Reflect(vis, key);
      Reflect(vis, it.second);
    }
    vis.EndArray();
  }
}

// TODO: Move this to indexer.cc
void Reflect(Reader &vis, IndexInclude &v) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER(line);
  REFLECT_MEMBER(resolved_path);
  REFLECT_MEMBER_END();
}
void Reflect(Writer &vis, IndexInclude &v) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER(line);
  if (gTestOutputMode) {
    std::string basename = llvm::sys::path::filename(v.resolved_path);
    if (v.resolved_path[0] != '&')
      basename = "&" + basename;
    REFLECT_MEMBER2("resolved_path", basename);
  } else {
    REFLECT_MEMBER(resolved_path);
  }
  REFLECT_MEMBER_END();
}

template <typename Def> void ReflectHoverAndComments(Reader &vis, Def &def) {
  ReflectMember(vis, "hover", def.hover);
  ReflectMember(vis, "comments", def.comments);
}

template <typename Def> void ReflectHoverAndComments(Writer &vis, Def &def) {
  // Don't emit empty hover and comments in JSON test mode.
  if (!gTestOutputMode || def.hover[0])
    ReflectMember(vis, "hover", def.hover);
  if (!gTestOutputMode || def.comments[0])
    ReflectMember(vis, "comments", def.comments);
}

template <typename Def> void ReflectShortName(Reader &vis, Def &def) {
  if (gTestOutputMode) {
    std::string short_name;
    ReflectMember(vis, "short_name", short_name);
    def.short_name_offset =
        std::string_view(def.detailed_name).find(short_name);
    assert(def.short_name_offset != std::string::npos);
    def.short_name_size = short_name.size();
  } else {
    ReflectMember(vis, "short_name_offset", def.short_name_offset);
    ReflectMember(vis, "short_name_size", def.short_name_size);
  }
}

template <typename Def> void ReflectShortName(Writer &vis, Def &def) {
  if (gTestOutputMode) {
    std::string_view short_name(def.detailed_name + def.short_name_offset,
                                def.short_name_size);
    ReflectMember(vis, "short_name", short_name);
  } else {
    ReflectMember(vis, "short_name_offset", def.short_name_offset);
    ReflectMember(vis, "short_name_size", def.short_name_size);
  }
}

template <typename TVisitor> void Reflect(TVisitor &vis, IndexFunc &v) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER2("usr", v.usr);
  REFLECT_MEMBER2("detailed_name", v.def.detailed_name);
  REFLECT_MEMBER2("qual_name_offset", v.def.qual_name_offset);
  ReflectShortName(vis, v.def);
  REFLECT_MEMBER2("spell", v.def.spell);
  ReflectHoverAndComments(vis, v.def);
  REFLECT_MEMBER2("bases", v.def.bases);
  REFLECT_MEMBER2("vars", v.def.vars);
  REFLECT_MEMBER2("callees", v.def.callees);
  REFLECT_MEMBER2("kind", v.def.kind);
  REFLECT_MEMBER2("parent_kind", v.def.parent_kind);
  REFLECT_MEMBER2("storage", v.def.storage);

  REFLECT_MEMBER2("declarations", v.declarations);
  REFLECT_MEMBER2("derived", v.derived);
  REFLECT_MEMBER2("uses", v.uses);
  REFLECT_MEMBER_END();
}

template <typename TVisitor> void Reflect(TVisitor &vis, IndexType &v) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER2("usr", v.usr);
  REFLECT_MEMBER2("detailed_name", v.def.detailed_name);
  REFLECT_MEMBER2("qual_name_offset", v.def.qual_name_offset);
  ReflectShortName(vis, v.def);
  ReflectHoverAndComments(vis, v.def);
  REFLECT_MEMBER2("spell", v.def.spell);
  REFLECT_MEMBER2("bases", v.def.bases);
  REFLECT_MEMBER2("funcs", v.def.funcs);
  REFLECT_MEMBER2("types", v.def.types);
  REFLECT_MEMBER2("vars", v.def.vars);
  REFLECT_MEMBER2("alias_of", v.def.alias_of);
  REFLECT_MEMBER2("kind", v.def.kind);
  REFLECT_MEMBER2("parent_kind", v.def.parent_kind);

  REFLECT_MEMBER2("declarations", v.declarations);
  REFLECT_MEMBER2("derived", v.derived);
  REFLECT_MEMBER2("instances", v.instances);
  REFLECT_MEMBER2("uses", v.uses);
  REFLECT_MEMBER_END();
}

template <typename TVisitor> void Reflect(TVisitor &vis, IndexVar &v) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER2("usr", v.usr);
  REFLECT_MEMBER2("detailed_name", v.def.detailed_name);
  REFLECT_MEMBER2("qual_name_offset", v.def.qual_name_offset);
  ReflectShortName(vis, v.def);
  ReflectHoverAndComments(vis, v.def);
  REFLECT_MEMBER2("spell", v.def.spell);
  REFLECT_MEMBER2("type", v.def.type);
  REFLECT_MEMBER2("kind", v.def.kind);
  REFLECT_MEMBER2("parent_kind", v.def.parent_kind);
  REFLECT_MEMBER2("storage", v.def.storage);

  REFLECT_MEMBER2("declarations", v.declarations);
  REFLECT_MEMBER2("uses", v.uses);
  REFLECT_MEMBER_END();
}

// IndexFile
bool ReflectMemberStart(Writer &vis, IndexFile &v) {
  vis.StartObject();
  return true;
}
template <typename TVisitor> void Reflect(TVisitor &vis, IndexFile &v) {
  REFLECT_MEMBER_START();
  if (!gTestOutputMode) {
    REFLECT_MEMBER(mtime);
    REFLECT_MEMBER(language);
    REFLECT_MEMBER(lid2path);
    REFLECT_MEMBER(import_file);
    REFLECT_MEMBER(args);
    REFLECT_MEMBER(dependencies);
  }
  REFLECT_MEMBER(includes);
  REFLECT_MEMBER(skipped_ranges);
  REFLECT_MEMBER(usr2func);
  REFLECT_MEMBER(usr2type);
  REFLECT_MEMBER(usr2var);
  REFLECT_MEMBER_END();
}

void Reflect(Reader &vis, SerializeFormat &v) {
  v = vis.GetString()[0] == 'j' ? SerializeFormat::Json
                                : SerializeFormat::Binary;
}

void Reflect(Writer &vis, SerializeFormat &v) {
  switch (v) {
  case SerializeFormat::Binary:
    vis.String("binary");
    break;
  case SerializeFormat::Json:
    vis.String("json");
    break;
  }
}

static BumpPtrAllocator Alloc;
static DenseSet<CachedHashStringRef> Strings;
static std::mutex AllocMutex;

CachedHashStringRef InternH(StringRef S) {
  if (S.empty())
    S = "";
  CachedHashString HS(S);
  std::lock_guard lock(AllocMutex);
  auto R = Strings.insert(HS);
  if (R.second) {
    char *P = Alloc.Allocate<char>(S.size() + 1);
    memcpy(P, S.data(), S.size());
    P[S.size()] = '\0';
    *R.first = CachedHashStringRef(StringRef(P, S.size()), HS.hash());
  }
  return *R.first;
}

const char *Intern(StringRef S) {
  return InternH(S).val().data();
}

std::string Serialize(SerializeFormat format, IndexFile &file) {
  switch (format) {
  case SerializeFormat::Binary: {
    BinaryWriter writer;
    int major = IndexFile::kMajorVersion;
    int minor = IndexFile::kMinorVersion;
    Reflect(writer, major);
    Reflect(writer, minor);
    Reflect(writer, file);
    return writer.Take();
  }
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
  }
  return "";
}

std::unique_ptr<IndexFile>
Deserialize(SerializeFormat format, const std::string &path,
            const std::string &serialized_index_content,
            const std::string &file_content,
            std::optional<int> expected_version) {
  if (serialized_index_content.empty())
    return nullptr;

  std::unique_ptr<IndexFile> file;
  switch (format) {
  case SerializeFormat::Binary: {
    try {
      int major, minor;
      if (serialized_index_content.size() < 8)
        throw std::invalid_argument("Invalid");
      BinaryReader reader(serialized_index_content);
      Reflect(reader, major);
      Reflect(reader, minor);
      if (major != IndexFile::kMajorVersion ||
          minor != IndexFile::kMinorVersion)
        throw std::invalid_argument("Invalid version");
      file = std::make_unique<IndexFile>(sys::fs::UniqueID(0, 0), path,
                                         file_content);
      Reflect(reader, *file);
    } catch (std::invalid_argument &e) {
      LOG_S(INFO) << "failed to deserialize '" << path << "': " << e.what();
      return nullptr;
    }
    break;
  }
  case SerializeFormat::Json: {
    rapidjson::Document reader;
    if (gTestOutputMode || !expected_version) {
      reader.Parse(serialized_index_content.c_str());
    } else {
      const char *p = strchr(serialized_index_content.c_str(), '\n');
      if (!p)
        return nullptr;
      if (atoi(serialized_index_content.c_str()) != *expected_version)
        return nullptr;
      reader.Parse(p + 1);
    }
    if (reader.HasParseError())
      return nullptr;

    file = std::make_unique<IndexFile>(sys::fs::UniqueID(0, 0), path,
                                       file_content);
    JsonReader json_reader{&reader};
    try {
      Reflect(json_reader, *file);
    } catch (std::invalid_argument &e) {
      LOG_S(INFO) << "'" << path << "': failed to deserialize "
                  << json_reader.GetPath() << "." << e.what();
      return nullptr;
    }
    break;
  }
  }

  // Restore non-serialized state.
  file->path = path;
  if (g_config->clang.pathMappings.size()) {
    DoPathMapping(file->import_file);
    std::vector<const char *> args;
    for (const char *arg : file->args) {
      std::string s(arg);
      DoPathMapping(s);
      args.push_back(Intern(s));
    }
    file->args = std::move(args);
    for (auto &[_, path] : file->lid2path)
      DoPathMapping(path);
    for (auto &include : file->includes) {
      std::string p(include.resolved_path);
      DoPathMapping(p);
      include.resolved_path = Intern(p);
    }
    decltype(file->dependencies) dependencies;
    for (auto &it : file->dependencies) {
      std::string path = it.first.val().str();
      DoPathMapping(path);
      dependencies[InternH(path)] = it.second;
    }
    file->dependencies = std::move(dependencies);
  }
  return file;
}
} // namespace ccls
