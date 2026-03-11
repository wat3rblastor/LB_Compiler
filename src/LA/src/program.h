#pragma once

#include "LA.h"
#include "instruction.h"

namespace LA {

class Function : public ASTNode {
 public:
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
  void addParameter(std::unique_ptr<ASTNode>);
  void addInstruction(std::unique_ptr<Instruction>);
  void setInstructions(std::vector<std::unique_ptr<Instruction>>&&);
  const ASTNode& getType() const;
  const ASTNode& getFunctionName() const;
  const std::vector<std::unique_ptr<ASTNode>>& getParameters() const;
  const std::vector<std::unique_ptr<Instruction>>& getInstructions() const;
  std::vector<std::unique_ptr<Instruction>>& getInstructions();
  Function(std::unique_ptr<ASTNode>, std::unique_ptr<ASTNode>);

 private:
  // Should be Type
  std::unique_ptr<ASTNode> type;
  // Should be Name
  std::unique_ptr<ASTNode> functionName;
  // Should be Parameter
  std::vector<std::unique_ptr<ASTNode>> parameters;
  // Should be Instruction
  std::vector<std::unique_ptr<Instruction>> instructions;
};

class Program : public ASTNode {
 public:
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
  void addFunction(std::unique_ptr<Function>);
  std::vector<std::unique_ptr<Function>>& getFunctions();

 private:
  // Should be Function
  std::vector<std::unique_ptr<Function>> functions;
};

}
