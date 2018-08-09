#include "language.h"

#include "utils.h"

LanguageId SourceFileLanguage(std::string_view path) {
  if (EndsWith(path, ".c"))
    return LanguageId::C;
  else if (EndsWith(path, ".cpp") || EndsWith(path, ".cc"))
    return LanguageId::Cpp;
  else if (EndsWith(path, ".mm"))
    return LanguageId::ObjCpp;
  else if (EndsWith(path, ".m"))
    return LanguageId::ObjC;
  return LanguageId::Unknown;
}

const char *LanguageIdentifier(LanguageId lang) {
  switch (lang) {
  case LanguageId::C:
    return "c";
  case LanguageId::Cpp:
    return "cpp";
  case LanguageId::ObjC:
    return "objective-c";
  case LanguageId::ObjCpp:
    return "objective-cpp";
  default:
    return "";
  }
}
