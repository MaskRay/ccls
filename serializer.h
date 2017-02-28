#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>

struct IndexedFile;
using Writer = rapidjson::PrettyWriter<rapidjson::StringBuffer>;
using Reader = rapidjson::Document;

void Serialize(Writer& writer, IndexedFile* file);
void Deserialize(Reader& reader, IndexedFile* file);