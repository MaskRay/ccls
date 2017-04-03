#include "serializer.h"
#include "indexer.h"

// int
void Reflect(Reader& visitor, int& value) {
  value = visitor.GetInt();
}
void Reflect(Writer& visitor, int& value) {
  visitor.Int(value);
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
  visitor.String(value.c_str(), value.size());
}


// ReflectMember
void ReflectMember(Writer& visitor, const char* name, std::string& value) {
  if (value.empty())
    return;
  visitor.Key(name);
  Reflect(visitor, value);
}


// Location
void Reflect(Reader& visitor, Location& value) {
  value = Location(visitor.GetString());
}
void Reflect(Writer& visitor, Location& value) {
  // We only ever want to emit id=1 files.
  assert(value.raw_file_id == 1);

  std::string output = value.ToString();
  visitor.String(output.c_str(), output.size());
}


// Id<T>
template<typename T>
void Reflect(Reader& visitor, Id<T>& id) {
  id.id = visitor.GetUint64();
}
template<typename T>
void Reflect(Writer& visitor, Id<T>& value) {
  visitor.Uint64(value.id);
}


// Ref<IndexedFuncDef>
void Reflect(Reader& visitor, Ref<IndexedFuncDef>& value) {
  const char* str_value = visitor.GetString();
  uint64_t id = atoi(str_value);
  const char* loc_string = strchr(str_value, '@') + 1;

  value.id = Id<IndexedFuncDef>(id);
  value.loc = Location(loc_string);
}
void Reflect(Writer& visitor, Ref<IndexedFuncDef>& value) {
  std::string s = std::to_string(value.id.id) + "@" + value.loc.ToString();
  visitor.String(s.c_str());
}







// TODO: Move this to indexer.cpp
// TODO: Rename indexer.cpp to indexer.cc
// TODO: Do not serialize a USR if it has no usages/etc outside of USR info.

// IndexedTypeDef
bool ReflectMemberStart(Reader& reader, IndexedTypeDef& value) {
  //value.is_bad_def = false;
  return true;
}
bool ReflectMemberStart(Writer& writer, IndexedTypeDef& value) {
  //if (value.is_bad_def)
  //  return false;
  DefaultReflectMemberStart(writer);
  return true;
}
template<typename TVisitor>
void Reflect(TVisitor& visitor, IndexedTypeDef& value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER2("id", value.id);
  REFLECT_MEMBER2("usr", value.def.usr);
  REFLECT_MEMBER2("short_name", value.def.short_name);
  REFLECT_MEMBER2("qualified_name", value.def.qualified_name);
  REFLECT_MEMBER2("definition", value.def.definition);
  REFLECT_MEMBER2("alias_of", value.def.alias_of);
  REFLECT_MEMBER2("parents", value.def.parents);
  REFLECT_MEMBER2("derived", value.derived);
  REFLECT_MEMBER2("types", value.def.types);
  REFLECT_MEMBER2("funcs", value.def.funcs);
  REFLECT_MEMBER2("vars", value.def.vars);
  REFLECT_MEMBER2("uses", value.uses);
  REFLECT_MEMBER_END();
}


// IndexedFuncDef
bool ReflectMemberStart(Reader& reader, IndexedFuncDef& value) {
  //value.is_bad_def = false;
  return true;
}
bool ReflectMemberStart(Writer& writer, IndexedFuncDef& value) {
  //if (value.is_bad_def)
  //  return false;
  DefaultReflectMemberStart(writer);
  return true;
}
template<typename TVisitor>
void Reflect(TVisitor& visitor, IndexedFuncDef& value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER2("id", value.id);
  REFLECT_MEMBER2("usr", value.def.usr);
  REFLECT_MEMBER2("short_name", value.def.short_name);
  REFLECT_MEMBER2("qualified_name", value.def.qualified_name);
  REFLECT_MEMBER2("declarations", value.declarations);
  REFLECT_MEMBER2("definition", value.def.definition);
  REFLECT_MEMBER2("declaring_type", value.def.declaring_type);
  REFLECT_MEMBER2("base", value.def.base);
  REFLECT_MEMBER2("derived", value.derived);
  REFLECT_MEMBER2("locals", value.def.locals);
  REFLECT_MEMBER2("callers", value.callers);
  REFLECT_MEMBER2("callees", value.def.callees);
  REFLECT_MEMBER2("uses", value.uses);
  REFLECT_MEMBER_END();
}


// IndexedVarDef
bool ReflectMemberStart(Reader& reader, IndexedVarDef& value) {
  //value.is_bad_def = false;
  return true;
}
bool ReflectMemberStart(Writer& writer, IndexedVarDef& value) {
  //if (value.is_bad_def)
  //  return false;
  DefaultReflectMemberStart(writer);
  return true;
}
template<typename TVisitor>
void Reflect(TVisitor& visitor, IndexedVarDef& value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER2("id", value.id);
  REFLECT_MEMBER2("usr", value.def.usr);
  REFLECT_MEMBER2("short_name", value.def.short_name);
  REFLECT_MEMBER2("qualified_name", value.def.qualified_name);
  REFLECT_MEMBER2("declaration", value.def.declaration);
  REFLECT_MEMBER2("definition", value.def.definition);
  REFLECT_MEMBER2("variable_type", value.def.variable_type);
  REFLECT_MEMBER2("declaring_type", value.def.declaring_type);
  REFLECT_MEMBER2("uses", value.uses);
  REFLECT_MEMBER_END();
}


// IndexedFile
bool ReflectMemberStart(Writer& visitor, IndexedFile& value) {
  auto it = value.id_cache.usr_to_type_id.find("");
  if (it != value.id_cache.usr_to_type_id.end()) {
    value.Resolve(it->second)->def.short_name = "<fundamental>";
    assert(value.Resolve(it->second)->uses.size() == 0);
  }

  DefaultReflectMemberStart(visitor);
  return true;
}
template<typename TVisitor>
void Reflect(TVisitor& visitor, IndexedFile& value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER(types);
  REFLECT_MEMBER(funcs);
  REFLECT_MEMBER(vars);
  REFLECT_MEMBER_END();
}







std::string Serialize(IndexedFile& file) {
  rapidjson::StringBuffer output;
  rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(output);
  //Writer writer(output);
  writer.SetFormatOptions(
    rapidjson::PrettyFormatOptions::kFormatSingleLineArray);
  writer.SetIndent(' ', 2);

  Reflect(writer, file);

  return output.GetString();
}

optional<IndexedFile> Deserialize(std::string path, std::string serialized) {
  rapidjson::Document reader;
  reader.Parse(serialized.c_str());
  if (reader.HasParseError())
    return nullopt;

  IndexedFile file(path);
  Reflect(reader, file);

  return file;
}

#if false
#include "header.h"
#include "serializer.h"


struct SeparateFileDerived : Base {};

