#include <algorithm>
#include <iostream>
#include <sstream>
#include <stack>
#include <stdexcept>

#include "instruction.h"
#include "utils.h"

namespace LA {

std::string NameDefInstruction::to_string() const {
  std::ostringstream out;
  out << type->to_string();
  out << " ";
  if (dynamic_cast<Type*>(name.get())) {
    throw std::runtime_error("Is a type");
  }
  if (dynamic_cast<Number*>(name.get())) {
    throw std::runtime_error("is a number");
  }
  if (dynamic_cast<Label*>(name.get())) {
    throw std::runtime_error("is a label");
  }
  if (dynamic_cast<Parameter*>(name.get())) {
    throw std::runtime_error("is a parameter");
  }
  if (!dynamic_cast<Name*>(name.get())) {
    throw std::runtime_error("Not a name");
  }
  out << name->to_string();
  return out.str();
}

void NameDefInstruction::generate_code(std::ofstream& outputFile) const {
  outputFile << " ";
  type->generate_code(outputFile);
  outputFile << " ";
  name->generate_code(outputFile);
  outputFile << "\n";
}

const ASTNode& NameDefInstruction::get_type() const {
  return *type;
}

const ASTNode& NameDefInstruction::get_name() const {
  return *name;
}

NameDefInstruction::NameDefInstruction(std::unique_ptr<ASTNode> type, std::unique_ptr<ASTNode> name)
  : type(std::move(type)), name(std::move(name)) {}

std::string NameNumAssignInstruction::to_string() const {
  std::ostringstream out;
  out << dst->to_string();
  out << " <- ";
  out << src->to_string();
  return out.str();
}

void NameNumAssignInstruction::generate_code(std::ofstream& outputFile) const {
  outputFile << " ";
  dst->generate_code(outputFile);
  outputFile << " <- ";
  src->generate_code(outputFile);
  outputFile << "\n";
}

NameNumAssignInstruction::NameNumAssignInstruction(std::unique_ptr<ASTNode> dst, std::unique_ptr<ASTNode> src)
  : dst(std::move(dst)), src(std::move(src)) {}

const ASTNode& NameNumAssignInstruction::get_dst() const {
  return *dst;
}

const ASTNode& NameNumAssignInstruction::get_src() const {
  return *src;
}

std::string OperatorAssignInstruction::to_string() const {
  std::ostringstream out;
  out << dst->to_string();
  out << " <- ";
  out << src->to_string();
  return out.str();
}

void OperatorAssignInstruction::generate_code(std::ofstream& outputFile) const {
  outputFile << " ";
  dst->generate_code(outputFile);
  outputFile << " <- ";
  src->generate_code(outputFile);
  outputFile << "\n";
}

OperatorAssignInstruction::OperatorAssignInstruction(std::unique_ptr<ASTNode> dst, std::unique_ptr<ASTNode> src)
  : dst(std::move(dst)), src(std::move(src)) {}

const ASTNode& OperatorAssignInstruction::get_dst() const {
  return *dst;
}

const ASTNode& OperatorAssignInstruction::get_src() const {
  return *src;
}

std::string LabelInstruction::to_string() const {
  return label->to_string();
}

void LabelInstruction::generate_code(std::ofstream& outputFile) const {
  label->generate_code(outputFile);
  outputFile << "\n";
}

const ASTNode& LabelInstruction::get_label() const {
  return *label;
}

LabelInstruction::LabelInstruction(std::unique_ptr<ASTNode> label)
  : label(std::move(label)) {}

std::string BranchInstruction::to_string() const {
  std::ostringstream out;
  out << "br ";
  out << target->to_string();
  return out.str();
}

void BranchInstruction::generate_code(std::ofstream& outputFile) const {
  outputFile << " br ";
  target->generate_code(outputFile);
  outputFile << "\n";
}

const ASTNode& BranchInstruction::get_target() const {
  return *target;
}

BranchInstruction::BranchInstruction(std::unique_ptr<ASTNode> target)
  : target(std::move(target)) {}

std::string ConditionalBranchInstruction::to_string() const {
  std::ostringstream out;
  out << "br ";
  out << condition->to_string();
  out << " " << dst1->to_string();
  out << " " << dst2->to_string();
  return out.str();
}

void ConditionalBranchInstruction::generate_code(std::ofstream& outputFile) const {
  outputFile << " br ";
  condition->generate_code(outputFile);
  outputFile << " ";
  dst1->generate_code(outputFile);
  outputFile << " ";
  dst2->generate_code(outputFile);
  outputFile << "\n";
}

const ASTNode& ConditionalBranchInstruction::get_condition() const {
  return *condition;
}

const ASTNode& ConditionalBranchInstruction::get_true_target() const {
  return *dst1;
}

const ASTNode& ConditionalBranchInstruction::get_false_target() const {
  return *dst2;
}

ConditionalBranchInstruction::ConditionalBranchInstruction(
  std::unique_ptr<ASTNode> condition,
  std::unique_ptr<ASTNode> dst1,
  std::unique_ptr<ASTNode> dst2)
  : condition(std::move(condition)), dst1(std::move(dst1)),
    dst2(std::move(dst2)) {}

std::string ReturnInstruction::to_string() const {
  return "return";
}

void ReturnInstruction::generate_code(std::ofstream& outputFile) const {
  outputFile << " return\n";
}

std::string ReturnValueInstruction::to_string() const {
  std::ostringstream out;
  out << "return ";
  out << returnValue->to_string();
  return out.str();
}

void ReturnValueInstruction::generate_code(std::ofstream& outputFile) const {
  outputFile << " return ";
  returnValue->generate_code(outputFile);
  outputFile << "\n";
}

ReturnValueInstruction::ReturnValueInstruction(std::unique_ptr<ASTNode> returnValue)
  : returnValue(std::move(returnValue)) {}

const ASTNode& ReturnValueInstruction::get_return_value() const {
  return *returnValue;
}

std::string IndexAssignInstruction::to_string() const {
  std::ostringstream out;
  out << dst->to_string();
  out << " <- ";
  out << src->to_string();
  return out.str();
}

void IndexAssignInstruction::generate_code(std::ofstream& outputFile) const {
  outputFile << " ";
  dst->generate_code(outputFile);
  outputFile << " <- ";
  src->generate_code(outputFile);
  outputFile << "\n";
}

const ASTNode& IndexAssignInstruction::get_dst() const {
  return *dst;
}

const ASTNode& IndexAssignInstruction::get_src() const {
  return *src;
}

IndexAssignInstruction::IndexAssignInstruction(std::unique_ptr<ASTNode> dst, std::unique_ptr<ASTNode> src)
  : dst(std::move(dst)), src(std::move(src)) {}

std::string LengthAssignInstruction::to_string() const {
  std::ostringstream out;
  out << dst->to_string();
  out << " <- ";
  out << src->to_string();
  return out.str();
}

void LengthAssignInstruction::generate_code(std::ofstream& outputFile) const {
  outputFile << " ";
  dst->generate_code(outputFile);
  outputFile << " <- ";
  src->generate_code(outputFile);
  outputFile << "\n";
}

const ASTNode& LengthAssignInstruction::get_dst() const {
  return *dst;
}

const ASTNode& LengthAssignInstruction::get_src() const {
  return *src;
}

LengthAssignInstruction::LengthAssignInstruction(std::unique_ptr<ASTNode> dst, std::unique_ptr<ASTNode> src)
  : dst(std::move(dst)), src(std::move(src)) {}

std::string CallInstruction::to_string() const {
  return functionCall->to_string();
}

void CallInstruction::generate_code(std::ofstream& outputFile) const {
  outputFile << " ";
  functionCall->generate_code(outputFile);
  outputFile << "\n";
}

CallInstruction::CallInstruction(std::unique_ptr<ASTNode> functionCall)
  : functionCall(std::move(functionCall)) {}

const ASTNode& CallInstruction::get_function_call() const {
  return *functionCall;
}
  
std::string CallAssignInstruction::to_string() const {
  std::ostringstream out;
  out << dst->to_string();
  out << " <- ";
  out << functionCall->to_string();
  return out.str();
}

void CallAssignInstruction::generate_code(std::ofstream& outputFile) const {
  outputFile << " ";
  dst->generate_code(outputFile);
  outputFile << " <- ";
  functionCall->generate_code(outputFile);
  outputFile << "\n";
}

CallAssignInstruction::CallAssignInstruction(std::unique_ptr<ASTNode> dst, std::unique_ptr<ASTNode> functionCall)
  : dst(std::move(dst)), functionCall(std::move(functionCall)) {}

const ASTNode& CallAssignInstruction::get_dst() const {
  return *dst;
}

const ASTNode& CallAssignInstruction::get_function_call() const {
  return *functionCall;
}

std::string NewArrayAssignInstruction::to_string() const {
  std::ostringstream out;
  out << dst->to_string();
  out << " <- ";
  out << newArray->to_string();
  return out.str();
}

void NewArrayAssignInstruction::generate_code(std::ofstream& outputFile) const {
  outputFile << " ";
  dst->generate_code(outputFile);
  outputFile << " <- ";
  newArray->generate_code(outputFile);
  outputFile << "\n";
}

NewArrayAssignInstruction::NewArrayAssignInstruction(std::unique_ptr<ASTNode> dst, std::unique_ptr<ASTNode> newArray)
  : dst(std::move(dst)), newArray(std::move(newArray)) {}

const ASTNode& NewArrayAssignInstruction::get_dst() const {
  return *dst;
}

const ASTNode& NewArrayAssignInstruction::get_new_array() const {
  return *newArray;
}

std::string NewTupleAssignInstruction::to_string() const {
  std::ostringstream out;
  out << dst->to_string();
  out << " <- ";
  out << newTuple->to_string();
  return out.str();
}

void NewTupleAssignInstruction::generate_code(std::ofstream& outputFile) const {
  outputFile << " ";
  dst->generate_code(outputFile);
  outputFile << " <- ";
  newTuple->generate_code(outputFile);
  outputFile << "\n";
}

NewTupleAssignInstruction::NewTupleAssignInstruction(std::unique_ptr<ASTNode> dst, std::unique_ptr<ASTNode> newTuple)
  : dst(std::move(dst)), newTuple(std::move(newTuple)) {}

const ASTNode& NewTupleAssignInstruction::get_dst() const {
  return *dst;
}

const ASTNode& NewTupleAssignInstruction::get_new_tuple() const {
  return *newTuple;
}
 
}
