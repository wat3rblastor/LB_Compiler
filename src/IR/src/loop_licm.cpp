#include "loop_licm.h"

#include <algorithm>
#include <iterator>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "cfg_graph.h"
#include "node_utils.h"

namespace IR {
namespace {

using VarSet = std::unordered_set<std::string>;
using BlockSet = std::unordered_set<int>;

class VisitorBase : public ConstVisitor {
 public:
  void visit(const Var&) override {}
  void visit(const Label&) override {}
  void visit(const Type&) override {}
  void visit(const Number&) override {}
  void visit(const Callee&) override {}
  void visit(const Operator&) override {}
  void visit(const Index&) override {}
  void visit(const ArrayLength&) override {}
  void visit(const TupleLength&) override {}
  void visit(const FunctionCall&) override {}
  void visit(const NewArray&) override {}
  void visit(const NewTuple&) override {}
  void visit(const Parameter&) override {}

  void visit(const VarDefInstruction&) override {}
  void visit(const VarNumLabelAssignInstruction&) override {}
  void visit(const OperatorAssignInstruction&) override {}
  void visit(const IndexAssignInstruction&) override {}
  void visit(const LengthAssignInstruction&) override {}
  void visit(const CallInstruction&) override {}
  void visit(const CallAssignInstruction&) override {}
  void visit(const NewArrayAssignInstruction&) override {}
  void visit(const NewTupleAssignInstruction&) override {}
  void visit(const BranchInstruction&) override {}
  void visit(const ConditionalBranchInstruction&) override {}
  void visit(const ReturnInstruction&) override {}
  void visit(const ReturnValueInstruction&) override {}

