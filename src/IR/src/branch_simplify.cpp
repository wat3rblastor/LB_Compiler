#include "branch_simplify.h"

#include <cstdint>
#include <deque>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "cfg_graph.h"
#include "node_utils.h"

namespace IR {
namespace {

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

std::unique_ptr<ASTNode> clone_ast(const ASTNode& node) {
  class Cloner : public VisitorBase {
   public:
    std::unique_ptr<ASTNode> out;

    std::unique_ptr<ASTNode> clone(const ASTNode& n) {
      n.accept(*this);
      return std::move(out);
    }

    void visit(const Var& node) override { out = std::make_unique<Var>(node.name); }
    void visit(const Label& node) override { out = std::make_unique<Label>(node.name); }
    void visit(const Type& node) override { out = std::make_unique<Type>(node.name); }
    void visit(const Number& node) override { out = std::make_unique<Number>(node.value); }

    void visit(const Callee& node) override {
      if (const auto* enum_value = std::get_if<EnumCallee>(&node.calleeValue)) {
        EnumCallee tmp = *enum_value;
        out = std::make_unique<Callee>(tmp);
        return;
      }
      const auto* target = std::get_if<std::unique_ptr<ASTNode>>(&node.calleeValue);
      out = std::make_unique<Callee>(clone(**target));
    }

    void visit(const Operator& node) override {
      out = std::make_unique<Operator>(clone(*node.left), clone(*node.right), node.op);
    }

    void visit(const Index& node) override {
      std::vector<std::unique_ptr<ASTNode>> indices;
      indices.reserve(node.indices.size());
      for (const auto& idx : node.indices) {
        indices.push_back(clone(*idx));
      }
      out = std::make_unique<Index>(clone(*node.var), std::move(indices));
    }

    void visit(const ArrayLength& node) override {
      out = std::make_unique<ArrayLength>(clone(*node.var), clone(*node.index));
    }

    void visit(const TupleLength& node) override {
      out = std::make_unique<TupleLength>(clone(*node.var));
    }

    void visit(const FunctionCall& node) override {
      std::vector<std::unique_ptr<ASTNode>> args;
      args.reserve(node.args.size());
      for (const auto& arg : node.args) {
        args.push_back(clone(*arg));
      }
      out = std::make_unique<FunctionCall>(clone(*node.callee), std::move(args));
    }

    void visit(const NewArray& node) override {
      std::vector<std::unique_ptr<ASTNode>> args;
      args.reserve(node.args.size());
      for (const auto& arg : node.args) {
        args.push_back(clone(*arg));
      }
      out = std::make_unique<NewArray>(std::move(args));
    }

    void visit(const NewTuple& node) override {
      out = std::make_unique<NewTuple>(clone(*node.length));
    }

    void visit(const Parameter& node) override {
      out = std::make_unique<Parameter>(clone(*node.type), clone(*node.var));
    }
  } cloner;

  return cloner.clone(node);
}

std::optional<int64_t> get_number_value(const ASTNode& node) {
  if (const Number* num = as_number_or_null(node); num != nullptr) {
    return num->value;
  }
  return std::nullopt;
}

bool fold_binary(OP op, int64_t left, int64_t right, int64_t* out) {
  switch (op) {
    case OP::PLUS:
      *out = left + right;
      return true;
    case OP::SUB:
      *out = left - right;
      return true;
    case OP::MUL:
      *out = left * right;
      return true;
    case OP::AND:
      *out = left & right;
      return true;
    case OP::LSHIFT:
      *out = left << right;
      return true;
    case OP::RSHIFT:
      *out = left >> right;
      return true;
    case OP::LT:
      *out = (left < right) ? 1 : 0;
      return true;
    case OP::LE:
      *out = (left <= right) ? 1 : 0;
      return true;
    case OP::EQ:
      *out = (left == right) ? 1 : 0;
      return true;
    case OP::GE:
      *out = (left >= right) ? 1 : 0;
      return true;
    case OP::GT:
      *out = (left > right) ? 1 : 0;
      return true;
  }
  return false;
}

std::optional<int64_t> evaluate_constant_expr(const ASTNode& node) {
  if (const Number* num = as_number_or_null(node); num != nullptr) {
    return num->value;
  }

  const Operator* op = as_operator_or_null(node);
  if (op == nullptr) {
    return std::nullopt;
  }

  auto left = evaluate_constant_expr(*op->left);
  auto right = evaluate_constant_expr(*op->right);
  if (!left.has_value() || !right.has_value()) {
    return std::nullopt;
  }

  int64_t folded = 0;
  if (!fold_binary(op->op, *left, *right, &folded)) {
    return std::nullopt;
  }
  return folded;
}

}  // namespace

bool BranchSimplify::run(Program& program) {
  bool changed = false;

  for (auto& function : program.getFunctions()) {
    bool local_changed = true;
    while (local_changed) {
      local_changed = false;

      auto& blocks = function->get_basicBlocks();
      for (size_t i = 0; i < blocks.size(); ++i) {
        const Instruction& te = blocks[i]->get_te();
        const ConditionalBranchInstruction* cond = as_conditional_branch_or_null(te);
        if (cond == nullptr) {
          continue;
        }

        auto folded = evaluate_constant_expr(cond->get_condition());
        if (!folded.has_value()) {
          continue;
        }

        if (*folded != 0) {
          blocks[i]->set_te(
              std::make_unique<BranchInstruction>(clone_ast(cond->get_true_target())));
        } else {
          blocks[i]->set_te(
              std::make_unique<BranchInstruction>(clone_ast(cond->get_false_target())));
        }

        local_changed = true;
        changed = true;
      }

      std::unordered_map<std::string, int> label_to_index;
      label_to_index.reserve(blocks.size());
      for (size_t i = 0; i < blocks.size(); ++i) {
        label_to_index[blocks[i]->get_label().name] = static_cast<int>(i);
      }

      auto resolve_thread_target = [&](const std::string& label_name) {
        std::string current = label_name;
        std::unordered_set<std::string> seen;
        while (seen.insert(current).second) {
          auto it = label_to_index.find(current);
          if (it == label_to_index.end()) {
            break;
          }

          const BasicBlock& target_block = *blocks[it->second];
          if (!target_block.get_instructions().empty()) {
            break;
          }

          const BranchInstruction* branch = as_branch_or_null(target_block.get_te());
          if (branch == nullptr) {
            break;
          }

          const Label* next = as_label_or_null(branch->get_target());
          if (next == nullptr) {
            break;
          }
          current = next->name;
        }
        return current;
      };

      for (size_t i = 0; i < blocks.size(); ++i) {
        const Instruction& te = blocks[i]->get_te();
        if (const BranchInstruction* branch = as_branch_or_null(te); branch != nullptr) {
          const Label* target = as_label_or_null(branch->get_target());
          if (target == nullptr) {
            continue;
          }

          const std::string threaded_target = resolve_thread_target(target->name);
          if (threaded_target == target->name) {
            continue;
          }

          blocks[i]->set_te(
              std::make_unique<BranchInstruction>(std::make_unique<Label>(threaded_target)));
          local_changed = true;
          changed = true;
          continue;
        }

        const ConditionalBranchInstruction* cond = as_conditional_branch_or_null(te);
        if (cond == nullptr) {
          continue;
        }

        const Label* true_target = as_label_or_null(cond->get_true_target());
        const Label* false_target = as_label_or_null(cond->get_false_target());
        if (true_target == nullptr || false_target == nullptr) {
          continue;
        }

        const std::string threaded_true = resolve_thread_target(true_target->name);
        const std::string threaded_false = resolve_thread_target(false_target->name);

        if (threaded_true == threaded_false) {
          blocks[i]->set_te(std::make_unique<BranchInstruction>(
              std::make_unique<Label>(threaded_true)));
          local_changed = true;
          changed = true;
          continue;
        }

        if (threaded_true == true_target->name &&
            threaded_false == false_target->name) {
          continue;
        }

        blocks[i]->set_te(std::make_unique<ConditionalBranchInstruction>(
            clone_ast(cond->get_condition()),
            std::make_unique<Label>(threaded_true),
            std::make_unique<Label>(threaded_false)));
        local_changed = true;
        changed = true;
      }

      CFGGraphs all_graphs = build_cfg_graphs(program);
      auto it = all_graphs.find(function.get());
      if (it == all_graphs.end()) {
        break;
      }

      const CFGGraph& graph = it->second;
      std::vector<bool> reachable(blocks.size(), false);
      std::deque<int> queue;
      if (!blocks.empty()) {
        reachable[0] = true;
        queue.push_back(0);
      }

      while (!queue.empty()) {
        int idx = queue.front();
        queue.pop_front();

        for (int succ : graph.successors[idx]) {
          if (!reachable[succ]) {
            reachable[succ] = true;
            queue.push_back(succ);
          }
        }
      }

      bool removed_any = false;
      for (bool flag : reachable) {
        if (!flag) {
          removed_any = true;
          break;
        }
      }

      if (removed_any) {
        std::vector<std::unique_ptr<BasicBlock>> old_blocks;
        old_blocks.swap(blocks);
        blocks.reserve(old_blocks.size());
        for (size_t idx = 0; idx < old_blocks.size(); ++idx) {
          if (reachable[idx]) {
            blocks.push_back(std::move(old_blocks[idx]));
          }
        }

        local_changed = true;
        changed = true;
      }
    }
  }

  return changed;
}

}  // namespace IR
