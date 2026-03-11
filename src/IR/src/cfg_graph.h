#pragma once

#include <unordered_map>
#include <vector>

#include "program.h"

namespace IR {

struct CFGGraph {
  std::vector<std::vector<int>> predecessors;
  std::vector<std::vector<int>> successors;
};

using CFGGraphs = std::unordered_map<const Function*, CFGGraph>;

CFGGraphs build_cfg_graphs(const Program&);

}  // namespace IR
