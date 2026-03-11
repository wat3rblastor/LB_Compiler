#include <iostream>
#include <queue>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>

#include "LB.h"
#include "utils.h"

namespace LB {

std::string Name::to_string() const {
  return name;
}

void Name::generate_code(std::ofstream& outputFile) const {
  outputFile << name;
}

Name::Name(const std::string& name) : name(name) {}

std::string Number::to_string() const {
  return std::to_string(value);
}

void Number::generate_code(std::ofstream& outputFile) const {
  outputFile << value;
}

Number::Number(int64_t value) : value(value) {}

std::string Label::to_string() const {
  return name;
}

void Label::generate_code(std::ofstream& outputFile) const {
  outputFile << name;
}

Label::Label(const std::string& name) : name(name) {}

std::string Type::to_string() const {
  return name;
}

void Type::generate_code(std::ofstream& outputFile) const {
  outputFile << name;
}

Type::Type(const std::string& name) : name(name) {}

std::string Parameter::to_string() const {
  return type->to_string() + " " + name->to_string();
}

void Parameter::generate_code(std::ofstream& outputFile) const {
  type->generate_code(outputFile);
  outputFile << " ";
  name->generate_code(outputFile);
}

Parameter::Parameter(std::unique_ptr<ASTNode> type, std::unique_ptr<ASTNode> name)
    : type(std::move(type)), name(std::move(name)) {}

std::string Operator::to_string() const {
  std::string op_string;
  switch (op) {
    case OP::PLUS:
      op_string = "+";
      break;
    case OP::SUB:
      op_string = "-";
      break;
    case OP::MUL:
      op_string = "*";
      break;
    case OP::AND:
      op_string = "&";
      break;
    case OP::LSHIFT:
      op_string = "<<";
      break;
    case OP::RSHIFT:
      op_string = ">>";
      break;
    case OP::LT:
      op_string = "<";
      break;
    case OP::LE:
      op_string = "<=";
      break;
    case OP::EQ:
      op_string = "=";
      break;
    case OP::GE:
      op_string = ">=";
      break;
    case OP::GT:
      op_string = ">";
      break;
  }
  return left->to_string() + " " + op_string + " " + right->to_string();
}

void Operator::generate_code(std::ofstream& outputFile) const {
  left->generate_code(outputFile);
  outputFile << " ";
  switch (op) {
    case OP::PLUS:
      outputFile << "+";
      break;
    case OP::SUB:
      outputFile << "-";
      break;
    case OP::MUL:
      outputFile << "*";
      break;
    case OP::AND:
      outputFile << "&";
      break;
    case OP::LSHIFT:
      outputFile << "<<";
      break;
    case OP::RSHIFT:
      outputFile << ">>";
      break;
    case OP::LT:
      outputFile << "<";
      break;
    case OP::LE:
      outputFile << "<=";
      break;
    case OP::EQ:
      outputFile << "=";
      break;
    case OP::GE:
      outputFile << ">=";
      break;
    case OP::GT:
      outputFile << ">";
      break;
  }
  outputFile << " ";
  right->generate_code(outputFile);
}

Operator::Operator(std::unique_ptr<ASTNode> left, std::unique_ptr<ASTNode> right,
                   OP op)
    : left(std::move(left)), right(std::move(right)), op(op) {}

std::string CondOperator::to_string() const {
  std::string op_string;
  switch (op) {
    case OP::PLUS:
      op_string = "+";
      break;
    case OP::SUB:
      op_string = "-";
      break;
    case OP::MUL:
      op_string = "*";
      break;
    case OP::AND:
      op_string = "&";
      break;
    case OP::LSHIFT:
      op_string = "<<";
      break;
    case OP::RSHIFT:
      op_string = ">>";
      break;
    case OP::LT:
      op_string = "<";
      break;
    case OP::LE:
      op_string = "<=";
      break;
    case OP::EQ:
      op_string = "=";
      break;
    case OP::GE:
      op_string = ">=";
      break;
    case OP::GT:
      op_string = ">";
      break;
  }
  return left->to_string() + " " + op_string + " " + right->to_string();
}

void CondOperator::generate_code(std::ofstream& outputFile) const {
  left->generate_code(outputFile);
  outputFile << " ";
  switch (op) {
    case OP::PLUS:
      outputFile << "+";
      break;
    case OP::SUB:
      outputFile << "-";
      break;
    case OP::MUL:
      outputFile << "*";
      break;
    case OP::AND:
      outputFile << "&";
      break;
    case OP::LSHIFT:
      outputFile << "<<";
      break;
    case OP::RSHIFT:
      outputFile << ">>";
      break;
    case OP::LT:
      outputFile << "<";
      break;
    case OP::LE:
      outputFile << "<=";
      break;
    case OP::EQ:
      outputFile << "=";
      break;
    case OP::GE:
      outputFile << ">=";
      break;
    case OP::GT:
      outputFile << ">";
      break;
  }
  outputFile << " ";
  right->generate_code(outputFile);
}

CondOperator::CondOperator(std::unique_ptr<ASTNode> left,
                           std::unique_ptr<ASTNode> right, OP op)
    : left(std::move(left)), right(std::move(right)), op(op) {}

std::string Index::to_string() const {
  std::string s = name->to_string();
  for (const auto& index : indices) {
    s += "[" + index->to_string() + "]";
  }
  return s;
}

void Index::generate_code(std::ofstream& outputFile) const {
  name->generate_code(outputFile);
  for (const auto& index : indices) {
    outputFile << "[";
    index->generate_code(outputFile);
    outputFile << "]";
  }
}

Index::Index(int64_t lineNumber, std::unique_ptr<ASTNode> name,
             std::vector<std::unique_ptr<ASTNode>>&& indices)
    : lineNumber(lineNumber), name(std::move(name)), indices(std::move(indices)) {}

std::string Length::to_string() const {
  std::string s = "length " + name->to_string();
  if (hasIndex) {
    s += " " + index->to_string();
  }
  return s;
}

void Length::generate_code(std::ofstream& outputFile) const {
  outputFile << "length ";
  name->generate_code(outputFile);
  if (hasIndex) {
    outputFile << " ";
    index->generate_code(outputFile);
  }
}

Length::Length(int64_t lineNumber, std::unique_ptr<ASTNode> name)
    : lineNumber(lineNumber), hasIndex(false), name(std::move(name)) {}

Length::Length(int64_t lineNumber, std::unique_ptr<ASTNode> name,
               std::unique_ptr<ASTNode> index)
    : lineNumber(lineNumber),
      hasIndex(true),
      name(std::move(name)),
      index(std::move(index)) {}

std::string FunctionCall::to_string() const {
  std::string s = name->to_string() + "(";
  for (size_t i = 0; i < args.size(); ++i) {
    if (i != 0) {
      s += ", ";
    }
    s += args[i]->to_string();
  }
  s += ")";
  return s;
}

void FunctionCall::generate_code(std::ofstream& outputFile) const {
  name->generate_code(outputFile);
  outputFile << "(";
  for (size_t i = 0; i < args.size(); ++i) {
    if (i != 0) {
      outputFile << ", ";
    }
    args[i]->generate_code(outputFile);
  }
  outputFile << ")";
}

FunctionCall::FunctionCall(std::unique_ptr<ASTNode> name,
                           std::vector<std::unique_ptr<ASTNode>>&& args)
    : name(std::move(name)), args(std::move(args)) {}

std::string NewArray::to_string() const {
  std::string s = "new Array(";
  for (size_t i = 0; i < args.size(); ++i) {
    if (i != 0) {
      s += ", ";
    }
    s += args[i]->to_string();
  }
  s += ")";
  return s;
}

void NewArray::generate_code(std::ofstream& outputFile) const {
  outputFile << "new Array(";
  for (size_t i = 0; i < args.size(); ++i) {
    if (i != 0) {
      outputFile << ", ";
    }
    args[i]->generate_code(outputFile);
  }
  outputFile << ")";
}

NewArray::NewArray(std::vector<std::unique_ptr<ASTNode>>&& args)
    : args(std::move(args)) {}

std::string NewTuple::to_string() const {
  return "new Tuple(" + length->to_string() + ")";
}

void NewTuple::generate_code(std::ofstream& outputFile) const {
  outputFile << "new Tuple(";
  length->generate_code(outputFile);
  outputFile << ")";
}

NewTuple::NewTuple(std::unique_ptr<ASTNode> length)
    : length(std::move(length)) {}

} 
