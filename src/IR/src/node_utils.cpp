#include "node_utils.h"

#include <utility>

namespace IR {
namespace {

class NullConstVisitor : public ConstVisitor {
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

template <typename T>
const T& expect_or_throw(const T* ptr, const std::string& message) {
  if (ptr == nullptr) {
    throw std::runtime_error(message);
  }
  return *ptr;
}

}  // namespace

const Var* as_var_or_null(const ASTNode& node) {
  class Visitor : public NullConstVisitor {
   public:
    const Var* out = nullptr;
    void visit(const Var& value) override { out = &value; }
  } visitor;
  node.accept(visitor);
  return visitor.out;
}

const Label* as_label_or_null(const ASTNode& node) {
  class Visitor : public NullConstVisitor {
   public:
    const Label* out = nullptr;
    void visit(const Label& value) override { out = &value; }
  } visitor;
  node.accept(visitor);
  return visitor.out;
}

const Type* as_type_or_null(const ASTNode& node) {
  class Visitor : public NullConstVisitor {
   public:
    const Type* out = nullptr;
    void visit(const Type& value) override { out = &value; }
  } visitor;
  node.accept(visitor);
  return visitor.out;
}

const Index* as_index_or_null(const ASTNode& node) {
  class Visitor : public NullConstVisitor {
   public:
    const Index* out = nullptr;
    void visit(const Index& value) override { out = &value; }
  } visitor;
  node.accept(visitor);
  return visitor.out;
}

const ArrayLength* as_array_length_or_null(const ASTNode& node) {
  class Visitor : public NullConstVisitor {
   public:
    const ArrayLength* out = nullptr;
    void visit(const ArrayLength& value) override { out = &value; }
  } visitor;
  node.accept(visitor);
  return visitor.out;
}

const TupleLength* as_tuple_length_or_null(const ASTNode& node) {
  class Visitor : public NullConstVisitor {
   public:
    const TupleLength* out = nullptr;
    void visit(const TupleLength& value) override { out = &value; }
  } visitor;
  node.accept(visitor);
  return visitor.out;
}

const NewArray* as_new_array_or_null(const ASTNode& node) {
  class Visitor : public NullConstVisitor {
   public:
    const NewArray* out = nullptr;
    void visit(const NewArray& value) override { out = &value; }
  } visitor;
  node.accept(visitor);
  return visitor.out;
}

const NewTuple* as_new_tuple_or_null(const ASTNode& node) {
  class Visitor : public NullConstVisitor {
   public:
    const NewTuple* out = nullptr;
    void visit(const NewTuple& value) override { out = &value; }
  } visitor;
  node.accept(visitor);
  return visitor.out;
}

const Parameter* as_parameter_or_null(const ASTNode& node) {
  class Visitor : public NullConstVisitor {
   public:
    const Parameter* out = nullptr;
    void visit(const Parameter& value) override { out = &value; }
  } visitor;
  node.accept(visitor);
  return visitor.out;
}

const BranchInstruction* as_branch_or_null(const Instruction& inst) {
  class Visitor : public NullConstVisitor {
   public:
    const BranchInstruction* out = nullptr;
    void visit(const BranchInstruction& value) override { out = &value; }
  } visitor;
  inst.accept(visitor);
  return visitor.out;
}

const ConditionalBranchInstruction* as_conditional_branch_or_null(
    const Instruction& inst) {
  class Visitor : public NullConstVisitor {
   public:
    const ConditionalBranchInstruction* out = nullptr;
    void visit(const ConditionalBranchInstruction& value) override { out = &value; }
  } visitor;
  inst.accept(visitor);
  return visitor.out;
}

const Var& expect_var(const ASTNode& node, const std::string& message) {
  return expect_or_throw(as_var_or_null(node), message);
}

const Label& expect_label(const ASTNode& node, const std::string& message) {
  return expect_or_throw(as_label_or_null(node), message);
}

const Type& expect_type(const ASTNode& node, const std::string& message) {
  return expect_or_throw(as_type_or_null(node), message);
}

const Index& expect_index(const ASTNode& node, const std::string& message) {
  return expect_or_throw(as_index_or_null(node), message);
}

const ArrayLength& expect_array_length(const ASTNode& node,
                                       const std::string& message) {
  return expect_or_throw(as_array_length_or_null(node), message);
}

const TupleLength& expect_tuple_length(const ASTNode& node,
                                       const std::string& message) {
  return expect_or_throw(as_tuple_length_or_null(node), message);
}

const NewArray& expect_new_array(const ASTNode& node, const std::string& message) {
  return expect_or_throw(as_new_array_or_null(node), message);
}

const NewTuple& expect_new_tuple(const ASTNode& node, const std::string& message) {
  return expect_or_throw(as_new_tuple_or_null(node), message);
}

const Parameter& expect_parameter(const ASTNode& node,
                                  const std::string& message) {
  return expect_or_throw(as_parameter_or_null(node), message);
}

}  // namespace IR
