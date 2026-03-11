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

namespace L3 {

struct Var {
  std::string to_string() const;
  std::string name;
};

struct Label {
  std::string to_string() const;
  std::string name;
};

struct Number {
  std::string to_string() const;
  long long value;
};

enum class EnumArithmeticOp { ADD, SUB, MUL, AND, LSHIFT, RSHIFT };

enum class EnumCompareOp { LT, LE, EQ, GE, GT };

enum class EnumCallee { PRINT, ALLOCATE, INPUT, TUPLE_ERROR, TENSOR_ERROR };

using Operand = std::variant<Var, Number>;
using Callee = std::variant<Var, Label, EnumCallee>;

class ArithmeticOp {
 public:
  ArithmeticOp(Operand left, Operand right, EnumArithmeticOp op)
      : left(left), right(right), op(op) {}

  std::string to_string() const;

  Operand get_left() const;
  Operand get_right() const;
  EnumArithmeticOp get_op() const;

 private:
  Operand left;
  Operand right;
  EnumArithmeticOp op;
};

class CompareOp {
 public:
  CompareOp(Operand left, Operand right, EnumCompareOp op)
      : left(left), right(right), op(op) {}

  std::string to_string() const;

  Operand get_left() const;
  Operand get_right() const;
  EnumCompareOp get_op() const;

 private:
  Operand left;
  Operand right;
  EnumCompareOp op;
};

using Operator = std::variant<ArithmeticOp, CompareOp>;

class AssignInstruction {
 public:
  AssignInstruction(
      Var dst, std::variant<Var, Number, Label, ArithmeticOp, CompareOp> src)
      : dst(dst), src(src) {}

  std::string to_string() const;

  void modify_label(std::unordered_map<std::string, std::string>&, int&);
  Var get_dst() const;
  std::variant<Var, Number, Label, ArithmeticOp, CompareOp> get_src() const;

 private:
  Var dst;
  std::variant<Var, Number, Label, ArithmeticOp, CompareOp> src;
};

class LoadInstruction {
 public:
  LoadInstruction(Var dst, Var src) : dst(dst), src(src) {}
  std::string to_string() const;

  Var get_dst() const;
  Var get_src() const;

 private:
  Var dst;
  Var src;
};

class StoreInstruction {
 public:
  StoreInstruction(Var dst, std::variant<Var, Number, Label> src)
      : dst(dst), src(src) {}

  std::string to_string() const;

  void modify_label(std::unordered_map<std::string, std::string>&, int&);

  Var get_dst() const;
  std::variant<Var, Number, Label> get_src() const;

 private:
  Var dst;
  std::variant<Var, Number, Label> src;
};

class ReturnInstruction {
 public:
  std::string to_string() const;
};

class ReturnValInstruction {
 public:
  ReturnValInstruction(Operand returnVal) : returnVal(returnVal) {}
  std::string to_string() const;

  Operand get_returnVal() const;

 private:
  Operand returnVal;
};

class LabelInstruction {
 public:
  LabelInstruction(Label label) : label(label) {}
  std::string to_string() const;
  void modify_label(std::unordered_map<std::string, std::string>&, int&);

 private:
  Label label;
};

class BranchInstruction {
 public:
  BranchInstruction(Label target) : target(target) {}
  std::string to_string() const;
  void modify_label(std::unordered_map<std::string, std::string>&, int&);
  Label get_target() const;

 private:
  Label target;
};

class ConditionalBranchInstruction {
 public:
  ConditionalBranchInstruction(Operand condition, Label target)
      : condition(condition), target(target) {}

  std::string to_string() const;
  void modify_label(std::unordered_map<std::string, std::string>&, int&);
  Operand get_condition() const;
  Label get_target() const;

 private:
  Operand condition;
  Label target;
};

class CallInstruction {
 public:
  CallInstruction() = default;

  CallInstruction(Callee callee, std::vector<Operand> args)
      : callee(callee), args(args) {}

  std::string to_string() const;
  Callee get_callee() const;
  std::vector<Operand> get_args() const;

 private:
  Callee callee;
  std::vector<Operand> args;
};

class AssignCallInstruction {
 public:
  AssignCallInstruction(Var dst, Callee callee, std::vector<Operand> args)
      : dst(dst), callee(callee), args(args) {}

  std::string to_string() const;
  Var get_dst() const;
  Callee get_callee() const;
  std::vector<Operand> get_args() const;

 private:
  Var dst;
  Callee callee;
  std::vector<Operand> args;
};

using Instruction =
    std::variant<AssignInstruction, LoadInstruction, StoreInstruction,
                 ReturnInstruction, ReturnValInstruction, LabelInstruction,
                 BranchInstruction, ConditionalBranchInstruction,
                 CallInstruction, AssignCallInstruction>;

class Function {
 public:
  Function(Label label) : label(label) {}

  std::string to_string() const;
  std::vector<Instruction>& getInstructions();
  void addInstruction(Instruction&&);
  void addVar(Var);
  Label getLabel() const;
  std::vector<Var> getVars() const;

 private:
  Label label;
  std::vector<Var> vars;
  std::vector<Instruction> instructions;
};

}  // namespace L3