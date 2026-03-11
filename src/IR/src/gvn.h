#pragma once

#include "cfg_graph.h"
#include "program.h"

namespace IR {

class GlobalValueNumbering {
 public:
  bool run(Program&, const CFGGraphs&);
};

}  // namespace IR
