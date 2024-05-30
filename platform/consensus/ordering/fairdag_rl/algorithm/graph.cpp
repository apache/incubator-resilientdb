#include "platform/consensus/ordering/fairdag_rl/algorithm/graph.h"

#include <glog/logging.h>
#include <queue>
#include "common/utils/utils.h"


namespace resdb {
namespace fairdag_rl {

namespace {

class OrderHelper{
private:
  std::vector<int> fa_;
  int id_;
  int n_;
  std::map<std::string, int> hash2id_;
  std::map<int, std::string> id2hash_;

  public:
    void Init(int num){
      id_ = 1;
      n_ = num+1;
      fa_.resize(n_);
      for(int i = 1; i <=num; ++i){
        fa_[i] = i;
      }
    }


    std::vector<std::string> Order(
      const std::map<std::string, std::set<std::string> >& reach_sets){

      auto find = [&](const std::string&h1, const std::string& h2) {
        auto it = reach_sets.find(h1);
        assert(it != reach_sets.end());
        return it->second.find(h2) != it->second.end();
      };
      LOG(ERROR)<<" ?????:"<<reach_sets.size();

      for(auto it1 : reach_sets){
        for(auto it2 : reach_sets){
          if(it1.first >= it2.first){
            continue;
          }
          std::string hash1 = it1.first;
          std::string hash2 = it2.first;

          bool lr = find(hash1, hash2);
          bool rr = find(hash2, hash1);
          //LOG(ERROR)<<" hash1:"<<hash1<<" lr:"<<lr<<" hash2:"<<hash2<<" rr:"<<rr;

          if(lr && rr){
      LOG(ERROR)<<" merge:"<<reach_sets.size();
            Merge(hash1, hash2);
      LOG(ERROR)<<" merge done:"<<reach_sets.size();
          }
        }
      }
      LOG(ERROR)<<"????";

      std::map<int, std::vector<int> > e;
      std::vector<int> deg(n_);
      LOG(ERROR)<<" deg size:"<<deg.size()<<" reach set:"<<reach_sets.size();

      for(auto it1 : reach_sets){
        for(auto it2 : reach_sets){
          if(it1.first >= it2.first){
            continue;
          }
          std::string hash1 = it1.first;
          std::string hash2 = it2.first;

          bool lr = find(hash1, hash2);
          bool rr = find(hash2, hash1);

          if(lr && rr){
            continue;
          }
          else {
            int id1 = GetId(hash1);
            int id2 = GetId(hash2);

            assert(id1<n_);
            assert(id2<n_);
    
            if(f(id1) == f(id2)){
              continue;
            }
            LOG(ERROR)<<" id1:"<<id1<<" "<<"id2:"<<id2<<" lr:"<<lr<<" rr:"<<rr;
            if(lr){
              e[f(id1)].push_back(f(id2));
              deg[f(id2)]++;
            }
            else if(rr){
              e[f(id2)].push_back(f(id1));
              deg[f(id1)]++;
            }
          }
        }
      }

    LOG(ERROR)<<" get queue";
      std::map<int, std::vector<int> > groups;
      std::queue<int> q;
      std::set<std::string> hidx;
      for(int i = 1; i <n_; ++i){
        if(f(i) != i){
          groups[f(i)].push_back(i);
          continue;
        }
        if(deg[i] == 0){
          q.push(i);
        }
      }
      LOG(ERROR)<<" q size:"<<q.size()<<" n:"<<n_;
      std::vector<std::string> orders;
      while(!q.empty()){
        int u = q.front();
        q.pop();
        LOG(ERROR)<<" push u:"<<u;
        orders.push_back(GetHash(u));
        assert(hidx.find(GetHash(u)) == hidx.end());
        hidx.insert(GetHash(u));
        if(groups.find(u) != groups.end()){
          for(int w : groups[u]){
          LOG(ERROR)<<" push group w:"<<w;
            orders.push_back(GetHash(w));
            assert(hidx.find(GetHash(w)) == hidx.end());
            hidx.insert(GetHash(w));
          }
        }
        else {
        }

        for(int v : e[u]){
          deg[v]--;
          if(deg[v]==0){
            q.push(v);
          }
        }
      }
      return orders;
    }

