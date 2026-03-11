#include "cfg_graph.h"

#include <algorithm>
#include <string>
#include <unordered_map>

#include "node_utils.h"

namespace IR {

CFGGraphs build_cfg_graphs(const Program& program) {
  CFGGraphs graphs;

  for (const auto& function : program.getFunctions()) {
    const auto& blocks = function->get_basicBlocks();
    CFGGraph graph;
    graph.predecessors.assign(blocks.size(), {});
    graph.successors.assign(blocks.size(), {});

    std::unordered_map<std::string, int> label_to_index;
    label_to_index.reserve(blocks.size());
    for (size_t i = 0; i < blocks.size(); ++i) {
      label_to_index[blocks[i]->get_label().name] = static_cast<int>(i);
    }

    for (size_t i = 0; i < blocks.size(); ++i) {
      const Instruction& te = blocks[i]->get_te();
      std::vector<int> succ;

      if (const BranchInstruction* branch = as_branch_or_null(te); branch != nullptr) {
        const Label* target = as_label_or_null(branch->get_target());
        if (target != nullptr) {
          auto it = label_to_index.find(target->name);
          if (it != label_to_index.end()) {
            succ.push_back(it->second);
          }
        }
      } else if (const ConditionalBranchInstruction* cond =
                     as_conditional_branch_or_null(te);
                 cond != nullptr) {
        const Label* true_target = as_label_or_null(cond->get_true_target());
        const Label* false_target = as_label_or_null(cond->get_false_target());
        if (true_target != nullptr) {
          auto it = label_to_index.find(true_target->name);
          if (it != label_to_index.end()) {
            succ.push_back(it->second);
          }
        }
        if (false_target != nullptr) {
          auto it = label_to_index.find(false_target->name);
          if (it != label_to_index.end()) {
            succ.push_back(it->second);
          }
        }
      }

      std::sort(succ.begin(), succ.end());
      succ.erase(std::unique(succ.begin(), succ.end()), succ.end());
      graph.successors[i] = std::move(succ);
    }

    for (size_t i = 0; i < graph.successors.size(); ++i) {
      for (int succ : graph.successors[i]) {
        graph.predecessors[succ].push_back(static_cast<int>(i));
      }
    }

    graphs[function.get()] = std::move(graph);
  }

  return graphs;
}

}  // namespace IR
