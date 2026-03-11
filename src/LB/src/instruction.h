#pragma once 

#include "LB.h"

namespace LB {

class Instruction : public ASTNode {
 public:
  virtual ~Instruction() = default;
  virtual std::string to_string() const = 0;
  virtual void generate_code(std::ofstream&) const = 0;
};

class NameDefInstruction : public Instruction {
 public:
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
  const ASTNode& get_type() const;
  const ASTNode& get_name() const;
  NameDefInstruction(std::unique_ptr<ASTNode>, std::unique_ptr<ASTNode>);

 private:
  // Should be Type
  std::unique_ptr<ASTNode> type;
  // Should be Name
  std::unique_ptr<ASTNode> name;
};

class NameNumAssignInstruction : public Instruction {
 public:
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
  const ASTNode& get_dst() const;
  const ASTNode& get_src() const;
  NameNumAssignInstruction(std::unique_ptr<ASTNode>, std::unique_ptr<ASTNode>);

 private:
  // Should be Name
  std::unique_ptr<ASTNode> dst;
  // Should be Name, Number
  std::unique_ptr<ASTNode> src;
};

class OperatorAssignInstruction : public Instruction {
 public:
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
  const ASTNode& get_dst() const;
  const ASTNode& get_src() const;
  OperatorAssignInstruction(std::unique_ptr<ASTNode>, std::unique_ptr<ASTNode>);

 private:
  // Should be Name
  std::unique_ptr<ASTNode> dst;
  // Should be Operator
  std::unique_ptr<ASTNode> src;
};

class LabelInstruction : public Instruction {
 public:
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
  const ASTNode& get_label() const;
  LabelInstruction(std::unique_ptr<ASTNode>);

private:
  // Should be Label
  std::unique_ptr<ASTNode> label;
};

class ConditionalBranchInstruction : public Instruction {
 public:
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
  const ASTNode& get_condition() const;
  const ASTNode& get_true_target() const;
  const ASTNode& get_false_target() const;
  ConditionalBranchInstruction(std::unique_ptr<ASTNode>, std::unique_ptr<ASTNode>, std::unique_ptr<ASTNode>);

 private:
  // Should be CondOperator
  std::unique_ptr<ASTNode> condition;
  // Should be Label
  std::unique_ptr<ASTNode> dst1;
  std::unique_ptr<ASTNode> dst2;
};

class BranchInstruction : public Instruction {
 public:
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
  const ASTNode& get_target() const;
  BranchInstruction(std::unique_ptr<ASTNode>);

 private:
  // Should be Label
  std::unique_ptr<ASTNode> target;
};

class ReturnInstruction : public Instruction {
 public:
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
};

class ReturnValueInstruction : public Instruction {
 public:
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
  const ASTNode& get_return_value() const;
  ReturnValueInstruction(std::unique_ptr<ASTNode>);

 private:
  // Should be Name, Number
  std::unique_ptr<ASTNode> returnValue;
};

class WhileInstruction : public Instruction {
 public:
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
  const ASTNode& get_condition() const;
  const ASTNode& get_true_target() const;
  const ASTNode& get_false_target() const;
  WhileInstruction(std::unique_ptr<ASTNode>, std::unique_ptr<ASTNode>, std::unique_ptr<ASTNode>);

 private:
  // Should be CondOperator
  std::unique_ptr<ASTNode> condition;
  // Should be Label
  std::unique_ptr<ASTNode> dst1;
  std::unique_ptr<ASTNode> dst2;
};

class ContinueInstruction : public Instruction {
 public:
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
};

class BreakInstruction : public Instruction {
 public:
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
};

class IndexAssignInstruction : public Instruction {
 public:
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
  const ASTNode& get_dst() const;
  const ASTNode& get_src() const;
  IndexAssignInstruction(std::unique_ptr<ASTNode>, std::unique_ptr<ASTNode>);

 private:
  // Should be Name, Index
  std::unique_ptr<ASTNode> dst;
  // Should be Name, Index
  std::unique_ptr<ASTNode> src; 
};

class LengthAssignInstruction : public Instruction {
 public:
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
  const ASTNode& get_dst() const;
  const ASTNode& get_src() const;
  LengthAssignInstruction(std::unique_ptr<ASTNode>, std::unique_ptr<ASTNode>);

 private:
  // Should be Name
  std::unique_ptr<ASTNode> dst;
  // Should be Length
  std::unique_ptr<ASTNode> src;
};

class CallInstruction : public Instruction {
 public:
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
  const ASTNode& get_function_call() const;
  CallInstruction(std::unique_ptr<ASTNode>);

 private:
  // Should be FunctionCall
  std::unique_ptr<ASTNode> functionCall;
};

class CallAssignInstruction : public Instruction {
 public:
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
  const ASTNode& get_dst() const;
  const ASTNode& get_function_call() const;
  CallAssignInstruction(std::unique_ptr<ASTNode>, std::unique_ptr<ASTNode>);

 private:
  // Should be Name
  std::unique_ptr<ASTNode> dst;
  // Should be FunctionCall
  std::unique_ptr<ASTNode> functionCall;
};

class NewArrayAssignInstruction : public Instruction {
 public:
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
  const ASTNode& get_dst() const;
  const ASTNode& get_new_array() const;
  NewArrayAssignInstruction(std::unique_ptr<ASTNode>, std::unique_ptr<ASTNode>);
  
 private:
  // Should be Name
  std::unique_ptr<ASTNode> dst;
  // Should be New Array
  std::unique_ptr<ASTNode> newArray;
};

class NewTupleAssignInstruction : public Instruction {
 public:
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
  const ASTNode& get_dst() const;
  const ASTNode& get_new_tuple() const;
  NewTupleAssignInstruction(std::unique_ptr<ASTNode>, std::unique_ptr<ASTNode>);
  
 private:
  // Should be Name
  std::unique_ptr<ASTNode> dst;
  // Should be New Tuple
  std::unique_ptr<ASTNode> newTuple;
};

}