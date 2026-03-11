#include "const_prop.h"

#include <algorithm>
#include <deque>
#include <memory>
#include <stdexcept>

#include "cfg_graph.h"
#include "node_utils.h"

namespace IR {
namespace {

class VisitorBase : public ConstVisitor {
 public:
  void visit(const Var&) override {}
  void visit(const Label&) override {}
  void visit(const Type&) override {}
  void visit(const Number&) override {}
  void visit(const Callee&) override {}
  void visit(const Operator&) override {}
  void visit(const Index&) override {}
  void visit(const ArrayLength&) override {}
  void visit(const TupleLength&) override {}
  void visit(const FunctionCall&) override {}
  void visit(const NewArray&) override {}
  void visit(const NewTuple&) override {}
  void visit(const Parameter&) override {}

  void visit(const VarDefInstruction&) override {}
  void visit(const VarNumLabelAssignInstruction&) override {}
  void visit(const OperatorAssignInstruction&) override {}
  void visit(const IndexAssignInstruction&) override {}
  void visit(const LengthAssignInstruction&) override {}
  void visit(const CallInstruction&) override {}
  void visit(const CallAssignInstruction&) override {}
  void visit(const NewArrayAssignInstruction&) override {}
  void visit(const NewTupleAssignInstruction&) override {}
  void visit(const BranchInstruction&) override {}
  void visit(const ConditionalBranchInstruction&) override {}
  void visit(const ReturnInstruction&) override {}
  void visit(const ReturnValueInstruction&) override {}

