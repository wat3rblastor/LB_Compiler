#include "IR.h"

namespace IR {

void Var::accept(ConstVisitor& visitor) const {
  visitor.visit(*this);
}

Var::Var(const std::string& name) : name(name) {}

void Label::accept(ConstVisitor& visitor) const {
  visitor.visit(*this);
}

Label::Label(const std::string& name) : name(name) {}

void Type::accept(ConstVisitor& visitor) const {
  visitor.visit(*this);
}

Type::Type(const std::string& name) : name(name) {}

void Number::accept(ConstVisitor& visitor) const {
  visitor.visit(*this);
}

Number::Number(int64_t value) : value(value) {}

void Callee::accept(ConstVisitor& visitor) const {
  visitor.visit(*this);
}

Callee::Callee(EnumCallee& calleeValue) : calleeValue(calleeValue) {}

Callee::Callee(std::unique_ptr<ASTNode> calleeValue)
    : calleeValue(std::move(calleeValue)) {}

void Operator::accept(ConstVisitor& visitor) const {
  visitor.visit(*this);
}

Operator::Operator(std::unique_ptr<ASTNode> left, std::unique_ptr<ASTNode> right,
                   OP op)
    : left(std::move(left)), right(std::move(right)), op(op) {}

void Index::accept(ConstVisitor& visitor) const {
  visitor.visit(*this);
}

Index::Index(std::unique_ptr<ASTNode> var,
             std::vector<std::unique_ptr<ASTNode>>&& indices)
    : var(std::move(var)), indices(std::move(indices)) {}

void ArrayLength::accept(ConstVisitor& visitor) const {
  visitor.visit(*this);
}

ArrayLength::ArrayLength(std::unique_ptr<ASTNode> var,
                         std::unique_ptr<ASTNode> index)
    : var(std::move(var)), index(std::move(index)) {}

void TupleLength::accept(ConstVisitor& visitor) const {
  visitor.visit(*this);
}

TupleLength::TupleLength(std::unique_ptr<ASTNode> var) : var(std::move(var)) {}

void FunctionCall::accept(ConstVisitor& visitor) const {
  visitor.visit(*this);
}

FunctionCall::FunctionCall(std::unique_ptr<ASTNode> callee,
                           std::vector<std::unique_ptr<ASTNode>>&& args)
    : callee(std::move(callee)), args(std::move(args)) {}

void NewArray::accept(ConstVisitor& visitor) const {
  visitor.visit(*this);
}

NewArray::NewArray(std::vector<std::unique_ptr<ASTNode>>&& args)
    : args(std::move(args)) {}

void NewTuple::accept(ConstVisitor& visitor) const {
  visitor.visit(*this);
}

NewTuple::NewTuple(std::unique_ptr<ASTNode> length)
    : length(std::move(length)) {}

void Parameter::accept(ConstVisitor& visitor) const {
  visitor.visit(*this);
}

Parameter::Parameter(std::unique_ptr<ASTNode> type, std::unique_ptr<ASTNode> var)
    : type(std::move(type)), var(std::move(var)) {}

}  // namespace IR
