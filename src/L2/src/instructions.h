#pragma once

#include "L2.h"

namespace L2 {

// to represent Registers and Vars
using Location = std::variant<RegisterID, std::string>;

struct LocationHash {
  size_t operator()(const Location& l) const {
    return std::visit(
        [](auto&& x) {
          using T = std::decay_t<decltype(x)>;
          if constexpr (std::is_same_v<T, RegisterID>) {
            return std::hash<int>()(static_cast<int>(x));
          } else if (std::is_same_v<T, std::string>) {
            return std::hash<std::string>()(x);
          } else {
            throw std::runtime_error("Invalid type for LocationHash");
          }
        },
        l);
  }
};

using LocationSet = std::unordered_set<Location, LocationHash>;

struct GenKill {
  LocationSet gen_locations;
  LocationSet kill_locations;
};

struct InOut {
  LocationSet in_locations;
  LocationSet out_locations;
};

class Instruction : public ASTNode {
 public:
  virtual ~Instruction() = default;
  virtual GenKill generate_GEN_KILL() = 0;
};

class AssignmentInstruction : public Instruction {
 public:
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
  std::unique_ptr<ASTNode> clone() const override;
  GenKill generate_GEN_KILL() override;
  AssignmentInstruction(std::unique_ptr<ASTNode> dst,
                        std::unique_ptr<ASTNode> src)
      : dst(std::move(dst)), src(std::move(src)) {}

  // Should be Register, MemoryAddress, Var
  std::unique_ptr<ASTNode> dst;
  // Should be Register, Number, Label, MemoryAddress
  std::unique_ptr<ASTNode> src;
};

class ArithmeticInstruction : public Instruction {
 public:
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
  std::unique_ptr<ASTNode> clone() const override;
  GenKill generate_GEN_KILL() override;
  ArithmeticInstruction(std::unique_ptr<ArithmeticOp> op) : op(std::move(op)) {}

  std::unique_ptr<ArithmeticOp> op;
};

class CompareInstruction : public Instruction {
 public:
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
  std::unique_ptr<ASTNode> clone() const override;
  GenKill generate_GEN_KILL() override;
  CompareInstruction(std::unique_ptr<ASTNode> dst,
                     std::unique_ptr<CompareOp> op)
      : dst(std::move(dst)), op(std::move(op)) {}

  std::unique_ptr<ASTNode> dst;
  std::unique_ptr<CompareOp> op;
};

class ShiftInstruction : public Instruction {
 public:
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
  std::unique_ptr<ASTNode> clone() const override;
  GenKill generate_GEN_KILL() override;
  ShiftInstruction(std::unique_ptr<ShiftOp> op) : op(std::move(op)) {}
  std::unique_ptr<ShiftOp> op;
};

class ConditionalJumpInstruction : public Instruction {
 public:
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
  std::unique_ptr<ASTNode> clone() const override;
  GenKill generate_GEN_KILL() override;
  ConditionalJumpInstruction(std::unique_ptr<CompareOp> op,
                             std::unique_ptr<ASTNode> label)
      : op(std::move(op)), label(std::move(label)) {}

  std::unique_ptr<CompareOp> op;
  std::unique_ptr<ASTNode> label;
};

class LabelInstruction : public Instruction {
 public:
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
  std::unique_ptr<ASTNode> clone() const override;
  GenKill generate_GEN_KILL() override;
  LabelInstruction(std::unique_ptr<ASTNode> label) : label(std::move(label)) {}
  std::unique_ptr<ASTNode> label;
};

class JumpInstruction : public Instruction {
 public:
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
  std::unique_ptr<ASTNode> clone() const override;
  GenKill generate_GEN_KILL() override;
  JumpInstruction(std::unique_ptr<ASTNode> label) : label(std::move(label)) {}
  std::unique_ptr<ASTNode> label;
};

class ReturnInstruction : public Instruction {
 public:
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
  std::unique_ptr<ASTNode> clone() const override;
  GenKill generate_GEN_KILL() override;
};

class CallInstruction : public Instruction {
 public:
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
  std::unique_ptr<ASTNode> clone() const override;
  GenKill generate_GEN_KILL() override;
  CallInstruction(std::unique_ptr<ASTNode> target,
                  std::unique_ptr<ASTNode> numArgs)
      : target(std::move(target)), numArgs(std::move(numArgs)) {}

  std::unique_ptr<ASTNode> target;
  std::unique_ptr<ASTNode> numArgs;
};

class CallPrintInstruction : public Instruction {
 public:
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
  std::unique_ptr<ASTNode> clone() const override;
  GenKill generate_GEN_KILL() override;
};

class CallAllocateInstruction : public Instruction {
 public:
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
  std::unique_ptr<ASTNode> clone() const override;
  GenKill generate_GEN_KILL() override;
};

class CallInputInstruction : public Instruction {
 public:
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
  std::unique_ptr<ASTNode> clone() const override;
  GenKill generate_GEN_KILL() override;
};

class CallTupleErrorInstruction : public Instruction {
 public:
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
  std::unique_ptr<ASTNode> clone() const override;
  GenKill generate_GEN_KILL() override;
};

class CallTensorErrorInstruction : public Instruction {
 public:
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
  std::unique_ptr<ASTNode> clone() const override;
  GenKill generate_GEN_KILL() override;
  CallTensorErrorInstruction(std::unique_ptr<ASTNode> f) : f(std::move(f)) {}

  std::unique_ptr<ASTNode> f;
};

class IncrementInstruction : public Instruction {
 public:
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
  std::unique_ptr<ASTNode> clone() const override;
  GenKill generate_GEN_KILL() override;
  IncrementInstruction(std::unique_ptr<ASTNode> reg) : reg(std::move(reg)) {}

  std::unique_ptr<ASTNode> reg;
};

class DecrementInstruction : public Instruction {
 public:
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
  std::unique_ptr<ASTNode> clone() const override;
  GenKill generate_GEN_KILL() override;
  DecrementInstruction(std::unique_ptr<ASTNode> reg) : reg(std::move(reg)) {}

  std::unique_ptr<ASTNode> reg;
};

class LeaInstruction : public Instruction {
 public:
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
  std::unique_ptr<ASTNode> clone() const override;
  GenKill generate_GEN_KILL() override;
  LeaInstruction(std::unique_ptr<ASTNode> reg1, std::unique_ptr<ASTNode> reg2,
                 std::unique_ptr<ASTNode> reg3, std::unique_ptr<ASTNode> e)
      : reg1(std::move(reg1)),
        reg2(std::move(reg2)),
        reg3(std::move(reg3)),
        e(std::move(e)) {}

  std::unique_ptr<ASTNode> reg1;
  std::unique_ptr<ASTNode> reg2;
  std::unique_ptr<ASTNode> reg3;
  std::unique_ptr<ASTNode> e;
};

}  // namespace L2