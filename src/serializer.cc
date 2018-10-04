/* Copyright 2017-2018 ccls Authors

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#include "serializer.h"

#include "filesystem.hh"
#include "indexer.h"
#include "log.hh"
#include "serializers/binary.h"
#include "serializers/json.h"

#include <llvm/ADT/CachedHashString.h>
#include <llvm/ADT/DenseSet.h>

#include <mutex>
#include <stdexcept>

using namespace ccls;
using namespace llvm;

bool gTestOutputMode = false;

//// Elementary types

void Reflect(Reader &visitor, uint8_t &value) { value = visitor.GetUInt8(); }
void Reflect(Writer &visitor, uint8_t &value) { visitor.UInt8(value); }

void Reflect(Reader &visitor, short &value) {
  if (!visitor.IsInt())
    throw std::invalid_argument("short");
  value = (short)visitor.GetInt();
}
void Reflect(Writer &visitor, short &value) { visitor.Int(value); }

void Reflect(Reader &visitor, unsigned short &value) {
  if (!visitor.IsInt())
    throw std::invalid_argument("unsigned short");
  value = (unsigned short)visitor.GetInt();
}
void Reflect(Writer &visitor, unsigned short &value) { visitor.Int(value); }

void Reflect(Reader &visitor, int &value) {
  if (!visitor.IsInt())
    throw std::invalid_argument("int");
  value = visitor.GetInt();
}
void Reflect(Writer &visitor, int &value) { visitor.Int(value); }

void Reflect(Reader &visitor, unsigned &value) {
  if (!visitor.IsUInt64())
    throw std::invalid_argument("unsigned");
  value = visitor.GetUInt32();
}
void Reflect(Writer &visitor, unsigned &value) { visitor.UInt32(value); }

void Reflect(Reader &visitor, long &value) {
  if (!visitor.IsInt64())
    throw std::invalid_argument("long");
  value = long(visitor.GetInt64());
}
void Reflect(Writer &visitor, long &value) { visitor.Int64(value); }

void Reflect(Reader &visitor, unsigned long &value) {
  if (!visitor.IsUInt64())
    throw std::invalid_argument("unsigned long");
  value = (unsigned long)visitor.GetUInt64();
}
void Reflect(Writer &visitor, unsigned long &value) { visitor.UInt64(value); }

void Reflect(Reader &visitor, long long &value) {
  if (!visitor.IsInt64())
    throw std::invalid_argument("long long");
  value = visitor.GetInt64();
}
void Reflect(Writer &visitor, long long &value) { visitor.Int64(value); }

void Reflect(Reader &visitor, unsigned long long &value) {
  if (!visitor.IsUInt64())
    throw std::invalid_argument("unsigned long long");
  value = visitor.GetUInt64();
}
void Reflect(Writer &visitor, unsigned long long &value) {
  visitor.UInt64(value);
}

void Reflect(Reader &visitor, double &value) {
  if (!visitor.IsDouble())
    throw std::invalid_argument("double");
  value = visitor.GetDouble();
}
void Reflect(Writer &visitor, double &value) { visitor.Double(value); }

void Reflect(Reader &visitor, bool &value) {
  if (!visitor.IsBool())
    throw std::invalid_argument("bool");
  value = visitor.GetBool();
}
void Reflect(Writer &visitor, bool &value) { visitor.Bool(value); }

void Reflect(Reader &visitor, std::string &value) {
  if (!visitor.IsString())
    throw std::invalid_argument("std::string");
  value = visitor.GetString();
}
void Reflect(Writer &visitor, std::string &value) {
  visitor.String(value.c_str(), (rapidjson::SizeType)value.size());
}

void Reflect(Reader &, std::string_view &) { assert(0); }
void Reflect(Writer &visitor, std::string_view &data) {
  if (data.empty())
    visitor.String("");
  else
    visitor.String(&data[0], (rapidjson::SizeType)data.size());
}

void Reflect(Reader &vis, const char *&v) {
  const char *str = vis.GetString();
  v = Intern(str);
}
void Reflect(Writer &vis, const char *&v) { vis.String(v); }

void Reflect(Reader &visitor, JsonNull &value) {
  assert(visitor.Format() == SerializeFormat::Json);
  visitor.GetNull();
}

void Reflect(Writer &visitor, JsonNull &value) { visitor.Null(); }

// std::unordered_map
template <typename V>
void Reflect(Reader &visitor, std::unordered_map<Usr, V> &map) {
  visitor.IterArray([&](Reader &entry) {
    V val;
    Reflect(entry, val);
    auto usr = val.usr;
    map[usr] = std::move(val);
  });
}
template <typename V>
void Reflect(Writer &visitor, std::unordered_map<Usr, V> &map) {
  std::vector<std::pair<uint64_t, V>> xs(map.begin(), map.end());
  std::sort(xs.begin(), xs.end(),
            [](const auto &a, const auto &b) { return a.first < b.first; });
  visitor.StartArray(xs.size());
  for (auto &it : xs)
    Reflect(visitor, it.second);
  visitor.EndArray();
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
void Reflect(Reader &visitor, IndexInclude &value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER(line);
  REFLECT_MEMBER(resolved_path);
  REFLECT_MEMBER_END();
}
void Reflect(Writer &visitor, IndexInclude &value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER(line);
  if (gTestOutputMode) {
    std::string basename = llvm::sys::path::filename(value.resolved_path);
    if (!StartsWith(value.resolved_path, "&"))
      basename = "&" + basename;
    REFLECT_MEMBER2("resolved_path", basename);
  } else {
    REFLECT_MEMBER(resolved_path);
  }
  REFLECT_MEMBER_END();
}

template <typename Def>
void ReflectHoverAndComments(Reader &visitor, Def &def) {
  ReflectMember(visitor, "hover", def.hover);
  ReflectMember(visitor, "comments", def.comments);
}

template <typename Def>
void ReflectHoverAndComments(Writer &visitor, Def &def) {
  // Don't emit empty hover and comments in JSON test mode.
  if (!gTestOutputMode || def.hover[0])
    ReflectMember(visitor, "hover", def.hover);
  if (!gTestOutputMode || def.comments[0])
    ReflectMember(visitor, "comments", def.comments);
}

template <typename Def> void ReflectShortName(Reader &visitor, Def &def) {
  if (gTestOutputMode) {
    std::string short_name;
    ReflectMember(visitor, "short_name", short_name);
    def.short_name_offset =
        std::string_view(def.detailed_name).find(short_name);
    assert(def.short_name_offset != std::string::npos);
    def.short_name_size = short_name.size();
  } else {
    ReflectMember(visitor, "short_name_offset", def.short_name_offset);
    ReflectMember(visitor, "short_name_size", def.short_name_size);
  }
}

template <typename Def> void ReflectShortName(Writer &visitor, Def &def) {
  if (gTestOutputMode) {
    std::string_view short_name(def.detailed_name + def.short_name_offset,
                                def.short_name_size);
    ReflectMember(visitor, "short_name", short_name);
  } else {
    ReflectMember(visitor, "short_name_offset", def.short_name_offset);
    ReflectMember(visitor, "short_name_size", def.short_name_size);
  }
}

template <typename TVisitor> void Reflect(TVisitor &visitor, IndexFunc &value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER2("usr", value.usr);
  REFLECT_MEMBER2("detailed_name", value.def.detailed_name);
  REFLECT_MEMBER2("qual_name_offset", value.def.qual_name_offset);
  ReflectShortName(visitor, value.def);
  REFLECT_MEMBER2("spell", value.def.spell);
  ReflectHoverAndComments(visitor, value.def);
  REFLECT_MEMBER2("bases", value.def.bases);
  REFLECT_MEMBER2("vars", value.def.vars);
  REFLECT_MEMBER2("callees", value.def.callees);
  REFLECT_MEMBER2("kind", value.def.kind);
  REFLECT_MEMBER2("parent_kind", value.def.parent_kind);
  REFLECT_MEMBER2("storage", value.def.storage);

  REFLECT_MEMBER2("declarations", value.declarations);
  REFLECT_MEMBER2("derived", value.derived);
  REFLECT_MEMBER2("uses", value.uses);
  REFLECT_MEMBER_END();
}

template <typename TVisitor> void Reflect(TVisitor &visitor, IndexType &value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER2("usr", value.usr);
  REFLECT_MEMBER2("detailed_name", value.def.detailed_name);
  REFLECT_MEMBER2("qual_name_offset", value.def.qual_name_offset);
  ReflectShortName(visitor, value.def);
  ReflectHoverAndComments(visitor, value.def);
  REFLECT_MEMBER2("spell", value.def.spell);
  REFLECT_MEMBER2("bases", value.def.bases);
  REFLECT_MEMBER2("funcs", value.def.funcs);
  REFLECT_MEMBER2("types", value.def.types);
  REFLECT_MEMBER2("vars", value.def.vars);
  REFLECT_MEMBER2("alias_of", value.def.alias_of);
  REFLECT_MEMBER2("kind", value.def.kind);
  REFLECT_MEMBER2("parent_kind", value.def.parent_kind);

  REFLECT_MEMBER2("declarations", value.declarations);
  REFLECT_MEMBER2("derived", value.derived);
  REFLECT_MEMBER2("instances", value.instances);
  REFLECT_MEMBER2("uses", value.uses);
  REFLECT_MEMBER_END();
}

template <typename TVisitor> void Reflect(TVisitor &visitor, IndexVar &value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER2("usr", value.usr);
  REFLECT_MEMBER2("detailed_name", value.def.detailed_name);
  REFLECT_MEMBER2("qual_name_offset", value.def.qual_name_offset);
  ReflectShortName(visitor, value.def);
  ReflectHoverAndComments(visitor, value.def);
  REFLECT_MEMBER2("spell", value.def.spell);
  REFLECT_MEMBER2("type", value.def.type);
  REFLECT_MEMBER2("kind", value.def.kind);
  REFLECT_MEMBER2("parent_kind", value.def.parent_kind);
  REFLECT_MEMBER2("storage", value.def.storage);

  REFLECT_MEMBER2("declarations", value.declarations);
  REFLECT_MEMBER2("uses", value.uses);
  REFLECT_MEMBER_END();
}

// IndexFile
bool ReflectMemberStart(Writer &visitor, IndexFile &value) {
  visitor.StartObject();
  return true;
}
template <typename TVisitor> void Reflect(TVisitor &visitor, IndexFile &value) {
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

void Reflect(Writer &visitor, SerializeFormat &value) {
  switch (value) {
  case SerializeFormat::Binary:
    visitor.String("binary");
    break;
  case SerializeFormat::Json:
    visitor.String("json");
    break;
  }
}

namespace ccls {
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