  void visit(const BasicBlock&) override {}
  void visit(const Function&) override {}
  void visit(const Program&) override {}
};

const Number* as_number_or_null(const ASTNode& node) {
  class NumberMatcher : public VisitorBase {
   public:
    const Number* out = nullptr;
    void visit(const Number& value) override { out = &value; }
  } matcher;
  node.accept(matcher);
  return matcher.out;
}

const Operator* as_operator_or_null(const ASTNode& node) {
  class OperatorMatcher : public VisitorBase {
   public:
    const Operator* out = nullptr;
    void visit(const Operator& value) override { out = &value; }
  } matcher;
  node.accept(matcher);
  return matcher.out;
}

const VarNumLabelAssignInstruction* as_var_num_label_assign_or_null(
    const Instruction& inst) {
  class Matcher : public VisitorBase {
   public:
    const VarNumLabelAssignInstruction* out = nullptr;
    void visit(const VarNumLabelAssignInstruction& value) override { out = &value; }
  } matcher;
  inst.accept(matcher);
  return matcher.out;
}

const OperatorAssignInstruction* as_operator_assign_or_null(const Instruction& inst) {
  class Matcher : public VisitorBase {
   public:
    const OperatorAssignInstruction* out = nullptr;
    void visit(const OperatorAssignInstruction& value) override { out = &value; }
  } matcher;
  inst.accept(matcher);
  return matcher.out;
}

const IndexAssignInstruction* as_index_assign_or_null(const Instruction& inst) {
  class Matcher : public VisitorBase {
   public:
    const IndexAssignInstruction* out = nullptr;
    void visit(const IndexAssignInstruction& value) override { out = &value; }
  } matcher;
  inst.accept(matcher);
  return matcher.out;
}

const LengthAssignInstruction* as_length_assign_or_null(const Instruction& inst) {
  class Matcher : public VisitorBase {
   public:
    const LengthAssignInstruction* out = nullptr;
    void visit(const LengthAssignInstruction& value) override { out = &value; }
  } matcher;
  inst.accept(matcher);
  return matcher.out;
}

const CallInstruction* as_call_instruction_or_null(const Instruction& inst) {
  class Matcher : public VisitorBase {
   public:
    const CallInstruction* out = nullptr;
    void visit(const CallInstruction& value) override { out = &value; }
  } matcher;
  inst.accept(matcher);
  return matcher.out;
}

const CallAssignInstruction* as_call_assign_or_null(const Instruction& inst) {
  class Matcher : public VisitorBase {
   public:
    const CallAssignInstruction* out = nullptr;
    void visit(const CallAssignInstruction& value) override { out = &value; }
  } matcher;
  inst.accept(matcher);
  return matcher.out;
}

const NewArrayAssignInstruction* as_new_array_assign_or_null(const Instruction& inst) {
  class Matcher : public VisitorBase {
   public:
    const NewArrayAssignInstruction* out = nullptr;
    void visit(const NewArrayAssignInstruction& value) override { out = &value; }
  } matcher;
  inst.accept(matcher);
  return matcher.out;
}

const NewTupleAssignInstruction* as_new_tuple_assign_or_null(const Instruction& inst) {
  class Matcher : public VisitorBase {
   public:
    const NewTupleAssignInstruction* out = nullptr;
    void visit(const NewTupleAssignInstruction& value) override { out = &value; }
  } matcher;
  inst.accept(matcher);
  return matcher.out;
}

const ReturnValueInstruction* as_return_value_instruction_or_null(const Instruction& inst) {
  class Matcher : public VisitorBase {
   public:
    const ReturnValueInstruction* out = nullptr;
    void visit(const ReturnValueInstruction& value) override { out = &value; }
  } matcher;
  inst.accept(matcher);
  return matcher.out;
}

const VarDefInstruction* as_var_def_instruction_or_null(const Instruction& inst) {
  class Matcher : public VisitorBase {
   public:
    const VarDefInstruction* out = nullptr;
    void visit(const VarDefInstruction& value) override { out = &value; }
  } matcher;
  inst.accept(matcher);
  return matcher.out;
}

std::unordered_set<std::string> collect_vars_from_node(const ASTNode& node) {
  class VarCollector : public VisitorBase {
   public:
    std::unordered_set<std::string> names;

    void collect(const ASTNode& n) { n.accept(*this); }

    void visit(const Var& node) override { names.insert(node.name); }

    void visit(const Callee& node) override {
      if (const auto* target = std::get_if<std::unique_ptr<ASTNode>>(&node.calleeValue)) {
        collect(**target);
      }
    }

    void visit(const Operator& node) override {
      collect(*node.left);
      collect(*node.right);
    }

    void visit(const Index& node) override {
      collect(*node.var);
      for (const auto& idx : node.indices) {
        collect(*idx);
      }
    }

    void visit(const ArrayLength& node) override {
      collect(*node.var);
      collect(*node.index);
    }

    void visit(const TupleLength& node) override { collect(*node.var); }

    void visit(const FunctionCall& node) override {
      collect(*node.callee);
      for (const auto& arg : node.args) {
        collect(*arg);
      }
    }

    void visit(const NewArray& node) override {
      for (const auto& arg : node.args) {
        collect(*arg);
      }
    }

    void visit(const NewTuple& node) override { collect(*node.length); }

    void visit(const Parameter& node) override {
      collect(*node.type);
      collect(*node.var);
    }
  } collector;

  collector.collect(node);
  return collector.names;
}

std::unique_ptr<ASTNode> clone_ast(const ASTNode& node) {
  class Cloner : public VisitorBase {
   public:
    std::unique_ptr<ASTNode> out;

    std::unique_ptr<ASTNode> clone(const ASTNode& n) {
      n.accept(*this);
      if (out == nullptr) {
        throw std::runtime_error("AST clone failed");
      }
      return std::move(out);
    }

    void visit(const Var& node) override { out = std::make_unique<Var>(node.name); }
    void visit(const Label& node) override { out = std::make_unique<Label>(node.name); }
    void visit(const Type& node) override { out = std::make_unique<Type>(node.name); }
    void visit(const Number& node) override { out = std::make_unique<Number>(node.value); }

    void visit(const Callee& node) override {
      if (const auto* enum_value = std::get_if<EnumCallee>(&node.calleeValue)) {
        EnumCallee tmp = *enum_value;
        out = std::make_unique<Callee>(tmp);
        return;
      }
      const auto* target = std::get_if<std::unique_ptr<ASTNode>>(&node.calleeValue);
      out = std::make_unique<Callee>(clone(**target));
    }

    void visit(const Operator& node) override {
      out = std::make_unique<Operator>(clone(*node.left), clone(*node.right), node.op);
    }

    void visit(const Index& node) override {
      std::vector<std::unique_ptr<ASTNode>> indices;
      indices.reserve(node.indices.size());
      for (const auto& idx : node.indices) {
        indices.push_back(clone(*idx));
      }
      out = std::make_unique<Index>(clone(*node.var), std::move(indices));
    }

    void visit(const ArrayLength& node) override {
      out = std::make_unique<ArrayLength>(clone(*node.var), clone(*node.index));
    }

    void visit(const TupleLength& node) override {
      out = std::make_unique<TupleLength>(clone(*node.var));
    }

    void visit(const FunctionCall& node) override {
      std::vector<std::unique_ptr<ASTNode>> args;
      args.reserve(node.args.size());
      for (const auto& arg : node.args) {
        args.push_back(clone(*arg));
      }
      out = std::make_unique<FunctionCall>(clone(*node.callee), std::move(args));
    }

    void visit(const NewArray& node) override {
      std::vector<std::unique_ptr<ASTNode>> args;
      args.reserve(node.args.size());
      for (const auto& arg : node.args) {
        args.push_back(clone(*arg));
      }
      out = std::make_unique<NewArray>(std::move(args));
    }

    void visit(const NewTuple& node) override {
      out = std::make_unique<NewTuple>(clone(*node.length));
    }

    void visit(const Parameter& node) override {
      out = std::make_unique<Parameter>(clone(*node.type), clone(*node.var));
    }
  } cloner;

  return cloner.clone(node);
}

bool compute_binary_op(OP op, int64_t left, int64_t right, int64_t* result) {
  switch (op) {
    case OP::PLUS:
      *result = left + right;
      return true;
    case OP::SUB:
      *result = left - right;
      return true;
    case OP::MUL:
      *result = left * right;
      return true;
    case OP::AND:
      *result = left & right;
      return true;
    case OP::LSHIFT:
      *result = left << right;
      return true;
    case OP::RSHIFT:
      *result = left >> right;
      return true;
    case OP::LT:
      *result = (left < right) ? 1 : 0;
      return true;
    case OP::LE:
      *result = (left <= right) ? 1 : 0;
      return true;
    case OP::EQ:
      *result = (left == right) ? 1 : 0;
      return true;
    case OP::GE:
      *result = (left >= right) ? 1 : 0;
      return true;
    case OP::GT:
      *result = (left > right) ? 1 : 0;
      return true;
  }
  return false;
}

}  // namespace

ConstantPropagationAnalysis::CPValue ConstantPropagationAnalysis::CPValue::undef() {
  return CPValue{CPKind::UNDEF, 0};
}

ConstantPropagationAnalysis::CPValue ConstantPropagationAnalysis::CPValue::constant_value(
    int64_t value) {
  return CPValue{CPKind::CONST, value};
}

ConstantPropagationAnalysis::CPValue ConstantPropagationAnalysis::CPValue::nac() {
  return CPValue{CPKind::NAC, 0};
}

bool ConstantPropagationAnalysis::CPValue::operator==(const CPValue& other) const {
  return kind == other.kind && (kind != CPKind::CONST || constant == other.constant);
}

bool ConstantPropagationAnalysis::CPValue::operator!=(const CPValue& other) const {
  return !(*this == other);
}

ConstantPropagationAnalysis::CPValue ConstantPropagationAnalysis::meet_value(
    const CPValue& lhs, const CPValue& rhs) const {
  if (lhs.kind == CPKind::NAC || rhs.kind == CPKind::NAC) {
    return CPValue::nac();
  }
  if (lhs.kind == CPKind::UNDEF) {
    return rhs;
  }
  if (rhs.kind == CPKind::UNDEF) {
    return lhs;
  }
  if (lhs.constant == rhs.constant) {
    return lhs;
  }
  return CPValue::nac();
}

ConstantPropagationAnalysis::FactMap ConstantPropagationAnalysis::merge_fact_maps(
    const FactMap& lhs, const FactMap& rhs) const {
  FactMap merged;
  for (const auto& [name, value] : lhs) {
    merged[name] = meet_value(value, lookup_value(rhs, name));
  }
  for (const auto& [name, value] : rhs) {
    auto it = merged.find(name);
    if (it == merged.end()) {
      merged[name] = meet_value(lookup_value(lhs, name), value);
    }
  }
  return merged;
}

ConstantPropagationAnalysis::CPValue ConstantPropagationAnalysis::lookup_value(
    const FactMap& facts, const std::string& var_name) const {
  auto it = facts.find(var_name);
  if (it == facts.end()) {
    return CPValue::undef();
  }
  return it->second;
}

void ConstantPropagationAnalysis::generate_pred_graph(const CFGGraphs& cfg_graphs) {
  cfg_graphs_ = cfg_graphs;
}

void ConstantPropagationAnalysis::generate_GEN_KILL(const Program& program) {
  gen_kill_.clear();

  for (const auto& function : program.getFunctions()) {
    std::vector<std::vector<InstructionGenKill>> function_info;
    const auto& blocks = function->get_basicBlocks();
    function_info.reserve(blocks.size());

    for (const auto& block : blocks) {
      std::vector<InstructionGenKill> block_info;
      block_info.reserve(block->get_instructions().size() + 1);

      auto extract = [&](const Instruction& inst) {
        InstructionGenKill info;

        class GenKillVisitor : public VisitorBase {
         public:
          InstructionGenKill info;

          explicit GenKillVisitor(InstructionGenKill seed) : info(std::move(seed)) {}

          void visit(const VarDefInstruction& node) override {
            const Var* dst = as_var_or_null(node.get_var());
            if (dst != nullptr) {
              info.def = dst->name;
            }
          }

          void visit(const VarNumLabelAssignInstruction& node) override {
            const Var* dst = as_var_or_null(node.get_dst());
            if (dst != nullptr) {
              info.def = dst->name;
            }
            info.uses = collect_vars_from_node(node.get_src());
          }

          void visit(const OperatorAssignInstruction& node) override {
            const Var* dst = as_var_or_null(node.get_dst());
            if (dst != nullptr) {
              info.def = dst->name;
            }
            info.uses = collect_vars_from_node(node.get_src());
          }

          void visit(const IndexAssignInstruction& node) override {
            if (as_var_or_null(node.get_dst()) != nullptr) {
              const Var* dst = as_var_or_null(node.get_dst());
              info.def = dst->name;
            }
            info.uses = collect_vars_from_node(node.get_dst());
            auto rhs_uses = collect_vars_from_node(node.get_src());
            info.uses.insert(rhs_uses.begin(), rhs_uses.end());
          }

          void visit(const LengthAssignInstruction& node) override {
            const Var* dst = as_var_or_null(node.get_dst());
            if (dst != nullptr) {
              info.def = dst->name;
            }
            info.uses = collect_vars_from_node(node.get_src());
          }

          void visit(const CallInstruction& node) override {
            info.uses = collect_vars_from_node(node.get_call());
          }

          void visit(const CallAssignInstruction& node) override {
            const Var* dst = as_var_or_null(node.get_dst());
            if (dst != nullptr) {
              info.def = dst->name;
            }
            info.uses = collect_vars_from_node(node.get_call());
          }

          void visit(const NewArrayAssignInstruction& node) override {
            const Var* dst = as_var_or_null(node.get_dst());
            if (dst != nullptr) {
              info.def = dst->name;
            }
            info.uses = collect_vars_from_node(node.get_new_array());
          }

          void visit(const NewTupleAssignInstruction& node) override {
            const Var* dst = as_var_or_null(node.get_dst());
            if (dst != nullptr) {
              info.def = dst->name;
            }
            info.uses = collect_vars_from_node(node.get_new_tuple());
          }

          void visit(const BranchInstruction& node) override {
            info.uses = collect_vars_from_node(node.get_target());
          }

          void visit(const ConditionalBranchInstruction& node) override {
            info.uses = collect_vars_from_node(node.get_condition());
          }

          void visit(const ReturnValueInstruction& node) override {
            info.uses = collect_vars_from_node(node.get_return_value());
          }
        } visitor(std::move(info));

        inst.accept(visitor);
        block_info.push_back(std::move(visitor.info));
      };

      for (const auto& inst : block->get_instructions()) {
        extract(*inst);
      }
      extract(block->get_te());

      function_info.push_back(std::move(block_info));
    }

    gen_kill_[function.get()] = std::move(function_info);
  }
}

void ConstantPropagationAnalysis::generate_IN_OUT(const Program& program) {
  in_out_.clear();

  for (const auto& function : program.getFunctions()) {
    FunctionInOut state;
    const auto& blocks = function->get_basicBlocks();

    state.block_in.assign(blocks.size(), {});
    state.block_out.assign(blocks.size(), {});
    state.instruction_in.reserve(blocks.size());
    state.instruction_out.reserve(blocks.size());

    for (const auto& block : blocks) {
      const size_t instruction_count = block->get_instructions().size() + 1;
      state.instruction_in.push_back(std::vector<FactMap>(instruction_count));
      state.instruction_out.push_back(std::vector<FactMap>(instruction_count));
    }

    for (const auto& parameter : function->get_parameters()) {
      const Parameter* param = as_parameter_or_null(*parameter);
      if (param == nullptr) {
        continue;
      }
      const Var* var = as_var_or_null(*param->var);
      if (var == nullptr) {
        continue;
      }
      state.entry_seed[var->name] = CPValue::nac();
    }

    in_out_[function.get()] = std::move(state);
  }
}

void ConstantPropagationAnalysis::data_flow_analysis(const Program& program) {
  class TransferVisitor : public VisitorBase {
   public:
    TransferVisitor(const ConstantPropagationAnalysis* analysis,
                    ConstantPropagationAnalysis::FactMap in)
        : analysis_(analysis), out_(std::move(in)) {}

    ConstantPropagationAnalysis::FactMap result() const { return out_; }

    void visit(const VarDefInstruction& node) override {
      const Var* dst = as_var_or_null(node.get_var());
      if (dst != nullptr) {
        out_[dst->name] = ConstantPropagationAnalysis::CPValue::undef();
      }
    }

    void visit(const VarNumLabelAssignInstruction& node) override {
      const Var* dst = as_var_or_null(node.get_dst());
      if (dst == nullptr) {
        return;
      }

      if (const Number* num = as_number_or_null(node.get_src()); num != nullptr) {
        out_[dst->name] = ConstantPropagationAnalysis::CPValue::constant_value(num->value);
        return;
      }

      if (const Var* src = as_var_or_null(node.get_src()); src != nullptr) {
        out_[dst->name] = analysis_->lookup_value(out_, src->name);
        return;
      }

      out_[dst->name] = ConstantPropagationAnalysis::CPValue::nac();
    }

    void visit(const OperatorAssignInstruction& node) override {
      const Var* dst = as_var_or_null(node.get_dst());
      if (dst == nullptr) {
        return;
      }

      const Operator* op = as_operator_or_null(node.get_src());
      if (op == nullptr) {
        out_[dst->name] = ConstantPropagationAnalysis::CPValue::nac();
        return;
      }

      ConstantPropagationAnalysis::CPValue left = eval_value(*op->left);
      ConstantPropagationAnalysis::CPValue right = eval_value(*op->right);

      if (left.kind == ConstantPropagationAnalysis::CPKind::CONST &&
          right.kind == ConstantPropagationAnalysis::CPKind::CONST) {
        int64_t folded = 0;
        if (compute_binary_op(op->op, left.constant, right.constant, &folded)) {
          out_[dst->name] = ConstantPropagationAnalysis::CPValue::constant_value(folded);
          return;
        }
      }

      if (left.kind == ConstantPropagationAnalysis::CPKind::NAC ||
          right.kind == ConstantPropagationAnalysis::CPKind::NAC) {
        out_[dst->name] = ConstantPropagationAnalysis::CPValue::nac();
      } else {
        out_[dst->name] = ConstantPropagationAnalysis::CPValue::undef();
      }
    }

    void visit(const IndexAssignInstruction& node) override {
      if (as_var_or_null(node.get_dst()) != nullptr &&
          as_index_or_null(node.get_src()) != nullptr) {
        const Var* dst = as_var_or_null(node.get_dst());
        out_[dst->name] = ConstantPropagationAnalysis::CPValue::nac();
      }
    }

    void visit(const LengthAssignInstruction& node) override {
      const Var* dst = as_var_or_null(node.get_dst());
      if (dst != nullptr) {
        out_[dst->name] = ConstantPropagationAnalysis::CPValue::nac();
      }
    }

    void visit(const CallAssignInstruction& node) override {
      const Var* dst = as_var_or_null(node.get_dst());
      if (dst != nullptr) {
        out_[dst->name] = ConstantPropagationAnalysis::CPValue::nac();
      }
    }

    void visit(const NewArrayAssignInstruction& node) override {
      const Var* dst = as_var_or_null(node.get_dst());
      if (dst != nullptr) {
        out_[dst->name] = ConstantPropagationAnalysis::CPValue::nac();
      }
    }

    void visit(const NewTupleAssignInstruction& node) override {
      const Var* dst = as_var_or_null(node.get_dst());
      if (dst != nullptr) {
        out_[dst->name] = ConstantPropagationAnalysis::CPValue::nac();
      }
    }

   private:
    ConstantPropagationAnalysis::CPValue eval_value(const ASTNode& node) {
      if (const Number* num = as_number_or_null(node); num != nullptr) {
        return ConstantPropagationAnalysis::CPValue::constant_value(num->value);
      }
      if (const Var* var = as_var_or_null(node); var != nullptr) {
        return analysis_->lookup_value(out_, var->name);
      }
      return ConstantPropagationAnalysis::CPValue::nac();
    }

    const ConstantPropagationAnalysis* analysis_;
    ConstantPropagationAnalysis::FactMap out_;
  };

  for (const auto& function : program.getFunctions()) {
    auto graph_it = cfg_graphs_.find(function.get());
    auto state_it = in_out_.find(function.get());
    if (graph_it == cfg_graphs_.end() || state_it == in_out_.end()) {
      continue;
    }

    const CFGGraph& graph = graph_it->second;
    FunctionInOut& state = state_it->second;
    const auto& blocks = function->get_basicBlocks();

    bool changed = true;
    while (changed) {
      changed = false;

      for (size_t block_idx = 0; block_idx < blocks.size(); ++block_idx) {
        FactMap merged_in;
        if (block_idx == 0) {
          merged_in = state.entry_seed;
        }

        for (int pred_idx : graph.predecessors[block_idx]) {
          if (merged_in.empty() && !(block_idx == 0)) {
            merged_in = state.block_out[pred_idx];
          } else {
            merged_in = merge_fact_maps(merged_in, state.block_out[pred_idx]);
          }
        }

        if (state.block_in[block_idx] != merged_in) {
          state.block_in[block_idx] = merged_in;
          changed = true;
        }

        FactMap current = merged_in;
        const auto& instructions = blocks[block_idx]->get_instructions();
        for (size_t inst_idx = 0; inst_idx < instructions.size(); ++inst_idx) {
          if (state.instruction_in[block_idx][inst_idx] != current) {
            state.instruction_in[block_idx][inst_idx] = current;
            changed = true;
          }

          TransferVisitor visitor(this, current);
          instructions[inst_idx]->accept(visitor);
          current = visitor.result();

          if (state.instruction_out[block_idx][inst_idx] != current) {
            state.instruction_out[block_idx][inst_idx] = current;
            changed = true;
          }
        }

        const size_t te_idx = instructions.size();
        if (state.instruction_in[block_idx][te_idx] != current) {
          state.instruction_in[block_idx][te_idx] = current;
          changed = true;
        }

        TransferVisitor te_visitor(this, current);
        blocks[block_idx]->get_te().accept(te_visitor);
        current = te_visitor.result();

        if (state.instruction_out[block_idx][te_idx] != current) {
          state.instruction_out[block_idx][te_idx] = current;
          changed = true;
        }

        if (state.block_out[block_idx] != current) {
          state.block_out[block_idx] = current;
          changed = true;
        }
      }
    }
  }
}

bool ConstantPropagationAnalysis::apply_transformations(Program& program) {
  class ASTRewriter : public VisitorBase {
   public:
    ASTRewriter(const FactMap* in, bool allow_var_replacement, bool* changed)
        : in(in), allow_var_replacement(allow_var_replacement), changed(changed) {}

    std::unique_ptr<ASTNode> rewrite(const ASTNode& node) {
      node.accept(*this);
      if (out == nullptr) {
        throw std::runtime_error("AST rewrite failed");
      }
      return std::move(out);
    }

    void visit(const Var& node) override {
      if (allow_var_replacement) {
        auto it = in->find(node.name);
        if (it != in->end() &&
            it->second.kind == ConstantPropagationAnalysis::CPKind::CONST) {
          *changed = true;
          out = std::make_unique<Number>(it->second.constant);
          return;
        }
      }
      out = std::make_unique<Var>(node.name);
    }

    void visit(const Label& node) override { out = std::make_unique<Label>(node.name); }
    void visit(const Type& node) override { out = std::make_unique<Type>(node.name); }
    void visit(const Number& node) override { out = std::make_unique<Number>(node.value); }

    void visit(const Callee& node) override {
      if (const auto* enum_value = std::get_if<EnumCallee>(&node.calleeValue)) {
        EnumCallee tmp = *enum_value;
        out = std::make_unique<Callee>(tmp);
        return;
      }
      const auto* target = std::get_if<std::unique_ptr<ASTNode>>(&node.calleeValue);
      ASTRewriter nested(in, false, changed);
      out = std::make_unique<Callee>(nested.rewrite(**target));
    }

    void visit(const Operator& node) override {
      ASTRewriter left_rewriter(in, true, changed);
      ASTRewriter right_rewriter(in, true, changed);
      out = std::make_unique<Operator>(left_rewriter.rewrite(*node.left),
                                       right_rewriter.rewrite(*node.right), node.op);
    }

    void visit(const Index& node) override {
      ASTRewriter base_rewriter(in, false, changed);
      std::vector<std::unique_ptr<ASTNode>> indices;
      indices.reserve(node.indices.size());
      for (const auto& idx : node.indices) {
        ASTRewriter idx_rewriter(in, true, changed);
        indices.push_back(idx_rewriter.rewrite(*idx));
      }
      out = std::make_unique<Index>(base_rewriter.rewrite(*node.var), std::move(indices));
    }

    void visit(const ArrayLength& node) override {
      ASTRewriter var_rewriter(in, false, changed);
      ASTRewriter idx_rewriter(in, true, changed);
      out = std::make_unique<ArrayLength>(var_rewriter.rewrite(*node.var),
                                          idx_rewriter.rewrite(*node.index));
    }

    void visit(const TupleLength& node) override {
      ASTRewriter var_rewriter(in, false, changed);
      out = std::make_unique<TupleLength>(var_rewriter.rewrite(*node.var));
    }

    void visit(const FunctionCall& node) override {
      ASTRewriter callee_rewriter(in, false, changed);
      std::vector<std::unique_ptr<ASTNode>> args;
      args.reserve(node.args.size());
      for (const auto& arg : node.args) {
        ASTRewriter arg_rewriter(in, true, changed);
        args.push_back(arg_rewriter.rewrite(*arg));
      }
      out = std::make_unique<FunctionCall>(callee_rewriter.rewrite(*node.callee),
                                           std::move(args));
    }

    void visit(const NewArray& node) override {
      std::vector<std::unique_ptr<ASTNode>> args;
      args.reserve(node.args.size());
      for (const auto& arg : node.args) {
        ASTRewriter arg_rewriter(in, true, changed);
        args.push_back(arg_rewriter.rewrite(*arg));
      }
      out = std::make_unique<NewArray>(std::move(args));
    }

    void visit(const NewTuple& node) override {
      ASTRewriter length_rewriter(in, true, changed);
      out = std::make_unique<NewTuple>(length_rewriter.rewrite(*node.length));
    }

    void visit(const Parameter& node) override {
      ASTRewriter type_rewriter(in, false, changed);
      ASTRewriter var_rewriter(in, false, changed);
      out = std::make_unique<Parameter>(type_rewriter.rewrite(*node.type),
                                        var_rewriter.rewrite(*node.var));
    }

    const FactMap* in;
    bool allow_var_replacement;
    bool* changed;
    std::unique_ptr<ASTNode> out;
  };

  auto rewrite_ast = [&](const ASTNode& node, const FactMap& in, bool allow_var_replacement,
                         bool* changed) {
    ASTRewriter rewriter(&in, allow_var_replacement, changed);
    return rewriter.rewrite(node);
  };

  auto fold_operator_if_possible = [&](const ASTNode& node, bool* folded) -> std::unique_ptr<ASTNode> {
    const Operator* op = as_operator_or_null(node);
    if (op == nullptr) {
      return clone_ast(node);
    }

    const Number* left = as_number_or_null(*op->left);
    const Number* right = as_number_or_null(*op->right);
    if (left == nullptr || right == nullptr) {
      return clone_ast(node);
    }

    int64_t result = 0;
    if (!compute_binary_op(op->op, left->value, right->value, &result)) {
      return clone_ast(node);
    }

    *folded = true;
    return std::make_unique<Number>(result);
  };

  bool changed = false;

  for (auto& function : program.getFunctions()) {
    auto state_it = in_out_.find(function.get());
    if (state_it == in_out_.end()) {
      continue;
    }

    auto& blocks = function->get_basicBlocks();
    auto& state = state_it->second;

    for (size_t block_idx = 0; block_idx < blocks.size(); ++block_idx) {
      auto& instructions = blocks[block_idx]->get_instructions_mut();
      std::unordered_map<std::string, std::string> length_cache;
      std::unordered_map<std::string, std::unordered_set<std::string>>
          length_cache_deps;
      auto clear_length_cache = [&]() {
        length_cache.clear();
        length_cache_deps.clear();
      };
      auto invalidate_length_cache_for_write = [&](const std::string& written_name) {
        if (written_name.empty()) {
          return;
        }
        std::vector<std::string> to_erase;
        for (const auto& kv : length_cache) {
          const auto dep_it = length_cache_deps.find(kv.first);
          if (kv.second == written_name ||
              (dep_it != length_cache_deps.end() &&
               dep_it->second.find(written_name) != dep_it->second.end())) {
            to_erase.push_back(kv.first);
          }
        }
        for (const auto& key : to_erase) {
          length_cache.erase(key);
          length_cache_deps.erase(key);
        }
      };

      for (size_t inst_idx = 0; inst_idx < instructions.size(); ++inst_idx) {
        const FactMap& in = state.instruction_in[block_idx][inst_idx];
        const Instruction& inst = *instructions[inst_idx];

        if (const VarNumLabelAssignInstruction* assign =
                as_var_num_label_assign_or_null(inst);
            assign != nullptr) {
          std::string written_name;
          if (const Var* dst_var = as_var_or_null(assign->get_dst());
              dst_var != nullptr) {
            written_name = dst_var->name;
          }
          bool local_changed = false;
          auto dst = clone_ast(assign->get_dst());
          auto src = rewrite_ast(assign->get_src(), in, true, &local_changed);
          if (local_changed) {
            instructions[inst_idx] =
                std::make_unique<VarNumLabelAssignInstruction>(std::move(dst), std::move(src));
            changed = true;
          }
          if (!written_name.empty()) {
            invalidate_length_cache_for_write(written_name);
          }
          continue;
        }

        if (const OperatorAssignInstruction* assign = as_operator_assign_or_null(inst);
            assign != nullptr) {
          std::string written_name;
          if (const Var* dst_var = as_var_or_null(assign->get_dst());
              dst_var != nullptr) {
            written_name = dst_var->name;
          }
          bool local_changed = false;
          auto dst = clone_ast(assign->get_dst());
          auto src = rewrite_ast(assign->get_src(), in, true, &local_changed);

          bool folded = false;
          auto folded_src = fold_operator_if_possible(*src, &folded);
          if (folded) {
            instructions[inst_idx] = std::make_unique<VarNumLabelAssignInstruction>(
                std::move(dst), std::move(folded_src));
            changed = true;
          } else if (local_changed) {
            instructions[inst_idx] =
                std::make_unique<OperatorAssignInstruction>(std::move(dst), std::move(src));
            changed = true;
          }
          if (!written_name.empty()) {
            invalidate_length_cache_for_write(written_name);
          }
          continue;
        }

        if (const IndexAssignInstruction* index_assign = as_index_assign_or_null(inst);
            index_assign != nullptr) {
          std::string written_name;
          if (const Var* dst_var = as_var_or_null(index_assign->get_dst());
              dst_var != nullptr) {
            written_name = dst_var->name;
          }
          bool local_changed = false;
          auto dst = rewrite_ast(index_assign->get_dst(), in,
                                 as_index_or_null(index_assign->get_dst()) != nullptr,
                                 &local_changed);
          auto src = rewrite_ast(index_assign->get_src(), in,
                                 as_index_or_null(index_assign->get_src()) != nullptr,
                                 &local_changed);
          if (local_changed) {
            instructions[inst_idx] =
                std::make_unique<IndexAssignInstruction>(std::move(dst), std::move(src));
            changed = true;
          }
          if (!written_name.empty()) {
            invalidate_length_cache_for_write(written_name);
          }
          continue;
        }

        if (const LengthAssignInstruction* length_assign = as_length_assign_or_null(inst);
            length_assign != nullptr) {
          std::string written_name;
          if (const Var* dst_var = as_var_or_null(length_assign->get_dst());
              dst_var != nullptr) {
            written_name = dst_var->name;
          }
          bool local_changed = false;
          auto dst = rewrite_ast(length_assign->get_dst(), in, false, &local_changed);
          auto src = rewrite_ast(length_assign->get_src(), in, true, &local_changed);
          if (local_changed) {
            instructions[inst_idx] =
                std::make_unique<LengthAssignInstruction>(std::move(dst), std::move(src));
            changed = true;
          }
          if (!written_name.empty()) {
            invalidate_length_cache_for_write(written_name);
          }
          continue;
        }

        if (const CallInstruction* call = as_call_instruction_or_null(inst);
            call != nullptr) {
          clear_length_cache();
          bool local_changed = false;
          auto rewritten_call = rewrite_ast(call->get_call(), in, true, &local_changed);
          if (local_changed) {
            instructions[inst_idx] = std::make_unique<CallInstruction>(std::move(rewritten_call));
            changed = true;
          }
          continue;
        }

        if (const CallAssignInstruction* call_assign = as_call_assign_or_null(inst);
            call_assign != nullptr) {
          clear_length_cache();
          std::string written_name;
          if (const Var* dst_var = as_var_or_null(call_assign->get_dst());
              dst_var != nullptr) {
            written_name = dst_var->name;
          }
          bool local_changed = false;
          auto dst = rewrite_ast(call_assign->get_dst(), in, false, &local_changed);
          auto call = rewrite_ast(call_assign->get_call(), in, true, &local_changed);
          if (local_changed) {
            instructions[inst_idx] =
                std::make_unique<CallAssignInstruction>(std::move(dst), std::move(call));
            changed = true;
          }
          if (!written_name.empty()) {
            invalidate_length_cache_for_write(written_name);
          }
          continue;
        }

        if (const NewArrayAssignInstruction* new_array_assign =
                as_new_array_assign_or_null(inst);
            new_array_assign != nullptr) {
          clear_length_cache();
          std::string written_name;
          if (const Var* dst_var = as_var_or_null(new_array_assign->get_dst());
              dst_var != nullptr) {
            written_name = dst_var->name;
          }
          bool local_changed = false;
          auto dst = rewrite_ast(new_array_assign->get_dst(), in, false, &local_changed);
          auto new_array =
              rewrite_ast(new_array_assign->get_new_array(), in, true, &local_changed);
          if (local_changed) {
            instructions[inst_idx] = std::make_unique<NewArrayAssignInstruction>(
                std::move(dst), std::move(new_array));
            changed = true;
          }
          if (!written_name.empty()) {
            invalidate_length_cache_for_write(written_name);
          }
          continue;
        }

        if (const NewTupleAssignInstruction* new_tuple_assign =
                as_new_tuple_assign_or_null(inst);
            new_tuple_assign != nullptr) {
          clear_length_cache();
          std::string written_name;
          if (const Var* dst_var = as_var_or_null(new_tuple_assign->get_dst());
              dst_var != nullptr) {
            written_name = dst_var->name;
          }
          bool local_changed = false;
          auto dst = rewrite_ast(new_tuple_assign->get_dst(), in, false, &local_changed);
          auto new_tuple =
              rewrite_ast(new_tuple_assign->get_new_tuple(), in, true, &local_changed);
          if (local_changed) {
            instructions[inst_idx] = std::make_unique<NewTupleAssignInstruction>(
                std::move(dst), std::move(new_tuple));
            changed = true;
          }
          if (!written_name.empty()) {
            invalidate_length_cache_for_write(written_name);
          }
          continue;
        }

        if (const ReturnValueInstruction* ret = as_return_value_instruction_or_null(inst);
            ret != nullptr) {
          bool local_changed = false;
          auto ret_value = rewrite_ast(ret->get_return_value(), in, true, &local_changed);
          if (local_changed) {
            instructions[inst_idx] =
                std::make_unique<ReturnValueInstruction>(std::move(ret_value));
            changed = true;
          }
          continue;
        }

        if (const VarDefInstruction* var_def = as_var_def_instruction_or_null(inst);
            var_def != nullptr) {
          std::string written_name;
          if (const Var* dst_var = as_var_or_null(var_def->get_var());
              dst_var != nullptr) {
            written_name = dst_var->name;
          }
          instructions[inst_idx] = std::make_unique<VarDefInstruction>(
              clone_ast(var_def->get_type()), clone_ast(var_def->get_var()));
          if (!written_name.empty()) {
            invalidate_length_cache_for_write(written_name);
          }
          continue;
        }
      }

      const size_t te_idx = blocks[block_idx]->get_instructions().size();
      const FactMap& te_in = state.instruction_in[block_idx][te_idx];
      const Instruction& te = blocks[block_idx]->get_te();
      if (const ConditionalBranchInstruction* cond = as_conditional_branch_or_null(te);
          cond != nullptr) {
        bool local_changed = false;
        auto condition = rewrite_ast(cond->get_condition(), te_in, true, &local_changed);
        const Number* folded_cond = as_number_or_null(*condition);
        if (folded_cond != nullptr) {
          if (folded_cond->value != 0) {
            blocks[block_idx]->set_te(
                std::make_unique<BranchInstruction>(clone_ast(cond->get_true_target())));
          } else {
            blocks[block_idx]->set_te(
                std::make_unique<BranchInstruction>(clone_ast(cond->get_false_target())));
          }
          changed = true;
        } else if (local_changed) {
          blocks[block_idx]->set_te(std::make_unique<ConditionalBranchInstruction>(
              std::move(condition), clone_ast(cond->get_true_target()),
              clone_ast(cond->get_false_target())));
          changed = true;
        }
      } else if (const ReturnValueInstruction* ret = as_return_value_instruction_or_null(te);
                 ret != nullptr) {
        bool local_changed = false;
        auto rewritten = rewrite_ast(ret->get_return_value(), te_in, true, &local_changed);
        if (local_changed) {
          blocks[block_idx]->set_te(
              std::make_unique<ReturnValueInstruction>(std::move(rewritten)));
          changed = true;
        }
      }
    }

    std::unordered_map<std::string, int> label_to_index;
    label_to_index.reserve(blocks.size());
    for (size_t i = 0; i < blocks.size(); ++i) {
      label_to_index[blocks[i]->get_label().name] = static_cast<int>(i);
    }

    std::vector<bool> reachable(blocks.size(), false);
    std::deque<int> queue;
    if (!blocks.empty()) {
      reachable[0] = true;
      queue.push_back(0);
    }

    while (!queue.empty()) {
      int idx = queue.front();
      queue.pop_front();

      const Instruction& te = blocks[idx]->get_te();
      if (const BranchInstruction* branch = as_branch_or_null(te); branch != nullptr) {
        const Label* target = as_label_or_null(branch->get_target());
        if (target != nullptr) {
          auto it = label_to_index.find(target->name);
          if (it != label_to_index.end() && !reachable[it->second]) {
            reachable[it->second] = true;
            queue.push_back(it->second);
          }
        }
      } else if (const ConditionalBranchInstruction* cond = as_conditional_branch_or_null(te);
                 cond != nullptr) {
        const Label* true_target = as_label_or_null(cond->get_true_target());
        const Label* false_target = as_label_or_null(cond->get_false_target());
        if (true_target != nullptr) {
          auto it = label_to_index.find(true_target->name);
          if (it != label_to_index.end() && !reachable[it->second]) {
            reachable[it->second] = true;
            queue.push_back(it->second);
          }
        }
        if (false_target != nullptr) {
          auto it = label_to_index.find(false_target->name);
          if (it != label_to_index.end() && !reachable[it->second]) {
            reachable[it->second] = true;
            queue.push_back(it->second);
          }
        }
      }
    }

    if (std::any_of(reachable.begin(), reachable.end(), [](bool value) { return !value; })) {
      std::vector<std::unique_ptr<BasicBlock>> old_blocks;
      old_blocks.swap(blocks);
      blocks.reserve(old_blocks.size());
      for (size_t i = 0; i < old_blocks.size(); ++i) {
        if (reachable[i]) {
          blocks.push_back(std::move(old_blocks[i]));
        }
      }
      changed = true;
    }
  }

  return changed;
}

}  // namespace IR
