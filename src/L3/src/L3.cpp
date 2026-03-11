#include "L3.h"

#include <iostream>
#include <queue>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <string>
#include <utility>

#include "utils.h"

namespace L3 {

std::string Var::to_string() const { return name; }

std::string Label::to_string() const { return name; }

std::string Number::to_string() const { return std::to_string(value); }

std::string ArithmeticOp::to_string() const {
  std::ostringstream out;
  out << std::visit([](auto&& x) { return x.to_string(); }, left) << " ";
  switch (op) {
    case EnumArithmeticOp::ADD:
      out << "+";
      break;
    case EnumArithmeticOp::SUB:
      out << "-";
      break;
    case EnumArithmeticOp::MUL:
      out << "*";
      break;
    case EnumArithmeticOp::AND:
      out << "&";
      break;
    case EnumArithmeticOp::LSHIFT:
      out << "<<";
      break;
    case EnumArithmeticOp::RSHIFT:
      out << ">>";
      break;
    default:
      throw std::runtime_error("Invalid EnumArithmeticOp");
  }
  out << " " << std::visit([](auto&& x) { return x.to_string(); }, right);
  return out.str();
}

Operand ArithmeticOp::get_left() const { return left; }

Operand ArithmeticOp::get_right() const { return right; }

EnumArithmeticOp ArithmeticOp::get_op() const { return op; }

std::string CompareOp::to_string() const {
  std::ostringstream out;
  out << std::visit([](auto&& x) { return x.to_string(); }, left)
      << " ";
  switch (op) {
    case EnumCompareOp::LT:
      out << "<";
      break;
    case EnumCompareOp::LE:
      out << "<=";
      break;
    case EnumCompareOp::EQ:
      out << "=";
      break;
    case EnumCompareOp::GE:
      out << ">=";
      break;
    case EnumCompareOp::GT:
      out << ">";
      break;
    default:
      throw std::runtime_error("Invalid EnumCompareOp");
  }
  out << " " << std::visit([](auto&& x) { return x.to_string(); }, right);
  return out.str();
}

Operand CompareOp::get_left() const { return left; }

Operand CompareOp::get_right() const { return right; }

EnumCompareOp CompareOp::get_op() const { return op; }

std::string AssignInstruction::to_string() const {
  std::ostringstream out;
  out << dst.to_string() << " <- ";
  out << std::visit([](auto&& x) { return x.to_string(); }, src);
  return out.str();
}

void AssignInstruction::modify_label(
    std::unordered_map<std::string, std::string>& L3_to_L2,
    int& globalCounter) {
  if (Label* label = std::get_if<Label>(&src)) {

    // Check if it is not a function label
    if (label->name[0] != ':') return;

    if (L3_to_L2.find(label->name) == L3_to_L2.end()) {
      L3_to_L2[label->name] = ":__label__" + std::to_string(globalCounter);
      ++globalCounter;
    }
    label->name = L3_to_L2[label->name];
  }
}

Var AssignInstruction::get_dst() const { return dst; }

std::variant<Var, Number, Label, ArithmeticOp, CompareOp>
AssignInstruction::get_src() const {
  return src;
}

std::string LoadInstruction::to_string() const {
  std::ostringstream out;
  out << dst.to_string() << " <- load " << src.to_string();
  return out.str();
}

Var LoadInstruction::get_dst() const { return dst; }

Var LoadInstruction::get_src() const { return src; }

std::string StoreInstruction::to_string() const {
  std::ostringstream out;
  out << "store " << dst.to_string();
  out << " <- " << std::visit([](auto&& x) { return x.to_string(); }, src);
  return out.str();
}

void StoreInstruction::modify_label(
    std::unordered_map<std::string, std::string>& L3_to_L2,
    int& globalCounter) {
  
  if (Label* label = std::get_if<Label>(&src)) {

    // Checking not function label
    if (label->name[0] != ':') return;

    if (L3_to_L2.find(label->name) == L3_to_L2.end()) {
      L3_to_L2[label->name] = ":__label__" + std::to_string(globalCounter);
      ++globalCounter;
    }
    label->name = L3_to_L2[label->name];
  }
}

Var StoreInstruction::get_dst() const { return dst; }

std::variant<Var, Number, Label> StoreInstruction::get_src() const {
  return src;
}

std::string ReturnInstruction::to_string() const { return "return"; }

std::string ReturnValInstruction::to_string() const {
  std::ostringstream out;
  out << "return "
      << std::visit([](auto&& x) { return x.to_string(); }, returnVal);
  return out.str();
}

Operand ReturnValInstruction::get_returnVal() const { return returnVal; }

std::string LabelInstruction::to_string() const { return label.to_string(); }

void LabelInstruction::modify_label(
    std::unordered_map<std::string, std::string>& L3_to_L2,
    int& globalCounter) {

  if (L3_to_L2.find(label.name) == L3_to_L2.end()) {
    L3_to_L2[label.name] = ":__label__" + std::to_string(globalCounter);
    ++globalCounter;
  }
  label.name = L3_to_L2[label.name];
}

std::string BranchInstruction::to_string() const {
  std::ostringstream out;
  out << "br ";
  out << target.to_string();
  return out.str();
}

void BranchInstruction::modify_label(
    std::unordered_map<std::string, std::string>& L3_to_L2,
    int& globalCounter) {

  if (L3_to_L2.find(target.name) == L3_to_L2.end()) {
    L3_to_L2[target.name] = ":__label__" + std::to_string(globalCounter);
    ++globalCounter;
  }
  target.name = L3_to_L2[target.name];
}

Label BranchInstruction::get_target() const { return target; }

std::string ConditionalBranchInstruction::to_string() const {
  std::ostringstream out;
  out << "br " << std::visit([](auto&& x) { return x.to_string(); }, condition);
  out << " " << target.to_string();
  return out.str();
}

void ConditionalBranchInstruction::modify_label(
    std::unordered_map<std::string, std::string>& L3_to_L2,
    int& globalCounter) {

  if (L3_to_L2.find(target.name) == L3_to_L2.end()) {
    L3_to_L2[target.name] = ":__label__" + std::to_string(globalCounter);
    ++globalCounter;
  }
  target.name = L3_to_L2[target.name];
}

Operand ConditionalBranchInstruction::get_condition() const {
  return condition;
}

Label ConditionalBranchInstruction::get_target() const { return target; }

std::string CallInstruction::to_string() const {
  std::ostringstream out;
  out << "call ";
  if (std::holds_alternative<EnumCallee>(callee)) {
    EnumCallee enumCallee = std::get<EnumCallee>(callee);
    switch (enumCallee) {
      case EnumCallee::PRINT:
        out << "print";
        break;
      case EnumCallee::ALLOCATE:
        out << "allocate";
        break;
      case EnumCallee::INPUT:
        out << "input";
        break;
      case EnumCallee::TUPLE_ERROR:
        out << "tuple-error";
        break;
      case EnumCallee::TENSOR_ERROR:
        out << "tensor-error";
        break;
      default:
        throw std::runtime_error(
            "CallInstruction::to_string: Invalid EnumCallee");
    }
  } else if (const Var* var = std::get_if<Var>(&callee)) {
    out << var->to_string();
  } else if (const Label* label = std::get_if<Label>(&callee)) {
    out << label->to_string();
  } else {
    throw std::runtime_error("CallInstruction::to_string: Invalid callee type");
  }
  out << " (";

  int numArgs = args.size();
  for (int i = 0; i < numArgs; ++i) {
    out << std::visit([](auto&& x) { return x.to_string(); }, args[i]);
    if (i < numArgs - 1) {
      out << ", ";
    }
  }

  out << ")";

  return out.str();
}

Callee CallInstruction::get_callee() const {
  return callee;
}

std::vector<Operand> CallInstruction::get_args() const {
  return args;
}

std::string AssignCallInstruction::to_string() const {
  std::ostringstream out;

  out << dst.to_string();
  out << " <- call ";

  if (std::holds_alternative<EnumCallee>(callee)) {
    EnumCallee enumCallee = std::get<EnumCallee>(callee);
    switch (enumCallee) {
      case EnumCallee::PRINT:
        out << "print";
        break;
      case EnumCallee::ALLOCATE:
        out << "allocate";
        break;
      case EnumCallee::INPUT:
        out << "input";
        break;
      case EnumCallee::TUPLE_ERROR:
        out << "tuple-error";
        break;
      case EnumCallee::TENSOR_ERROR:
        out << "tensor-error";
        break;
      default:
        throw std::runtime_error(
            "CallInstruction::to_string: Invalid EnumCallee");
    }
  } else if (const Var* var = std::get_if<Var>(&callee)) {
    out << var->to_string();
  } else if (const Label* label = std::get_if<Label>(&callee)) {
    out << label->to_string();
  } else {
    throw std::runtime_error("CallInstruction::to_string: Invalid callee type");
  }
  out << " (";

  int numArgs = args.size();
  for (int i = 0; i < numArgs; ++i) {
    out << std::visit([](auto&& x) { return x.to_string(); }, args[i]);
    if (i < numArgs - 1) {
      out << ", ";
    }
  }

  out << ")";

  return out.str();
}

Var AssignCallInstruction::get_dst() const {
  return dst;
}

Callee AssignCallInstruction::get_callee() const {
  return callee;
}

std::vector<Operand> AssignCallInstruction::get_args() const {
  return args;
}

std::string Function::to_string() const {
  std::ostringstream out;
  out << "define " << label.to_string() << "(";

  int numVars = vars.size();
  for (int i = 0; i < numVars; ++i) {
    out << vars[i].to_string();
    if (i < numVars - 1) {
      out << ", ";
    }
  }
  out << ") {\n";

  for (const Instruction& instruction : instructions) {
    out << "\t";
    std::visit([&out](auto&& instruction) { out << instruction.to_string(); },
               instruction);
    out << "\n";
  }

  out << "}";
  return out.str();
}

std::vector<Instruction>& Function::getInstructions() { return instructions; }

void Function::addInstruction(Instruction&& instruction) {
  instructions.push_back(std::move(instruction));
}

void Function::addVar(Var var) { vars.insert(vars.begin(), var); }

Label Function::getLabel() const { return label; }

std::vector<Var> Function::getVars() const { return vars; }
}  // namespace L3