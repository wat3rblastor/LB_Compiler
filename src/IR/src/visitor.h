#pragma once

namespace IR {

class Var;
class Label;
class Type;
class Number;
class Callee;
class Operator;
class Index;
class ArrayLength;
class TupleLength;
class FunctionCall;
class NewArray;
class NewTuple;
class Parameter;

class Instruction;
class VarDefInstruction;
class VarNumLabelAssignInstruction;
class OperatorAssignInstruction;
class IndexAssignInstruction;
class LengthAssignInstruction;
class CallInstruction;
class CallAssignInstruction;
class NewArrayAssignInstruction;
class NewTupleAssignInstruction;
class BranchInstruction;
class ConditionalBranchInstruction;
class ReturnInstruction;
class ReturnValueInstruction;

class BasicBlock;
class Function;
class Program;

class ConstVisitor {
 public:
  virtual ~ConstVisitor() = default;

  virtual void visit(const Var&) = 0;
  virtual void visit(const Label&) = 0;
  virtual void visit(const Type&) = 0;
  virtual void visit(const Number&) = 0;
  virtual void visit(const Callee&) = 0;
  virtual void visit(const Operator&) = 0;
  virtual void visit(const Index&) = 0;
  virtual void visit(const ArrayLength&) = 0;
  virtual void visit(const TupleLength&) = 0;
  virtual void visit(const FunctionCall&) = 0;
  virtual void visit(const NewArray&) = 0;
  virtual void visit(const NewTuple&) = 0;
  virtual void visit(const Parameter&) = 0;

  virtual void visit(const VarDefInstruction&) = 0;
  virtual void visit(const VarNumLabelAssignInstruction&) = 0;
  virtual void visit(const OperatorAssignInstruction&) = 0;
  virtual void visit(const IndexAssignInstruction&) = 0;
  virtual void visit(const LengthAssignInstruction&) = 0;
  virtual void visit(const CallInstruction&) = 0;
  virtual void visit(const CallAssignInstruction&) = 0;
  virtual void visit(const NewArrayAssignInstruction&) = 0;
  virtual void visit(const NewTupleAssignInstruction&) = 0;
  virtual void visit(const BranchInstruction&) = 0;
  virtual void visit(const ConditionalBranchInstruction&) = 0;
  virtual void visit(const ReturnInstruction&) = 0;
  virtual void visit(const ReturnValueInstruction&) = 0;

  virtual void visit(const BasicBlock&) = 0;
  virtual void visit(const Function&) = 0;
  virtual void visit(const Program&) = 0;
};

}  // namespace IR
