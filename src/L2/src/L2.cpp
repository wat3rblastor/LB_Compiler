#include "L2.h"

#include <iostream>
#include <queue>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <string>
#include <utility>

#include "utils.h"

namespace L2 {

std::string Label::to_string() const { return name; }

void Label::generate_code(std::ofstream& outputFile) const {
  outputFile << name;
}

std::unique_ptr<ASTNode> Label::clone() const {
  return std::make_unique<Label>(name);
}

std::string Number::to_string() const { return std::to_string(num); }

void Number::generate_code(std::ofstream& outputFile) const {
  outputFile << num;
}

std::unique_ptr<ASTNode> Number::clone() const {
  return std::make_unique<Number>(num);
}

std::string Var::to_string() const { return name; }

void Var::generate_code(std::ofstream& outputFile) const { outputFile << name; }

std::unique_ptr<ASTNode> Var::clone() const {
  return std::make_unique<Var>(name);
}

std::string Register::to_string() const {
  switch (id) {
    case RegisterID::rdi:
      return "rdi";
    case RegisterID::rsi:
      return "rsi";
    case RegisterID::rdx:
      return "rdx";
    case RegisterID::rcx:
      return "rcx";
    case RegisterID::r8:
      return "r8";
    case RegisterID::r9:
      return "r9";
    case RegisterID::rax:
      return "rax";
    case RegisterID::rbx:
      return "rbx";
    case RegisterID::rbp:
      return "rbp";
    case RegisterID::r10:
      return "r10";
    case RegisterID::r11:
      return "r11";
    case RegisterID::r12:
      return "r12";
    case RegisterID::r13:
      return "r13";
    case RegisterID::r14:
      return "r14";
    case RegisterID::r15:
      return "r15"; 
    case RegisterID::rsp:
      return "rsp";
    default:
      throw std::runtime_error("Undefined RegisterID in register");
  }
}

void Register::generate_code(std::ofstream& outputFile) const {
  switch (id) {
    case RegisterID::rdi:
      outputFile << "rdi";
      break;
    case RegisterID::rsi:
      outputFile << "rsi";
      break;
    case RegisterID::rdx:
      outputFile << "rdx";
      break;
    case RegisterID::rcx:
      outputFile <<  "rcx";
      break;
    case RegisterID::r8:
      outputFile << "r8";
      break;
    case RegisterID::r9:
      outputFile <<  "r9";
      break;
    case RegisterID::rax:
      outputFile << "rax";
      break;
    case RegisterID::rbx:
      outputFile << "rbx";
      break;
    case RegisterID::rbp:
      outputFile << "rbp";
      break;
    case RegisterID::r10:
      outputFile << "r10";
      break;
    case RegisterID::r11:
      outputFile << "r11";
      break;
    case RegisterID::r12:
      outputFile << "r12";
      break;
    case RegisterID::r13:
      outputFile << "r13";
      break;
    case RegisterID::r14:
      outputFile << "r14";
      break;
    case RegisterID::r15:
      outputFile << "r15"; 
      break;
    case RegisterID::rsp:
      outputFile << "rsp";
      break;
    default:
      throw std::runtime_error("Undefined RegisterID in register");
  }
}

std::unique_ptr<ASTNode> Register::clone() const {
  return std::make_unique<Register>(id);
}

void Register::generate_code_8bit(std::ofstream& outputFile) const {
  switch (id) {
    case RegisterID::rdi:
      outputFile << "%dil";
      break;
    case RegisterID::rsi:
      outputFile << "%sil";
      break;
    case RegisterID::rdx:
      outputFile << "%dl";
      break;
    case RegisterID::rcx:
      outputFile << "%cl";
      break;
    case RegisterID::r8:
      outputFile << "%r8b";
      break;
    case RegisterID::r9:
      outputFile << "%r9b";
      break;
    case RegisterID::rax:
      outputFile << "%al";
      break;
  }
}

std::string MemoryAddress::to_string() const {
  std::ostringstream out;
  out << "mem " << reg->to_string() << " " << offset->to_string();
  return out.str();
}

void MemoryAddress::generate_code(std::ofstream& outputFile) const {
  outputFile << "mem ";
  reg->generate_code(outputFile);
  outputFile << " ";
  offset->generate_code(outputFile);
}

std::unique_ptr<ASTNode> MemoryAddress::clone() const {
  return std::make_unique<MemoryAddress>(reg->clone(), offset->clone());
}

std::string StackArg::to_string() const { 
  std::ostringstream out;
  out << "stack-arg " << offset->to_string();
  return out.str();
}

void StackArg::generate_code(std::ofstream& outputFile) const {
  // I really hope I am allowed to have -0
  outputFile << "mem rsp -";
  offset->generate_code(outputFile); 
}

std::unique_ptr<ASTNode> StackArg::clone() const {
  return std::make_unique<StackArg>(offset->clone());
}

std::string ArithmeticOp::to_string() const {
  std::ostringstream out;
  std::string str_op;
  switch (op) {
    case EnumArithmeticOps::ADD_EQUAL:
      str_op = "+=";
      break;
    case EnumArithmeticOps::SUB_EQUAL:
      str_op = "-=";
      break;
    case EnumArithmeticOps::MUL_EQUAL:
      str_op = "*=";
      break;
    case EnumArithmeticOps::AND_EQUAL:
      str_op = "&=";
      break;
  }
  out << dst->to_string();
  out << " " << str_op << " ";
  out << src->to_string();
  return out.str();
}

void ArithmeticOp::generate_code(std::ofstream& outputFile) const {
  std::string str_op;
  switch (op) {
    case EnumArithmeticOps::ADD_EQUAL:
      str_op = "+=";
      break;
    case EnumArithmeticOps::SUB_EQUAL:
      str_op = "-=";
      break;
    case EnumArithmeticOps::MUL_EQUAL:
      str_op = "*=";
      break;
    case EnumArithmeticOps::AND_EQUAL:
      str_op = "&=";
      break;
  }
  dst->generate_code(outputFile);
  outputFile << " " << str_op << " ";
  src->generate_code(outputFile);
}

std::unique_ptr<ASTNode> ArithmeticOp::clone() const {
  return std::make_unique<ArithmeticOp>(dst->clone(), src->clone(), op);
}

std::string ShiftOp::to_string() const {
  std::ostringstream out;
  std::string str_op;
  if (op == EnumShiftOps::LEFT_SHIFT) {
    str_op = "<<=";
  } else if (op == EnumShiftOps::RIGHT_SHIFT) {
    str_op = ">>=";
  } else {
    throw std::runtime_error("ShiftOp::to_string() invalid op");
  }
  out << dst->to_string();
  out << " " << str_op << " ";
  out << src->to_string();
  return out.str();
}

void ShiftOp::generate_code(std::ofstream& outputFile) const {
  std::string str_op;
  if (op == EnumShiftOps::LEFT_SHIFT) {
    str_op = "<<=";
  } else if (op == EnumShiftOps::RIGHT_SHIFT) {
    str_op = ">>=";
  } else {
    throw std::runtime_error("ShiftOp::generate_code() invalid op");
  }
  outputFile << dst->to_string();
  outputFile << " " << str_op << " ";
  outputFile << src->to_string();
}

std::unique_ptr<ASTNode> ShiftOp::clone() const {
  return std::make_unique<ShiftOp>(dst->clone(), src->clone(), op);
}

std::string CompareOp::to_string() const {
  std::string str_op;
  std::ostringstream out;
  switch (op) {
    case EnumCompareOps::LESS_THAN:
      str_op = "<";
      break;
    case EnumCompareOps::LESS_EQUAL:
      str_op = "<=";
      break;
    case EnumCompareOps::EQUAL:
      str_op = "=";
      break;
    default:
      throw std::runtime_error("CompareOp::to_string: Invalid op");
  }
  out << left->to_string();
  out << " " << str_op << " ";
  out << right->to_string();
  return out.str();
}

void CompareOp::generate_code(std::ofstream& outputFile) const {
  std::string str_op;
  switch (op) {
    case EnumCompareOps::LESS_THAN:
      str_op = "<";
      break;
    case EnumCompareOps::LESS_EQUAL:
      str_op = "<=";
      break;
    case EnumCompareOps::EQUAL:
      str_op = "=";
      break;
    default:
      throw std::runtime_error("CompareOp::to_string: Invalid op");
  }
  outputFile << left->to_string();
  outputFile << " " << str_op << " ";
  outputFile << right->to_string();
}

std::unique_ptr<ASTNode> CompareOp::clone() const {
  return std::make_unique<CompareOp>(left->clone(), right->clone(), op);
}

}  // namespace L2