#include "message_handler.h"
#include "query_utils.h"
#include "queue_manager.h"

#include <math.h>
#include <stdlib.h>
#include <numeric>

namespace {
struct Ipc_CqueryRandom : public RequestMessage<Ipc_CqueryRandom> {
  const static IpcId kIpcId = IpcId::CqueryRandom;
};
MAKE_REFLECT_STRUCT(Ipc_CqueryRandom, id);
REGISTER_IPC_MESSAGE(Ipc_CqueryRandom);

const double kDeclWeight = 3;
const double kDamping = 0.1;

template <typename Q>
struct Kind;
template <>
struct Kind<QueryFunc> { static constexpr SymbolKind value = SymbolKind::Func; };
template <>
struct Kind<QueryType> { static constexpr SymbolKind value = SymbolKind::Type; };
template <>
struct Kind<QueryVar> { static constexpr SymbolKind value = SymbolKind::Var; };

template <typename Q>
void Add(const std::unordered_map<SymbolIdx, int>& sym2id,
         std::vector<std::unordered_map<int, double>>& adj,
         const std::vector<Id<Q>>& ids,
         int n,
         double w = 1) {
  for (Id<Q> id : ids) {
    auto it = sym2id.find(SymbolIdx{id, Kind<Q>::value});
    if (it != sym2id.end())
      adj[it->second][n] += w;
  }
}

struct CqueryRandomHandler : BaseMessageHandler<Ipc_CqueryRandom> {
  void Run(Ipc_CqueryRandom* request) override {
    std::unordered_map<SymbolIdx, int> sym2id;
    std::vector<SymbolIdx> syms;
    int n = 0;

    for (RawId i = 0; i < db->funcs.size(); i++)
      if (db->funcs[i].def) {
        syms.push_back(SymbolIdx{Id<void>(i), SymbolKind::Func});
        sym2id[syms.back()] = n++;
      }
    for (RawId i = 0; i < db->types.size(); i++)
      if (db->types[i].def) {
        syms.push_back(SymbolIdx{Id<void>(i), SymbolKind::Type});
        sym2id[syms.back()] = n++;
      }
    for (RawId i = 0; i < db->vars.size(); i++)
      if (db->vars[i].AnyDef()) {
        syms.push_back(SymbolIdx{Id<void>(i), SymbolKind::Var});
        sym2id[syms.back()] = n++;
      }

    std::vector<std::unordered_map<int, double>> adj(n);
    auto add = [&](const std::vector<Use>& uses, double w) {
      for (Use use : uses) {
        auto it = sym2id.find(use);
        if (it != sym2id.end())
          adj[it->second][n] += w;
      }
    };
    n = 0;
    for (QueryFunc& func : db->funcs)
      if (func.AnyDef()) {
        add(func.declarations, kDeclWeight);
        add(func.uses, 1);
        Add(sym2id, adj, func.derived, n);
        n++;
      }
    for (QueryType& type : db->types)
      if (type.AnyDef()) {
        add(type.uses, 1);
        Add(sym2id, adj, type.instances, n);
        Add(sym2id, adj, type.def->funcs, n);
        Add(sym2id, adj, type.def->types, n);
        Add(sym2id, adj, type.def->vars, n);
        n++;
      }
    for (QueryVar& var : db->vars)
      if (var.AnyDef()) {
        add(var.declarations, kDeclWeight);
        add(var.uses, 1);
        n++;
      }
    for (int i = 0; i < n; i++) {
      double sum = 0;
      adj[i][i] += 1;
      for (auto& it : adj[i])
        sum += it.second;
      for (auto& it : adj[i])
        it.second = it.second / sum * (1 - kDamping);
    }

    std::vector<double> x(n, 1), y;
    for (int j = 0; j < 30; j++) {
      y.assign(n, kDamping);
      for (int i = 0; i < n; i++)
        for (auto& it : adj[i])
          y[it.first] += x[i] * it.second;
      double d = 0;
      for (int i = 0; i < n; i++)
        d = std::max(d, fabs(x[i] - y[i]));
      if (d < 1e-5) break;
      x.swap(y);
    }

    double sum = std::accumulate(x.begin(), x.end(), 0.);
    Out_LocationList out;
    out.id = request->id;
    double roulette = rand() / (RAND_MAX + 1.0) * sum;
    sum = 0;
    for (int i = 0; i < n; i++) {
      sum += x[i];
      if (sum >= roulette) {
        Maybe<Use> use = GetDefinitionExtentOfSymbol(db, syms[i]);
        if (!use)
          continue;
        optional<lsLocation> ls_loc = GetLsLocation(db, working_files, *use);
        if (ls_loc)
          out.result.push_back(*ls_loc);
        break;
      }
    }
    QueueManager::WriteStdout(IpcId::CqueryRandom, out);
  }
};
REGISTER_MESSAGE_HANDLER(CqueryRandomHandler);
}  // namespace
