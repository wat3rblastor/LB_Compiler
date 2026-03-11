#pragma once

#include "IR.h"

namespace IR {

class Instruction : public ASTNode {
 public:
  virtual ~Instruction() = default;
};

class VarDefInstruction : public Instruction {
 public:
  void accept(ConstVisitor&) const override;
  VarDefInstruction(std::unique_ptr<ASTNode>, std::unique_ptr<ASTNode>);

  const ASTNode& get_type() const;
  const ASTNode& get_var() const;

 private:
  std::unique_ptr<ASTNode> type;
  std::unique_ptr<ASTNode> var;
};

class VarNumLabelAssignInstruction : public Instruction {
 public:
  void accept(ConstVisitor&) const override;
  VarNumLabelAssignInstruction(std::unique_ptr<ASTNode>, std::unique_ptr<ASTNode>);

  const ASTNode& get_dst() const;
  const ASTNode& get_src() const;

 private:
  std::unique_ptr<ASTNode> dst;
  std::unique_ptr<ASTNode> src;
};

class OperatorAssignInstruction : public Instruction {
 public:
  void accept(ConstVisitor&) const override;
  OperatorAssignInstruction(std::unique_ptr<ASTNode>, std::unique_ptr<ASTNode>);

  const ASTNode& get_dst() const;
  const ASTNode& get_src() const;

 private:
  std::unique_ptr<ASTNode> dst;
  std::unique_ptr<ASTNode> src;
};

class IndexAssignInstruction : public Instruction {
 public:
  void accept(ConstVisitor&) const override;
  IndexAssignInstruction(std::unique_ptr<ASTNode>, std::unique_ptr<ASTNode>);

  const ASTNode& get_dst() const;
  const ASTNode& get_src() const;

 private:
  std::unique_ptr<ASTNode> dst;
  std::unique_ptr<ASTNode> src;
};

class LengthAssignInstruction : public Instruction {
 public:
  void accept(ConstVisitor&) const override;
  LengthAssignInstruction(std::unique_ptr<ASTNode>, std::unique_ptr<ASTNode>);

  const ASTNode& get_dst() const;
  const ASTNode& get_src() const;

 private:
  std::unique_ptr<ASTNode> dst;
  std::unique_ptr<ASTNode> src;
};

class CallInstruction : public Instruction {
 public:
  void accept(ConstVisitor&) const override;
  explicit CallInstruction(std::unique_ptr<ASTNode>);

  const ASTNode& get_call() const;

 private:
  std::unique_ptr<ASTNode> functionCall;
};

class CallAssignInstruction : public Instruction {
 public:
  void accept(ConstVisitor&) const override;
  CallAssignInstruction(std::unique_ptr<ASTNode>, std::unique_ptr<ASTNode>);

  const ASTNode& get_dst() const;
  const ASTNode& get_call() const;

 private:
  std::unique_ptr<ASTNode> dst;
  std::unique_ptr<ASTNode> functionCall;
};

class NewArrayAssignInstruction : public Instruction {
 public:
  void accept(ConstVisitor&) const override;
  NewArrayAssignInstruction(std::unique_ptr<ASTNode>, std::unique_ptr<ASTNode>);

  const ASTNode& get_dst() const;
  const ASTNode& get_new_array() const;

 private:
  std::unique_ptr<ASTNode> dst;
  std::unique_ptr<ASTNode> newArray;
};

class NewTupleAssignInstruction : public Instruction {
 public:
  void accept(ConstVisitor&) const override;
  NewTupleAssignInstruction(std::unique_ptr<ASTNode>, std::unique_ptr<ASTNode>);

  const ASTNode& get_dst() const;
  const ASTNode& get_new_tuple() const;

 private:
  std::unique_ptr<ASTNode> dst;
  std::unique_ptr<ASTNode> newTuple;
};

class BranchInstruction : public Instruction {
 public:
  void accept(ConstVisitor&) const override;
  const ASTNode& get_target() const;
  explicit BranchInstruction(std::unique_ptr<ASTNode>);

 private:
  std::unique_ptr<ASTNode> target;
};

class ConditionalBranchInstruction : public Instruction {
 public:
  void accept(ConstVisitor&) const override;
  const ASTNode& get_condition() const;
  const ASTNode& get_true_target() const;
  const ASTNode& get_false_target() const;
  ConditionalBranchInstruction(std::unique_ptr<ASTNode>, std::unique_ptr<ASTNode>, std::unique_ptr<ASTNode>);

 private:
  std::unique_ptr<ASTNode> condition;
  std::unique_ptr<ASTNode> dst1;
  std::unique_ptr<ASTNode> dst2;
};

class ReturnInstruction : public Instruction {
 public:
  void accept(ConstVisitor&) const override;
};

class ReturnValueInstruction : public Instruction {
 public:
  void accept(ConstVisitor&) const override;
  explicit ReturnValueInstruction(std::unique_ptr<ASTNode>);

  const ASTNode& get_return_value() const;

 private:
  std::unique_ptr<ASTNode> returnValue;
};

}  // namespace IR
