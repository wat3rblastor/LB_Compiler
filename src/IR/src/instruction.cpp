#include "instruction.h"

namespace IR {

void VarDefInstruction::accept(ConstVisitor& visitor) const {
  visitor.visit(*this);
}

VarDefInstruction::VarDefInstruction(std::unique_ptr<ASTNode> type,
                                     std::unique_ptr<ASTNode> var)
    : type(std::move(type)), var(std::move(var)) {}

const ASTNode& VarDefInstruction::get_type() const {
  return *type;
}

const ASTNode& VarDefInstruction::get_var() const {
  return *var;
}

void VarNumLabelAssignInstruction::accept(ConstVisitor& visitor) const {
  visitor.visit(*this);
}

VarNumLabelAssignInstruction::VarNumLabelAssignInstruction(
    std::unique_ptr<ASTNode> dst, std::unique_ptr<ASTNode> src)
    : dst(std::move(dst)), src(std::move(src)) {}

const ASTNode& VarNumLabelAssignInstruction::get_dst() const {
  return *dst;
}

const ASTNode& VarNumLabelAssignInstruction::get_src() const {
  return *src;
}

void OperatorAssignInstruction::accept(ConstVisitor& visitor) const {
  visitor.visit(*this);
}

OperatorAssignInstruction::OperatorAssignInstruction(std::unique_ptr<ASTNode> dst,
                                                     std::unique_ptr<ASTNode> src)
    : dst(std::move(dst)), src(std::move(src)) {}

const ASTNode& OperatorAssignInstruction::get_dst() const {
  return *dst;
}

const ASTNode& OperatorAssignInstruction::get_src() const {
  return *src;
}

void IndexAssignInstruction::accept(ConstVisitor& visitor) const {
  visitor.visit(*this);
}

IndexAssignInstruction::IndexAssignInstruction(std::unique_ptr<ASTNode> dst,
                                               std::unique_ptr<ASTNode> src)
    : dst(std::move(dst)), src(std::move(src)) {}

const ASTNode& IndexAssignInstruction::get_dst() const {
  return *dst;
}

const ASTNode& IndexAssignInstruction::get_src() const {
  return *src;
}

void LengthAssignInstruction::accept(ConstVisitor& visitor) const {
  visitor.visit(*this);
}

LengthAssignInstruction::LengthAssignInstruction(std::unique_ptr<ASTNode> dst,
                                                 std::unique_ptr<ASTNode> src)
    : dst(std::move(dst)), src(std::move(src)) {}

const ASTNode& LengthAssignInstruction::get_dst() const {
  return *dst;
}

const ASTNode& LengthAssignInstruction::get_src() const {
  return *src;
}

void CallInstruction::accept(ConstVisitor& visitor) const {
  visitor.visit(*this);
}

CallInstruction::CallInstruction(std::unique_ptr<ASTNode> functionCall)
    : functionCall(std::move(functionCall)) {}

const ASTNode& CallInstruction::get_call() const {
  return *functionCall;
}

void CallAssignInstruction::accept(ConstVisitor& visitor) const {
  visitor.visit(*this);
}

CallAssignInstruction::CallAssignInstruction(std::unique_ptr<ASTNode> dst,
                                             std::unique_ptr<ASTNode> functionCall)
    : dst(std::move(dst)), functionCall(std::move(functionCall)) {}

const ASTNode& CallAssignInstruction::get_dst() const {
  return *dst;
}

const ASTNode& CallAssignInstruction::get_call() const {
  return *functionCall;
}

void NewArrayAssignInstruction::accept(ConstVisitor& visitor) const {
  visitor.visit(*this);
}

NewArrayAssignInstruction::NewArrayAssignInstruction(
    std::unique_ptr<ASTNode> dst, std::unique_ptr<ASTNode> newArray)
    : dst(std::move(dst)), newArray(std::move(newArray)) {}

const ASTNode& NewArrayAssignInstruction::get_dst() const {
  return *dst;
}

const ASTNode& NewArrayAssignInstruction::get_new_array() const {
  return *newArray;
}

void NewTupleAssignInstruction::accept(ConstVisitor& visitor) const {
  visitor.visit(*this);
}

NewTupleAssignInstruction::NewTupleAssignInstruction(
    std::unique_ptr<ASTNode> dst, std::unique_ptr<ASTNode> newTuple)
    : dst(std::move(dst)), newTuple(std::move(newTuple)) {}

const ASTNode& NewTupleAssignInstruction::get_dst() const {
  return *dst;
}

const ASTNode& NewTupleAssignInstruction::get_new_tuple() const {
  return *newTuple;
}

void BranchInstruction::accept(ConstVisitor& visitor) const {
  visitor.visit(*this);
}

const ASTNode& BranchInstruction::get_target() const {
  return *target;
}

BranchInstruction::BranchInstruction(std::unique_ptr<ASTNode> target)
    : target(std::move(target)) {}

void ConditionalBranchInstruction::accept(ConstVisitor& visitor) const {
  visitor.visit(*this);
}

const ASTNode& ConditionalBranchInstruction::get_condition() const {
  return *condition;
}

const ASTNode& ConditionalBranchInstruction::get_true_target() const {
  return *dst1;
}

const ASTNode& ConditionalBranchInstruction::get_false_target() const {
  return *dst2;
}

ConditionalBranchInstruction::ConditionalBranchInstruction(
    std::unique_ptr<ASTNode> condition, std::unique_ptr<ASTNode> dst1,
    std::unique_ptr<ASTNode> dst2)
    : condition(std::move(condition)), dst1(std::move(dst1)),
      dst2(std::move(dst2)) {}

void ReturnInstruction::accept(ConstVisitor& visitor) const {
  visitor.visit(*this);
}

void ReturnValueInstruction::accept(ConstVisitor& visitor) const {
  visitor.visit(*this);
}

ReturnValueInstruction::ReturnValueInstruction(
    std::unique_ptr<ASTNode> returnValue)
    : returnValue(std::move(returnValue)) {}

const ASTNode& ReturnValueInstruction::get_return_value() const {
  return *returnValue;
}

}  // namespace IR
