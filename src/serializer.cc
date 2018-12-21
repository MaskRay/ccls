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

#include "serializer.hh"

#include "filesystem.hh"
#include "indexer.hh"
#include "log.hh"
#include "message_handler.hh"

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>

#include <llvm/ADT/CachedHashString.h>
#include <llvm/ADT/DenseSet.h>

#include <mutex>
#include <stdexcept>

using namespace llvm;

bool gTestOutputMode = false;

namespace ccls {

void JsonReader::IterArray(std::function<void()> fn) {
  if (!m->IsArray())
    throw std::invalid_argument("array");
  // Use "0" to indicate any element for now.
  path_.push_back("0");
  for (auto &entry : m->GetArray()) {
    auto saved = m;
    m = &entry;
    fn();
    m = saved;
  }
  path_.pop_back();
}
void JsonReader::Member(const char *name, std::function<void()> fn) {
  path_.push_back(name);
  auto it = m->FindMember(name);
  if (it != m->MemberEnd()) {
    auto saved = m;
    m = &it->value;
    fn();
    m = saved;
  }
  path_.pop_back();
}
bool JsonReader::IsNull() { return m->IsNull(); }
std::string JsonReader::GetString() { return m->GetString(); }
std::string JsonReader::GetPath() const {
  std::string ret;
  for (auto &t : path_)
    if (t[0] == '0') {
      ret += '[';
      ret += t;
      ret += ']';
    } else {
      ret += '/';
      ret += t;
    }
  return ret;
}

void JsonWriter::StartArray() { m->StartArray(); }
void JsonWriter::EndArray() { m->EndArray(); }
void JsonWriter::StartObject() { m->StartObject(); }
void JsonWriter::EndObject() { m->EndObject(); }
void JsonWriter::Key(const char *name) { m->Key(name); }
void JsonWriter::Null() { m->Null(); }
void JsonWriter::Int(int v) { m->Int(v); }
void JsonWriter::String(const char *s) { m->String(s); }
void JsonWriter::String(const char *s, size_t len) { m->String(s, len); }

// clang-format off
void Reflect(JsonReader &vis, bool &v              ) { if (!vis.m->IsBool())   throw std::invalid_argument("bool");               v = vis.m->GetBool(); }
void Reflect(JsonReader &vis, unsigned char &v     ) { if (!vis.m->IsInt())    throw std::invalid_argument("uint8_t");            v = (uint8_t)vis.m->GetInt(); }
void Reflect(JsonReader &vis, short &v             ) { if (!vis.m->IsInt())    throw std::invalid_argument("short");              v = (short)vis.m->GetInt(); }
void Reflect(JsonReader &vis, unsigned short &v    ) { if (!vis.m->IsInt())    throw std::invalid_argument("unsigned short");     v = (unsigned short)vis.m->GetInt(); }
void Reflect(JsonReader &vis, int &v               ) { if (!vis.m->IsInt())    throw std::invalid_argument("int");                v = vis.m->GetInt(); }
void Reflect(JsonReader &vis, unsigned &v          ) { if (!vis.m->IsUint64()) throw std::invalid_argument("unsigned");           v = (unsigned)vis.m->GetUint64(); }
void Reflect(JsonReader &vis, long &v              ) { if (!vis.m->IsInt64())  throw std::invalid_argument("long");               v = (long)vis.m->GetInt64(); }
void Reflect(JsonReader &vis, unsigned long &v     ) { if (!vis.m->IsUint64()) throw std::invalid_argument("unsigned long");      v = (unsigned long)vis.m->GetUint64(); }
void Reflect(JsonReader &vis, long long &v         ) { if (!vis.m->IsInt64())  throw std::invalid_argument("long long");          v = vis.m->GetInt64(); }
void Reflect(JsonReader &vis, unsigned long long &v) { if (!vis.m->IsUint64()) throw std::invalid_argument("unsigned long long"); v = vis.m->GetUint64(); }
void Reflect(JsonReader &vis, double &v            ) { if (!vis.m->IsDouble()) throw std::invalid_argument("double");             v = vis.m->GetDouble(); }
void Reflect(JsonReader &vis, const char *&v       ) { if (!vis.m->IsString()) throw std::invalid_argument("string");             v = Intern(vis.GetString()); }
void Reflect(JsonReader &vis, std::string &v       ) { if (!vis.m->IsString()) throw std::invalid_argument("string");             v = vis.GetString(); }

void Reflect(JsonWriter &vis, bool &v              ) { vis.m->Bool(v); }
void Reflect(JsonWriter &vis, unsigned char &v     ) { vis.m->Int(v); }
void Reflect(JsonWriter &vis, short &v             ) { vis.m->Int(v); }
void Reflect(JsonWriter &vis, unsigned short &v    ) { vis.m->Int(v); }
void Reflect(JsonWriter &vis, int &v               ) { vis.m->Int(v); }
void Reflect(JsonWriter &vis, unsigned &v          ) { vis.m->Uint64(v); }
void Reflect(JsonWriter &vis, long &v              ) { vis.m->Int64(v); }
void Reflect(JsonWriter &vis, unsigned long &v     ) { vis.m->Uint64(v); }
void Reflect(JsonWriter &vis, long long &v         ) { vis.m->Int64(v); }
void Reflect(JsonWriter &vis, unsigned long long &v) { vis.m->Uint64(v); }
void Reflect(JsonWriter &vis, double &v            ) { vis.m->Double(v); }
void Reflect(JsonWriter &vis, const char *&v       ) { vis.String(v); }
void Reflect(JsonWriter &vis, std::string &v       ) { vis.String(v.c_str(), v.size()); }

void Reflect(BinaryReader &vis, bool &v              ) { v = vis.Get<bool>(); }
void Reflect(BinaryReader &vis, unsigned char &v     ) { v = vis.Get<unsigned char>(); }
void Reflect(BinaryReader &vis, short &v             ) { v = (short)vis.VarInt(); }
void Reflect(BinaryReader &vis, unsigned short &v    ) { v = (unsigned short)vis.VarUInt(); }
void Reflect(BinaryReader &vis, int &v               ) { v = (int)vis.VarInt(); }
void Reflect(BinaryReader &vis, unsigned &v          ) { v = (unsigned)vis.VarUInt(); }
void Reflect(BinaryReader &vis, long &v              ) { v = (long)vis.VarInt(); }
void Reflect(BinaryReader &vis, unsigned long &v     ) { v = (unsigned long)vis.VarUInt(); }
void Reflect(BinaryReader &vis, long long &v         ) { v = vis.VarInt(); }
void Reflect(BinaryReader &vis, unsigned long long &v) { v = vis.VarUInt(); }
void Reflect(BinaryReader &vis, double &v            ) { v = vis.Get<double>(); }
void Reflect(BinaryReader &vis, const char *&v       ) { v = Intern(vis.GetString()); }
void Reflect(BinaryReader &vis, std::string &v       ) { v = vis.GetString(); }

void Reflect(BinaryWriter &vis, bool &v              ) { vis.Pack(v); }
void Reflect(BinaryWriter &vis, unsigned char &v     ) { vis.Pack(v); }
void Reflect(BinaryWriter &vis, short &v             ) { vis.VarInt(v); }
void Reflect(BinaryWriter &vis, unsigned short &v    ) { vis.VarUInt(v); }
void Reflect(BinaryWriter &vis, int &v               ) { vis.VarInt(v); }
void Reflect(BinaryWriter &vis, unsigned &v          ) { vis.VarUInt(v); }
void Reflect(BinaryWriter &vis, long &v              ) { vis.VarInt(v); }
void Reflect(BinaryWriter &vis, unsigned long &v     ) { vis.VarUInt(v); }
void Reflect(BinaryWriter &vis, long long &v         ) { vis.VarInt(v); }
void Reflect(BinaryWriter &vis, unsigned long long &v) { vis.VarUInt(v); }
void Reflect(BinaryWriter &vis, double &v            ) { vis.Pack(v); }
void Reflect(BinaryWriter &vis, const char *&v       ) { vis.String(v); }
void Reflect(BinaryWriter &vis, std::string &v       ) { vis.String(v.c_str(), v.size()); }
// clang-format on

void Reflect(JsonWriter &vis, std::string_view &data) {
  if (data.empty())
    vis.String("");
  else
    vis.String(&data[0], (rapidjson::SizeType)data.size());
}

void Reflect(JsonReader &vis, JsonNull &v) {}
void Reflect(JsonWriter &vis, JsonNull &v) { vis.m->Null(); }

template <typename V>
void Reflect(JsonReader &vis, std::unordered_map<Usr, V> &v) {
  vis.IterArray([&]() {
    V val;
    Reflect(vis, val);
    v[val.usr] = std::move(val);
  });
}
template <typename V>
void Reflect(JsonWriter &vis, std::unordered_map<Usr, V> &v) {
  // Determinism
  std::vector<std::pair<uint64_t, V>> xs(v.begin(), v.end());
  std::sort(xs.begin(), xs.end(),
            [](const auto &a, const auto &b) { return a.first < b.first; });
  vis.StartArray();
  for (auto &it : xs)
    Reflect(vis, it.second);
  vis.EndArray();
}
template <typename V>
void Reflect(BinaryReader &vis, std::unordered_map<Usr, V> &v) {
  for (auto n = vis.VarUInt(); n; n--) {
    V val;
    Reflect(vis, val);
    v[val.usr] = std::move(val);
  }
}
template <typename V>
void Reflect(BinaryWriter &vis, std::unordered_map<Usr, V> &v) {
  vis.VarUInt(v.size());
  for (auto &it : v)
    Reflect(vis, it.second);
}

// Used by IndexFile::dependencies.
void Reflect(JsonReader &vis, DenseMap<CachedHashStringRef, int64_t> &v) {
  std::string name;
  for (auto it = vis.m->MemberBegin(); it != vis.m->MemberEnd(); ++it)
    v[InternH(it->name.GetString())] = it->value.GetInt64();
}
void Reflect(JsonWriter &vis, DenseMap<CachedHashStringRef, int64_t> &v) {
  vis.StartObject();
  for (auto &it : v) {
    vis.m->Key(it.first.val().data()); // llvm 8 -> data()
    vis.m->Int64(it.second);
  }
  vis.EndObject();
}
void Reflect(BinaryReader &vis, DenseMap<CachedHashStringRef, int64_t> &v) {
  std::string name;
  for (auto n = vis.VarUInt(); n; n--) {
    Reflect(vis, name);
    Reflect(vis, v[InternH(name)]);
  }
}
void Reflect(BinaryWriter &vis, DenseMap<CachedHashStringRef, int64_t> &v) {
  std::string key;
  vis.VarUInt(v.size());
  for (auto &it : v) {
    key = it.first.val().str();
    Reflect(vis, key);
    Reflect(vis, it.second);
  }
}

template <typename Vis> void Reflect(Vis &vis, IndexInclude &v) {
  ReflectMemberStart(vis);
  REFLECT_MEMBER(line);
  REFLECT_MEMBER(resolved_path);
  ReflectMemberEnd(vis);
}
void Reflect(JsonWriter &vis, IndexInclude &v) {
  ReflectMemberStart(vis);
  REFLECT_MEMBER(line);
  if (gTestOutputMode) {
    std::string basename = llvm::sys::path::filename(v.resolved_path);
    if (v.resolved_path[0] != '&')
      basename = "&" + basename;
    REFLECT_MEMBER2("resolved_path", basename);
  } else {
    REFLECT_MEMBER(resolved_path);
  }
  ReflectMemberEnd(vis);
}

template <typename Def>
void ReflectHoverAndComments(JsonReader &vis, Def &def) {
  ReflectMember(vis, "hover", def.hover);
  ReflectMember(vis, "comments", def.comments);
}
template <typename Def>
void ReflectHoverAndComments(JsonWriter &vis, Def &def) {
  // Don't emit empty hover and comments in JSON test mode.
  if (!gTestOutputMode || def.hover[0])
    ReflectMember(vis, "hover", def.hover);
  if (!gTestOutputMode || def.comments[0])
    ReflectMember(vis, "comments", def.comments);
}
template <typename Def>
void ReflectHoverAndComments(BinaryReader &vis, Def &def) {
  Reflect(vis, def.hover);
  Reflect(vis, def.comments);
}
template <typename Def>
void ReflectHoverAndComments(BinaryWriter &vis, Def &def) {
  Reflect(vis, def.hover);
  Reflect(vis, def.comments);
}

template <typename Def> void ReflectShortName(JsonReader &vis, Def &def) {
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
template <typename Def> void ReflectShortName(JsonWriter &vis, Def &def) {
  if (gTestOutputMode) {
    std::string_view short_name(def.detailed_name + def.short_name_offset,
                                def.short_name_size);
    ReflectMember(vis, "short_name", short_name);
  } else {
    ReflectMember(vis, "short_name_offset", def.short_name_offset);
    ReflectMember(vis, "short_name_size", def.short_name_size);
  }
}
template <typename Def> void ReflectShortName(BinaryReader &vis, Def &def) {
  Reflect(vis, def.short_name_offset);
  Reflect(vis, def.short_name_size);
}
template <typename Def> void ReflectShortName(BinaryWriter &vis, Def &def) {
  Reflect(vis, def.short_name_offset);
  Reflect(vis, def.short_name_size);
}

template <typename TVisitor> void Reflect1(TVisitor &vis, IndexFunc &v) {
  ReflectMemberStart(vis);
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
  ReflectMemberEnd(vis);
}
void Reflect(JsonReader &vis, IndexFunc &v) { Reflect1(vis, v); }
void Reflect(JsonWriter &vis, IndexFunc &v) { Reflect1(vis, v); }
void Reflect(BinaryReader &vis, IndexFunc &v) { Reflect1(vis, v); }
void Reflect(BinaryWriter &vis, IndexFunc &v) { Reflect1(vis, v); }

template <typename TVisitor> void Reflect1(TVisitor &vis, IndexType &v) {
  ReflectMemberStart(vis);
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
  ReflectMemberEnd(vis);
}
void Reflect(JsonReader &vis, IndexType &v) { Reflect1(vis, v); }
void Reflect(JsonWriter &vis, IndexType &v) { Reflect1(vis, v); }
void Reflect(BinaryReader &vis, IndexType &v) { Reflect1(vis, v); }
void Reflect(BinaryWriter &vis, IndexType &v) { Reflect1(vis, v); }

template <typename TVisitor> void Reflect1(TVisitor &vis, IndexVar &v) {
  ReflectMemberStart(vis);
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
  ReflectMemberEnd(vis);
}
void Reflect(JsonReader &vis, IndexVar &v) { Reflect1(vis, v); }
void Reflect(JsonWriter &vis, IndexVar &v) { Reflect1(vis, v); }
void Reflect(BinaryReader &vis, IndexVar &v) { Reflect1(vis, v); }
void Reflect(BinaryWriter &vis, IndexVar &v) { Reflect1(vis, v); }

// IndexFile
template <typename TVisitor> void Reflect1(TVisitor &vis, IndexFile &v) {
  ReflectMemberStart(vis);
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
  ReflectMemberEnd(vis);
}
void ReflectFile(JsonReader &vis, IndexFile &v) { Reflect1(vis, v); }
void ReflectFile(JsonWriter &vis, IndexFile &v) { Reflect1(vis, v); }
void ReflectFile(BinaryReader &vis, IndexFile &v) { Reflect1(vis, v); }
void ReflectFile(BinaryWriter &vis, IndexFile &v) { Reflect1(vis, v); }

void Reflect(JsonReader &vis, SerializeFormat &v) {
  v = vis.GetString()[0] == 'j' ? SerializeFormat::Json
                                : SerializeFormat::Binary;
}

void Reflect(JsonWriter &vis, SerializeFormat &v) {
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
    ReflectFile(writer, file);
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
    ReflectFile(json_writer, file);
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
      file = std::make_unique<IndexFile>(path, file_content);
      ReflectFile(reader, *file);
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

    file = std::make_unique<IndexFile>(path, file_content);
    JsonReader json_reader{&reader};
    try {
      ReflectFile(json_reader, *file);
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
