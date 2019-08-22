// Copyright 2017-2020 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#include "serializer.hh"

#include "filesystem.hh"
#include "indexer.hh"
#include "log.hh"
#include "message_handler.hh"

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>

#include <llvm/ADT/CachedHashString.h>
#include <llvm/ADT/DenseSet.h>
#include <llvm/Support/Allocator.h>

#include <mutex>
#include <stdexcept>

using namespace llvm;

bool gTestOutputMode = false;

namespace ccls {

void JsonReader::iterArray(std::function<void()> fn) {
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
void JsonReader::member(const char *name, std::function<void()> fn) {
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
bool JsonReader::isNull() { return m->IsNull(); }
std::string JsonReader::getString() { return m->GetString(); }
std::string JsonReader::getPath() const {
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

void JsonWriter::startArray() { m->StartArray(); }
void JsonWriter::endArray() { m->EndArray(); }
void JsonWriter::startObject() { m->StartObject(); }
void JsonWriter::endObject() { m->EndObject(); }
void JsonWriter::key(const char *name) { m->Key(name); }
void JsonWriter::null_() { m->Null(); }
void JsonWriter::int_(int v) { m->Int(v); }
void JsonWriter::string(const char *s) { m->String(s); }
void JsonWriter::string(const char *s, size_t len) { m->String(s, len); }

// clang-format off
void reflect(JsonReader &vis, bool &v              ) { if (!vis.m->IsBool())   throw std::invalid_argument("bool");               v = vis.m->GetBool(); }
void reflect(JsonReader &vis, unsigned char &v     ) { if (!vis.m->IsInt())    throw std::invalid_argument("uint8_t");            v = (uint8_t)vis.m->GetInt(); }
void reflect(JsonReader &vis, short &v             ) { if (!vis.m->IsInt())    throw std::invalid_argument("short");              v = (short)vis.m->GetInt(); }
void reflect(JsonReader &vis, unsigned short &v    ) { if (!vis.m->IsInt())    throw std::invalid_argument("unsigned short");     v = (unsigned short)vis.m->GetInt(); }
void reflect(JsonReader &vis, int &v               ) { if (!vis.m->IsInt())    throw std::invalid_argument("int");                v = vis.m->GetInt(); }
void reflect(JsonReader &vis, unsigned &v          ) { if (!vis.m->IsUint64()) throw std::invalid_argument("unsigned");           v = (unsigned)vis.m->GetUint64(); }
void reflect(JsonReader &vis, long &v              ) { if (!vis.m->IsInt64())  throw std::invalid_argument("long");               v = (long)vis.m->GetInt64(); }
void reflect(JsonReader &vis, unsigned long &v     ) { if (!vis.m->IsUint64()) throw std::invalid_argument("unsigned long");      v = (unsigned long)vis.m->GetUint64(); }
void reflect(JsonReader &vis, long long &v         ) { if (!vis.m->IsInt64())  throw std::invalid_argument("long long");          v = vis.m->GetInt64(); }
void reflect(JsonReader &vis, unsigned long long &v) { if (!vis.m->IsUint64()) throw std::invalid_argument("unsigned long long"); v = vis.m->GetUint64(); }
void reflect(JsonReader &vis, double &v            ) { if (!vis.m->IsDouble()) throw std::invalid_argument("double");             v = vis.m->GetDouble(); }
void reflect(JsonReader &vis, const char *&v       ) { if (!vis.m->IsString()) throw std::invalid_argument("string");             v = intern(vis.getString()); }
void reflect(JsonReader &vis, std::string &v       ) { if (!vis.m->IsString()) throw std::invalid_argument("string");             v = vis.getString(); }

void reflect(JsonWriter &vis, bool &v              ) { vis.m->Bool(v); }
void reflect(JsonWriter &vis, unsigned char &v     ) { vis.m->Int(v); }
void reflect(JsonWriter &vis, short &v             ) { vis.m->Int(v); }
void reflect(JsonWriter &vis, unsigned short &v    ) { vis.m->Int(v); }
void reflect(JsonWriter &vis, int &v               ) { vis.m->Int(v); }
void reflect(JsonWriter &vis, unsigned &v          ) { vis.m->Uint64(v); }
void reflect(JsonWriter &vis, long &v              ) { vis.m->Int64(v); }
void reflect(JsonWriter &vis, unsigned long &v     ) { vis.m->Uint64(v); }
void reflect(JsonWriter &vis, long long &v         ) { vis.m->Int64(v); }
void reflect(JsonWriter &vis, unsigned long long &v) { vis.m->Uint64(v); }
void reflect(JsonWriter &vis, double &v            ) { vis.m->Double(v); }
void reflect(JsonWriter &vis, const char *&v       ) { vis.string(v); }
void reflect(JsonWriter &vis, std::string &v       ) { vis.string(v.c_str(), v.size()); }

void reflect(BinaryReader &vis, bool &v              ) { v = vis.get<bool>(); }
void reflect(BinaryReader &vis, unsigned char &v     ) { v = vis.get<unsigned char>(); }
void reflect(BinaryReader &vis, short &v             ) { v = (short)vis.varInt(); }
void reflect(BinaryReader &vis, unsigned short &v    ) { v = (unsigned short)vis.varUInt(); }
void reflect(BinaryReader &vis, int &v               ) { v = (int)vis.varInt(); }
void reflect(BinaryReader &vis, unsigned &v          ) { v = (unsigned)vis.varUInt(); }
void reflect(BinaryReader &vis, long &v              ) { v = (long)vis.varInt(); }
void reflect(BinaryReader &vis, unsigned long &v     ) { v = (unsigned long)vis.varUInt(); }
void reflect(BinaryReader &vis, long long &v         ) { v = vis.varInt(); }
void reflect(BinaryReader &vis, unsigned long long &v) { v = vis.varUInt(); }
void reflect(BinaryReader &vis, double &v            ) { v = vis.get<double>(); }
void reflect(BinaryReader &vis, const char *&v       ) { v = intern(vis.getString()); }
void reflect(BinaryReader &vis, std::string &v       ) { v = vis.getString(); }

void reflect(BinaryWriter &vis, bool &v              ) { vis.pack(v); }
void reflect(BinaryWriter &vis, unsigned char &v     ) { vis.pack(v); }
void reflect(BinaryWriter &vis, short &v             ) { vis.varInt(v); }
void reflect(BinaryWriter &vis, unsigned short &v    ) { vis.varUInt(v); }
void reflect(BinaryWriter &vis, int &v               ) { vis.varInt(v); }
void reflect(BinaryWriter &vis, unsigned &v          ) { vis.varUInt(v); }
void reflect(BinaryWriter &vis, long &v              ) { vis.varInt(v); }
void reflect(BinaryWriter &vis, unsigned long &v     ) { vis.varUInt(v); }
void reflect(BinaryWriter &vis, long long &v         ) { vis.varInt(v); }
void reflect(BinaryWriter &vis, unsigned long long &v) { vis.varUInt(v); }
void reflect(BinaryWriter &vis, double &v            ) { vis.pack(v); }
void reflect(BinaryWriter &vis, const char *&v       ) { vis.string(v); }
void reflect(BinaryWriter &vis, std::string &v       ) { vis.string(v.c_str(), v.size()); }
// clang-format on

void reflect(JsonWriter &vis, std::string_view &data) {
  if (data.empty())
    vis.string("");
  else
    vis.string(&data[0], (rapidjson::SizeType)data.size());
}

void reflect(JsonReader &vis, JsonNull &v) {}
void reflect(JsonWriter &vis, JsonNull &v) { vis.m->Null(); }

template <typename V>
void reflect(JsonReader &vis, std::unordered_map<Usr, V> &v) {
  vis.iterArray([&]() {
    V val;
    reflect(vis, val);
    v[val.usr] = std::move(val);
  });
}
template <typename V>
void reflect(JsonWriter &vis, std::unordered_map<Usr, V> &v) {
  // Determinism
  std::vector<std::pair<uint64_t, V>> xs(v.begin(), v.end());
  std::sort(xs.begin(), xs.end(),
            [](const auto &a, const auto &b) { return a.first < b.first; });
  vis.startArray();
  for (auto &it : xs)
    reflect(vis, it.second);
  vis.endArray();
}
template <typename V>
void reflect(BinaryReader &vis, std::unordered_map<Usr, V> &v) {
  for (auto n = vis.varUInt(); n; n--) {
    V val;
    reflect(vis, val);
    v[val.usr] = std::move(val);
  }
}
template <typename V>
void reflect(BinaryWriter &vis, std::unordered_map<Usr, V> &v) {
  vis.varUInt(v.size());
  for (auto &it : v)
    reflect(vis, it.second);
}

// Used by IndexFile::dependencies.
void reflect(JsonReader &vis, DenseMap<CachedHashStringRef, int64_t> &v) {
  std::string name;
  for (auto it = vis.m->MemberBegin(); it != vis.m->MemberEnd(); ++it)
    v[internH(it->name.GetString())] = it->value.GetInt64();
}
void reflect(JsonWriter &vis, DenseMap<CachedHashStringRef, int64_t> &v) {
  vis.startObject();
  for (auto &it : v) {
    vis.m->Key(it.first.val().data()); // llvm 8 -> data()
    vis.m->Int64(it.second);
  }
  vis.endObject();
}
void reflect(BinaryReader &vis, DenseMap<CachedHashStringRef, int64_t> &v) {
  std::string name;
  for (auto n = vis.varUInt(); n; n--) {
    reflect(vis, name);
    reflect(vis, v[internH(name)]);
  }
}
void reflect(BinaryWriter &vis, DenseMap<CachedHashStringRef, int64_t> &v) {
  std::string key;
  vis.varUInt(v.size());
  for (auto &it : v) {
    key = it.first.val().str();
    reflect(vis, key);
    reflect(vis, it.second);
  }
}

template <typename Vis> void reflect(Vis &vis, IndexInclude &v) {
  reflectMemberStart(vis);
  REFLECT_MEMBER(line);
  REFLECT_MEMBER(resolved_path);
  reflectMemberEnd(vis);
}
void reflect(JsonWriter &vis, IndexInclude &v) {
  reflectMemberStart(vis);
  REFLECT_MEMBER(line);
  if (gTestOutputMode) {
    std::string basename = llvm::sys::path::filename(v.resolved_path);
    if (v.resolved_path[0] != '&')
      basename = "&" + basename;
    REFLECT_MEMBER2("resolved_path", basename);
  } else {
    REFLECT_MEMBER(resolved_path);
  }
  reflectMemberEnd(vis);
}

template <typename Def>
void reflectHoverAndComments(JsonReader &vis, Def &def) {
  reflectMember(vis, "hover", def.hover);
  reflectMember(vis, "comments", def.comments);
}
template <typename Def>
void reflectHoverAndComments(JsonWriter &vis, Def &def) {
  // Don't emit empty hover and comments in JSON test mode.
  if (!gTestOutputMode || def.hover[0])
    reflectMember(vis, "hover", def.hover);
  if (!gTestOutputMode || def.comments[0])
    reflectMember(vis, "comments", def.comments);
}
template <typename Def>
void reflectHoverAndComments(BinaryReader &vis, Def &def) {
  reflect(vis, def.hover);
  reflect(vis, def.comments);
}
template <typename Def>
void reflectHoverAndComments(BinaryWriter &vis, Def &def) {
  reflect(vis, def.hover);
  reflect(vis, def.comments);
}

template <typename Def> void reflectShortName(JsonReader &vis, Def &def) {
  if (gTestOutputMode) {
    std::string short_name;
    reflectMember(vis, "short_name", short_name);
    def.short_name_offset =
        std::string_view(def.detailed_name).find(short_name);
    assert(def.short_name_offset != std::string::npos);
    def.short_name_size = short_name.size();
  } else {
    reflectMember(vis, "short_name_offset", def.short_name_offset);
    reflectMember(vis, "short_name_size", def.short_name_size);
  }
}
template <typename Def> void reflectShortName(JsonWriter &vis, Def &def) {
  if (gTestOutputMode) {
    std::string_view short_name(def.detailed_name + def.short_name_offset,
                                def.short_name_size);
    reflectMember(vis, "short_name", short_name);
  } else {
    reflectMember(vis, "short_name_offset", def.short_name_offset);
    reflectMember(vis, "short_name_size", def.short_name_size);
  }
}
template <typename Def> void reflectShortName(BinaryReader &vis, Def &def) {
  reflect(vis, def.short_name_offset);
  reflect(vis, def.short_name_size);
}
template <typename Def> void reflectShortName(BinaryWriter &vis, Def &def) {
  reflect(vis, def.short_name_offset);
  reflect(vis, def.short_name_size);
}

template <typename TVisitor> void reflect1(TVisitor &vis, IndexFunc &v) {
  reflectMemberStart(vis);
  REFLECT_MEMBER2("usr", v.usr);
  REFLECT_MEMBER2("detailed_name", v.def.detailed_name);
  REFLECT_MEMBER2("qual_name_offset", v.def.qual_name_offset);
  reflectShortName(vis, v.def);
  REFLECT_MEMBER2("spell", v.def.spell);
  reflectHoverAndComments(vis, v.def);
  REFLECT_MEMBER2("bases", v.def.bases);
  REFLECT_MEMBER2("vars", v.def.vars);
  REFLECT_MEMBER2("callees", v.def.callees);
  REFLECT_MEMBER2("kind", v.def.kind);
  REFLECT_MEMBER2("parent_kind", v.def.parent_kind);
  REFLECT_MEMBER2("storage", v.def.storage);

  REFLECT_MEMBER2("declarations", v.declarations);
  REFLECT_MEMBER2("derived", v.derived);
  REFLECT_MEMBER2("uses", v.uses);
  reflectMemberEnd(vis);
}
void reflect(JsonReader &vis, IndexFunc &v) { reflect1(vis, v); }
void reflect(JsonWriter &vis, IndexFunc &v) { reflect1(vis, v); }
void reflect(BinaryReader &vis, IndexFunc &v) { reflect1(vis, v); }
void reflect(BinaryWriter &vis, IndexFunc &v) { reflect1(vis, v); }

template <typename Vis> void reflect1(Vis &vis, IndexType &v) {
  reflectMemberStart(vis);
  REFLECT_MEMBER2("usr", v.usr);
  REFLECT_MEMBER2("detailed_name", v.def.detailed_name);
  REFLECT_MEMBER2("qual_name_offset", v.def.qual_name_offset);
  reflectShortName(vis, v.def);
  reflectHoverAndComments(vis, v.def);
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
  reflectMemberEnd(vis);
}
void reflect(JsonReader &vis, IndexType &v) { reflect1(vis, v); }
void reflect(JsonWriter &vis, IndexType &v) { reflect1(vis, v); }
void reflect(BinaryReader &vis, IndexType &v) { reflect1(vis, v); }
void reflect(BinaryWriter &vis, IndexType &v) { reflect1(vis, v); }

template <typename TVisitor> void reflect1(TVisitor &vis, IndexVar &v) {
  reflectMemberStart(vis);
  REFLECT_MEMBER2("usr", v.usr);
  REFLECT_MEMBER2("detailed_name", v.def.detailed_name);
  REFLECT_MEMBER2("qual_name_offset", v.def.qual_name_offset);
  reflectShortName(vis, v.def);
  reflectHoverAndComments(vis, v.def);
  REFLECT_MEMBER2("spell", v.def.spell);
  REFLECT_MEMBER2("type", v.def.type);
  REFLECT_MEMBER2("kind", v.def.kind);
  REFLECT_MEMBER2("parent_kind", v.def.parent_kind);
  REFLECT_MEMBER2("storage", v.def.storage);

  REFLECT_MEMBER2("declarations", v.declarations);
  REFLECT_MEMBER2("uses", v.uses);
  reflectMemberEnd(vis);
}
void reflect(JsonReader &vis, IndexVar &v) { reflect1(vis, v); }
void reflect(JsonWriter &vis, IndexVar &v) { reflect1(vis, v); }
void reflect(BinaryReader &vis, IndexVar &v) { reflect1(vis, v); }
void reflect(BinaryWriter &vis, IndexVar &v) { reflect1(vis, v); }

// IndexFile
template <typename TVisitor> void reflect1(TVisitor &vis, IndexFile &v) {
  reflectMemberStart(vis);
  if (!gTestOutputMode) {
    REFLECT_MEMBER(mtime);
    REFLECT_MEMBER(language);
    REFLECT_MEMBER(no_linkage);
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
  reflectMemberEnd(vis);
}
void reflectFile(JsonReader &vis, IndexFile &v) { reflect1(vis, v); }
void reflectFile(JsonWriter &vis, IndexFile &v) { reflect1(vis, v); }
void reflectFile(BinaryReader &vis, IndexFile &v) { reflect1(vis, v); }
void reflectFile(BinaryWriter &vis, IndexFile &v) { reflect1(vis, v); }

void reflect(JsonReader &vis, SerializeFormat &v) {
  v = vis.getString()[0] == 'j' ? SerializeFormat::Json
                                : SerializeFormat::Binary;
}

void reflect(JsonWriter &vis, SerializeFormat &v) {
  switch (v) {
  case SerializeFormat::Binary:
    vis.string("binary");
    break;
  case SerializeFormat::Json:
    vis.string("json");
    break;
  }
}

void reflectMemberStart(JsonReader &vis) {
  if (!vis.m->IsObject())
    throw std::invalid_argument("object");
}

static BumpPtrAllocator alloc;
static DenseSet<CachedHashStringRef> strings;
static std::mutex allocMutex;

CachedHashStringRef internH(StringRef s) {
  if (s.empty())
    s = "";
  CachedHashString hs(s);
  std::lock_guard lock(allocMutex);
  auto r = strings.insert(hs);
  if (r.second) {
    char *p = alloc.Allocate<char>(s.size() + 1);
    memcpy(p, s.data(), s.size());
    p[s.size()] = '\0';
    *r.first = CachedHashStringRef(StringRef(p, s.size()), hs.hash());
  }
  return *r.first;
}

const char *intern(StringRef s) { return internH(s).val().data(); }

std::string serialize(SerializeFormat format, IndexFile &file) {
  switch (format) {
  case SerializeFormat::Binary: {
    BinaryWriter writer;
    int major = IndexFile::kMajorVersion;
    int minor = IndexFile::kMinorVersion;
    reflect(writer, major);
    reflect(writer, minor);
    reflectFile(writer, file);
    return writer.take();
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
    reflectFile(json_writer, file);
    return output.GetString();
  }
  }
  return "";
}

std::unique_ptr<IndexFile>
deserialize(SerializeFormat format, const std::string &path,
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
      reflect(reader, major);
      reflect(reader, minor);
      if (major != IndexFile::kMajorVersion ||
          minor != IndexFile::kMinorVersion)
        throw std::invalid_argument("Invalid version");
      file = std::make_unique<IndexFile>(path, file_content, false);
      reflectFile(reader, *file);
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

    file = std::make_unique<IndexFile>(path, file_content, false);
    JsonReader json_reader{&reader};
    try {
      reflectFile(json_reader, *file);
    } catch (std::invalid_argument &e) {
      LOG_S(INFO) << "'" << path << "': failed to deserialize "
                  << json_reader.getPath() << "." << e.what();
      return nullptr;
    }
    break;
  }
  }

  // Restore non-serialized state.
  file->path = path;
  if (g_config->clang.pathMappings.size()) {
    doPathMapping(file->import_file);
    std::vector<const char *> args;
    for (const char *arg : file->args) {
      std::string s(arg);
      doPathMapping(s);
      args.push_back(intern(s));
    }
    file->args = std::move(args);
    for (auto &[_, path] : file->lid2path)
      doPathMapping(path);
    for (auto &include : file->includes) {
      std::string p(include.resolved_path);
      doPathMapping(p);
      include.resolved_path = intern(p);
    }
    decltype(file->dependencies) dependencies;
    for (auto &it : file->dependencies) {
      std::string path = it.first.val().str();
      doPathMapping(path);
      dependencies[internH(path)] = it.second;
    }
    file->dependencies = std::move(dependencies);
  }
  return file;
}
} // namespace ccls
