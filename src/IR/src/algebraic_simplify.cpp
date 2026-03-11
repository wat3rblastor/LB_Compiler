#include "algebraic_simplify.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <utility>

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

const OperatorAssignInstruction* as_operator_assign_or_null(const Instruction& inst) {
  class Matcher : public VisitorBase {
   public:
    const OperatorAssignInstruction* out = nullptr;
    void visit(const OperatorAssignInstruction& value) override { out = &value; }
  } matcher;
  inst.accept(matcher);
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
    void visit(const Number& node) override { out = std::make_unique<Number>(node.value); }

    void visit(const Label& node) override { out = std::make_unique<Label>(node.name); }
    void visit(const Type& node) override { out = std::make_unique<Type>(node.name); }

    void visit(const Operator& node) override {
      auto left = clone(*node.left);
      auto right = clone(*node.right);
      out = std::make_unique<Operator>(std::move(left), std::move(right), node.op);
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

    void visit(const Callee& node) override {
      if (const auto* enum_value = std::get_if<EnumCallee>(&node.calleeValue)) {
        EnumCallee tmp = *enum_value;
        out = std::make_unique<Callee>(tmp);
        return;
      }
      const auto* target = std::get_if<std::unique_ptr<ASTNode>>(&node.calleeValue);
      out = std::make_unique<Callee>(clone(**target));
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
  class Matcher : public VisitorBase {
   public:
    std::optional<int64_t> out;
    void visit(const Number& value) override { out = value.value; }
  } matcher;
  node.accept(matcher);
  return matcher.out;
}

bool is_zero(const ASTNode& node) {
  auto value = get_number_value(node);
  return value.has_value() && *value == 0;
}

bool is_one(const ASTNode& node) {
  auto value = get_number_value(node);
  return value.has_value() && *value == 1;
}

bool is_minus_one(const ASTNode& node) {
  auto value = get_number_value(node);
  return value.has_value() && *value == -1;
}

bool is_commutative(OP op) {
  return op == OP::PLUS || op == OP::MUL || op == OP::AND || op == OP::EQ;
}

bool ast_equal(const ASTNode& lhs, const ASTNode& rhs) {
  if (const Var* lvar = as_var_or_null(lhs); lvar != nullptr) {
    const Var* rvar = as_var_or_null(rhs);
    return rvar != nullptr && lvar->name == rvar->name;
  }
  if (const Number* lnum = [&]() {
        class Matcher : public VisitorBase {
         public:
          const Number* out = nullptr;
          void visit(const Number& value) override { out = &value; }
        } matcher;
        lhs.accept(matcher);
        return matcher.out;
      }();
      lnum != nullptr) {
    const Number* rnum = [&]() {
      class Matcher : public VisitorBase {
       public:
        const Number* out = nullptr;
        void visit(const Number& value) override { out = &value; }
      } matcher;
      rhs.accept(matcher);
      return matcher.out;
    }();
    return rnum != nullptr && lnum->value == rnum->value;
  }

  const Operator* lop = as_operator_or_null(lhs);
  const Operator* rop = as_operator_or_null(rhs);
  if (lop == nullptr || rop == nullptr) {
    return false;
  }
  return lop->op == rop->op && ast_equal(*lop->left, *rop->left) &&
         ast_equal(*lop->right, *rop->right);
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

struct SimplifiedExpr {
  std::unique_ptr<ASTNode> expr;
  bool changed = false;
  bool operator_expr = false;
};

SimplifiedExpr simplify_node(const ASTNode& node);

std::unique_ptr<ASTNode> clone_or_number(const ASTNode& node) {
  if (const auto val = get_number_value(node); val.has_value()) {
    return std::make_unique<Number>(*val);
  }
  return clone_ast(node);
}

SimplifiedExpr simplify_operator_expr(const Operator& op) {
  SimplifiedExpr left = simplify_node(*op.left);
  SimplifiedExpr right = simplify_node(*op.right);
  bool changed = left.changed || right.changed;

  std::unique_ptr<ASTNode> l = std::move(left.expr);
  std::unique_ptr<ASTNode> r = std::move(right.expr);

  if (is_commutative(op.op) && get_number_value(*l).has_value() &&
      !get_number_value(*r).has_value()) {
    std::swap(l, r);
    changed = true;
  }

  const auto lv = get_number_value(*l);
  const auto rv = get_number_value(*r);
  if (lv.has_value() && rv.has_value()) {
    int64_t folded = 0;
    if (fold_binary(op.op, *lv, *rv, &folded)) {
      return {std::make_unique<Number>(folded), true, false};
    }
  }

  if (ast_equal(*l, *r)) {
    switch (op.op) {
      case OP::SUB:
        return {std::make_unique<Number>(0), true, false};
      case OP::AND:
        return {std::move(l), true, false};
      case OP::EQ:
      case OP::LE:
      case OP::GE:
        return {std::make_unique<Number>(1), true, false};
      case OP::LT:
      case OP::GT:
        return {std::make_unique<Number>(0), true, false};
      default:
        break;
    }
  }

  switch (op.op) {
    case OP::PLUS:
      if (is_zero(*r)) {
        return {std::move(l), true, false};
      }
      if (is_zero(*l)) {
        return {std::move(r), true, false};
      }
      break;

    case OP::SUB:
      if (is_zero(*r)) {
        return {std::move(l), true, false};
      }
      break;

    case OP::MUL:
      if (is_zero(*l) || is_zero(*r)) {
        return {std::make_unique<Number>(0), true, false};
      }
      if (is_one(*r)) {
        return {std::move(l), true, false};
      }
      if (is_one(*l)) {
        return {std::move(r), true, false};
      }
      break;

    case OP::AND:
      if (is_zero(*l) || is_zero(*r)) {
        return {std::make_unique<Number>(0), true, false};
      }
      if (is_minus_one(*r)) {
        return {std::move(l), true, false};
      }
      if (is_minus_one(*l)) {
        return {std::move(r), true, false};
      }
      break;

    case OP::LSHIFT:
    case OP::RSHIFT:
      if (is_zero(*r)) {
        return {std::move(l), true, false};
      }
      break;

    default:
      break;
  }

  if (rv.has_value()) {
    if (const Operator* inner = as_operator_or_null(*l); inner != nullptr) {
      const auto c1 = get_number_value(*inner->right);
      if (c1.has_value()) {
        if (op.op == OP::PLUS && inner->op == OP::PLUS) {
          int64_t c = *c1 + *rv;
          auto rebuilt = std::make_unique<Operator>(clone_or_number(*inner->left),
                                                    std::make_unique<Number>(c), OP::PLUS);
          return simplify_operator_expr(*rebuilt);
        }
        if (op.op == OP::SUB && inner->op == OP::PLUS) {
          int64_t c = *c1 - *rv;
          auto rebuilt = std::make_unique<Operator>(clone_or_number(*inner->left),
                                                    std::make_unique<Number>(c), OP::PLUS);
          return simplify_operator_expr(*rebuilt);
        }
        if (op.op == OP::MUL && inner->op == OP::MUL) {
          int64_t c = *c1 * *rv;
          auto rebuilt = std::make_unique<Operator>(clone_or_number(*inner->left),
                                                    std::make_unique<Number>(c), OP::MUL);
          return simplify_operator_expr(*rebuilt);
        }
        if (op.op == OP::LSHIFT && inner->op == OP::LSHIFT) {
          int64_t c = *c1 + *rv;
          auto rebuilt = std::make_unique<Operator>(clone_or_number(*inner->left),
                                                    std::make_unique<Number>(c), OP::LSHIFT);
          return simplify_operator_expr(*rebuilt);
        }
        if (op.op == OP::RSHIFT && inner->op == OP::RSHIFT) {
          int64_t c = *c1 + *rv;
          auto rebuilt = std::make_unique<Operator>(clone_or_number(*inner->left),
                                                    std::make_unique<Number>(c), OP::RSHIFT);
          return simplify_operator_expr(*rebuilt);
        }
      }
    }
  }

  return {std::make_unique<Operator>(std::move(l), std::move(r), op.op), changed, true};
}

SimplifiedExpr simplify_node(const ASTNode& node) {
  if (const Operator* op = as_operator_or_null(node); op != nullptr) {
    return simplify_operator_expr(*op);
  }
  return {clone_ast(node), false, false};
}

}  // namespace

bool AlgebraicSimplify::run(Program& program) {
  bool changed = false;

  for (auto& function : program.getFunctions()) {
    for (auto& block : function->get_basicBlocks()) {
      auto& instructions = block->get_instructions_mut();
      for (auto& inst : instructions) {
        const OperatorAssignInstruction* op_assign = as_operator_assign_or_null(*inst);
        if (op_assign == nullptr) {
          continue;
        }

        const Operator* op = as_operator_or_null(op_assign->get_src());
        if (op == nullptr) {
          continue;
        }

        SimplifiedExpr simplified = simplify_operator_expr(*op);
        if (!simplified.changed) {
          continue;
        }

        auto dst = clone_ast(op_assign->get_dst());
        if (simplified.operator_expr) {
          inst = std::make_unique<OperatorAssignInstruction>(std::move(dst),
                                                             std::move(simplified.expr));
        } else {
          inst = std::make_unique<VarNumLabelAssignInstruction>(std::move(dst),
                                                                std::move(simplified.expr));
        }
        changed = true;
      }
    }
  }

  return changed;
}

}  // namespace IR
