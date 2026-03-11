#pragma once

#include "L3.h"
#include "instruction_selection.h"

namespace L3 {

class Program {
 public:
  std::string to_string() const;
  void addFunction(Function&&);
  std::vector<Function>& getFunctions();

  InstructionSelector selector;

 private:
  std::vector<Function> functions;
};

void select_instructions(Program&);
void generate_code(Program&);

}