#include <algorithm>
#include <iostream>
#include <sstream>
#include <stack>
#include <stdexcept>

#include "instruction.h"
#include "utils.h"

namespace LB {

std::string NameDefInstruction::to_string() const {
  return type->to_string() + " " + name->to_string();
}

void NameDefInstruction::generate_code(std::ofstream& outputFile) const {
  outputFile << "  ";
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

NameDefInstruction::NameDefInstruction(std::unique_ptr<ASTNode> type,
                                       std::unique_ptr<ASTNode> name)
    : type(std::move(type)), name(std::move(name)) {}

std::string NameNumAssignInstruction::to_string() const {
  return dst->to_string() + " <- " + src->to_string();
}

void NameNumAssignInstruction::generate_code(std::ofstream& outputFile) const {
  outputFile << "  ";
  dst->generate_code(outputFile);
  outputFile << " <- ";
  src->generate_code(outputFile);
  outputFile << "\n";
}

const ASTNode& NameNumAssignInstruction::get_dst() const {
  return *dst;
}

const ASTNode& NameNumAssignInstruction::get_src() const {
  return *src;
}

NameNumAssignInstruction::NameNumAssignInstruction(
    std::unique_ptr<ASTNode> dst, std::unique_ptr<ASTNode> src)
    : dst(std::move(dst)), src(std::move(src)) {}

std::string OperatorAssignInstruction::to_string() const {
  return dst->to_string() + " <- " + src->to_string();
}

void OperatorAssignInstruction::generate_code(std::ofstream& outputFile) const {
  outputFile << "  ";
  dst->generate_code(outputFile);
  outputFile << " <- ";
  src->generate_code(outputFile);
  outputFile << "\n";
}

const ASTNode& OperatorAssignInstruction::get_dst() const {
  return *dst;
}

const ASTNode& OperatorAssignInstruction::get_src() const {
  return *src;
}

OperatorAssignInstruction::OperatorAssignInstruction(
    std::unique_ptr<ASTNode> dst, std::unique_ptr<ASTNode> src)
    : dst(std::move(dst)), src(std::move(src)) {}

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

std::string ConditionalBranchInstruction::to_string() const {
  return "if (" + condition->to_string() + ") " + dst1->to_string() + " " +
         dst2->to_string();
}

void ConditionalBranchInstruction::generate_code(std::ofstream& outputFile) const {
  outputFile << "  br ";
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
    std::unique_ptr<ASTNode> condition, std::unique_ptr<ASTNode> dst1,
    std::unique_ptr<ASTNode> dst2)
    : condition(std::move(condition)),
      dst1(std::move(dst1)),
      dst2(std::move(dst2)) {}

std::string BranchInstruction::to_string() const {
  return "goto " + target->to_string();
}

void BranchInstruction::generate_code(std::ofstream& outputFile) const {
  outputFile << "  br ";
  target->generate_code(outputFile);
  outputFile << "\n";
}

const ASTNode& BranchInstruction::get_target() const {
  return *target;
}

BranchInstruction::BranchInstruction(std::unique_ptr<ASTNode> target)
    : target(std::move(target)) {}

std::string ReturnInstruction::to_string() const {
  return "return";
}

void ReturnInstruction::generate_code(std::ofstream& outputFile) const {
  outputFile << "  return\n";
}

std::string ReturnValueInstruction::to_string() const {
  return "return " + returnValue->to_string();
}

void ReturnValueInstruction::generate_code(std::ofstream& outputFile) const {
  outputFile << "  return ";
  returnValue->generate_code(outputFile);
  outputFile << "\n";
}

const ASTNode& ReturnValueInstruction::get_return_value() const {
  return *returnValue;
}

ReturnValueInstruction::ReturnValueInstruction(
    std::unique_ptr<ASTNode> returnValue)
    : returnValue(std::move(returnValue)) {}

std::string WhileInstruction::to_string() const {
  return "while (" + condition->to_string() + ") " + dst1->to_string() + " " +
         dst2->to_string();
}

void WhileInstruction::generate_code(std::ofstream& outputFile) const {
  outputFile << "  while (";
  condition->generate_code(outputFile);
  outputFile << ") ";
  dst1->generate_code(outputFile);
  outputFile << " ";
  dst2->generate_code(outputFile);
  outputFile << "\n";
}

const ASTNode& WhileInstruction::get_condition() const {
  return *condition;
}

const ASTNode& WhileInstruction::get_true_target() const {
  return *dst1;
}

const ASTNode& WhileInstruction::get_false_target() const {
  return *dst2;
}

WhileInstruction::WhileInstruction(std::unique_ptr<ASTNode> condition,
                                   std::unique_ptr<ASTNode> dst1,
                                   std::unique_ptr<ASTNode> dst2)
    : condition(std::move(condition)),
      dst1(std::move(dst1)),
      dst2(std::move(dst2)) {}

std::string ContinueInstruction::to_string() const {
  return "continue";
}

void ContinueInstruction::generate_code(std::ofstream& outputFile) const {
  outputFile << "  continue\n";
}

std::string BreakInstruction::to_string() const {
  return "break";
}

void BreakInstruction::generate_code(std::ofstream& outputFile) const {
  outputFile << "  break\n";
}

std::string IndexAssignInstruction::to_string() const {
  return dst->to_string() + " <- " + src->to_string();
}

void IndexAssignInstruction::generate_code(std::ofstream& outputFile) const {
  outputFile << "  ";
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

IndexAssignInstruction::IndexAssignInstruction(std::unique_ptr<ASTNode> dst,
                                               std::unique_ptr<ASTNode> src)
    : dst(std::move(dst)), src(std::move(src)) {}

std::string LengthAssignInstruction::to_string() const {
  return dst->to_string() + " <- " + src->to_string();
}

void LengthAssignInstruction::generate_code(std::ofstream& outputFile) const {
  outputFile << "  ";
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

LengthAssignInstruction::LengthAssignInstruction(std::unique_ptr<ASTNode> dst,
                                                 std::unique_ptr<ASTNode> src)
    : dst(std::move(dst)), src(std::move(src)) {}

std::string CallInstruction::to_string() const {
  return functionCall->to_string();
}

void CallInstruction::generate_code(std::ofstream& outputFile) const {
  outputFile << "  ";
  functionCall->generate_code(outputFile);
  outputFile << "\n";
}

const ASTNode& CallInstruction::get_function_call() const {
  return *functionCall;
}

CallInstruction::CallInstruction(std::unique_ptr<ASTNode> functionCall)
    : functionCall(std::move(functionCall)) {}

std::string CallAssignInstruction::to_string() const {
  return dst->to_string() + " <- " + functionCall->to_string();
}

void CallAssignInstruction::generate_code(std::ofstream& outputFile) const {
  outputFile << "  ";
  dst->generate_code(outputFile);
  outputFile << " <- ";
  functionCall->generate_code(outputFile);
  outputFile << "\n";
}

const ASTNode& CallAssignInstruction::get_dst() const {
  return *dst;
}

const ASTNode& CallAssignInstruction::get_function_call() const {
  return *functionCall;
}

CallAssignInstruction::CallAssignInstruction(
    std::unique_ptr<ASTNode> dst, std::unique_ptr<ASTNode> functionCall)
    : dst(std::move(dst)), functionCall(std::move(functionCall)) {}

std::string NewArrayAssignInstruction::to_string() const {
  return dst->to_string() + " <- " + newArray->to_string();
}

void NewArrayAssignInstruction::generate_code(std::ofstream& outputFile) const {
  outputFile << "  ";
  dst->generate_code(outputFile);
  outputFile << " <- ";
  newArray->generate_code(outputFile);
  outputFile << "\n";
}

const ASTNode& NewArrayAssignInstruction::get_dst() const {
  return *dst;
}

const ASTNode& NewArrayAssignInstruction::get_new_array() const {
  return *newArray;
}

NewArrayAssignInstruction::NewArrayAssignInstruction(
    std::unique_ptr<ASTNode> dst, std::unique_ptr<ASTNode> newArray)
    : dst(std::move(dst)), newArray(std::move(newArray)) {}

std::string NewTupleAssignInstruction::to_string() const {
  return dst->to_string() + " <- " + newTuple->to_string();
}

void NewTupleAssignInstruction::generate_code(std::ofstream& outputFile) const {
  outputFile << "  ";
  dst->generate_code(outputFile);
  outputFile << " <- ";
  newTuple->generate_code(outputFile);
  outputFile << "\n";
}

const ASTNode& NewTupleAssignInstruction::get_dst() const {
  return *dst;
}

const ASTNode& NewTupleAssignInstruction::get_new_tuple() const {
  return *newTuple;
}

NewTupleAssignInstruction::NewTupleAssignInstruction(
    std::unique_ptr<ASTNode> dst, std::unique_ptr<ASTNode> newTuple)
    : dst(std::move(dst)), newTuple(std::move(newTuple)) {}

} 
