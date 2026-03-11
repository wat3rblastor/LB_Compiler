#pragma once

#include "program.h"

namespace IR {

class LoopInvariantCodeMotion {
 public:
  bool run(Program&);
};

}  // namespace IR