    private:
    int f(int t){
      if(fa_[t] != t){
        fa_[t] = f(fa_[t]);
      }
      return fa_[t];
    }

    std::string GetHash(int id){
      return id2hash_[id];
    }

    int GetId(const std::string& h, bool assign = true){
      if(assign){
        if(hash2id_.find(h) == hash2id_.end()){
          id2hash_[id_] = h;
          hash2id_[h] = id_++;
        }
      }
      else {
        assert(hash2id_.find(h) != hash2id_.end());
      }
      return hash2id_[h];
    }

    void Merge(const std::string&h1, const std::string&h2) {
      int id1 = GetId(h1);
      int id2 = GetId(h2);
      if(f(id1) != f(id2)){
        fa_[f(id2)] = f(id1);
      }
    }
};

} // namespace

void Graph::AddTxn(const Transaction& a, const Transaction& b){
  if(hash2idx_.find(a.hash()) == hash2idx_.end()){
    hash2idx_[a.hash()] = idx_++;
  }
  if(hash2idx_.find(b.hash()) == hash2idx_.end()){
    hash2idx_[b.hash()] = idx_++;
  }
  g_[a.hash()].insert(b.hash());
  preg_[b.hash()].insert(a.hash());
}

void Graph::RemoveTxn(const std::string& hash){
  assert(hash2idx_.find(hash) != hash2idx_.end());
  LOG(ERROR)<<" remove:"<<hash2idx_[hash];
  for(const std::string& u : preg_[hash]){
    assert(g_[u].find(hash) != g_[u].end());
    g_[u].erase(g_[u].find(hash));
  }
  for(const std::string& u : g_[hash]){
    preg_[u].erase(preg_[u].find(hash));
  }
  assert(g_.find(hash) != g_.end());
  g_.erase(g_.find(hash));
  assert(preg_.find(hash) != preg_.end());
  preg_.erase(preg_.find(hash));
}

void Graph::Dfs(const std::string& hash, std::set<std::string>& res){
  //LOG(ERROR)<<" find dfs:"<<hash2idx_[hash];
  res.insert(hash);
  if(g_.find(hash) == g_.end()){
    return;
  }
  for(const std::string& h : g_[hash]){
    if(res.find(h) != res.end()){
      continue;
    }
    Dfs(h, res);
  }
}

void Graph::CheckGraph(){
  for(auto it: g_){
    const std::string& u = it.first;
    for(const std::string& w : g_[u]){
      assert(preg_[w].find(u) != preg_[w].end());
    }
  }
  for(auto it: preg_){
    const std::string& u = it.first;
    for(const std::string& w : preg_[u]){
      assert(g_[w].find(u) != g_[w].end());
    }
  }
}

std::vector<std::string> Graph::GetOrder(const std::vector<std::string>& commit_txns) {
    LOG(ERROR)<<" commit size"<<commit_txns.size();
  std::map<std::string, std::set<std::string> > reach_sets;
  for (int i = 0; i < commit_txns.size(); ++i){
    if(g_.find(commit_txns[i]) == g_.end()){
      LOG(ERROR)<<"g not found";
      continue;
    }
    assert(reach_sets.find(commit_txns[i]) == reach_sets.end());

    LOG(ERROR)<<" check dfs"<<hash2idx_[commit_txns[i]];
    Dfs(commit_txns[i], reach_sets[commit_txns[i]]);
    LOG(ERROR)<<" check dfs done"<<hash2idx_[commit_txns[i]];
  }
    return commit_txns;

  LOG(ERROR)<<" commit size"<<commit_txns.size();
  CheckGraph();
  LOG(ERROR)<<" commit size"<<commit_txns.size();

    LOG(ERROR)<<" dfs:"<<reach_sets.size();
  OrderHelper handler;
  handler.Init(reach_sets.size());
  return handler.Order(reach_sets);
}

}  // namespace fairdag_rl
}  // namespace resdb
