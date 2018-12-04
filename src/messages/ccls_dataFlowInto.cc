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

#include "message_handler.hh"
#include "pipeline.hh"
#include "query.hh"

#include <unordered_set>

namespace ccls {
namespace {
struct Param : TextDocumentPositionParam {
};
REFLECT_STRUCT(Param, textDocument, position);
} // namespace

struct Out_cclsDataFlowInto {
  int id;
  Location location;
  // Empty if the |levels| limit is reached.
  std::vector<Out_cclsDataFlowInto> children;
};
REFLECT_STRUCT(Out_cclsDataFlowInto, id ,location, children);

Out_cclsDataFlowInto BuildDataFlow(Usr usr, Kind kind, Use use, std::unordered_set<Usr> seen, DB* db, WorkingFiles* wfiles, int& id) {
  Out_cclsDataFlowInto result = {id++};
  if (auto loc = GetLsLocation(db, wfiles, use)) {
    result.location = *loc;
  }
  if (!seen.insert(usr).second) return result;

  const std::vector<DataFlow>* data_flow_sources = nullptr;
  if (kind == Kind::Var) {
    auto& var = db->Var(usr);
    data_flow_sources = &var.data_flow_into;
  } else if (kind == Kind::Func) {
    auto& func = db->Func(usr);
    data_flow_sources = &func.data_flow_into_return;
  }
  if (data_flow_sources != nullptr) {
    if (data_flow_sources->empty()) {
      if (kind == Kind::Var) {
        auto& var = db->Var(usr);
        for (auto& def : var.def) {
          if (def.spell) {
            if (auto loc = GetLsLocation(db, wfiles, *def.spell)) {
              result.children.push_back({id++, *loc});
            }
          }
        }
      } else if (kind == Kind::Func) {
        auto& func = db->Func(usr);
        for (auto& def : func.def) {
          if (def.spell) {
            if (auto loc = GetLsLocation(db, wfiles, *def.spell)) {
              result.children.push_back({id++, *loc});
            }
          }
        }
      }
    } else {
      for (auto& write : *data_flow_sources) {
        if (write.use.role == Role::Read) {
          result.children.push_back(BuildDataFlow(write.from, Kind::Var, write.use, seen, db, wfiles, id));
        } else if (write.use.role == Role::Call) {
          result.children.push_back(BuildDataFlow(write.from, Kind::Func, write.use, seen, db, wfiles, id));
        } else if (auto loc = GetLsLocation(db, wfiles, write.use)) {
          result.children.push_back({id++, *loc});
        }
      }
    }
  }
  return result;
}

void MessageHandler::ccls_dataFlowInto(JsonReader &reader, ReplyOnce &reply) {
  Param param;
  Reflect(reader, param);
  QueryFile *file = FindFile(param.textDocument.uri.GetPath());
  WorkingFile *wf = file ? wfiles->GetFile(file->def->path) : nullptr;
  if (!wf) {
    reply.NotReady(file);
    return;
  }

  std::optional<Out_cclsDataFlowInto> result;
  for (SymbolRef sym : FindSymbolsAtLocation(wf, file, param.position)) {
    // Found symbol. Return references.
    int id = 0;
    std::unordered_set<Usr> seen = {};
    result = BuildDataFlow(sym.usr, sym.kind, Use{{sym.range, sym.role},file->id}, seen, db, wfiles, id);
    break;
  }
  reply(result);
}
} // namespace ccls