  void visit(const BasicBlock&) override {}
  void visit(const Function&) override {}
  void visit(const Program&) override {}
};

const Number* as_number_or_null(const ASTNode& node) {
  class Matcher : public VisitorBase {
   public:
    const Number* out = nullptr;
    void visit(const Number& value) override { out = &value; }
  } matcher;
  node.accept(matcher);
  return matcher.out;
}

const Operator* as_operator_or_null(const ASTNode& node) {
  class Matcher : public VisitorBase {
   public:
    const Operator* out = nullptr;
    void visit(const Operator& value) override { out = &value; }
  } matcher;
  node.accept(matcher);
  return matcher.out;
}

const VarDefInstruction* as_var_def_or_null(const Instruction& inst) {
  class Matcher : public VisitorBase {
   public:
    const VarDefInstruction* out = nullptr;
    void visit(const VarDefInstruction& value) override { out = &value; }
  } matcher;
  inst.accept(matcher);
  return matcher.out;
}

const VarNumLabelAssignInstruction* as_var_num_label_assign_or_null(
    const Instruction& inst) {
  class Matcher : public VisitorBase {
   public:
    const VarNumLabelAssignInstruction* out = nullptr;
    void visit(const VarNumLabelAssignInstruction& value) override { out = &value; }
  } matcher;
  inst.accept(matcher);
  return matcher.out;
}

const OperatorAssignInstruction* as_operator_assign_or_null(const Instruction& inst) {
  class Matcher : public VisitorBase {
   public:
    const OperatorAssignInstruction* out = nullptr;
    void visit(const OperatorAssignInstruction& value) override { out = &value; }
  } matcher;
  inst.accept(matcher);
  return matcher.out;
}

const IndexAssignInstruction* as_index_assign_or_null(const Instruction& inst) {
  class Matcher : public VisitorBase {
   public:
    const IndexAssignInstruction* out = nullptr;
    void visit(const IndexAssignInstruction& value) override { out = &value; }
  } matcher;
  inst.accept(matcher);
  return matcher.out;
}

const LengthAssignInstruction* as_length_assign_or_null(const Instruction& inst) {
  class Matcher : public VisitorBase {
   public:
    const LengthAssignInstruction* out = nullptr;
    void visit(const LengthAssignInstruction& value) override { out = &value; }
  } matcher;
  inst.accept(matcher);
  return matcher.out;
}

const CallAssignInstruction* as_call_assign_or_null(const Instruction& inst) {
  class Matcher : public VisitorBase {
   public:
    const CallAssignInstruction* out = nullptr;
    void visit(const CallAssignInstruction& value) override { out = &value; }
  } matcher;
  inst.accept(matcher);
  return matcher.out;
}

const NewArrayAssignInstruction* as_new_array_assign_or_null(const Instruction& inst) {
  class Matcher : public VisitorBase {
   public:
    const NewArrayAssignInstruction* out = nullptr;
    void visit(const NewArrayAssignInstruction& value) override { out = &value; }
  } matcher;
  inst.accept(matcher);
  return matcher.out;
}

const NewTupleAssignInstruction* as_new_tuple_assign_or_null(const Instruction& inst) {
  class Matcher : public VisitorBase {
   public:
    const NewTupleAssignInstruction* out = nullptr;
    void visit(const NewTupleAssignInstruction& value) override { out = &value; }
  } matcher;
  inst.accept(matcher);
  return matcher.out;
}

VarSet collect_vars_from_node(const ASTNode& node) {
  class Collector : public VisitorBase {
   public:
    VarSet vars;

    void collect(const ASTNode& n) { n.accept(*this); }

    void visit(const Var& node) override { vars.insert(node.name); }

    void visit(const Callee& node) override {
      if (const auto* target = std::get_if<std::unique_ptr<ASTNode>>(&node.calleeValue)) {
        collect(**target);
      }
    }

    void visit(const Operator& node) override {
      collect(*node.left);
      collect(*node.right);
    }

    void visit(const Index& node) override {
      collect(*node.var);
      for (const auto& idx : node.indices) {
        collect(*idx);
      }
    }

    void visit(const ArrayLength& node) override {
      collect(*node.var);
      collect(*node.index);
    }

    void visit(const TupleLength& node) override { collect(*node.var); }

    void visit(const FunctionCall& node) override {
      collect(*node.callee);
      for (const auto& arg : node.args) {
        collect(*arg);
      }
    }

    void visit(const NewArray& node) override {
      for (const auto& arg : node.args) {
        collect(*arg);
      }
    }

    void visit(const NewTuple& node) override { collect(*node.length); }
  } collector;

  collector.collect(node);
  return collector.vars;
}

bool is_pure_loop_invariant_expr(const ASTNode& node) {
  if (as_var_or_null(node) != nullptr || as_number_or_null(node) != nullptr ||
      as_label_or_null(node) != nullptr) {
    return true;
  }

  const Operator* op = as_operator_or_null(node);
  if (op == nullptr) {
    return false;
  }

  return is_pure_loop_invariant_expr(*op->left) &&
         is_pure_loop_invariant_expr(*op->right);
}

std::optional<std::string> get_instruction_def(const Instruction& inst) {
  if (const VarDefInstruction* i = as_var_def_or_null(inst); i != nullptr) {
    if (const Var* var = as_var_or_null(i->get_var()); var != nullptr) {
      return var->name;
    }
    return std::nullopt;
  }

  if (const VarNumLabelAssignInstruction* i = as_var_num_label_assign_or_null(inst);
      i != nullptr) {
    if (const Var* dst = as_var_or_null(i->get_dst()); dst != nullptr) {
      return dst->name;
    }
    return std::nullopt;
  }

  if (const OperatorAssignInstruction* i = as_operator_assign_or_null(inst);
      i != nullptr) {
    if (const Var* dst = as_var_or_null(i->get_dst()); dst != nullptr) {
      return dst->name;
    }
    return std::nullopt;
  }

  if (const LengthAssignInstruction* i = as_length_assign_or_null(inst); i != nullptr) {
    if (const Var* dst = as_var_or_null(i->get_dst()); dst != nullptr) {
      return dst->name;
    }
    return std::nullopt;
  }

  if (const CallAssignInstruction* i = as_call_assign_or_null(inst); i != nullptr) {
    if (const Var* dst = as_var_or_null(i->get_dst()); dst != nullptr) {
      return dst->name;
    }
    return std::nullopt;
  }

  if (const NewArrayAssignInstruction* i = as_new_array_assign_or_null(inst);
      i != nullptr) {
    if (const Var* dst = as_var_or_null(i->get_dst()); dst != nullptr) {
      return dst->name;
    }
    return std::nullopt;
  }

  if (const NewTupleAssignInstruction* i = as_new_tuple_assign_or_null(inst);
      i != nullptr) {
    if (const Var* dst = as_var_or_null(i->get_dst()); dst != nullptr) {
      return dst->name;
    }
    return std::nullopt;
  }

  if (const IndexAssignInstruction* i = as_index_assign_or_null(inst); i != nullptr) {
    if (const Var* dst = as_var_or_null(i->get_dst()); dst != nullptr) {
      return dst->name;
    }
    return std::nullopt;
  }

  return std::nullopt;
}

bool extract_hoistable_info(const Instruction& inst, std::string* dst_name,
                            VarSet* uses) {
  if (const VarNumLabelAssignInstruction* i = as_var_num_label_assign_or_null(inst);
      i != nullptr) {
    const Var* dst = as_var_or_null(i->get_dst());
    if (dst == nullptr || !is_pure_loop_invariant_expr(i->get_src())) {
      return false;
    }
    *dst_name = dst->name;
    *uses = collect_vars_from_node(i->get_src());
    return true;
  }

  if (const OperatorAssignInstruction* i = as_operator_assign_or_null(inst);
      i != nullptr) {
    const Var* dst = as_var_or_null(i->get_dst());
    if (dst == nullptr || !is_pure_loop_invariant_expr(i->get_src())) {
      return false;
    }
    *dst_name = dst->name;
    *uses = collect_vars_from_node(i->get_src());
    return true;
  }

  return false;
}

std::vector<BlockSet> compute_dominators(const CFGGraph& graph) {
  const size_t block_count = graph.successors.size();
  std::vector<BlockSet> dominators(block_count);

  BlockSet all_nodes;
  all_nodes.reserve(block_count);
  for (size_t i = 0; i < block_count; ++i) {
    all_nodes.insert(static_cast<int>(i));
  }

  if (block_count == 0) {
    return dominators;
  }

  dominators[0].insert(0);
  for (size_t i = 1; i < block_count; ++i) {
    dominators[i] = all_nodes;
  }

  bool changed = true;
  while (changed) {
    changed = false;

    for (size_t b = 1; b < block_count; ++b) {
      BlockSet new_dom;
      if (graph.predecessors[b].empty()) {
        new_dom.insert(static_cast<int>(b));
      } else {
        new_dom = dominators[graph.predecessors[b][0]];
        for (size_t i = 1; i < graph.predecessors[b].size(); ++i) {
          BlockSet intersection;
          const BlockSet& pred_dom = dominators[graph.predecessors[b][i]];
          for (const auto& value : new_dom) {
            if (pred_dom.find(value) != pred_dom.end()) {
              intersection.insert(value);
            }
          }
          new_dom = std::move(intersection);
        }
        new_dom.insert(static_cast<int>(b));
      }

      if (new_dom != dominators[b]) {
        dominators[b] = std::move(new_dom);
        changed = true;
      }
    }
  }

  return dominators;
}

struct BackEdge {
  int tail;
  int head;
};

std::vector<BackEdge> find_backedges(const CFGGraph& graph,
                                     const std::vector<BlockSet>& dominators) {
  std::vector<BackEdge> backedges;
  for (size_t tail = 0; tail < graph.successors.size(); ++tail) {
    const auto& succs = graph.successors[tail];
    for (int head : succs) {
      if (dominators[tail].find(head) != dominators[tail].end()) {
        backedges.push_back(BackEdge{static_cast<int>(tail), head});
      }
    }
  }
  return backedges;
}

std::unordered_set<int> build_natural_loop(const CFGGraph& graph, int tail, int head) {
  std::unordered_set<int> nodes;
  nodes.insert(head);
  nodes.insert(tail);

  std::vector<int> worklist;
  worklist.push_back(tail);

  while (!worklist.empty()) {
    const int current = worklist.back();
    worklist.pop_back();

    for (int pred : graph.predecessors[current]) {
      if (nodes.insert(pred).second && pred != head) {
        worklist.push_back(pred);
      }
    }
  }

  return nodes;
}

std::optional<int> find_simple_preheader(
    const CFGGraph& graph, const std::vector<std::unique_ptr<BasicBlock>>& blocks,
    int header, const std::unordered_set<int>& loop_nodes) {
  std::vector<int> outside_preds;
  for (int pred : graph.predecessors[header]) {
    if (loop_nodes.find(pred) == loop_nodes.end()) {
      outside_preds.push_back(pred);
    }
  }

  if (outside_preds.size() != 1) {
    return std::nullopt;
  }

  const int preheader = outside_preds[0];
  if (graph.successors[preheader].size() != 1 ||
      graph.successors[preheader][0] != header) {
    return std::nullopt;
  }

  const BranchInstruction* br = as_branch_or_null(blocks[preheader]->get_te());
  if (br == nullptr) {
    return std::nullopt;
  }

  const Label* target = as_label_or_null(br->get_target());
  if (target == nullptr || target->name != blocks[header]->get_label().name) {
    return std::nullopt;
  }

  return preheader;
}

void collect_loop_defs(const std::vector<std::unique_ptr<BasicBlock>>& blocks,
                       const std::unordered_set<int>& loop_nodes, VarSet* defs,
                       std::unordered_map<std::string, int>* def_count) {
  for (int block_index : loop_nodes) {
    const BasicBlock& block = *blocks[block_index];
    for (const auto& inst : block.get_instructions()) {
      auto def = get_instruction_def(*inst);
      if (!def.has_value()) {
        continue;
      }
      defs->insert(*def);
      ++(*def_count)[*def];
    }
    auto te_def = get_instruction_def(block.get_te());
    if (te_def.has_value()) {
      defs->insert(*te_def);
      ++(*def_count)[*te_def];
    }
  }
}

}  // namespace

bool LoopInvariantCodeMotion::run(Program& program) {
  bool changed = false;
  CFGGraphs cfg_graphs = build_cfg_graphs(program);

  for (auto& function : program.getFunctions()) {
    auto graph_it = cfg_graphs.find(function.get());
    if (graph_it == cfg_graphs.end()) {
      continue;
    }

    const CFGGraph& graph = graph_it->second;
    auto& blocks = function->get_basicBlocks();
    const auto dominators = compute_dominators(graph);
    const auto backedges = find_backedges(graph, dominators);

    for (const BackEdge& edge : backedges) {
      const std::unordered_set<int> loop_nodes =
          build_natural_loop(graph, edge.tail, edge.head);
      const std::optional<int> preheader_index =
          find_simple_preheader(graph, blocks, edge.head, loop_nodes);
      if (!preheader_index.has_value()) {
        continue;
      }

      VarSet loop_defs;
      std::unordered_map<std::string, int> def_count;
      collect_loop_defs(blocks, loop_nodes, &loop_defs, &def_count);

      auto& header_insts = blocks[edge.head]->get_instructions_mut();
      std::vector<size_t> move_indices;
      move_indices.reserve(header_insts.size());

      VarSet blocked_defs = loop_defs;
      for (size_t i = 0; i < header_insts.size(); ++i) {
        std::string dst_name;
        VarSet uses;
        if (!extract_hoistable_info(*header_insts[i], &dst_name, &uses)) {
          continue;
        }

        const auto def_count_it = def_count.find(dst_name);
        if (def_count_it == def_count.end() || def_count_it->second != 1) {
          continue;
        }

        bool invariant = true;
        for (const auto& use_var : uses) {
          if (blocked_defs.find(use_var) != blocked_defs.end()) {
            invariant = false;
            break;
          }
        }
        if (!invariant) {
          continue;
        }

        move_indices.push_back(i);
        blocked_defs.erase(dst_name);
      }

      if (move_indices.empty()) {
        continue;
      }

      std::vector<bool> move_mask(header_insts.size(), false);
      for (size_t index : move_indices) {
        move_mask[index] = true;
      }

      std::vector<std::unique_ptr<Instruction>> moved_insts;
      std::vector<std::unique_ptr<Instruction>> kept_insts;
      moved_insts.reserve(move_indices.size());
      kept_insts.reserve(header_insts.size() - move_indices.size());
      for (size_t i = 0; i < header_insts.size(); ++i) {
        if (move_mask[i]) {
          moved_insts.push_back(std::move(header_insts[i]));
        } else {
          kept_insts.push_back(std::move(header_insts[i]));
        }
      }

      header_insts = std::move(kept_insts);
      auto& preheader_insts = blocks[*preheader_index]->get_instructions_mut();
      preheader_insts.insert(preheader_insts.end(),
                             std::make_move_iterator(moved_insts.begin()),
                             std::make_move_iterator(moved_insts.end()));
      changed = true;
    }
  }

  return changed;
}

}  // namespace IR
