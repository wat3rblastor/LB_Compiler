#pragma once

#include "program.h"

namespace IR {

class BranchSimplify {
 public:
  bool run(Program&);
};

}  // namespace IR
