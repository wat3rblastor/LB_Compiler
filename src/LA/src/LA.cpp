#include <iostream>
#include <queue>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>

#include "LA.h"
#include "utils.h"

namespace LA {

namespace {

bool is_runtime_callee(const std::string& name) {
  return name == "print" || name == "input" || name == "tensor-error" || name == "tuple-error";
}

std::unordered_set<std::string> functionNames;

}  // namespace

std::string Name::to_string() const {
  return name;
}

void Name::generate_code(std::ofstream& outputFile) const {
  if (!name.empty() && (name[0] == '%' || name[0] == '@' || name[0] == ':')) {
    outputFile << name;
    return;
  }
  if (is_function_name(name)) {
    outputFile << "@" << name;
    return;
  }
  outputFile << "%" << name;
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
  std::ostringstream out;
  out << type->to_string();
  out << " ";
  out << name->to_string();
  return out.str();
}

void Parameter::generate_code(std::ofstream& outputFile) const {
  name->generate_code(outputFile);
}  

Parameter::Parameter(std::unique_ptr<ASTNode> type, std::unique_ptr<ASTNode> name) 
  : type(std::move(type)), name(std::move(name)) {}

std::string Operator::to_string() const {
  std::ostringstream out;
  out << left->to_string() << " ";
  switch (op) {
    case OP::PLUS:
      out << "+";
      break;
    case OP::SUB:
      out << "-";
      break;
    case OP::MUL:
      out << "*";
      break;
    case OP::AND:
      out << "&";
      break;
    case OP::LSHIFT:
      out << "<<";
      break;
    case OP::RSHIFT:
      out << ">>";
      break;
    case OP::LT:
      out << "<";
      break;
    case OP::LE:
      out << "<=";
      break;
    case OP::EQ:
      out << "=";
      break;
    case OP::GE:
      out << ">=";
      break;
    case OP::GT:
      out << ">";
      break;
    default:
      throw std::runtime_error("Operator::generate_code:Invalid OP");
  }
  out << " " << right->to_string();
  return out.str();
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
    default:
      throw std::runtime_error("Operator::generate_code:Invalid OP");
  }
  outputFile << " ";
  right->generate_code(outputFile);
}
  
Operator::Operator(std::unique_ptr<ASTNode> left, std::unique_ptr<ASTNode> right, OP op)
  : left(std::move(left)), right(std::move(right)), op(op) {}

std::string Index::to_string() const {
  std::ostringstream out;
  out << name->to_string();
  for (int i = 0; i < indices.size(); ++i) {
    out << "[";
    out << indices[i]->to_string();
    out << "]";
  }
  return out.str();
}

void Index::generate_code(std::ofstream& outputFile) const {
  name->generate_code(outputFile);
  for (int i = 0; i < indices.size(); ++i) {
    outputFile << "[";
    indices[i]->generate_code(outputFile);
    outputFile << "]";
  }
}

Index::Index(int64_t lineNumber, std::unique_ptr<ASTNode> name, std::vector<std::unique_ptr<ASTNode>>&& indices) 
  : lineNumber(lineNumber), name(std::move(name)), indices(std::move(indices)) {}

std::string Length::to_string() const {
  std::ostringstream out;
  out << "length ";
  out << name->to_string();
  if (hasIndex) out << " " << index->to_string();
  return out.str();
}

void Length::generate_code(std::ofstream& outputFile) const {
  outputFile << "length ";
  name->generate_code(outputFile);
  outputFile << " ";
  if (hasIndex) index->generate_code(outputFile);
}  

Length::Length(int64_t lineNumber, std::unique_ptr<ASTNode> name)
  : lineNumber(lineNumber), hasIndex(false), name(std::move(name)) {}

Length::Length(int64_t lineNumber, std::unique_ptr<ASTNode> name, std::unique_ptr<ASTNode> index)
  : lineNumber(lineNumber), hasIndex(true), name(std::move(name)), index(std::move(index)) {}

std::string FunctionCall::to_string() const {
  std::ostringstream out;
  out << "call ";
  if (const auto* callee = dynamic_cast<const Name*>(name.get())) {
    if (is_runtime_callee(callee->name)) {
      out << callee->name;
    } else if (is_function_name(callee->name)) {
      out << "@" << callee->name;
    } else {
      out << "%" << callee->name;
    }
  } else {
    out << name->to_string();
  }
  out << "(";
  for (int i = 0; i < args.size(); ++i) {
    out << args[i]->to_string();
    if (i + 1!= args.size()) {
      out << ", ";
    }
  }
  out << ")";
  return out.str();
}

void FunctionCall::generate_code(std::ofstream& outputFile) const {
  outputFile << "call ";
  if (const auto* callee = dynamic_cast<const Name*>(name.get())) {
    if (is_runtime_callee(callee->name)) {
      outputFile << callee->name;
    } else if (is_function_name(callee->name)) {
      outputFile << "@" << callee->name;
    } else {
      outputFile << "%" << callee->name;
    }
  } else {
    name->generate_code(outputFile);
  }
  outputFile << " (";
  for (int i = 0; i < args.size(); ++i) {
    args[i]->generate_code(outputFile);
    if (i + 1 != args.size()) {
      outputFile << ", ";
    }
  }
  outputFile << ")";
}  

FunctionCall::FunctionCall(std::unique_ptr<ASTNode> name, std::vector<std::unique_ptr<ASTNode>>&& args)
  : name(std::move(name)), args(std::move(args)) {}

std::string NewArray::to_string() const {
  std::ostringstream out;
  out << "new Array(";
  for (int i = 0; i < args.size(); ++i) {
    out << args[i]->to_string();
    if (i + 1 != args.size()) {
      out << ", ";
    }
  }
  out << ")";
  return out.str();
}

void NewArray::generate_code(std::ofstream& outputFile) const {
  outputFile << "new Array(";
  for (int i = 0; i < args.size(); ++i) {
    args[i]->generate_code(outputFile);
    if (i + 1 != args.size()) {
      outputFile << ", ";
    }
  }
  outputFile << ")";
}  

NewArray::NewArray(std::vector<std::unique_ptr<ASTNode>>&& args)
  : args(std::move(args)) {}

std::string NewTuple::to_string() const {
  std::ostringstream out;
  out << "new Tuple(";
  out << length->to_string();
  out << ")";
  return out.str();
}

void NewTuple::generate_code(std::ofstream& outputFile) const {
  outputFile << "new Tuple(";
  length->generate_code(outputFile);
  outputFile << ")";
}  

NewTuple::NewTuple(std::unique_ptr<ASTNode> length)
  : length(std::move(length)) {}

void clear_function_names() {
  functionNames.clear();
}

void register_function_name(const std::string& name) {
  functionNames.insert(name);
}

bool is_function_name(const std::string& name) {
  return functionNames.find(name) != functionNames.end();
}

}
