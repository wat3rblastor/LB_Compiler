#pragma once

#include "program.h"

namespace IR {

class AlgebraicSimplify {
 public:
  bool run(Program&);
};

}  // namespace IR