void f() {
  rapidjson::StringBuffer output;
  rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(output);
  //Writer writer(output);
  writer.SetFormatOptions(
    rapidjson::PrettyFormatOptions::kFormatSingleLineArray);
  writer.SetIndent(' ', 2);

  Foo2<int> a;
}
#endif

/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@Reader",
      "instantiations": [0, 4, 8, 15, 20, 24, 32, 38, 44],
      "uses": ["*1:5:14", "*1:12:14", "*1:19:14", "*1:37:14", "*1:51:14", "*1:61:14", "*1:85:25", "*1:115:25", "*1:146:25"]
    }, {
      "id": 1,
      "usr": "c:@Writer",
      "instantiations": [2, 6, 10, 12, 17, 22, 29, 34, 40, 46, 50],
      "uses": ["*1:8:14", "*1:15:14", "*1:22:14", "*1:28:20", "*1:40:14", "*1:55:14", "*1:69:14", "*1:89:25", "*1:119:25", "*1:150:25", "*1:173:25"]
    }, {
      "id": 2,
      "usr": "c:@N@std@T@string",
      "instantiations": [9, 11, 14, 19, 31, 58, 59],
      "uses": ["*1:19:36", "*1:22:36", "*1:28:60", "*1:44:8", "*1:70:8", "*1:198:6", "*1:211:40", "*1:211:58"]
    }, {
      "id": 3,
      "usr": "c:@S@Location",
      "instantiations": [16, 18],
      "uses": ["*1:37:31", "*1:40:31"]
    }, {
      "id": 4,
      "usr": "c:@ST>1#T@Id",
      "instantiations": [21, 23],
      "uses": ["*1:51:31", "*1:55:31"]
    }, {
      "id": 5,
      "usr": "c:impl.cc@1122",
      "uses": ["*1:51:34"]
    }, {
      "id": 6,
      "usr": "c:impl.cc@1223",
      "uses": ["*1:55:34"]
    }, {
      "id": 7,
      "usr": "c:@ST>1#T@Ref",
      "instantiations": [25, 30],
      "uses": ["*1:61:31", "*1:69:31"]
    }, {
      "id": 8,
      "usr": "c:@S@IndexedFuncDef",
      "instantiations": [39, 41, 43],
      "uses": ["*1:61:35", "*1:69:35", "*1:115:41", "*1:119:41", "*1:126:33"]
    }, {
      "id": 9,
      "usr": "c:@T@uint64_t",
      "instantiations": [27],
      "uses": ["*1:63:3"]
    }, {
      "id": 10,
      "usr": "c:@S@IndexedTypeDef",
      "instantiations": [33, 35, 37],
      "uses": ["*1:85:41", "*1:89:41", "*1:96:33"]
    }, {
      "id": 11,
      "usr": "c:impl.cc@2280",
      "uses": ["*1:96:14"]
    }, {
      "id": 12,
      "usr": "c:impl.cc@3310",
      "uses": ["*1:126:14"]
    }, {
      "id": 13,
      "usr": "c:@S@IndexedVarDef",
      "instantiations": [45, 47, 49],
      "uses": ["*1:146:41", "*1:150:41", "*1:157:33"]
    }, {
      "id": 14,
      "usr": "c:impl.cc@4407",
      "uses": ["*1:157:14"]
    }, {
      "id": 15,
      "usr": "c:@S@IndexedFile",
      "instantiations": [51, 54, 55, 61],
      "uses": ["*1:173:42", "*1:184:33", "*1:198:23", "*1:211:10", "*1:217:3"]
    }, {
      "id": 16,
      "usr": "c:impl.cc@5404",
      "uses": ["*1:184:14"]
    }, {
      "id": 17,
      "usr": "c:stringbuffer.h@N@rapidjson@T@StringBuffer",
      "instantiations": [56],
      "uses": ["*1:199:14"]
    }, {
      "id": 18,
      "usr": "c:@N@std@N@experimental@ST>1#T@optional",
      "uses": ["*1:211:1"]
    }, {
      "id": 19,
      "usr": "c:document.h@N@rapidjson@T@Document",
      "instantiations": [60],
      "uses": ["*1:212:14"]
    }],
  "funcs": [{
      "id": 0,
      "usr": "c:@F@Reflect#&$@N@rapidjson@S@GenericValue>#$@N@rapidjson@S@UTF8>#C#$@N@rapidjson@S@MemoryPoolAllocator>#$@N@rapidjson@S@CrtAllocator#&I#",
      "short_name": "Reflect",
      "qualified_name": "Reflect",
      "definition": "1:5:6",
      "uses": ["1:5:6"]
    }, {
      "id": 1,
      "usr": "c:@F@Reflect#&$@N@rapidjson@S@Writer>#$@N@rapidjson@S@GenericStringBuffer>#$@N@rapidjson@S@UTF8>#C#$@N@rapidjson@S@CrtAllocator#S3_#S3_#S4_#Vi0#&I#",
      "short_name": "Reflect",
      "qualified_name": "Reflect",
      "definition": "1:8:6",
      "callees": ["2@1:9:11"],
      "uses": ["1:8:6"]
    }, {
      "id": 2,
      "usr": "c:@N@rapidjson@ST>5#T#T#T#T#Ni@Writer@F@Int#I#",
      "callers": ["1@1:9:11"],
      "uses": ["1:9:11"]
    }, {
      "id": 3,
      "usr": "c:@F@Reflect#&$@N@rapidjson@S@GenericValue>#$@N@rapidjson@S@UTF8>#C#$@N@rapidjson@S@MemoryPoolAllocator>#$@N@rapidjson@S@CrtAllocator#&b#",
      "short_name": "Reflect",
      "qualified_name": "Reflect",
      "definition": "1:12:6",
      "uses": ["1:12:6"]
    }, {
      "id": 4,
      "usr": "c:@F@Reflect#&$@N@rapidjson@S@Writer>#$@N@rapidjson@S@GenericStringBuffer>#$@N@rapidjson@S@UTF8>#C#$@N@rapidjson@S@CrtAllocator#S3_#S3_#S4_#Vi0#&b#",
      "short_name": "Reflect",
      "qualified_name": "Reflect",
      "definition": "1:15:6",
      "callees": ["5@1:16:11"],
      "uses": ["1:15:6"]
    }, {
      "id": 5,
      "usr": "c:@N@rapidjson@ST>5#T#T#T#T#Ni@Writer@F@Bool#b#",
      "callers": ["4@1:16:11"],
      "uses": ["1:16:11"]
    }, {
      "id": 6,
      "usr": "c:@F@Reflect#&$@N@rapidjson@S@GenericValue>#$@N@rapidjson@S@UTF8>#C#$@N@rapidjson@S@MemoryPoolAllocator>#$@N@rapidjson@S@CrtAllocator#&$@N@std@S@basic_string>#C#$@N@std@S@char_traits>#C#$@N@std@S@allocator>#C#",
      "short_name": "Reflect",
      "qualified_name": "Reflect",
      "definition": "1:19:6",
      "uses": ["1:19:6"]
    }, {
      "id": 7,
      "usr": "c:@F@Reflect#&$@N@rapidjson@S@Writer>#$@N@rapidjson@S@GenericStringBuffer>#$@N@rapidjson@S@UTF8>#C#$@N@rapidjson@S@CrtAllocator#S3_#S3_#S4_#Vi0#&$@N@std@S@basic_string>#C#$@N@std@S@char_traits>#C#$@N@std@S@allocator>#C#",
      "short_name": "Reflect",
      "qualified_name": "Reflect",
      "definition": "1:22:6",
      "callers": ["8@1:32:3"],
      "uses": ["1:22:6", "1:32:3"]
    }, {
      "id": 8,
      "usr": "c:@F@ReflectMember#&$@N@rapidjson@S@Writer>#$@N@rapidjson@S@GenericStringBuffer>#$@N@rapidjson@S@UTF8>#C#$@N@rapidjson@S@CrtAllocator#S3_#S3_#S4_#Vi0#*1C#&$@N@std@S@basic_string>#C#$@N@std@S@char_traits>#C#$@N@std@S@allocator>#C#",
      "short_name": "ReflectMember",
      "qualified_name": "ReflectMember",
      "definition": "1:28:6",
      "callees": ["9@1:31:11", "7@1:32:3"],
      "uses": ["1:28:6"]
    }, {
      "id": 9,
      "usr": "c:@N@rapidjson@ST>5#T#T#T#T#Ni@Writer@F@Key#*1^type-parameter-0-1:::Ch#",
      "callers": ["8@1:31:11"],
      "uses": ["1:31:11"]
    }, {
      "id": 10,
      "usr": "c:@F@Reflect#&$@N@rapidjson@S@GenericValue>#$@N@rapidjson@S@UTF8>#C#$@N@rapidjson@S@MemoryPoolAllocator>#$@N@rapidjson@S@CrtAllocator#&$@S@Location#",
      "short_name": "Reflect",
      "qualified_name": "Reflect",
      "definition": "1:37:6",
      "uses": ["1:37:6"]
    }, {
      "id": 11,
      "usr": "c:@F@Reflect#&$@N@rapidjson@S@Writer>#$@N@rapidjson@S@GenericStringBuffer>#$@N@rapidjson@S@UTF8>#C#$@N@rapidjson@S@CrtAllocator#S3_#S3_#S4_#Vi0#&$@S@Location#",
      "short_name": "Reflect",
      "qualified_name": "Reflect",
      "definition": "1:40:6",
      "callees": ["12@1:44:30"],
      "uses": ["1:40:6"]
    }, {
      "id": 12,
      "usr": "c:@S@Location@F@ToString#",
      "callers": ["11@1:44:30"],
      "uses": ["1:44:30"]
    }, {
      "id": 13,
      "usr": "c:@FT@>1#TReflect#&$@N@rapidjson@S@GenericValue>#$@N@rapidjson@S@UTF8>#C#$@N@rapidjson@S@MemoryPoolAllocator>#$@N@rapidjson@S@CrtAllocator#&>@ST>1#T@Id1t0.0#v#",
      "short_name": "Reflect",
      "qualified_name": "Reflect",
      "definition": "1:51:6",
      "uses": ["1:51:6"]
    }, {
      "id": 14,
      "usr": "c:@FT@>1#TReflect#&$@N@rapidjson@S@Writer>#$@N@rapidjson@S@GenericStringBuffer>#$@N@rapidjson@S@UTF8>#C#$@N@rapidjson@S@CrtAllocator#S3_#S3_#S4_#Vi0#&>@ST>1#T@Id1t0.0#v#",
      "short_name": "Reflect",
      "qualified_name": "Reflect",
      "definition": "1:55:6",
      "uses": ["1:55:6"]
    }, {
      "id": 15,
      "usr": "c:@F@Reflect#&$@N@rapidjson@S@GenericValue>#$@N@rapidjson@S@UTF8>#C#$@N@rapidjson@S@MemoryPoolAllocator>#$@N@rapidjson@S@CrtAllocator#&$@S@Ref>#$@S@IndexedFuncDef#",
      "short_name": "Reflect",
      "qualified_name": "Reflect",
      "definition": "1:61:6",
      "uses": ["1:61:6"]
    }, {
      "id": 16,
      "usr": "c:@F@Reflect#&$@N@rapidjson@S@Writer>#$@N@rapidjson@S@GenericStringBuffer>#$@N@rapidjson@S@UTF8>#C#$@N@rapidjson@S@CrtAllocator#S3_#S3_#S4_#Vi0#&$@S@Ref>#$@S@IndexedFuncDef#",
      "short_name": "Reflect",
      "qualified_name": "Reflect",
      "definition": "1:69:6",
      "callees": ["17@1:71:11"],
      "uses": ["1:69:6"]
    }, {
      "id": 17,
      "usr": "c:@N@rapidjson@ST>5#T#T#T#T#Ni@Writer@F@String#*1^type-parameter-0-1:::Ch#",
      "callers": ["16@1:71:11"],
      "uses": ["1:71:11"]
    }, {
      "id": 18,
      "usr": "c:@F@ReflectMemberStart#&$@N@rapidjson@S@GenericValue>#$@N@rapidjson@S@UTF8>#C#$@N@rapidjson@S@MemoryPoolAllocator>#$@N@rapidjson@S@CrtAllocator#&$@S@IndexedTypeDef#",
      "short_name": "ReflectMemberStart",
      "qualified_name": "ReflectMemberStart",
      "definition": "1:85:6",
      "uses": ["1:85:6"]
    }, {
      "id": 19,
      "usr": "c:@F@ReflectMemberStart#&$@N@rapidjson@S@Writer>#$@N@rapidjson@S@GenericStringBuffer>#$@N@rapidjson@S@UTF8>#C#$@N@rapidjson@S@CrtAllocator#S3_#S3_#S4_#Vi0#&$@S@IndexedTypeDef#",
      "short_name": "ReflectMemberStart",
      "qualified_name": "ReflectMemberStart",
      "definition": "1:89:6",
      "callees": ["20@1:92:3"],
      "uses": ["1:89:6"]
    }, {
      "id": 20,
      "usr": "c:@F@DefaultReflectMemberStart#&$@N@rapidjson@S@Writer>#$@N@rapidjson@S@GenericStringBuffer>#$@N@rapidjson@S@UTF8>#C#$@N@rapidjson@S@CrtAllocator#S3_#S3_#S4_#Vi0#",
      "callers": ["19@1:92:3", "23@1:122:3", "26@1:153:3", "28@1:180:3"],
      "uses": ["1:92:3", "1:122:3", "1:153:3", "1:180:3"]
    }, {
      "id": 21,
      "usr": "c:@FT@>1#TReflect#&t0.0#&$@S@IndexedTypeDef#v#",
      "short_name": "Reflect",
      "qualified_name": "Reflect",
      "definition": "1:96:6",
      "uses": ["1:96:6"]
    }, {
      "id": 22,
      "usr": "c:@F@ReflectMemberStart#&$@N@rapidjson@S@GenericValue>#$@N@rapidjson@S@UTF8>#C#$@N@rapidjson@S@MemoryPoolAllocator>#$@N@rapidjson@S@CrtAllocator#&$@S@IndexedFuncDef#",
      "short_name": "ReflectMemberStart",
      "qualified_name": "ReflectMemberStart",
      "definition": "1:115:6",
      "uses": ["1:115:6"]
    }, {
      "id": 23,
      "usr": "c:@F@ReflectMemberStart#&$@N@rapidjson@S@Writer>#$@N@rapidjson@S@GenericStringBuffer>#$@N@rapidjson@S@UTF8>#C#$@N@rapidjson@S@CrtAllocator#S3_#S3_#S4_#Vi0#&$@S@IndexedFuncDef#",
      "short_name": "ReflectMemberStart",
      "qualified_name": "ReflectMemberStart",
      "definition": "1:119:6",
      "callees": ["20@1:122:3"],
      "uses": ["1:119:6"]
    }, {
      "id": 24,
      "usr": "c:@FT@>1#TReflect#&t0.0#&$@S@IndexedFuncDef#v#",
      "short_name": "Reflect",
      "qualified_name": "Reflect",
      "definition": "1:126:6",
      "uses": ["1:126:6"]
    }, {
      "id": 25,
      "usr": "c:@F@ReflectMemberStart#&$@N@rapidjson@S@GenericValue>#$@N@rapidjson@S@UTF8>#C#$@N@rapidjson@S@MemoryPoolAllocator>#$@N@rapidjson@S@CrtAllocator#&$@S@IndexedVarDef#",
      "short_name": "ReflectMemberStart",
      "qualified_name": "ReflectMemberStart",
      "definition": "1:146:6",
      "uses": ["1:146:6"]
    }, {
      "id": 26,
      "usr": "c:@F@ReflectMemberStart#&$@N@rapidjson@S@Writer>#$@N@rapidjson@S@GenericStringBuffer>#$@N@rapidjson@S@UTF8>#C#$@N@rapidjson@S@CrtAllocator#S3_#S3_#S4_#Vi0#&$@S@IndexedVarDef#",
      "short_name": "ReflectMemberStart",
      "qualified_name": "ReflectMemberStart",
      "definition": "1:150:6",
      "callees": ["20@1:153:3"],
      "uses": ["1:150:6"]
    }, {
      "id": 27,
      "usr": "c:@FT@>1#TReflect#&t0.0#&$@S@IndexedVarDef#v#",
      "short_name": "Reflect",
      "qualified_name": "Reflect",
      "definition": "1:157:6",
      "uses": ["1:157:6"]
    }, {
      "id": 28,
      "usr": "c:@F@ReflectMemberStart#&$@N@rapidjson@S@Writer>#$@N@rapidjson@S@GenericStringBuffer>#$@N@rapidjson@S@UTF8>#C#$@N@rapidjson@S@CrtAllocator#S3_#S3_#S4_#Vi0#&$@S@IndexedFile#",
      "short_name": "ReflectMemberStart",
      "qualified_name": "ReflectMemberStart",
      "definition": "1:173:6",
      "callees": ["20@1:180:3"],
      "uses": ["1:173:6"]
    }, {
      "id": 29,
      "usr": "c:@FT@>1#TReflect#&t0.0#&$@S@IndexedFile#v#",
      "short_name": "Reflect",
      "qualified_name": "Reflect",
      "definition": "1:184:6",
      "uses": ["1:184:6"]
    }, {
      "id": 30,
      "usr": "c:@F@Serialize#&$@S@IndexedFile#",
      "short_name": "Serialize",
      "qualified_name": "Serialize",
      "definition": "1:198:13",
      "uses": ["1:198:13"]
    }, {
      "id": 31,
      "usr": "c:@F@Deserialize#$@N@std@S@basic_string>#C#$@N@std@S@char_traits>#C#$@N@std@S@allocator>#C#S0_#",
      "short_name": "Deserialize",
      "qualified_name": "Deserialize",
      "definition": "1:211:23",
      "callees": ["32@1:217:15"],
      "uses": ["1:211:23"]
    }, {
      "id": 32,
      "usr": "c:@S@IndexedFile@F@IndexedFile#&1$@N@std@S@basic_string>#C#$@N@std@S@char_traits>#C#$@N@std@S@allocator>#C#",
      "callers": ["31@1:217:15"],
      "uses": ["1:217:15"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:impl.cc@70@F@Reflect#&$@N@rapidjson@S@GenericValue>#$@N@rapidjson@S@UTF8>#C#$@N@rapidjson@S@MemoryPoolAllocator>#$@N@rapidjson@S@CrtAllocator#&I#@visitor",
      "short_name": "visitor",
      "qualified_name": "visitor",
      "definition": "1:5:22",
      "variable_type": 0,
      "uses": ["1:5:22"]
    }, {
      "id": 1,
      "usr": "c:impl.cc@87@F@Reflect#&$@N@rapidjson@S@GenericValue>#$@N@rapidjson@S@UTF8>#C#$@N@rapidjson@S@MemoryPoolAllocator>#$@N@rapidjson@S@CrtAllocator#&I#@value",
      "short_name": "value",
      "qualified_name": "value",
      "definition": "1:5:36",
      "uses": ["1:5:36"]
    }, {
      "id": 2,
      "usr": "c:impl.cc@147@F@Reflect#&$@N@rapidjson@S@Writer>#$@N@rapidjson@S@GenericStringBuffer>#$@N@rapidjson@S@UTF8>#C#$@N@rapidjson@S@CrtAllocator#S3_#S3_#S4_#Vi0#&I#@visitor",
      "short_name": "visitor",
      "qualified_name": "visitor",
      "definition": "1:8:22",
      "variable_type": 1,
      "uses": ["1:8:22", "1:9:3"]
    }, {
      "id": 3,
      "usr": "c:impl.cc@164@F@Reflect#&$@N@rapidjson@S@Writer>#$@N@rapidjson@S@GenericStringBuffer>#$@N@rapidjson@S@UTF8>#C#$@N@rapidjson@S@CrtAllocator#S3_#S3_#S4_#Vi0#&I#@value",
      "short_name": "value",
      "qualified_name": "value",
      "definition": "1:8:36",
      "uses": ["1:8:36", "1:9:15"]
    }, {
      "id": 4,
      "usr": "c:impl.cc@227@F@Reflect#&$@N@rapidjson@S@GenericValue>#$@N@rapidjson@S@UTF8>#C#$@N@rapidjson@S@MemoryPoolAllocator>#$@N@rapidjson@S@CrtAllocator#&b#@visitor",
      "short_name": "visitor",
      "qualified_name": "visitor",
      "definition": "1:12:22",
      "variable_type": 0,
      "uses": ["1:12:22"]
    }, {
      "id": 5,
      "usr": "c:impl.cc@244@F@Reflect#&$@N@rapidjson@S@GenericValue>#$@N@rapidjson@S@UTF8>#C#$@N@rapidjson@S@MemoryPoolAllocator>#$@N@rapidjson@S@CrtAllocator#&b#@value",
      "short_name": "value",
      "qualified_name": "value",
      "definition": "1:12:37",
      "uses": ["1:12:37"]
    }, {
      "id": 6,
      "usr": "c:impl.cc@306@F@Reflect#&$@N@rapidjson@S@Writer>#$@N@rapidjson@S@GenericStringBuffer>#$@N@rapidjson@S@UTF8>#C#$@N@rapidjson@S@CrtAllocator#S3_#S3_#S4_#Vi0#&b#@visitor",
      "short_name": "visitor",
      "qualified_name": "visitor",
      "definition": "1:15:22",
      "variable_type": 1,
      "uses": ["1:15:22", "1:16:3"]
    }, {
      "id": 7,
      "usr": "c:impl.cc@323@F@Reflect#&$@N@rapidjson@S@Writer>#$@N@rapidjson@S@GenericStringBuffer>#$@N@rapidjson@S@UTF8>#C#$@N@rapidjson@S@CrtAllocator#S3_#S3_#S4_#Vi0#&b#@value",
      "short_name": "value",
      "qualified_name": "value",
      "definition": "1:15:37",
      "uses": ["1:15:37", "1:16:16"]
    }, {
      "id": 8,
      "usr": "c:impl.cc@395@F@Reflect#&$@N@rapidjson@S@GenericValue>#$@N@rapidjson@S@UTF8>#C#$@N@rapidjson@S@MemoryPoolAllocator>#$@N@rapidjson@S@CrtAllocator#&$@N@std@S@basic_string>#C#$@N@std@S@char_traits>#C#$@N@std@S@allocator>#C#@visitor",
      "short_name": "visitor",
      "qualified_name": "visitor",
      "definition": "1:19:22",
      "variable_type": 0,
      "uses": ["1:19:22"]
    }, {
      "id": 9,
      "usr": "c:impl.cc@412@F@Reflect#&$@N@rapidjson@S@GenericValue>#$@N@rapidjson@S@UTF8>#C#$@N@rapidjson@S@MemoryPoolAllocator>#$@N@rapidjson@S@CrtAllocator#&$@N@std@S@basic_string>#C#$@N@std@S@char_traits>#C#$@N@std@S@allocator>#C#@value",
      "short_name": "value",
      "qualified_name": "value",
      "definition": "1:19:44",
      "variable_type": 2,
      "uses": ["1:19:44"]
    }, {
      "id": 10,
      "usr": "c:impl.cc@483@F@Reflect#&$@N@rapidjson@S@Writer>#$@N@rapidjson@S@GenericStringBuffer>#$@N@rapidjson@S@UTF8>#C#$@N@rapidjson@S@CrtAllocator#S3_#S3_#S4_#Vi0#&$@N@std@S@basic_string>#C#$@N@std@S@char_traits>#C#$@N@std@S@allocator>#C#@visitor",
      "short_name": "visitor",
      "qualified_name": "visitor",
      "definition": "1:22:22",
      "variable_type": 1,
      "uses": ["1:22:22"]
    }, {
      "id": 11,
      "usr": "c:impl.cc@500@F@Reflect#&$@N@rapidjson@S@Writer>#$@N@rapidjson@S@GenericStringBuffer>#$@N@rapidjson@S@UTF8>#C#$@N@rapidjson@S@CrtAllocator#S3_#S3_#S4_#Vi0#&$@N@std@S@basic_string>#C#$@N@std@S@char_traits>#C#$@N@std@S@allocator>#C#@value",
      "short_name": "value",
      "qualified_name": "value",
      "definition": "1:22:44",
      "variable_type": 2,
      "uses": ["1:22:44"]
    }, {
      "id": 12,
      "usr": "c:impl.cc@615@F@ReflectMember#&$@N@rapidjson@S@Writer>#$@N@rapidjson@S@GenericStringBuffer>#$@N@rapidjson@S@UTF8>#C#$@N@rapidjson@S@CrtAllocator#S3_#S3_#S4_#Vi0#*1C#&$@N@std@S@basic_string>#C#$@N@std@S@char_traits>#C#$@N@std@S@allocator>#C#@visitor",
      "short_name": "visitor",
      "qualified_name": "visitor",
      "definition": "1:28:28",
      "variable_type": 1,
      "uses": ["1:28:28", "1:31:3", "1:32:11"]
    }, {
      "id": 13,
      "usr": "c:impl.cc@632@F@ReflectMember#&$@N@rapidjson@S@Writer>#$@N@rapidjson@S@GenericStringBuffer>#$@N@rapidjson@S@UTF8>#C#$@N@rapidjson@S@CrtAllocator#S3_#S3_#S4_#Vi0#*1C#&$@N@std@S@basic_string>#C#$@N@std@S@char_traits>#C#$@N@std@S@allocator>#C#@name",
      "short_name": "name",
      "qualified_name": "name",
      "definition": "1:28:49",
      "uses": ["1:28:49", "1:31:15"]
    }, {
      "id": 14,
      "usr": "c:impl.cc@650@F@ReflectMember#&$@N@rapidjson@S@Writer>#$@N@rapidjson@S@GenericStringBuffer>#$@N@rapidjson@S@UTF8>#C#$@N@rapidjson@S@CrtAllocator#S3_#S3_#S4_#Vi0#*1C#&$@N@std@S@basic_string>#C#$@N@std@S@char_traits>#C#$@N@std@S@allocator>#C#@value",
      "short_name": "value",
      "qualified_name": "value",
      "definition": "1:28:68",
      "variable_type": 2,
      "uses": ["1:28:68", "1:29:7", "1:32:20"]
    }, {
      "id": 15,
      "usr": "c:impl.cc@791@F@Reflect#&$@N@rapidjson@S@GenericValue>#$@N@rapidjson@S@UTF8>#C#$@N@rapidjson@S@MemoryPoolAllocator>#$@N@rapidjson@S@CrtAllocator#&$@S@Location#@visitor",
      "short_name": "visitor",
      "qualified_name": "visitor",
      "definition": "1:37:22",
      "variable_type": 0,
      "uses": ["1:37:22"]
    }, {
      "id": 16,
      "usr": "c:impl.cc@808@F@Reflect#&$@N@rapidjson@S@GenericValue>#$@N@rapidjson@S@UTF8>#C#$@N@rapidjson@S@MemoryPoolAllocator>#$@N@rapidjson@S@CrtAllocator#&$@S@Location#@value",
      "short_name": "value",
      "qualified_name": "value",
      "definition": "1:37:41",
      "variable_type": 3,
      "uses": ["1:37:41"]
    }, {
      "id": 17,
      "usr": "c:impl.cc@886@F@Reflect#&$@N@rapidjson@S@Writer>#$@N@rapidjson@S@GenericStringBuffer>#$@N@rapidjson@S@UTF8>#C#$@N@rapidjson@S@CrtAllocator#S3_#S3_#S4_#Vi0#&$@S@Location#@visitor",
      "short_name": "visitor",
      "qualified_name": "visitor",
      "definition": "1:40:22",
      "variable_type": 1,
      "uses": ["1:40:22"]
    }, {
      "id": 18,
      "usr": "c:impl.cc@903@F@Reflect#&$@N@rapidjson@S@Writer>#$@N@rapidjson@S@GenericStringBuffer>#$@N@rapidjson@S@UTF8>#C#$@N@rapidjson@S@CrtAllocator#S3_#S3_#S4_#Vi0#&$@S@Location#@value",
      "short_name": "value",
      "qualified_name": "value",
      "definition": "1:40:41",
      "variable_type": 3,
      "uses": ["1:40:41", "1:44:24"]
    }, {
      "id": 19,
      "usr": "c:impl.cc@1006@F@Reflect#&$@N@rapidjson@S@Writer>#$@N@rapidjson@S@GenericStringBuffer>#$@N@rapidjson@S@UTF8>#C#$@N@rapidjson@S@CrtAllocator#S3_#S3_#S4_#Vi0#&$@S@Location#@output",
      "short_name": "output",
      "qualified_name": "output",
      "definition": "1:44:15",
      "variable_type": 2,
      "uses": ["1:44:15"]
    }, {
      "id": 20,
      "usr": "c:impl.cc@1148@FT@>1#TReflect#&$@N@rapidjson@S@GenericValue>#$@N@rapidjson@S@UTF8>#C#$@N@rapidjson@S@MemoryPoolAllocator>#$@N@rapidjson@S@CrtAllocator#&>@ST>1#T@Id1t0.0#v#@visitor",
      "short_name": "visitor",
      "qualified_name": "visitor",
      "definition": "1:51:22",
      "variable_type": 0,
      "uses": ["1:51:22"]
    }, {
      "id": 21,
      "usr": "c:impl.cc@1165@FT@>1#TReflect#&$@N@rapidjson@S@GenericValue>#$@N@rapidjson@S@UTF8>#C#$@N@rapidjson@S@MemoryPoolAllocator>#$@N@rapidjson@S@CrtAllocator#&>@ST>1#T@Id1t0.0#v#@id",
      "short_name": "id",
      "qualified_name": "id",
      "definition": "1:51:38",
      "variable_type": 4,
      "uses": ["1:51:38"]
    }, {
      "id": 22,
      "usr": "c:impl.cc@1249@FT@>1#TReflect#&$@N@rapidjson@S@Writer>#$@N@rapidjson@S@GenericStringBuffer>#$@N@rapidjson@S@UTF8>#C#$@N@rapidjson@S@CrtAllocator#S3_#S3_#S4_#Vi0#&>@ST>1#T@Id1t0.0#v#@visitor",
      "short_name": "visitor",
      "qualified_name": "visitor",
      "definition": "1:55:22",
      "variable_type": 1,
      "uses": ["1:55:22"]
    }, {
      "id": 23,
      "usr": "c:impl.cc@1266@FT@>1#TReflect#&$@N@rapidjson@S@Writer>#$@N@rapidjson@S@GenericStringBuffer>#$@N@rapidjson@S@UTF8>#C#$@N@rapidjson@S@CrtAllocator#S3_#S3_#S4_#Vi0#&>@ST>1#T@Id1t0.0#v#@value",
      "short_name": "value",
      "qualified_name": "value",
      "definition": "1:55:38",
      "variable_type": 4,
      "uses": ["1:55:38"]
    }, {
      "id": 24,
      "usr": "c:impl.cc@1356@F@Reflect#&$@N@rapidjson@S@GenericValue>#$@N@rapidjson@S@UTF8>#C#$@N@rapidjson@S@MemoryPoolAllocator>#$@N@rapidjson@S@CrtAllocator#&$@S@Ref>#$@S@IndexedFuncDef#@visitor",
      "short_name": "visitor",
      "qualified_name": "visitor",
      "definition": "1:61:22",
      "variable_type": 0,
      "uses": ["1:61:22"]
    }, {
      "id": 25,
      "usr": "c:impl.cc@1373@F@Reflect#&$@N@rapidjson@S@GenericValue>#$@N@rapidjson@S@UTF8>#C#$@N@rapidjson@S@MemoryPoolAllocator>#$@N@rapidjson@S@CrtAllocator#&$@S@Ref>#$@S@IndexedFuncDef#@value",
      "short_name": "value",
      "qualified_name": "value",
      "definition": "1:61:52",
      "variable_type": 7,
      "uses": ["1:61:52"]
    }, {
      "id": 26,
      "usr": "c:impl.cc@1406@F@Reflect#&$@N@rapidjson@S@GenericValue>#$@N@rapidjson@S@UTF8>#C#$@N@rapidjson@S@MemoryPoolAllocator>#$@N@rapidjson@S@CrtAllocator#&$@S@Ref>#$@S@IndexedFuncDef#@str_value",
      "short_name": "str_value",
      "qualified_name": "str_value",
      "definition": "1:62:15",
      "uses": ["1:62:15", "1:63:22", "1:64:35"]
    }, {
      "id": 27,
      "usr": "c:impl.cc@1454@F@Reflect#&$@N@rapidjson@S@GenericValue>#$@N@rapidjson@S@UTF8>#C#$@N@rapidjson@S@MemoryPoolAllocator>#$@N@rapidjson@S@CrtAllocator#&$@S@Ref>#$@S@IndexedFuncDef#@id",
      "short_name": "id",
      "qualified_name": "id",
      "definition": "1:63:12",
      "variable_type": 9,
      "uses": ["1:63:12"]
    }, {
      "id": 28,
      "usr": "c:impl.cc@1488@F@Reflect#&$@N@rapidjson@S@GenericValue>#$@N@rapidjson@S@UTF8>#C#$@N@rapidjson@S@MemoryPoolAllocator>#$@N@rapidjson@S@CrtAllocator#&$@S@Ref>#$@S@IndexedFuncDef#@loc_string",
      "short_name": "loc_string",
      "qualified_name": "loc_string",
      "definition": "1:64:15",
      "uses": ["1:64:15"]
    }, {
      "id": 29,
      "usr": "c:impl.cc@1635@F@Reflect#&$@N@rapidjson@S@Writer>#$@N@rapidjson@S@GenericStringBuffer>#$@N@rapidjson@S@UTF8>#C#$@N@rapidjson@S@CrtAllocator#S3_#S3_#S4_#Vi0#&$@S@Ref>#$@S@IndexedFuncDef#@visitor",
      "short_name": "visitor",
      "qualified_name": "visitor",
      "definition": "1:69:22",
      "variable_type": 1,
      "uses": ["1:69:22", "1:71:3"]
    }, {
      "id": 30,
      "usr": "c:impl.cc@1652@F@Reflect#&$@N@rapidjson@S@Writer>#$@N@rapidjson@S@GenericStringBuffer>#$@N@rapidjson@S@UTF8>#C#$@N@rapidjson@S@CrtAllocator#S3_#S3_#S4_#Vi0#&$@S@Ref>#$@S@IndexedFuncDef#@value",
      "short_name": "value",
      "qualified_name": "value",
      "definition": "1:69:52",
      "variable_type": 7,
      "uses": ["1:69:52"]
    }, {
      "id": 31,
      "usr": "c:impl.cc@1685@F@Reflect#&$@N@rapidjson@S@Writer>#$@N@rapidjson@S@GenericStringBuffer>#$@N@rapidjson@S@UTF8>#C#$@N@rapidjson@S@CrtAllocator#S3_#S3_#S4_#Vi0#&$@S@Ref>#$@S@IndexedFuncDef#@s",
      "short_name": "s",
      "qualified_name": "s",
      "definition": "1:70:15",
      "variable_type": 2,
      "uses": ["1:70:15", "1:71:18"]
    }, {
      "id": 32,
      "usr": "c:impl.cc@2008@F@ReflectMemberStart#&$@N@rapidjson@S@GenericValue>#$@N@rapidjson@S@UTF8>#C#$@N@rapidjson@S@MemoryPoolAllocator>#$@N@rapidjson@S@CrtAllocator#&$@S@IndexedTypeDef#@reader",
      "short_name": "reader",
      "qualified_name": "reader",
      "definition": "1:85:33",
      "variable_type": 0,
      "uses": ["1:85:33"]
    }, {
      "id": 33,
      "usr": "c:impl.cc@2024@F@ReflectMemberStart#&$@N@rapidjson@S@GenericValue>#$@N@rapidjson@S@UTF8>#C#$@N@rapidjson@S@MemoryPoolAllocator>#$@N@rapidjson@S@CrtAllocator#&$@S@IndexedTypeDef#@value",
      "short_name": "value",
      "qualified_name": "value",
      "definition": "1:85:57",
      "variable_type": 10,
      "uses": ["1:85:57"]
    }, {
      "id": 34,
      "usr": "c:impl.cc@2124@F@ReflectMemberStart#&$@N@rapidjson@S@Writer>#$@N@rapidjson@S@GenericStringBuffer>#$@N@rapidjson@S@UTF8>#C#$@N@rapidjson@S@CrtAllocator#S3_#S3_#S4_#Vi0#&$@S@IndexedTypeDef#@writer",
      "short_name": "writer",
      "qualified_name": "writer",
      "definition": "1:89:33",
      "variable_type": 1,
      "uses": ["1:89:33", "1:92:29"]
    }, {
      "id": 35,
      "usr": "c:impl.cc@2140@F@ReflectMemberStart#&$@N@rapidjson@S@Writer>#$@N@rapidjson@S@GenericStringBuffer>#$@N@rapidjson@S@UTF8>#C#$@N@rapidjson@S@CrtAllocator#S3_#S3_#S4_#Vi0#&$@S@IndexedTypeDef#@value",
      "short_name": "value",
      "qualified_name": "value",
      "definition": "1:89:57",
      "variable_type": 10,
      "uses": ["1:89:57"]
    }, {
      "id": 36,
      "usr": "c:impl.cc@2313@FT@>1#TReflect#&t0.0#&$@S@IndexedTypeDef#v#@visitor",
      "short_name": "visitor",
      "qualified_name": "visitor",
      "definition": "1:96:24",
      "uses": ["1:96:24"]
    }, {
      "id": 37,
      "usr": "c:impl.cc@2332@FT@>1#TReflect#&t0.0#&$@S@IndexedTypeDef#v#@value",
      "short_name": "value",
      "qualified_name": "value",
      "definition": "1:96:49",
      "variable_type": 10,
      "uses": ["1:96:49"]
    }, {
      "id": 38,
      "usr": "c:impl.cc@3038@F@ReflectMemberStart#&$@N@rapidjson@S@GenericValue>#$@N@rapidjson@S@UTF8>#C#$@N@rapidjson@S@MemoryPoolAllocator>#$@N@rapidjson@S@CrtAllocator#&$@S@IndexedFuncDef#@reader",
      "short_name": "reader",
      "qualified_name": "reader",
      "definition": "1:115:33",
      "variable_type": 0,
      "uses": ["1:115:33"]
    }, {
      "id": 39,
      "usr": "c:impl.cc@3054@F@ReflectMemberStart#&$@N@rapidjson@S@GenericValue>#$@N@rapidjson@S@UTF8>#C#$@N@rapidjson@S@MemoryPoolAllocator>#$@N@rapidjson@S@CrtAllocator#&$@S@IndexedFuncDef#@value",
      "short_name": "value",
      "qualified_name": "value",
      "definition": "1:115:57",
      "variable_type": 8,
      "uses": ["1:115:57"]
    }, {
      "id": 40,
      "usr": "c:impl.cc@3154@F@ReflectMemberStart#&$@N@rapidjson@S@Writer>#$@N@rapidjson@S@GenericStringBuffer>#$@N@rapidjson@S@UTF8>#C#$@N@rapidjson@S@CrtAllocator#S3_#S3_#S4_#Vi0#&$@S@IndexedFuncDef#@writer",
      "short_name": "writer",
      "qualified_name": "writer",
      "definition": "1:119:33",
      "variable_type": 1,
      "uses": ["1:119:33", "1:122:29"]
    }, {
      "id": 41,
      "usr": "c:impl.cc@3170@F@ReflectMemberStart#&$@N@rapidjson@S@Writer>#$@N@rapidjson@S@GenericStringBuffer>#$@N@rapidjson@S@UTF8>#C#$@N@rapidjson@S@CrtAllocator#S3_#S3_#S4_#Vi0#&$@S@IndexedFuncDef#@value",
      "short_name": "value",
      "qualified_name": "value",
      "definition": "1:119:57",
      "variable_type": 8,
      "uses": ["1:119:57"]
    }, {
      "id": 42,
      "usr": "c:impl.cc@3343@FT@>1#TReflect#&t0.0#&$@S@IndexedFuncDef#v#@visitor",
      "short_name": "visitor",
      "qualified_name": "visitor",
      "definition": "1:126:24",
      "uses": ["1:126:24"]
    }, {
      "id": 43,
      "usr": "c:impl.cc@3362@FT@>1#TReflect#&t0.0#&$@S@IndexedFuncDef#v#@value",
      "short_name": "value",
      "qualified_name": "value",
      "definition": "1:126:49",
      "variable_type": 8,
      "uses": ["1:126:49"]
    }, {
      "id": 44,
      "usr": "c:impl.cc@4137@F@ReflectMemberStart#&$@N@rapidjson@S@GenericValue>#$@N@rapidjson@S@UTF8>#C#$@N@rapidjson@S@MemoryPoolAllocator>#$@N@rapidjson@S@CrtAllocator#&$@S@IndexedVarDef#@reader",
      "short_name": "reader",
      "qualified_name": "reader",
      "definition": "1:146:33",
      "variable_type": 0,
      "uses": ["1:146:33"]
    }, {
      "id": 45,
      "usr": "c:impl.cc@4153@F@ReflectMemberStart#&$@N@rapidjson@S@GenericValue>#$@N@rapidjson@S@UTF8>#C#$@N@rapidjson@S@MemoryPoolAllocator>#$@N@rapidjson@S@CrtAllocator#&$@S@IndexedVarDef#@value",
      "short_name": "value",
      "qualified_name": "value",
      "definition": "1:146:56",
      "variable_type": 13,
      "uses": ["1:146:56"]
    }, {
      "id": 46,
      "usr": "c:impl.cc@4252@F@ReflectMemberStart#&$@N@rapidjson@S@Writer>#$@N@rapidjson@S@GenericStringBuffer>#$@N@rapidjson@S@UTF8>#C#$@N@rapidjson@S@CrtAllocator#S3_#S3_#S4_#Vi0#&$@S@IndexedVarDef#@writer",
      "short_name": "writer",
      "qualified_name": "writer",
      "definition": "1:150:33",
      "variable_type": 1,
      "uses": ["1:150:33", "1:153:29"]
    }, {
      "id": 47,
      "usr": "c:impl.cc@4268@F@ReflectMemberStart#&$@N@rapidjson@S@Writer>#$@N@rapidjson@S@GenericStringBuffer>#$@N@rapidjson@S@UTF8>#C#$@N@rapidjson@S@CrtAllocator#S3_#S3_#S4_#Vi0#&$@S@IndexedVarDef#@value",
      "short_name": "value",
      "qualified_name": "value",
      "definition": "1:150:56",
      "variable_type": 13,
      "uses": ["1:150:56"]
    }, {
      "id": 48,
      "usr": "c:impl.cc@4440@FT@>1#TReflect#&t0.0#&$@S@IndexedVarDef#v#@visitor",
      "short_name": "visitor",
      "qualified_name": "visitor",
      "definition": "1:157:24",
      "uses": ["1:157:24"]
    }, {
      "id": 49,
      "usr": "c:impl.cc@4459@FT@>1#TReflect#&t0.0#&$@S@IndexedVarDef#v#@value",
      "short_name": "value",
      "qualified_name": "value",
      "definition": "1:157:48",
      "variable_type": 13,
      "uses": ["1:157:48"]
    }, {
      "id": 50,
      "usr": "c:impl.cc@5061@F@ReflectMemberStart#&$@N@rapidjson@S@Writer>#$@N@rapidjson@S@GenericStringBuffer>#$@N@rapidjson@S@UTF8>#C#$@N@rapidjson@S@CrtAllocator#S3_#S3_#S4_#Vi0#&$@S@IndexedFile#@visitor",
      "short_name": "visitor",
      "qualified_name": "visitor",
      "definition": "1:173:33",
      "variable_type": 1,
      "uses": ["1:173:33", "1:180:29"]
    }, {
      "id": 51,
      "usr": "c:impl.cc@5078@F@ReflectMemberStart#&$@N@rapidjson@S@Writer>#$@N@rapidjson@S@GenericStringBuffer>#$@N@rapidjson@S@UTF8>#C#$@N@rapidjson@S@CrtAllocator#S3_#S3_#S4_#Vi0#&$@S@IndexedFile#@value",
      "short_name": "value",
      "qualified_name": "value",
      "definition": "1:173:55",
      "variable_type": 15,
      "uses": ["1:173:55"]
    }, {
      "id": 52,
      "usr": "c:impl.cc@5103@F@ReflectMemberStart#&$@N@rapidjson@S@Writer>#$@N@rapidjson@S@GenericStringBuffer>#$@N@rapidjson@S@UTF8>#C#$@N@rapidjson@S@CrtAllocator#S3_#S3_#S4_#Vi0#&$@S@IndexedFile#@it",
      "short_name": "it",
      "qualified_name": "it",
      "definition": "1:174:8",
      "uses": ["1:174:8"]
    }, {
      "id": 53,
      "usr": "c:impl.cc@5437@FT@>1#TReflect#&t0.0#&$@S@IndexedFile#v#@visitor",
      "short_name": "visitor",
      "qualified_name": "visitor",
      "definition": "1:184:24",
      "uses": ["1:184:24"]
    }, {
      "id": 54,
      "usr": "c:impl.cc@5456@FT@>1#TReflect#&t0.0#&$@S@IndexedFile#v#@value",
      "short_name": "value",
      "qualified_name": "value",
      "definition": "1:184:46",
      "variable_type": 15,
      "uses": ["1:184:46"]
    }, {
      "id": 55,
      "usr": "c:impl.cc@5647@F@Serialize#&$@S@IndexedFile#@file",
      "short_name": "file",
      "qualified_name": "file",
      "definition": "1:198:36",
      "variable_type": 15,
      "uses": ["1:198:36"]
    }, {
      "id": 56,
      "usr": "c:impl.cc@5671@F@Serialize#&$@S@IndexedFile#@output",
      "short_name": "output",
      "qualified_name": "output",
      "definition": "1:199:27",
      "variable_type": 17,
      "uses": ["1:199:27"]
    }, {
      "id": 57,
      "usr": "c:impl.cc@5706@F@Serialize#&$@S@IndexedFile#@writer",
      "short_name": "writer",
      "qualified_name": "writer",
      "definition": "1:200:52",
      "uses": ["1:200:52"]
    }, {
      "id": 58,
      "usr": "c:impl.cc@6018@F@Deserialize#$@N@std@S@basic_string>#C#$@N@std@S@char_traits>#C#$@N@std@S@allocator>#C#S0_#@path",
      "short_name": "path",
      "qualified_name": "path",
      "definition": "1:211:47",
      "variable_type": 2,
      "uses": ["1:211:47", "1:217:20"]
    }, {
      "id": 59,
      "usr": "c:impl.cc@6036@F@Deserialize#$@N@std@S@basic_string>#C#$@N@std@S@char_traits>#C#$@N@std@S@allocator>#C#S0_#@serialized",
      "short_name": "serialized",
      "qualified_name": "serialized",
      "definition": "1:211:65",
      "variable_type": 2,
      "uses": ["1:211:65"]
    }, {
      "id": 60,
      "usr": "c:impl.cc@6065@F@Deserialize#$@N@std@S@basic_string>#C#$@N@std@S@char_traits>#C#$@N@std@S@allocator>#C#S0_#@reader",
      "short_name": "reader",
      "qualified_name": "reader",
      "definition": "1:212:23",
      "variable_type": 19,
      "uses": ["1:212:23"]
    }, {
      "id": 61,
      "usr": "c:impl.cc@6187@F@Deserialize#$@N@std@S@basic_string>#C#$@N@std@S@char_traits>#C#$@N@std@S@allocator>#C#S0_#@file",
      "short_name": "file",
      "qualified_name": "file",
      "definition": "1:217:15",
      "variable_type": 15,
      "uses": ["1:217:15"]
    }]
}
*/
