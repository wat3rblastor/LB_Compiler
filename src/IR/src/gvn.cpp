#include "gvn.h"

#include <algorithm>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "node_utils.h"

namespace IR {
namespace {

using VarSet = std::unordered_set<std::string>;
using BlockSet = std::unordered_set<int>;

struct ExpressionValue {
  std::string result_var;
  VarSet dependencies;
};

using ExpressionTable = std::unordered_map<std::string, ExpressionValue>;

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

const OperatorAssignInstruction* as_operator_assign_or_null(const Instruction& inst) {
  class Matcher : public VisitorBase {
   public:
    const OperatorAssignInstruction* out = nullptr;
    void visit(const OperatorAssignInstruction& value) override { out = &value; }
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

const CallInstruction* as_call_instruction_or_null(const Instruction& inst) {
  class Matcher : public VisitorBase {
   public:
    const CallInstruction* out = nullptr;
    void visit(const CallInstruction& value) override { out = &value; }
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

const IndexAssignInstruction* as_index_assign_or_null(const Instruction& inst) {
  class Matcher : public VisitorBase {
   public:
    const IndexAssignInstruction* out = nullptr;
    void visit(const IndexAssignInstruction& value) override { out = &value; }
  } matcher;
  inst.accept(matcher);
  return matcher.out;
}

bool is_commutative(OP op) {
  return op == OP::PLUS || op == OP::MUL || op == OP::AND || op == OP::EQ;
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

bool is_pure_cse_expr(const ASTNode& node) {
  if (as_var_or_null(node) != nullptr || as_number_or_null(node) != nullptr ||
      as_label_or_null(node) != nullptr) {
    return true;
  }

  if (const Operator* op = as_operator_or_null(node); op != nullptr) {
    return is_pure_cse_expr(*op->left) && is_pure_cse_expr(*op->right);
  }

  if (const ArrayLength* len = as_array_length_or_null(node); len != nullptr) {
    return is_pure_cse_expr(*len->var) && is_pure_cse_expr(*len->index);
  }

  if (const TupleLength* len = as_tuple_length_or_null(node); len != nullptr) {
    return is_pure_cse_expr(*len->var);
  }

  return false;
}

std::string expression_key(const ASTNode& node) {
  if (const Var* var = as_var_or_null(node); var != nullptr) {
    return "v:" + var->name;
  }
  if (const Number* num = as_number_or_null(node); num != nullptr) {
    return "n:" + std::to_string(num->value);
  }
  if (const Label* label = as_label_or_null(node); label != nullptr) {
    return "l:" + label->name;
  }
  if (const Operator* op = as_operator_or_null(node); op != nullptr) {
    std::string left = expression_key(*op->left);
    std::string right = expression_key(*op->right);
    if (is_commutative(op->op) && right < left) {
      std::swap(left, right);
    }
    return "op(" + std::to_string(static_cast<int>(op->op)) + "," + left + "," +
           right + ")";
  }
  if (const ArrayLength* len = as_array_length_or_null(node); len != nullptr) {
    return "alen(" + expression_key(*len->var) + "," + expression_key(*len->index) +
           ")";
  }
  if (const TupleLength* len = as_tuple_length_or_null(node); len != nullptr) {
    return "tlen(" + expression_key(*len->var) + ")";
  }
  return "unknown";
}

std::optional<std::string> get_defined_var(const Instruction& inst) {
  if (const VarDefInstruction* i = as_var_def_or_null(inst); i != nullptr) {
    if (const Var* var = as_var_or_null(i->get_var()); var != nullptr) {
      return var->name;
    }
    return std::nullopt;
  }

  if (const OperatorAssignInstruction* i = as_operator_assign_or_null(inst);
      i != nullptr) {
    if (const Var* var = as_var_or_null(i->get_dst()); var != nullptr) {
      return var->name;
    }
    return std::nullopt;
  }

  if (const LengthAssignInstruction* i = as_length_assign_or_null(inst); i != nullptr) {
    if (const Var* var = as_var_or_null(i->get_dst()); var != nullptr) {
      return var->name;
    }
    return std::nullopt;
  }

  if (const CallAssignInstruction* i = as_call_assign_or_null(inst); i != nullptr) {
    if (const Var* var = as_var_or_null(i->get_dst()); var != nullptr) {
      return var->name;
    }
    return std::nullopt;
  }

  if (const NewArrayAssignInstruction* i = as_new_array_assign_or_null(inst);
      i != nullptr) {
    if (const Var* var = as_var_or_null(i->get_dst()); var != nullptr) {
      return var->name;
    }
    return std::nullopt;
  }

  if (const NewTupleAssignInstruction* i = as_new_tuple_assign_or_null(inst);
      i != nullptr) {
    if (const Var* var = as_var_or_null(i->get_dst()); var != nullptr) {
      return var->name;
    }
    return std::nullopt;
  }

  if (const IndexAssignInstruction* i = as_index_assign_or_null(inst); i != nullptr) {
    if (const Var* var = as_var_or_null(i->get_dst()); var != nullptr) {
      return var->name;
    }
    return std::nullopt;
  }

  return std::nullopt;
}

void invalidate_for_var(ExpressionTable* expressions, const std::string& var_name) {
  for (auto it = expressions->begin(); it != expressions->end();) {
    if (it->second.result_var == var_name ||
        it->second.dependencies.find(var_name) != it->second.dependencies.end()) {
      it = expressions->erase(it);
    } else {
      ++it;
    }
  }
}

bool is_call_like_instruction(const Instruction& inst) {
  return as_call_instruction_or_null(inst) != nullptr ||
         as_call_assign_or_null(inst) != nullptr ||
         as_new_array_assign_or_null(inst) != nullptr ||
         as_new_tuple_assign_or_null(inst) != nullptr;
}

bool try_replace_with_cse(std::unique_ptr<Instruction>* inst,
                          const std::string& dst_name, const ASTNode& expr,
                          ExpressionTable* expressions, bool* changed) {
  // `dst_name` can alias storage owned by `*inst`. If we rewrite `*inst`,
  // that storage is invalidated; take a local copy first.
  const std::string dst_name_copy = dst_name;
  if (!is_pure_cse_expr(expr)) {
    return false;
  }

  const std::string key = expression_key(expr);
  const VarSet deps = collect_vars_from_node(expr);
  invalidate_for_var(expressions, dst_name_copy);

  auto it = expressions->find(key);
  if (it != expressions->end() && it->second.result_var != dst_name_copy) {
    *inst = std::make_unique<VarNumLabelAssignInstruction>(
        std::make_unique<Var>(dst_name_copy), std::make_unique<Var>(it->second.result_var));
    *changed = true;
  }

  ExpressionValue value;
  value.result_var = dst_name_copy;
  value.dependencies = deps;
  (*expressions)[key] = std::move(value);
  return true;
}

void process_instruction(std::unique_ptr<Instruction>* inst,
                         ExpressionTable* expressions, bool* changed) {
  if (const OperatorAssignInstruction* i = as_operator_assign_or_null(**inst);
      i != nullptr) {
    if (const Var* dst = as_var_or_null(i->get_dst()); dst != nullptr) {
      if (try_replace_with_cse(inst, dst->name, i->get_src(), expressions, changed)) {
        return;
      }
      invalidate_for_var(expressions, dst->name);
      return;
    }
  }

  if (const LengthAssignInstruction* i = as_length_assign_or_null(**inst);
      i != nullptr) {
    if (const Var* dst = as_var_or_null(i->get_dst()); dst != nullptr) {
      if (try_replace_with_cse(inst, dst->name, i->get_src(), expressions, changed)) {
        return;
      }
      invalidate_for_var(expressions, dst->name);
      return;
    }
  }

  if (is_call_like_instruction(**inst)) {
    expressions->clear();
    return;
  }

  auto def = get_defined_var(**inst);
  if (def.has_value()) {
    invalidate_for_var(expressions, *def);
  }
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
          for (int value : new_dom) {
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

std::vector<int> compute_immediate_dominator(const std::vector<BlockSet>& dominators) {
  const size_t block_count = dominators.size();
  std::vector<int> idom(block_count, -1);
  if (block_count == 0) {
    return idom;
  }

  for (size_t b = 1; b < block_count; ++b) {
    int best = -1;
    for (int candidate : dominators[b]) {
      if (candidate == static_cast<int>(b)) {
        continue;
      }
      if (best == -1 ||
          dominators[candidate].size() > dominators[best].size()) {
        best = candidate;
      }
    }
    idom[b] = best;
  }

  return idom;
}

std::vector<std::vector<int>> build_dominator_tree(
    const std::vector<int>& idom) {
  std::vector<std::vector<int>> tree(idom.size());
  for (size_t b = 1; b < idom.size(); ++b) {
    if (idom[b] >= 0) {
      tree[idom[b]].push_back(static_cast<int>(b));
    }
  }
  return tree;
}

void run_cse_over_dom_tree(Function& function, const CFGGraph& graph,
                           const std::vector<std::vector<int>>& dom_tree,
                           int block_index, ExpressionTable expressions,
                           bool* changed) {
  BasicBlock& block = *function.get_basicBlocks()[block_index];
  for (auto& inst : block.get_instructions_mut()) {
    process_instruction(&inst, &expressions, changed);
  }

  for (int child : dom_tree[block_index]) {
    ExpressionTable child_exprs;
    if (graph.predecessors[child].size() == 1 &&
        graph.predecessors[child][0] == block_index) {
      child_exprs = expressions;
    }
    run_cse_over_dom_tree(function, graph, dom_tree, child, std::move(child_exprs),
                          changed);
  }
}

}  // namespace

bool GlobalValueNumbering::run(Program& program, const CFGGraphs& cfg_graphs) {
  bool changed = false;

  for (auto& function : program.getFunctions()) {
    auto graph_it = cfg_graphs.find(function.get());
    if (graph_it == cfg_graphs.end()) {
      continue;
    }

    if (function->get_basicBlocks().empty()) {
      continue;
    }

    const CFGGraph& graph = graph_it->second;
    const auto dominators = compute_dominators(graph);
    const auto idom = compute_immediate_dominator(dominators);
    const auto dom_tree = build_dominator_tree(idom);

    ExpressionTable expressions;
    run_cse_over_dom_tree(*function, graph, dom_tree, 0, std::move(expressions),
                          &changed);
  }

  return changed;
}

}  // namespace IR
