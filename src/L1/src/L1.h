#pragma once

#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <variant>
#include <vector>

namespace L1 {

class ASTNode {
 public:
  virtual ~ASTNode() = default;
  virtual std::string to_string() const = 0;
  virtual void generate_code(std::ofstream&) const = 0;
};

struct Label : public ASTNode {
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
  Label() = default;
  Label(const std::string& name) : name(name) {}
  std::string name;
};

struct Number : public ASTNode {
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
  Number(int64_t num) : num(num) {}
  int64_t num;
};

enum RegisterID {
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
  void generate_code_8bit(std::ofstream&) const;
  Register(const RegisterID& id) : id(id) {}
  RegisterID id;
};

struct MemoryAddress : public ASTNode {
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
  MemoryAddress(std::unique_ptr<Register> reg, std::unique_ptr<Number> offset)
      : reg(std::move(reg)), offset(std::move(offset)) {}
  std::unique_ptr<Register> reg;
  std::unique_ptr<Number> offset;
};

class Operator : public ASTNode {
 public:
  virtual ~Operator() = default;
};

enum EnumArithmeticOps { ADD_EQUAL, SUB_EQUAL, MUL_EQUAL, AND_EQUAL };

class ArithmeticOp : public Operator {
 public:
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
  ArithmeticOp(std::unique_ptr<ASTNode> dst, std::unique_ptr<ASTNode> src,
               EnumArithmeticOps op)
      : dst(std::move(dst)), src(std::move(src)), op(op) {}

 private:
  // Should be Register, MemoryAddress
  std::unique_ptr<ASTNode> dst;
  // Should be Register, MemoryAddress, Number
  std::unique_ptr<ASTNode> src;
  EnumArithmeticOps op;
};

enum EnumShiftOps { LEFT_SHIFT, RIGHT_SHIFT };

class ShiftOp : public Operator {
 public:
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
  ShiftOp(std::unique_ptr<ASTNode> dst, std::unique_ptr<ASTNode> src,
          EnumShiftOps op)
      : dst(std::move(dst)), src(std::move(src)), op(op) {}

 private:
  // Should be Register, MemoryAddress
  std::unique_ptr<ASTNode> dst;
  // Should be Register, Number
  std::unique_ptr<ASTNode> src;
  EnumShiftOps op;
};

enum EnumCompareOps { LESS_THAN, LESS_EQUAL, EQUAL };

class CompareOp : public Operator {
 public:
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;

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

class Instruction : public ASTNode {
 public:
  virtual ~Instruction() = default;
};

class AssignmentInstruction : public Instruction {
 public:
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
  AssignmentInstruction(std::unique_ptr<ASTNode> dst,
                        std::unique_ptr<ASTNode> src)
      : dst(std::move(dst)), src(std::move(src)) {}

 private:
  // Should be Register, MemoryAddress
  std::unique_ptr<ASTNode> dst;
  // Should be Register, Number, Label, MemoryAddress
  std::unique_ptr<ASTNode> src;
};

class ArithmeticInstruction : public Instruction {
 public:
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
  ArithmeticInstruction(std::unique_ptr<ArithmeticOp> op) : op(std::move(op)) {}

 private:
  std::unique_ptr<ArithmeticOp> op;
};

class CompareInstruction : public Instruction {
 public:
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
  CompareInstruction(std::unique_ptr<ASTNode> dst,
                     std::unique_ptr<CompareOp> op)
      : dst(std::move(dst)), op(std::move(op)) {}

 private:
  std::unique_ptr<ASTNode> dst;
  std::unique_ptr<CompareOp> op;
};

class ShiftInstruction : public Instruction {
 public:
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
  ShiftInstruction(std::unique_ptr<ShiftOp> op) : op(std::move(op)) {}

 private:
  std::unique_ptr<ShiftOp> op;
};

class ConditionalJumpInstruction : public Instruction {
 public:
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
  ConditionalJumpInstruction(std::unique_ptr<CompareOp> op,
                             std::unique_ptr<ASTNode> label)
      : op(std::move(op)), label(std::move(label)) {}

 private:
  std::unique_ptr<CompareOp> op;
  std::unique_ptr<ASTNode> label;
};

class LabelInstruction : public Instruction {
 public:
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
  LabelInstruction(std::unique_ptr<ASTNode> label) : label(std::move(label)) {}

 private:
  std::unique_ptr<ASTNode> label;
};

class JumpInstruction : public Instruction {
 public:
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
  JumpInstruction(std::unique_ptr<ASTNode> label) : label(std::move(label)) {}

 private:
  std::unique_ptr<ASTNode> label;
};

class ReturnInstruction : public Instruction {
 public:
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
};

class CallInstruction : public Instruction {
 public:
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
  CallInstruction(std::unique_ptr<ASTNode> target, std::unique_ptr<ASTNode> numArgs)
      : target(std::move(target)), numArgs(std::move(numArgs)) {}

 private:
  std::unique_ptr<ASTNode> target;
  std::unique_ptr<ASTNode> numArgs;
};

class CallPrintInstruction : public Instruction {
 public:
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
};

class CallAllocateInstruction : public Instruction {
 public:
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
};

class CallInputInstruction : public Instruction {
 public:
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
};

class CallTupleErrorInstruction : public Instruction {
 public:
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
};

class CallTensorErrorInstruction : public Instruction {
 public:
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
  CallTensorErrorInstruction(std::unique_ptr<ASTNode> f) : f(std::move(f)) {}

 private:
  std::unique_ptr<ASTNode> f;
};

class IncrementInstruction : public Instruction {
 public:
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
  IncrementInstruction(std::unique_ptr<ASTNode> reg) : reg(std::move(reg)) {}

 private:
  std::unique_ptr<ASTNode> reg;
};

class DecrementInstruction : public Instruction {
 public:
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
  DecrementInstruction(std::unique_ptr<ASTNode> reg) : reg(std::move(reg)) {}

 private:
  std::unique_ptr<ASTNode> reg;
};

class LeaInstruction : public Instruction {
 public:
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
  LeaInstruction(std::unique_ptr<ASTNode> reg1, std::unique_ptr<ASTNode> reg2,
                 std::unique_ptr<ASTNode> reg3, std::unique_ptr<ASTNode> e)
      : reg1(std::move(reg1)),
        reg2(std::move(reg2)),
        reg3(std::move(reg3)),
        e(std::move(e)) {}

 private:
  std::unique_ptr<ASTNode> reg1;
  std::unique_ptr<ASTNode> reg2;
  std::unique_ptr<ASTNode> reg3;
  std::unique_ptr<ASTNode> e;
};

class Function : public ASTNode {
 public:
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;

  Label name;
  int64_t numArgs;
  int64_t numLocal;
  std::vector<std::unique_ptr<Instruction>> instructions;
};

class Program : public ASTNode {
 public:
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;

  Program() {
    entryPoint = Label("");
    functions = std::vector<std::unique_ptr<Function>>();
  };

  Label entryPoint;
  std::vector<std::unique_ptr<Function>> functions;
};

}  // namespace L1
