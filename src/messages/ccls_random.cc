#include "message_handler.h"
#include "query_utils.h"
#include "queue_manager.h"

#include <math.h>
#include <stdlib.h>
#include <numeric>

MAKE_HASHABLE(SymbolIdx, t.usr, t.kind);

namespace {
MethodType kMethodType = "$ccls/random";

struct In_CclsRandom : public RequestInMessage {
  MethodType GetMethodType() const override { return kMethodType; }
};
MAKE_REFLECT_STRUCT(In_CclsRandom, id);
REGISTER_IN_MESSAGE(In_CclsRandom);

const double kDeclWeight = 3;
const double kDamping = 0.1;

template <auto Q>
void Add(const std::unordered_map<SymbolIdx, int>& sym2id,
         std::vector<std::unordered_map<int, double>>& adj,
         const std::vector<Usr>& collection,
         int n,
         double w = 1) {
  for (Usr usr : collection) {
    auto it = sym2id.find(SymbolIdx{usr, Q});
    if (it != sym2id.end())
      adj[it->second][n] += w;
  }
}

struct Handler_CclsRandom : BaseMessageHandler<In_CclsRandom> {
  MethodType GetMethodType() const override { return kMethodType; }

  void Run(In_CclsRandom* request) override {
    std::unordered_map<SymbolIdx, int> sym2id;
    std::vector<SymbolIdx> syms;
    int n = 0;

    for (auto& it : db->usr2func)
      if (it.second.AnyDef()) {
        syms.push_back(SymbolIdx{it.first, SymbolKind::Func});
        sym2id[syms.back()] = n++;
      }
    for (auto& it : db->usr2type)
      if (it.second.AnyDef()) {
        syms.push_back(SymbolIdx{it.first, SymbolKind::Type});
        sym2id[syms.back()] = n++;
      }
    for (auto& it : db->usr2var)
      if (it.second.AnyDef()) {
        syms.push_back(SymbolIdx{it.first, SymbolKind::Var});
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
    for (auto& it : db->usr2func)
      if (it.second.AnyDef()) {
        add(it.second.declarations, kDeclWeight);
        add(it.second.uses, 1);
        Add<SymbolKind::Func>(sym2id, adj, it.second.derived, n);
        n++;
      }
    for (auto& it : db->usr2type)
      if (const auto* def = it.second.AnyDef()) {
        add(it.second.uses, 1);
        Add<SymbolKind::Var>(sym2id, adj, it.second.instances, n);
        Add<SymbolKind::Func>(sym2id, adj, def->funcs, n);
        Add<SymbolKind::Type>(sym2id, adj, def->types, n);
        Add<SymbolKind::Var>(sym2id, adj, def->vars, n);
        n++;
      }
    for (auto& it : db->usr2var)
      if (it.second.AnyDef()) {
        add(it.second.declarations, kDeclWeight);
        add(it.second.uses, 1);
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
    for (int j = 0; j < 8; j++) {
      y.assign(n, kDamping);
      for (int i = 0; i < n; i++)
        for (auto& it : adj[i])
          y[it.first] += x[i] * it.second;
      double d = 0;
      for (int i = 0; i < n; i++)
        d = std::max(d, fabs(x[i] - y[i]));
      if (d < 1e-5)
        break;
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
        if (Maybe<Use> use = GetDefinitionExtent(db, syms[i]))
          if (auto ls_loc = GetLsLocationEx(db, working_files, *use,
                                            g_config->xref.container))
            out.result.push_back(*ls_loc);
        break;
      }
    }
    QueueManager::WriteStdout(kMethodType, out);
  }
};
REGISTER_MESSAGE_HANDLER(Handler_CclsRandom);
}  // namespace
