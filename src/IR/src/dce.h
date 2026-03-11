#pragma once

#include "cfg_graph.h"
#include "program.h"

namespace IR {

class DeadCodeElimination {
 public:
  bool run(Program&, const CFGGraphs&);
};

}  // namespace IR
