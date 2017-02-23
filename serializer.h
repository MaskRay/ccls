#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>

struct IndexedFile;
using Writer = rapidjson::PrettyWriter<rapidjson::StringBuffer>;

void Serialize(Writer& writer, IndexedFile* file);