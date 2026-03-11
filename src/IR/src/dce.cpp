#include "dce.h"

#include <optional>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "node_utils.h"

namespace IR {
namespace {

using VarSet = std::unordered_set<std::string>;

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

const VarDefInstruction* as_var_def_or_null(const Instruction& inst) {
  class Matcher : public VisitorBase {
   public:
    const VarDefInstruction* out = nullptr;
    void visit(const VarDefInstruction& value) override { out = &value; }
  } matcher;
  inst.accept(matcher);
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

const ReturnValueInstruction* as_return_value_or_null(const Instruction& inst) {
  class Matcher : public VisitorBase {
   public:
    const ReturnValueInstruction* out = nullptr;
    void visit(const ReturnValueInstruction& value) override { out = &value; }
  } matcher;
  inst.accept(matcher);
  return matcher.out;
}

VarSet collect_vars_from_node(const ASTNode& node) {
  class Collector : public VisitorBase {
   public:
    VarSet vars;

    void collect(const ASTNode& n) { n.accept(*this); }

    void visit(const Var& node) override { vars.insert(node.name); }

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
  } collector;

  collector.collect(node);
  return collector.vars;
}

struct InstInfo {
  std::optional<std::string> def;
  VarSet use;
  bool removable = false;
};

void add_block_use_def(const InstInfo& info, VarSet* use, VarSet* def) {
  for (const auto& var : info.use) {
    if (def->find(var) == def->end()) {
      use->insert(var);
    }
  }
  if (info.def.has_value()) {
    def->insert(*info.def);
  }
}

void subtract_in_place(VarSet* lhs, const VarSet& rhs) {
  for (const auto& var : rhs) {
    lhs->erase(var);
  }
}

InstInfo analyze_instruction(const Instruction& inst) {
  InstInfo info;

  if (const VarDefInstruction* i = as_var_def_or_null(inst); i != nullptr) {
    const Var* var = as_var_or_null(i->get_var());
    if (var != nullptr) {
      info.def = var->name;
      info.removable = false;
    }
    return info;
  }

  if (const VarNumLabelAssignInstruction* i = as_var_num_label_assign_or_null(inst);
      i != nullptr) {
    const Var* dst = as_var_or_null(i->get_dst());
    if (dst != nullptr) {
      info.def = dst->name;
      info.removable = true;
    }
    info.use = collect_vars_from_node(i->get_src());
    return info;
  }

  if (const OperatorAssignInstruction* i = as_operator_assign_or_null(inst);
      i != nullptr) {
    const Var* dst = as_var_or_null(i->get_dst());
    if (dst != nullptr) {
      info.def = dst->name;
      info.removable = true;
    }
    info.use = collect_vars_from_node(i->get_src());
    return info;
  }

  if (const IndexAssignInstruction* i = as_index_assign_or_null(inst); i != nullptr) {
    const Var* dst = as_var_or_null(i->get_dst());
    const Index* src_index = as_index_or_null(i->get_src());
    if (dst != nullptr && src_index != nullptr) {
      info.def = dst->name;
      info.removable = true;
      info.use = collect_vars_from_node(i->get_src());
      return info;
    }

    info.removable = false;
    info.use = collect_vars_from_node(i->get_dst());
    VarSet rhs = collect_vars_from_node(i->get_src());
    info.use.insert(rhs.begin(), rhs.end());
    return info;
  }

  if (const LengthAssignInstruction* i = as_length_assign_or_null(inst); i != nullptr) {
    const Var* dst = as_var_or_null(i->get_dst());
    if (dst != nullptr) {
      info.def = dst->name;
      info.removable = true;
    }
    info.use = collect_vars_from_node(i->get_src());
    return info;
  }

  if (const CallInstruction* i = as_call_instruction_or_null(inst); i != nullptr) {
    info.removable = false;
    info.use = collect_vars_from_node(i->get_call());
    return info;
  }

  if (const CallAssignInstruction* i = as_call_assign_or_null(inst); i != nullptr) {
    info.removable = false;
    const Var* dst = as_var_or_null(i->get_dst());
    if (dst != nullptr) {
      info.def = dst->name;
    }
    info.use = collect_vars_from_node(i->get_call());
    return info;
  }

  if (const NewArrayAssignInstruction* i = as_new_array_assign_or_null(inst); i != nullptr) {
    const Var* dst = as_var_or_null(i->get_dst());
    if (dst != nullptr) {
      info.def = dst->name;
    }
    info.removable = false;
    info.use = collect_vars_from_node(i->get_new_array());
    return info;
  }

  if (const NewTupleAssignInstruction* i = as_new_tuple_assign_or_null(inst); i != nullptr) {
    const Var* dst = as_var_or_null(i->get_dst());
    if (dst != nullptr) {
      info.def = dst->name;
    }
    info.removable = false;
    info.use = collect_vars_from_node(i->get_new_tuple());
    return info;
  }

  if (const ConditionalBranchInstruction* i = as_conditional_branch_or_null(inst);
      i != nullptr) {
    info.removable = false;
    info.use = collect_vars_from_node(i->get_condition());
    return info;
  }

  if (const BranchInstruction* i = as_branch_or_null(inst); i != nullptr) {
    info.removable = false;
    info.use = collect_vars_from_node(i->get_target());
    return info;
  }

  if (const ReturnValueInstruction* i = as_return_value_or_null(inst); i != nullptr) {
    info.removable = false;
    info.use = collect_vars_from_node(i->get_return_value());
    return info;
  }

  info.removable = false;
  return info;
}

}  // namespace

bool DeadCodeElimination::run(Program& program, const CFGGraphs& cfg_graphs) {
  bool changed = false;

  for (auto& function : program.getFunctions()) {
    auto graph_it = cfg_graphs.find(function.get());
    if (graph_it == cfg_graphs.end()) {
      continue;
    }

    const CFGGraph& cfg = graph_it->second;
    auto& blocks = function->get_basicBlocks();

    std::vector<VarSet> block_use(blocks.size());
    std::vector<VarSet> block_def(blocks.size());
    std::vector<std::vector<InstInfo>> inst_info(blocks.size());

    for (size_t b = 0; b < blocks.size(); ++b) {
      auto& infos = inst_info[b];
      infos.reserve(blocks[b]->get_instructions().size() + 1);

      for (const auto& inst : blocks[b]->get_instructions()) {
        InstInfo info = analyze_instruction(*inst);
        add_block_use_def(info, &block_use[b], &block_def[b]);
        infos.push_back(std::move(info));
      }

      InstInfo te_info = analyze_instruction(blocks[b]->get_te());
      add_block_use_def(te_info, &block_use[b], &block_def[b]);
      infos.push_back(std::move(te_info));
    }

    std::vector<VarSet> block_in(blocks.size());
    std::vector<VarSet> block_out(blocks.size());

    bool local_change = true;
    while (local_change) {
      local_change = false;

      for (int b = static_cast<int>(blocks.size()) - 1; b >= 0; --b) {
        VarSet new_out;
        for (int succ : cfg.successors[b]) {
          new_out.insert(block_in[succ].begin(), block_in[succ].end());
        }

        VarSet new_in = block_use[b];
        VarSet out_minus_def = new_out;
        subtract_in_place(&out_minus_def, block_def[b]);
        new_in.insert(out_minus_def.begin(), out_minus_def.end());

        if (new_out != block_out[b]) {
          block_out[b] = std::move(new_out);
          local_change = true;
        }
        if (new_in != block_in[b]) {
          block_in[b] = std::move(new_in);
          local_change = true;
        }
      }
    }

    for (size_t b = 0; b < blocks.size(); ++b) {
      const auto& infos = inst_info[b];
      VarSet current = block_out[b];
      std::vector<VarSet> out_at(infos.size());

      for (int i = static_cast<int>(infos.size()) - 1; i >= 0; --i) {
        out_at[i] = current;
        if (infos[i].def.has_value()) {
          current.erase(*infos[i].def);
        }
        current.insert(infos[i].use.begin(), infos[i].use.end());
      }

      auto& instructions = blocks[b]->get_instructions_mut();
      std::vector<std::unique_ptr<Instruction>> kept;
      kept.reserve(instructions.size());

      for (size_t i = 0; i < instructions.size(); ++i) {
        const InstInfo& info = infos[i];
        bool remove = false;
        if (info.removable && info.def.has_value()) {
          remove = (out_at[i].find(*info.def) == out_at[i].end());
        }

        if (remove) {
          changed = true;
        } else {
          kept.push_back(std::move(instructions[i]));
        }
      }

      instructions = std::move(kept);
    }
  }

  return changed;
}

}  // namespace IR
