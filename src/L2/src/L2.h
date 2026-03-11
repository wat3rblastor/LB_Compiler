#pragma once

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

namespace L2 {

class ASTNode {
 public:
  virtual ~ASTNode() = default;
  virtual std::string to_string() const = 0;
  virtual void generate_code(std::ofstream&) const = 0;
  virtual std::unique_ptr<ASTNode> clone() const = 0;
};

struct Label : public ASTNode {
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
  std::unique_ptr<ASTNode> clone() const override;
  Label() = default;
  Label(const std::string& name) : name(name) {}
  std::string name;
};

struct Number : public ASTNode {
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
  std::unique_ptr<ASTNode> clone() const override;
  Number(int64_t num) : num(num) {}
  int64_t num;
};

struct Var : public ASTNode {
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
  std::unique_ptr<ASTNode> clone() const override;
  Var(const std::string& name) : name(name) {}
  std::string name;
};

enum class RegisterID {
  rdi,
  rsi,
  rdx,
  rcx,
  r8,
  r9,
  rax,
  rbx,
  rbp,
  r10,
  r11,
  r12,
  r13,
  r14,
  r15,
  rsp
};

struct Register : public ASTNode {
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
  std::unique_ptr<ASTNode> clone() const override;
  void generate_code_8bit(std::ofstream&) const;
  Register(const RegisterID& id) : id(id) {}
  RegisterID id;
};

struct MemoryAddress : public ASTNode {
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
  std::unique_ptr<ASTNode> clone() const override;
  MemoryAddress(std::unique_ptr<ASTNode> reg, std::unique_ptr<ASTNode> offset)
      : reg(std::move(reg)), offset(std::move(offset)) {}
  // can be register or var
  std::unique_ptr<ASTNode> reg;
  std::unique_ptr<ASTNode> offset;
};

struct StackArg : public ASTNode {
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
  std::unique_ptr<ASTNode> clone() const override;
  StackArg(std::unique_ptr<ASTNode> offset) : offset(std::move(offset)) {}
  // Should point to a number
  std::unique_ptr<ASTNode> offset;
};

class Operator : public ASTNode {
 public:
  virtual ~Operator() = default;
};

enum class EnumArithmeticOps { ADD_EQUAL, SUB_EQUAL, MUL_EQUAL, AND_EQUAL };

class ArithmeticOp : public Operator {
 public:
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
  std::unique_ptr<ASTNode> clone() const override;
  ArithmeticOp(std::unique_ptr<ASTNode> dst, std::unique_ptr<ASTNode> src,
               EnumArithmeticOps op)
      : dst(std::move(dst)), src(std::move(src)), op(op) {}

  // Should be Register, MemoryAddress
  std::unique_ptr<ASTNode> dst;
  // Should be Register, MemoryAddress, Number
  std::unique_ptr<ASTNode> src;
  EnumArithmeticOps op;
};

enum class EnumShiftOps { LEFT_SHIFT, RIGHT_SHIFT };

class ShiftOp : public Operator {
 public:
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
  std::unique_ptr<ASTNode> clone() const override;
  ShiftOp(std::unique_ptr<ASTNode> dst, std::unique_ptr<ASTNode> src,
          EnumShiftOps op)
      : dst(std::move(dst)), src(std::move(src)), op(op) {}

  // Should be Register, MemoryAddress, Var
  std::unique_ptr<ASTNode> dst;
  // Should be Register, Number, Var
  std::unique_ptr<ASTNode> src;
  EnumShiftOps op;
};

enum class EnumCompareOps { LESS_THAN, LESS_EQUAL, EQUAL };

class CompareOp : public Operator {
 public:
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
  std::unique_ptr<ASTNode> clone() const override;
  CompareOp(std::unique_ptr<ASTNode> left, std::unique_ptr<ASTNode> right,
            EnumCompareOps op)
      : left(std::move(left)), right(std::move(right)), op(op) {
    auto* l = dynamic_cast<Number*>(this->left.get());
    auto* r = dynamic_cast<Number*>(this->right.get());

    if (l && r) {
      staticComputable = true;

      switch (op) {
        case EnumCompareOps::LESS_THAN:
          staticValue = (l->num < r->num) ? 1 : 0;
          break;
        case EnumCompareOps::LESS_EQUAL:
          staticValue = (l->num <= r->num) ? 1 : 0;
          break;
        case EnumCompareOps::EQUAL:
          staticValue = (l->num == r->num) ? 1 : 0;
          break;
        default:
          staticComputable = false;
          staticValue = 0;
          break;
      }
    }
  }

  bool staticComputable = false;
  int64_t staticValue = 0;

  // Should be Register or Number
  std::unique_ptr<ASTNode> left;
  // Should be Register or Number
  std::unique_ptr<ASTNode> right;
  EnumCompareOps op;
};

}  // namespace L2
