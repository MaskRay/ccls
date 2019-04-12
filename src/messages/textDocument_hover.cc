// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#include "message_handler.hh"
#include "query.hh"

namespace ccls {
namespace {
struct MarkedString {
  std::optional<std::string> language;
  std::string value;
};
struct Hover {
  std::vector<MarkedString> contents;
  std::optional<lsRange> range;
};

void Reflect(JsonWriter &vis, MarkedString &v) {
  // If there is a language, emit a `{language:string, value:string}` object. If
  // not, emit a string.
  if (v.language) {
    vis.StartObject();
    REFLECT_MEMBER(language);
    REFLECT_MEMBER(value);
    vis.EndObject();
  } else {
    Reflect(vis, v.value);
  }
}
REFLECT_STRUCT(Hover, contents, range);

const char *LanguageIdentifier(LanguageId lang) {
  switch (lang) {
  // clang-format off
  case LanguageId::C: return "c";
  case LanguageId::Cpp: return "cpp";
  case LanguageId::ObjC: return "objective-c";
  case LanguageId::ObjCpp: return "objective-cpp";
  default: return "";
  // clang-format on
  }
}

// Returns the hover or detailed name for `sym`, if any.
std::pair<std::optional<MarkedString>, std::optional<MarkedString>>
GetHover(DB *db, LanguageId lang, SymbolRef sym, int file_id) {
  const char *comments = nullptr;
  std::optional<MarkedString> ls_comments, hover;
  WithEntity(db, sym, [&](const auto &entity) {
    for (auto &d : entity.def) {
      if (!comments && d.comments[0])
        comments = d.comments;
      if (d.spell) {
        if (d.comments[0])
          comments = d.comments;
        if (const char *s =
                d.hover[0] ? d.hover
                           : d.detailed_name[0] ? d.detailed_name : nullptr) {
          if (!hover)
            hover = {LanguageIdentifier(lang), s};
          else if (strlen(s) > hover->value.size())
            hover->value = s;
        }
        if (d.spell->file_id == file_id)
          break;
      }
    }
    if (!hover && entity.def.size()) {
      auto &d = entity.def[0];
      hover = {LanguageIdentifier(lang)};
      if (d.hover[0])
        hover->value = d.hover;
      else if (d.detailed_name[0])
        hover->value = d.detailed_name;
    }
    if (comments)
      ls_comments = MarkedString{std::nullopt, comments};
  });
  return {hover, ls_comments};
}
} // namespace

void MessageHandler::textDocument_hover(TextDocumentPositionParam &param,
                                        ReplyOnce &reply) {
  auto [file, wf] = FindOrFail(param.textDocument.uri.GetPath(), reply);
  if (!wf)
    return;

  Hover result;
  for (SymbolRef sym : FindSymbolsAtLocation(wf, file, param.position)) {
    std::optional<lsRange> ls_range =
        GetLsRange(wfiles->GetFile(file->def->path), sym.range);
    if (!ls_range)
      continue;

    auto [hover, comments] = GetHover(db, file->def->language, sym, file->id);
    if (comments || hover) {
      result.range = *ls_range;
      if (comments)
        result.contents.push_back(*comments);
      if (hover)
        result.contents.push_back(*hover);
      break;
    }
  }

  reply(result);
}
} // namespace ccls
