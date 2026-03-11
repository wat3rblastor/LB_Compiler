#include "linearize.h"

#include <unordered_map>
#include <vector>

#include "node_utils.h"

namespace IR {

void linearize_cfg(Program& program) {
  for (auto& function : program.getFunctions()) {
    auto& blocks = function->get_basicBlocks();
    if (blocks.empty()) {
      continue;
    }

    std::unordered_map<std::string, int> label_to_index;
    label_to_index.reserve(blocks.size());
    for (int i = 0; i < static_cast<int>(blocks.size()); ++i) {
      label_to_index[blocks[i]->get_label().name] = i;
    }

    std::vector<bool> marked(blocks.size(), false);
    std::vector<int> order;
    order.reserve(blocks.size());

    auto choose_next = [&](int bb_index) -> int {
      const Instruction& te = blocks[bb_index]->get_te();

      if (const BranchInstruction* branch = as_branch_or_null(te);
          branch != nullptr) {
        const Label* target = as_label_or_null(branch->get_target());
        if (target != nullptr) {
          auto it = label_to_index.find(target->name);
          if (it != label_to_index.end() && !marked[it->second]) {
            return it->second;
          }
        }
        return -1;
      }

      if (const ConditionalBranchInstruction* cond = as_conditional_branch_or_null(te);
          cond != nullptr) {
        const Label* false_target = as_label_or_null(cond->get_false_target());
        if (false_target != nullptr) {
          auto it = label_to_index.find(false_target->name);
          if (it != label_to_index.end() && !marked[it->second]) {
            return it->second;
          }
        }

        const Label* true_target = as_label_or_null(cond->get_true_target());
        if (true_target != nullptr) {
          auto it = label_to_index.find(true_target->name);
          if (it != label_to_index.end() && !marked[it->second]) {
            return it->second;
          }
        }
      }

      return -1;
    };

    while (order.size() < blocks.size()) {
      int start = -1;
      for (int i = 0; i < static_cast<int>(blocks.size()); ++i) {
        if (!marked[i]) {
          start = i;
          break;
        }
      }

      int current = start;
      while (current != -1 && !marked[current]) {
        marked[current] = true;
        order.push_back(current);
        current = choose_next(current);
      }
    }

    std::vector<std::unique_ptr<BasicBlock>> old_blocks;
    old_blocks.swap(blocks);
    blocks.reserve(old_blocks.size());
    for (int i : order) {
      blocks.push_back(std::move(old_blocks[i]));
    }
  }
}

}  // namespace IR
