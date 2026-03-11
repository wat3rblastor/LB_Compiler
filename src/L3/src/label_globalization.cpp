#include "label_globalization.h"

#include <unordered_map>

namespace L3 {

void globalize_labels(Program& program) {
  std::unordered_map<std::string, std::string> L3_to_L2;
  int globalCounter = 0;

  std::vector<Function>& functions = program.getFunctions();
  for (Function& function : functions) {
    std::vector<Instruction>& instructions = function.getInstructions();
    for (Instruction& instruction : instructions) {
      std::visit(
          [&L3_to_L2, &globalCounter](auto&& instruction) {
            using T = std::decay_t<decltype(instruction)>;
            if constexpr (std::is_same_v<T, AssignInstruction> ||
                          std::is_same_v<T, StoreInstruction> ||
                          std::is_same_v<T, LabelInstruction> ||
                          std::is_same_v<T, BranchInstruction> ||
                          std::is_same_v<T, ConditionalBranchInstruction>) {
              instruction.modify_label(L3_to_L2, globalCounter);
            }
          },
          instruction);
    }
    L3_to_L2.clear();
  }
}

}  // namespace L3