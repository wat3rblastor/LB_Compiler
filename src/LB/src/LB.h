#pragma once

#include <cstdint>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

#include "utils.h"

namespace LB {

class ASTNode {
 public:
  virtual ~ASTNode() = default;
  virtual std::string to_string() const = 0;
  virtual void generate_code(std::ofstream&) const = 0;
};

struct Name : public ASTNode {
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
  Name() = default;
  Name(const std::string&);
  std::string name;
};

struct Number : public ASTNode {
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
  Number() = default;
  Number(int64_t value);
  int64_t value;
};

struct Label : public ASTNode {
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
  Label() = default;
  Label(const std::string&);
  std::string name;
};

struct Type : ASTNode {
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
  Type() = default;
  Type(const std::string&);
  std::string name;
};

struct Parameter : public ASTNode {
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
  Parameter(std::unique_ptr<ASTNode>, std::unique_ptr<ASTNode>);

  // Should be Type
  std::unique_ptr<ASTNode> type;
  // Should be Name
  std::unique_ptr<ASTNode> name;
};

enum class OP { PLUS, SUB, MUL, AND, LSHIFT, RSHIFT, LT, LE, EQ, GE, GT };

struct Operator : public ASTNode {
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
  Operator() = default;
  Operator(std::unique_ptr<ASTNode>, std::unique_ptr<ASTNode>, OP);
  std::unique_ptr<ASTNode> left;
  std::unique_ptr<ASTNode> right;
  OP op;
};

struct CondOperator : public ASTNode {
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
  CondOperator() = default;
  CondOperator(std::unique_ptr<ASTNode>, std::unique_ptr<ASTNode>, OP);
  std::unique_ptr<ASTNode> left;
  std::unique_ptr<ASTNode> right;
  OP op;
};

struct Index : public ASTNode {
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
  Index(int64_t, std::unique_ptr<ASTNode>, std::vector<std::unique_ptr<ASTNode>>&&);
  int64_t lineNumber;
  std::unique_ptr<ASTNode> name;
  std::vector<std::unique_ptr<ASTNode>> indices;
};

struct Length : public ASTNode {
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
  Length() = default;
  Length(int64_t, std::unique_ptr<ASTNode>);
  Length(int64_t, std::unique_ptr<ASTNode>, std::unique_ptr<ASTNode>);

  int64_t lineNumber;
  bool hasIndex;
  // Should be Name
  std::unique_ptr<ASTNode> name;
  // Shoud be Name, Number
  std::unique_ptr<ASTNode> index;
};

struct FunctionCall : public ASTNode {
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
  FunctionCall(std::unique_ptr<ASTNode>,
               std::vector<std::unique_ptr<ASTNode>>&&);

  // Should be Name
  std::unique_ptr<ASTNode> name;
  // Should be Name or Number
  std::vector<std::unique_ptr<ASTNode>> args;
};

struct NewArray : public ASTNode {
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
  NewArray(std::vector<std::unique_ptr<ASTNode>>&&);

  // Should be Name or Number
  std::vector<std::unique_ptr<ASTNode>> args;
};

struct NewTuple : public ASTNode {
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
  NewTuple(std::unique_ptr<ASTNode>);

  // Should be Name or Number
  std::unique_ptr<ASTNode> length;
};

}