#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

#include "utils.h"
#include "visitor.h"

namespace IR {

class ASTNode {
 public:
  virtual ~ASTNode() = default;
  virtual void accept(ConstVisitor&) const = 0;
};

struct Var : public ASTNode {
  void accept(ConstVisitor&) const override;
  Var() = default;
  explicit Var(const std::string&);
  std::string name;
};

struct Label : public ASTNode {
  void accept(ConstVisitor&) const override;
  Label() = default;
  explicit Label(const std::string&);
  std::string name;
};

struct Type : ASTNode {
  void accept(ConstVisitor&) const override;
  Type() = default;
  explicit Type(const std::string&);
  std::string name;
};

enum class OP { PLUS, SUB, MUL, AND, LSHIFT, RSHIFT, LT, LE, EQ, GE, GT };

struct Number : public ASTNode {
  void accept(ConstVisitor&) const override;
  Number() = default;
  explicit Number(int64_t value);
  int64_t value;
};

enum class EnumCallee { PRINT, INPUT, TUPLE_ERROR, TENSOR_ERROR };

struct Callee : public ASTNode {
  void accept(ConstVisitor&) const override;
  Callee() = default;
  explicit Callee(EnumCallee&);
  explicit Callee(std::unique_ptr<ASTNode>);
  std::variant<EnumCallee, std::unique_ptr<ASTNode>> calleeValue;
};

struct Operator : public ASTNode {
  void accept(ConstVisitor&) const override;
  Operator() = default;
  Operator(std::unique_ptr<ASTNode>, std::unique_ptr<ASTNode>, OP);
  std::unique_ptr<ASTNode> left;
  std::unique_ptr<ASTNode> right;
  OP op;
};

struct Index : public ASTNode {
  void accept(ConstVisitor&) const override;
  Index(std::unique_ptr<ASTNode>, std::vector<std::unique_ptr<ASTNode>>&&);
  std::unique_ptr<ASTNode> var;
  std::vector<std::unique_ptr<ASTNode>> indices;
};

struct ArrayLength : public ASTNode {
  void accept(ConstVisitor&) const override;
  ArrayLength() = default;
  ArrayLength(std::unique_ptr<ASTNode>, std::unique_ptr<ASTNode>);

  std::unique_ptr<ASTNode> var;
  std::unique_ptr<ASTNode> index;
};

struct TupleLength : public ASTNode {
  void accept(ConstVisitor&) const override;
  TupleLength() = default;
  explicit TupleLength(std::unique_ptr<ASTNode>);

  std::unique_ptr<ASTNode> var;
};

struct FunctionCall : public ASTNode {
  void accept(ConstVisitor&) const override;
  FunctionCall(std::unique_ptr<ASTNode>, std::vector<std::unique_ptr<ASTNode>>&&);

  std::unique_ptr<ASTNode> callee;
  std::vector<std::unique_ptr<ASTNode>> args;
};

struct NewArray : public ASTNode {
  void accept(ConstVisitor&) const override;
  explicit NewArray(std::vector<std::unique_ptr<ASTNode>>&&);

  std::vector<std::unique_ptr<ASTNode>> args;
};

struct NewTuple : public ASTNode {
  void accept(ConstVisitor&) const override;
  explicit NewTuple(std::unique_ptr<ASTNode>);

  std::unique_ptr<ASTNode> length;
};

struct Parameter : public ASTNode {
  void accept(ConstVisitor&) const override;
  Parameter(std::unique_ptr<ASTNode>, std::unique_ptr<ASTNode>);

  std::unique_ptr<ASTNode> type;
  std::unique_ptr<ASTNode> var;
};

}  // namespace IR
