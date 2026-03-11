#include "program.h"
#include <stdexcept>
#include "utils.h"

namespace LB {

std::string Scope::to_string() const {
  std::string s = "{\n";
  for (const auto& inst : instructions) {
    s += "  " + inst->to_string() + "\n";
  }
  s += "}";
  return s;
}

void Scope::generate_code(std::ofstream& outputFile) const {
  for (const auto& inst : instructions) {
    inst->generate_code(outputFile);
  }
}

void Scope::addNode(std::unique_ptr<ASTNode> node) {
  instructions.push_back(std::move(node));
}

const std::vector<std::unique_ptr<ASTNode>>& Scope::getNodes() const {
  return instructions;
}

std::vector<std::unique_ptr<ASTNode>>& Scope::getNodes() {
  return instructions;
}

std::string Function::to_string() const {
  std::string s = type->to_string() + " " + functionName->to_string() + "(";
  for (size_t i = 0; i < parameters.size(); ++i) {
    if (i != 0) {
      s += ", ";
    }
    s += parameters[i]->to_string();
  }
  s += ") ";
  s += scope->to_string();
  return s;
}

void Function::generate_code(std::ofstream& outputFile) const {
  type->generate_code(outputFile);
  outputFile << " ";
  functionName->generate_code(outputFile);
  outputFile << "(";
  for (size_t i = 0; i < parameters.size(); ++i) {
    if (i != 0) {
      outputFile << ", ";
    }
    parameters[i]->generate_code(outputFile);
  }
  outputFile << ") {\n";
  scope->generate_code(outputFile);
  outputFile << "}\n";
}

void Function::addParameter(std::unique_ptr<ASTNode> parameter) {
  parameters.push_back(std::move(parameter));
}

void Function::setScope(std::unique_ptr<ASTNode> scope) {
  this->scope = std::move(scope);
}

const ASTNode& Function::getType() const {
  if (!type) {
    throw std::runtime_error("Function::getType: null type");
  }
  return *type;
}

const ASTNode& Function::getFunctionName() const {
  if (!functionName) {
    throw std::runtime_error("Function::getFunctionName: null function name");
  }
  return *functionName;
}

const std::vector<std::unique_ptr<ASTNode>>& Function::getParameters() const {
  return parameters;
}

std::vector<std::unique_ptr<ASTNode>>& Function::getParameters() {
  return parameters;
}

const ASTNode& Function::getScope() const {
  if (!scope) {
    throw std::runtime_error("Function::getScope: null scope");
  }
  return *scope;
}

ASTNode& Function::getScope() {
  if (!scope) {
    throw std::runtime_error("Function::getScope: null scope");
  }
  return *scope;
}

Function::Function(std::unique_ptr<ASTNode> type,
                   std::unique_ptr<ASTNode> functionName)
    : type(std::move(type)), functionName(std::move(functionName)) {}

std::string Program::to_string() const {
  std::string s;
  for (size_t i = 0; i < functions.size(); ++i) {
    if (i != 0) {
      s += "\n\n";
    }
    s += functions[i]->to_string();
  }
  if (!s.empty()) {
    s += "\n";
  }
  return s;
}

void Program::generate_code(std::ofstream& outputFile) const {
  for (size_t i = 0; i < functions.size(); ++i) {
    if (i != 0) {
      outputFile << "\n";
    }
    functions[i]->generate_code(outputFile);
  }
}

void Program::addFunction(std::unique_ptr<Function> function) {
  functions.push_back(std::move(function));
}

const std::vector<std::unique_ptr<Function>>& Program::getFunctions() const {
  return functions;
}

std::vector<std::unique_ptr<Function>>& Program::getFunctions() {
  return functions;
}

}  // namespace LB
