#include <stdexcept>

#include "program.h"
#include "utils.h"

namespace LA {

std::string Function::to_string() const {
  std::ostringstream out;
  out << type->to_string();
  out << " " << functionName->to_string();
  out << "(";
  for (int i = 0; i < parameters.size(); ++i) {
    out << parameters[i]->to_string();
    if (i + 1 != parameters.size()) {
      out << ", ";
    }
  }
  out << ") {\n";
  for (const auto& instruction : instructions) {
    out << instruction->to_string();
    out << "\n";
  }
  out << "}";
  return out.str();
}

void Function::generate_code(std::ofstream& outputFile) const {
  outputFile << "define ";
  type->generate_code(outputFile);
  outputFile << " @";
  functionName->generate_code(outputFile);
  outputFile << " (";
  for (int i = 0; i < parameters.size(); ++i) {
    const auto* parameter = dynamic_cast<const Parameter*>(parameters[i].get());
    if (!parameter) {
      throw std::runtime_error("Function::generate_code: expected Parameter");
    }
    parameter->type->generate_code(outputFile);
    outputFile << " ";
    parameter->name->generate_code(outputFile);
    if (i + 1 != parameters.size()) {
      outputFile << ", ";
    }
  }
  outputFile << ") {\n\n";
  for (const auto& instruction : instructions) {
    instruction->generate_code(outputFile);
  }
  outputFile << "}";
}

void Function::addParameter(std::unique_ptr<ASTNode> parameter) {
  parameters.push_back(std::move(parameter));
}

void Function::addInstruction(std::unique_ptr<Instruction> instruction) {
  instructions.push_back(std::move(instruction));
}

void Function::setInstructions(std::vector<std::unique_ptr<Instruction>>&& newInstructions) {
  instructions = std::move(newInstructions);
}

const ASTNode& Function::getType() const {
  return *type;
}

const ASTNode& Function::getFunctionName() const {
  return *functionName;
}

const std::vector<std::unique_ptr<ASTNode>>& Function::getParameters() const {
  return parameters;
}

const std::vector<std::unique_ptr<Instruction>>& Function::getInstructions() const {
  return instructions;
}

std::vector<std::unique_ptr<Instruction>>& Function::getInstructions() {
  return instructions;
}

Function::Function(std::unique_ptr<ASTNode> type, std::unique_ptr<ASTNode> functionName)
  : type(std::move(type)), functionName(std::move(functionName)) {}

std::string Program::to_string() const  {
  std::ostringstream out;
  for (const auto& function : functions) {
    out << function->to_string();
    out << "\n";
  }
  return out.str();
}

void Program::generate_code(std::ofstream& outputFile) const {
  clear_function_names();
  for (const auto& function : functions) {
    const ASTNode& functionNameNode = function->getFunctionName();
    if (const auto* label = dynamic_cast<const Label*>(&functionNameNode)) {
      register_function_name(label->name);
    } else if (const auto* name = dynamic_cast<const Name*>(&functionNameNode)) {
      register_function_name(name->name);
    }
  }

  for (int i = 0; i < functions.size(); ++i) {
    functions[i]->generate_code(outputFile);
    if (i + 1 != functions.size()) {
      outputFile << "\n";
    }
  }
}

void Program::addFunction(std::unique_ptr<Function> function) {
  functions.push_back(std::move(function));
}

std::vector<std::unique_ptr<Function>>& Program::getFunctions() {
  return functions;
}

}
