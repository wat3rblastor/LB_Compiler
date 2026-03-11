#include "program.h"

#include "node_utils.h"

namespace IR {

void BasicBlock::accept(ConstVisitor& visitor) const {
  visitor.visit(*this);
}

void BasicBlock::addInstruction(std::unique_ptr<Instruction> instruction) {
  instructions.push_back(std::move(instruction));
}

void BasicBlock::addTE(std::unique_ptr<Instruction> te) {
  this->te = std::move(te);
}

void BasicBlock::set_te(std::unique_ptr<Instruction> te) {
  this->te = std::move(te);
}

BasicBlock::BasicBlock(std::unique_ptr<ASTNode> label) : label(std::move(label)) {}

const Label& BasicBlock::get_label() const {
  return expect_label(*label, "BasicBlock label is not a Label node");
}

const Instruction& BasicBlock::get_te() const {
  return *te;
}

Instruction& BasicBlock::get_te_mut() {
  return *te;
}

const std::vector<std::unique_ptr<Instruction>>& BasicBlock::get_instructions() const {
  return instructions;
}

std::vector<std::unique_ptr<Instruction>>& BasicBlock::get_instructions_mut() {
  return instructions;
}

void Function::accept(ConstVisitor& visitor) const {
  visitor.visit(*this);
}

void Function::addParameter(std::unique_ptr<ASTNode> parameter) {
  parameters.push_back(std::move(parameter));
}

void Function::addBasicBlock(std::unique_ptr<BasicBlock> basicBlock) {
  basicBlocks.push_back(std::move(basicBlock));
}

std::vector<std::unique_ptr<BasicBlock>>& Function::get_basicBlocks() {
  return basicBlocks;
}

const std::vector<std::unique_ptr<BasicBlock>>& Function::get_basicBlocks() const {
  return basicBlocks;
}

const std::vector<std::unique_ptr<ASTNode>>& Function::get_parameters() const {
  return parameters;
}

const ASTNode& Function::get_type() const {
  return *type;
}

const ASTNode& Function::get_name() const {
  return *functionName;
}

Function::Function(std::unique_ptr<ASTNode> type,
                   std::unique_ptr<ASTNode> functionName)
    : type(std::move(type)), functionName(std::move(functionName)) {}

void Program::accept(ConstVisitor& visitor) const {
  visitor.visit(*this);
}

void Program::addFunction(std::unique_ptr<Function> function) {
  functions.push_back(std::move(function));
}

std::vector<std::unique_ptr<Function>>& Program::getFunctions() {
  return functions;
}

const std::vector<std::unique_ptr<Function>>& Program::getFunctions() const {
  return functions;
}

}  // namespace IR
