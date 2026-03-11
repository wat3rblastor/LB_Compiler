#pragma once

#include "IR.h"
#include "instruction.h"

namespace IR {

class BasicBlock : public ASTNode {
 public:
  void accept(ConstVisitor&) const override;
  void addInstruction(std::unique_ptr<Instruction>);
  void addTE(std::unique_ptr<Instruction>);
  void set_te(std::unique_ptr<Instruction>);
  const Label& get_label() const;
  const Instruction& get_te() const;
  Instruction& get_te_mut();
  const std::vector<std::unique_ptr<Instruction>>& get_instructions() const;
  std::vector<std::unique_ptr<Instruction>>& get_instructions_mut();
  BasicBlock(std::unique_ptr<ASTNode>);

 private:
  std::unique_ptr<ASTNode> label;
  std::vector<std::unique_ptr<Instruction>> instructions;
  std::unique_ptr<Instruction> te;
};

class Function : public ASTNode {
 public:
  void accept(ConstVisitor&) const override;
  void addParameter(std::unique_ptr<ASTNode>);
  void addBasicBlock(std::unique_ptr<BasicBlock>);
  std::vector<std::unique_ptr<BasicBlock>>& get_basicBlocks();
  const std::vector<std::unique_ptr<BasicBlock>>& get_basicBlocks() const;
  const std::vector<std::unique_ptr<ASTNode>>& get_parameters() const;
  const ASTNode& get_type() const;
  const ASTNode& get_name() const;
  Function(std::unique_ptr<ASTNode>, std::unique_ptr<ASTNode>);

 private:
  std::unique_ptr<ASTNode> type;
  std::unique_ptr<ASTNode> functionName;
  std::vector<std::unique_ptr<ASTNode>> parameters;
  std::vector<std::unique_ptr<BasicBlock>> basicBlocks;
};

class Program : public ASTNode {
 public:
  void accept(ConstVisitor&) const override;
  void addFunction(std::unique_ptr<Function>);
  std::vector<std::unique_ptr<Function>>& getFunctions();
  const std::vector<std::unique_ptr<Function>>& getFunctions() const;

 private:
  std::vector<std::unique_ptr<Function>> functions;
};

}  // namespace IR
