#include "to_string_visitor.h"

#include <sstream>
#include <stdexcept>
#include <type_traits>

namespace IR {
namespace {

std::string render_node(const ASTNode& node) {
  ToStringVisitor visitor;
  node.accept(visitor);
  return visitor.result();
}

const char* op_to_string(OP op) {
  switch (op) {
    case OP::PLUS:
      return "+";
    case OP::SUB:
      return "-";
    case OP::MUL:
      return "*";
    case OP::AND:
      return "&";
    case OP::LSHIFT:
      return "<<";
    case OP::RSHIFT:
      return ">>";
    case OP::LT:
      return "<";
    case OP::LE:
      return "<=";
    case OP::EQ:
      return "=";
    case OP::GE:
      return ">=";
    case OP::GT:
      return ">";
  }
  throw std::runtime_error("Invalid OP");
}

}  // namespace

void ToStringVisitor::visit(const Var& node) { text = node.name; }

void ToStringVisitor::visit(const Label& node) { text = node.name; }

void ToStringVisitor::visit(const Type& node) { text = node.name; }

void ToStringVisitor::visit(const Number& node) { text = std::to_string(node.value); }

void ToStringVisitor::visit(const Callee& node) {
  std::ostringstream out;
  std::visit(
      [&out](auto&& calleeValue) {
        using T = std::decay_t<decltype(calleeValue)>;
        if constexpr (std::is_same_v<T, EnumCallee>) {
          switch (calleeValue) {
            case EnumCallee::PRINT:
              out << "print";
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
          }
        } else if constexpr (std::is_same_v<T, std::unique_ptr<ASTNode>>) {
          out << render_node(*calleeValue);
        }
      },
      node.calleeValue);
  text = out.str();
}

void ToStringVisitor::visit(const Operator& node) {
  std::ostringstream out;
  out << render_node(*node.left) << " " << op_to_string(node.op) << " "
      << render_node(*node.right);
  text = out.str();
}

void ToStringVisitor::visit(const Index& node) {
  std::ostringstream out;
  out << render_node(*node.var);
  for (const auto& idx : node.indices) {
    out << "[" << render_node(*idx) << "]";
  }
  text = out.str();
}

void ToStringVisitor::visit(const ArrayLength& node) {
  std::ostringstream out;
  out << "length " << render_node(*node.var) << " " << render_node(*node.index);
  text = out.str();
}

void ToStringVisitor::visit(const TupleLength& node) {
  std::ostringstream out;
  out << "length " << render_node(*node.var);
  text = out.str();
}

void ToStringVisitor::visit(const FunctionCall& node) {
  std::ostringstream out;
  out << "call " << render_node(*node.callee) << "(";
  for (size_t i = 0; i < node.args.size(); ++i) {
    out << render_node(*node.args[i]);
    if (i + 1 != node.args.size()) {
      out << ", ";
    }
  }
  out << ")";
  text = out.str();
}

void ToStringVisitor::visit(const NewArray& node) {
  std::ostringstream out;
  out << "new Array(";
  for (size_t i = 0; i < node.args.size(); ++i) {
    out << render_node(*node.args[i]);
    if (i + 1 != node.args.size()) {
      out << ", ";
    }
  }
  out << ")";
  text = out.str();
}

void ToStringVisitor::visit(const NewTuple& node) {
  std::ostringstream out;
  out << "new Tuple(" << render_node(*node.length) << ")";
  text = out.str();
}

void ToStringVisitor::visit(const Parameter& node) {
  std::ostringstream out;
  out << render_node(*node.type) << " " << render_node(*node.var);
  text = out.str();
}

void ToStringVisitor::visit(const VarDefInstruction& node) {
  std::ostringstream out;
  out << render_node(node.get_type()) << " " << render_node(node.get_var());
  text = out.str();
}

void ToStringVisitor::visit(const VarNumLabelAssignInstruction& node) {
  std::ostringstream out;
  out << render_node(node.get_dst()) << " <- " << render_node(node.get_src());
  text = out.str();
}

void ToStringVisitor::visit(const OperatorAssignInstruction& node) {
  std::ostringstream out;
  out << render_node(node.get_dst()) << " <- " << render_node(node.get_src());
  text = out.str();
}

void ToStringVisitor::visit(const IndexAssignInstruction& node) {
  std::ostringstream out;
  out << render_node(node.get_dst()) << " <- " << render_node(node.get_src());
  text = out.str();
}

void ToStringVisitor::visit(const LengthAssignInstruction& node) {
  std::ostringstream out;
  out << render_node(node.get_dst()) << " <- " << render_node(node.get_src());
  text = out.str();
}

void ToStringVisitor::visit(const CallInstruction& node) {
  text = render_node(node.get_call());
}

void ToStringVisitor::visit(const CallAssignInstruction& node) {
  std::ostringstream out;
  out << render_node(node.get_dst()) << " <- " << render_node(node.get_call());
  text = out.str();
}

void ToStringVisitor::visit(const NewArrayAssignInstruction& node) {
  std::ostringstream out;
  out << render_node(node.get_dst()) << " <- " << render_node(node.get_new_array());
  text = out.str();
}

void ToStringVisitor::visit(const NewTupleAssignInstruction& node) {
  std::ostringstream out;
  out << render_node(node.get_dst()) << " <- " << render_node(node.get_new_tuple());
  text = out.str();
}

void ToStringVisitor::visit(const BranchInstruction& node) {
  std::ostringstream out;
  out << "br " << render_node(node.get_target());
  text = out.str();
}

void ToStringVisitor::visit(const ConditionalBranchInstruction& node) {
  std::ostringstream out;
  out << "br " << render_node(node.get_condition()) << " "
      << render_node(node.get_true_target()) << " "
      << render_node(node.get_false_target());
  text = out.str();
}

void ToStringVisitor::visit(const ReturnInstruction&) { text = "return"; }

void ToStringVisitor::visit(const ReturnValueInstruction& node) {
  std::ostringstream out;
  out << "return " << render_node(node.get_return_value());
  text = out.str();
}

void ToStringVisitor::visit(const BasicBlock& node) {
  std::ostringstream out;
  out << '\t' << render_node(node.get_label()) << "\n";
  for (const auto& inst : node.get_instructions()) {
    out << '\t' << render_node(*inst) << "\n";
  }
  out << '\t' << render_node(node.get_te()) << "\n";
  text = out.str();
}

void ToStringVisitor::visit(const Function& node) {
  std::ostringstream out;
  out << "define " << render_node(node.get_type()) << " "
      << render_node(node.get_name()) << "(";
  const auto& params = node.get_parameters();
  for (size_t i = 0; i < params.size(); ++i) {
    out << render_node(*params[i]);
    if (i + 1 != params.size()) {
      out << ", ";
    }
  }
  out << ") {\n";
  for (const auto& bb : node.get_basicBlocks()) {
    out << render_node(*bb) << "\n";
  }
  out << "}";
  text = out.str();
}

void ToStringVisitor::visit(const Program& node) {
  std::ostringstream out;
  const auto& functions = node.getFunctions();
  for (const auto& fn : functions) {
    out << render_node(*fn) << "\n";
  }
  text = out.str();
}

std::string ToStringVisitor::result() const { return text; }

std::string to_string(const ASTNode& node) {
  ToStringVisitor visitor;
  node.accept(visitor);
  return visitor.result();
}

std::string to_string(const Program& program) {
  return to_string(static_cast<const ASTNode&>(program));
}

}  // namespace IR
