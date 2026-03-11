#pragma once

#include <string>

#include "program.h"
#include "visitor.h"

namespace IR {

class ToStringVisitor : public ConstVisitor {
 public:
  void visit(const Var&) override;
  void visit(const Label&) override;
  void visit(const Type&) override;
  void visit(const Number&) override;
  void visit(const Callee&) override;
  void visit(const Operator&) override;
  void visit(const Index&) override;
  void visit(const ArrayLength&) override;
  void visit(const TupleLength&) override;
  void visit(const FunctionCall&) override;
  void visit(const NewArray&) override;
  void visit(const NewTuple&) override;
  void visit(const Parameter&) override;

  void visit(const VarDefInstruction&) override;
  void visit(const VarNumLabelAssignInstruction&) override;
  void visit(const OperatorAssignInstruction&) override;
  void visit(const IndexAssignInstruction&) override;
  void visit(const LengthAssignInstruction&) override;
  void visit(const CallInstruction&) override;
  void visit(const CallAssignInstruction&) override;
  void visit(const NewArrayAssignInstruction&) override;
  void visit(const NewTupleAssignInstruction&) override;
  void visit(const BranchInstruction&) override;
  void visit(const ConditionalBranchInstruction&) override;
  void visit(const ReturnInstruction&) override;
  void visit(const ReturnValueInstruction&) override;

  void visit(const BasicBlock&) override;
  void visit(const Function&) override;
  void visit(const Program&) override;

  std::string result() const;

 private:
  std::string text;
};

std::string to_string(const ASTNode&);
std::string to_string(const Program&);

}  // namespace IR
