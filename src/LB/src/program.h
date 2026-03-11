#pragma once

#include "LB.h"
#include "instruction.h"

namespace LB {

class Scope : public ASTNode {
 public:
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
  void addNode(std::unique_ptr<ASTNode>);
  const std::vector<std::unique_ptr<ASTNode>>& getNodes() const;
  std::vector<std::unique_ptr<ASTNode>>& getNodes();

 private:
  // Should be Instructions and Scope
  std::vector<std::unique_ptr<ASTNode>> instructions;
};

class Function : public ASTNode {
 public:
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
  void addParameter(std::unique_ptr<ASTNode>);
  void setScope(std::unique_ptr<ASTNode>);
  const ASTNode& getType() const;
  const ASTNode& getFunctionName() const;
  const std::vector<std::unique_ptr<ASTNode>>& getParameters() const;
  std::vector<std::unique_ptr<ASTNode>>& getParameters();
  const ASTNode& getScope() const;
  ASTNode& getScope();
  Function(std::unique_ptr<ASTNode>, std::unique_ptr<ASTNode>);

 private:
  // Should be Type
  std::unique_ptr<ASTNode> type;
  // Should be Name
  std::unique_ptr<ASTNode> functionName;
  // Should be Parameter
  std::vector<std::unique_ptr<ASTNode>> parameters;
  // Should be Scope
  std::unique_ptr<ASTNode> scope;
};

class Program : public ASTNode {
 public:
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
  void addFunction(std::unique_ptr<Function>);
  const std::vector<std::unique_ptr<Function>>& getFunctions() const;
  std::vector<std::unique_ptr<Function>>& getFunctions();

 private:
  std::vector<std::unique_ptr<Function>> functions;
};

}
