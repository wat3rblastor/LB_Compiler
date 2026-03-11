#include <fstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "code_generator.h"
#include "instruction.h"
#include "program.h"

namespace LB {

namespace {

using ScopeMaps = std::vector<std::unordered_map<std::string, std::string>>;

ASTNode& mutable_node(const ASTNode& node) {
  return const_cast<ASTNode&>(node);
}

std::string lookup_renamed_name(const ScopeMaps& scopeMaps,
                                const std::string& originalName) {
  for (auto it = scopeMaps.rbegin(); it != scopeMaps.rend(); ++it) {
    auto found = it->find(originalName);
    if (found != it->end()) {
      return found->second;
    }
  }
  return "";
}

void rewrite_names_in_ast(ASTNode& node, const ScopeMaps& scopeMaps) {
  if (auto* name = dynamic_cast<Name*>(&node)) {
    const std::string mapped = lookup_renamed_name(scopeMaps, name->name);
    if (!mapped.empty()) {
      name->name = mapped;
    }
    return;
  }

  if (auto* op = dynamic_cast<Operator*>(&node)) {
    rewrite_names_in_ast(*op->left, scopeMaps);
    rewrite_names_in_ast(*op->right, scopeMaps);
    return;
  }

  if (auto* cond = dynamic_cast<CondOperator*>(&node)) {
    rewrite_names_in_ast(*cond->left, scopeMaps);
    rewrite_names_in_ast(*cond->right, scopeMaps);
    return;
  }

  if (auto* index = dynamic_cast<Index*>(&node)) {
    rewrite_names_in_ast(*index->name, scopeMaps);
    for (auto& term : index->indices) {
      rewrite_names_in_ast(*term, scopeMaps);
    }
    return;
  }

  if (auto* length = dynamic_cast<Length*>(&node)) {
    rewrite_names_in_ast(*length->name, scopeMaps);
    if (length->hasIndex) {
      rewrite_names_in_ast(*length->index, scopeMaps);
    }
    return;
  }

  if (auto* call = dynamic_cast<FunctionCall*>(&node)) {
    rewrite_names_in_ast(*call->name, scopeMaps);
    for (auto& arg : call->args) {
      rewrite_names_in_ast(*arg, scopeMaps);
    }
    return;
  }

  if (auto* newArray = dynamic_cast<NewArray*>(&node)) {
    for (auto& arg : newArray->args) {
      rewrite_names_in_ast(*arg, scopeMaps);
    }
    return;
  }

  if (auto* newTuple = dynamic_cast<NewTuple*>(&node)) {
    rewrite_names_in_ast(*newTuple->length, scopeMaps);
    return;
  }
}

void rewrite_instruction_refs(Instruction& instruction, const ScopeMaps& scopeMaps) {
  if (auto* inst = dynamic_cast<NameNumAssignInstruction*>(&instruction)) {
    rewrite_names_in_ast(mutable_node(inst->get_dst()), scopeMaps);
    rewrite_names_in_ast(mutable_node(inst->get_src()), scopeMaps);
    return;
  }

  if (auto* inst = dynamic_cast<OperatorAssignInstruction*>(&instruction)) {
    rewrite_names_in_ast(mutable_node(inst->get_dst()), scopeMaps);
    rewrite_names_in_ast(mutable_node(inst->get_src()), scopeMaps);
    return;
  }

  if (auto* inst = dynamic_cast<ConditionalBranchInstruction*>(&instruction)) {
    rewrite_names_in_ast(mutable_node(inst->get_condition()), scopeMaps);
    return;
  }

  if (auto* inst = dynamic_cast<ReturnValueInstruction*>(&instruction)) {
    rewrite_names_in_ast(mutable_node(inst->get_return_value()), scopeMaps);
    return;
  }

  if (auto* inst = dynamic_cast<WhileInstruction*>(&instruction)) {
    rewrite_names_in_ast(mutable_node(inst->get_condition()), scopeMaps);
    return;
  }

  if (auto* inst = dynamic_cast<IndexAssignInstruction*>(&instruction)) {
    rewrite_names_in_ast(mutable_node(inst->get_dst()), scopeMaps);
    rewrite_names_in_ast(mutable_node(inst->get_src()), scopeMaps);
    return;
  }

  if (auto* inst = dynamic_cast<LengthAssignInstruction*>(&instruction)) {
    rewrite_names_in_ast(mutable_node(inst->get_dst()), scopeMaps);
    rewrite_names_in_ast(mutable_node(inst->get_src()), scopeMaps);
    return;
  }

  if (auto* inst = dynamic_cast<CallInstruction*>(&instruction)) {
    rewrite_names_in_ast(mutable_node(inst->get_function_call()), scopeMaps);
    return;
  }

  if (auto* inst = dynamic_cast<CallAssignInstruction*>(&instruction)) {
    rewrite_names_in_ast(mutable_node(inst->get_dst()), scopeMaps);
    rewrite_names_in_ast(mutable_node(inst->get_function_call()), scopeMaps);
    return;
  }

  if (auto* inst = dynamic_cast<NewArrayAssignInstruction*>(&instruction)) {
    rewrite_names_in_ast(mutable_node(inst->get_dst()), scopeMaps);
    rewrite_names_in_ast(mutable_node(inst->get_new_array()), scopeMaps);
    return;
  }

  if (auto* inst = dynamic_cast<NewTupleAssignInstruction*>(&instruction)) {
    rewrite_names_in_ast(mutable_node(inst->get_dst()), scopeMaps);
    rewrite_names_in_ast(mutable_node(inst->get_new_tuple()), scopeMaps);
    return;
  }
}

void rewrite_scope(Scope& scope, ScopeMaps& scopeMaps,
                   std::unordered_map<std::string, int>& nameCounters) {
  scopeMaps.emplace_back();
  for (auto& node : scope.getNodes()) {
    if (auto* nestedScope = dynamic_cast<Scope*>(node.get())) {
      rewrite_scope(*nestedScope, scopeMaps, nameCounters);
      continue;
    }

    auto* instruction = dynamic_cast<Instruction*>(node.get());
    if (!instruction) {
      continue;
    }

    if (auto* def = dynamic_cast<NameDefInstruction*>(instruction)) {
      auto* declaredName = dynamic_cast<Name*>(&mutable_node(def->get_name()));
      if (!declaredName) {
        continue;
      }

      const std::string originalName = declaredName->name;
      const int suffix = nameCounters[originalName]++;
      const std::string renamed = originalName + "_" + std::to_string(suffix);

      declaredName->name = renamed;
      scopeMaps.back()[originalName] = renamed;
      continue;
    }

    rewrite_instruction_refs(*instruction, scopeMaps);
  }
  scopeMaps.pop_back();
}

void flatten_scope_nodes(Scope& scope) {
  auto& nodes = scope.getNodes();
  std::vector<std::unique_ptr<ASTNode>> flattened;
  flattened.reserve(nodes.size());

  for (auto& node : nodes) {
    auto* nestedScope = dynamic_cast<Scope*>(node.get());
    if (!nestedScope) {
      flattened.push_back(std::move(node));
      continue;
    }

    flatten_scope_nodes(*nestedScope);
    auto& nestedNodes = nestedScope->getNodes();
    for (auto& nestedNode : nestedNodes) {
      flattened.push_back(std::move(nestedNode));
    }
  }

  nodes = std::move(flattened);
}

std::string label_name_from_node(const ASTNode& node) {
  if (const auto* label = dynamic_cast<const Label*>(&node)) {
    return label->name;
  }
  return "";
}

std::unique_ptr<ASTNode> clone_ast_node(const ASTNode& node) {
  if (const auto* name = dynamic_cast<const Name*>(&node)) {
    return std::make_unique<Name>(name->name);
  }
  if (const auto* number = dynamic_cast<const Number*>(&node)) {
    return std::make_unique<Number>(number->value);
  }
  if (const auto* label = dynamic_cast<const Label*>(&node)) {
    return std::make_unique<Label>(label->name);
  }
  if (const auto* type = dynamic_cast<const Type*>(&node)) {
    return std::make_unique<Type>(type->name);
  }
  if (const auto* op = dynamic_cast<const Operator*>(&node)) {
    return std::make_unique<Operator>(clone_ast_node(*op->left),
                                      clone_ast_node(*op->right), op->op);
  }
  if (const auto* cond = dynamic_cast<const CondOperator*>(&node)) {
    return std::make_unique<CondOperator>(clone_ast_node(*cond->left),
                                          clone_ast_node(*cond->right), cond->op);
  }
  if (const auto* index = dynamic_cast<const Index*>(&node)) {
    std::vector<std::unique_ptr<ASTNode>> indices;
    for (const auto& term : index->indices) {
      indices.push_back(clone_ast_node(*term));
    }
    return std::make_unique<Index>(index->lineNumber, clone_ast_node(*index->name),
                                   std::move(indices));
  }
  if (const auto* length = dynamic_cast<const Length*>(&node)) {
    if (length->hasIndex) {
      return std::make_unique<Length>(length->lineNumber,
                                      clone_ast_node(*length->name),
                                      clone_ast_node(*length->index));
    }
    return std::make_unique<Length>(length->lineNumber, clone_ast_node(*length->name));
  }
  if (const auto* call = dynamic_cast<const FunctionCall*>(&node)) {
    std::vector<std::unique_ptr<ASTNode>> args;
    for (const auto& arg : call->args) {
      args.push_back(clone_ast_node(*arg));
    }
    return std::make_unique<FunctionCall>(clone_ast_node(*call->name), std::move(args));
  }
  if (const auto* newArray = dynamic_cast<const NewArray*>(&node)) {
    std::vector<std::unique_ptr<ASTNode>> args;
    for (const auto& arg : newArray->args) {
      args.push_back(clone_ast_node(*arg));
    }
    return std::make_unique<NewArray>(std::move(args));
  }
  if (const auto* newTuple = dynamic_cast<const NewTuple*>(&node)) {
    return std::make_unique<NewTuple>(clone_ast_node(*newTuple->length));
  }
  throw std::runtime_error("clone_ast_node: unsupported AST node");
}

void collect_used_names_from_ast(const ASTNode& node,
                                 std::unordered_set<std::string>& usedNames) {
  if (const auto* name = dynamic_cast<const Name*>(&node)) {
    usedNames.insert(name->name);
    return;
  }

  if (const auto* op = dynamic_cast<const Operator*>(&node)) {
    collect_used_names_from_ast(*op->left, usedNames);
    collect_used_names_from_ast(*op->right, usedNames);
    return;
  }

  if (const auto* cond = dynamic_cast<const CondOperator*>(&node)) {
    collect_used_names_from_ast(*cond->left, usedNames);
    collect_used_names_from_ast(*cond->right, usedNames);
    return;
  }

  if (const auto* index = dynamic_cast<const Index*>(&node)) {
    collect_used_names_from_ast(*index->name, usedNames);
    for (const auto& term : index->indices) {
      collect_used_names_from_ast(*term, usedNames);
    }
    return;
  }

  if (const auto* length = dynamic_cast<const Length*>(&node)) {
    collect_used_names_from_ast(*length->name, usedNames);
    if (length->hasIndex) {
      collect_used_names_from_ast(*length->index, usedNames);
    }
    return;
  }

  if (const auto* call = dynamic_cast<const FunctionCall*>(&node)) {
    collect_used_names_from_ast(*call->name, usedNames);
    for (const auto& arg : call->args) {
      collect_used_names_from_ast(*arg, usedNames);
    }
    return;
  }

  if (const auto* newArray = dynamic_cast<const NewArray*>(&node)) {
    for (const auto& arg : newArray->args) {
      collect_used_names_from_ast(*arg, usedNames);
    }
    return;
  }

  if (const auto* newTuple = dynamic_cast<const NewTuple*>(&node)) {
    collect_used_names_from_ast(*newTuple->length, usedNames);
    return;
  }
}

void collect_used_names_from_instruction(const Instruction& instruction,
                                         std::unordered_set<std::string>& usedNames) {
  if (const auto* inst = dynamic_cast<const NameDefInstruction*>(&instruction)) {
    collect_used_names_from_ast(inst->get_name(), usedNames);
    return;
  }
  if (const auto* inst = dynamic_cast<const NameNumAssignInstruction*>(&instruction)) {
    collect_used_names_from_ast(inst->get_dst(), usedNames);
    collect_used_names_from_ast(inst->get_src(), usedNames);
    return;
  }
  if (const auto* inst = dynamic_cast<const OperatorAssignInstruction*>(&instruction)) {
    collect_used_names_from_ast(inst->get_dst(), usedNames);
    collect_used_names_from_ast(inst->get_src(), usedNames);
    return;
  }
  if (const auto* inst = dynamic_cast<const ConditionalBranchInstruction*>(&instruction)) {
    collect_used_names_from_ast(inst->get_condition(), usedNames);
    return;
  }
  if (const auto* inst = dynamic_cast<const WhileInstruction*>(&instruction)) {
    collect_used_names_from_ast(inst->get_condition(), usedNames);
    return;
  }
  if (const auto* inst = dynamic_cast<const ReturnValueInstruction*>(&instruction)) {
    collect_used_names_from_ast(inst->get_return_value(), usedNames);
    return;
  }
  if (const auto* inst = dynamic_cast<const IndexAssignInstruction*>(&instruction)) {
    collect_used_names_from_ast(inst->get_dst(), usedNames);
    collect_used_names_from_ast(inst->get_src(), usedNames);
    return;
  }
  if (const auto* inst = dynamic_cast<const LengthAssignInstruction*>(&instruction)) {
    collect_used_names_from_ast(inst->get_dst(), usedNames);
    collect_used_names_from_ast(inst->get_src(), usedNames);
    return;
  }
  if (const auto* inst = dynamic_cast<const CallInstruction*>(&instruction)) {
    collect_used_names_from_ast(inst->get_function_call(), usedNames);
    return;
  }
  if (const auto* inst = dynamic_cast<const CallAssignInstruction*>(&instruction)) {
    collect_used_names_from_ast(inst->get_dst(), usedNames);
    collect_used_names_from_ast(inst->get_function_call(), usedNames);
    return;
  }
  if (const auto* inst = dynamic_cast<const NewArrayAssignInstruction*>(&instruction)) {
    collect_used_names_from_ast(inst->get_dst(), usedNames);
    collect_used_names_from_ast(inst->get_new_array(), usedNames);
    return;
  }
  if (const auto* inst = dynamic_cast<const NewTupleAssignInstruction*>(&instruction)) {
    collect_used_names_from_ast(inst->get_dst(), usedNames);
    collect_used_names_from_ast(inst->get_new_tuple(), usedNames);
  }
}

std::string fresh_name(const std::string& prefix,
                       std::unordered_set<std::string>& usedNames,
                       int& counter) {
  while (true) {
    const std::string candidate = prefix + std::to_string(counter++);
    if (usedNames.insert(candidate).second) {
      return candidate;
    }
  }
}

std::string fresh_label(const std::string& prefix,
                        std::unordered_set<std::string>& usedLabels,
                        int& counter) {
  while (true) {
    const std::string candidate = ":" + prefix + std::to_string(counter++);
    if (usedLabels.insert(candidate).second) {
      return candidate;
    }
  }
}

}  // namespace

void translate_variable_decl(Program& p) {
  for (auto& function : p.getFunctions()) {
    auto* scope = dynamic_cast<Scope*>(&function->getScope());
    if (!scope) {
      continue;
    }

    ScopeMaps scopeMaps;
    std::unordered_map<std::string, int> nameCounters;
    rewrite_scope(*scope, scopeMaps, nameCounters);
  }
}

void flatten_nested_scopes(Program& p) {
  for (auto& function : p.getFunctions()) {
    auto* scope = dynamic_cast<Scope*>(&function->getScope());
    if (!scope) {
      continue;
    }
    flatten_scope_nodes(*scope);
  }
}

void translate_if_while(Program& p) {
  for (auto& function : p.getFunctions()) {
    auto* scope = dynamic_cast<Scope*>(&function->getScope());
    if (!scope) {
      continue;
    }

    auto& nodes = scope->getNodes();
    std::vector<std::pair<size_t, Instruction*>> instructions;
    instructions.reserve(nodes.size());
    for (size_t i = 0; i < nodes.size(); ++i) {
      auto* inst = dynamic_cast<Instruction*>(nodes[i].get());
      if (inst) {
        instructions.emplace_back(i, inst);
      }
    }

    struct WhileInfo {
      WhileInstruction* inst;
      std::string beginLabel;
      std::string endLabel;
      std::string condLabel;
    };

    std::vector<WhileInfo> whiles;
    whiles.reserve(instructions.size());
    std::unordered_map<WhileInstruction*, size_t> whileIndex;
    std::unordered_map<std::string, WhileInstruction*> beginToWhile;
    std::unordered_map<std::string, WhileInstruction*> endToWhile;
    std::unordered_set<std::string> usedLabels;
    std::unordered_set<std::string> usedNames;

    for (const auto& [idx, inst] : instructions) {
      (void)idx;
      collect_used_names_from_instruction(*inst, usedNames);
      if (const auto* labelInst = dynamic_cast<const LabelInstruction*>(inst)) {
        const std::string labelName = label_name_from_node(labelInst->get_label());
        if (!labelName.empty()) {
          usedLabels.insert(labelName);
        }
      }
      if (auto* whileInst = dynamic_cast<WhileInstruction*>(inst)) {
        const std::string beginLabel = label_name_from_node(whileInst->get_true_target());
        const std::string endLabel = label_name_from_node(whileInst->get_false_target());
        whileIndex[whileInst] = whiles.size();
        whiles.push_back({whileInst, beginLabel, endLabel, ""});
        if (!beginLabel.empty()) {
          beginToWhile[beginLabel] = whileInst;
        }
        if (!endLabel.empty()) {
          endToWhile[endLabel] = whileInst;
        }
      }
    }

    for (const auto& parameterNode : function->getParameters()) {
      if (const auto* parameter = dynamic_cast<const Parameter*>(parameterNode.get())) {
        collect_used_names_from_ast(*parameter->name, usedNames);
      }
    }

    int labelCounter = 0;
    for (auto& whileInfo : whiles) {
      whileInfo.condLabel =
          fresh_label("__lb_while_cond_", usedLabels, labelCounter);
    }

    std::unordered_map<const Instruction*, WhileInstruction*> loopOf;
    std::vector<WhileInstruction*> loopStack;
    for (const auto& [idx, inst] : instructions) {
      (void)idx;
      if (!loopStack.empty()) {
        loopOf[inst] = loopStack.back();
      }

      auto* labelInst = dynamic_cast<LabelInstruction*>(inst);
      if (!labelInst) {
        continue;
      }

      const std::string labelName = label_name_from_node(labelInst->get_label());
      auto beginIt = beginToWhile.find(labelName);
      if (beginIt != beginToWhile.end()) {
        loopStack.push_back(beginIt->second);
      }

      auto endIt = endToWhile.find(labelName);
      if (endIt == endToWhile.end()) {
        continue;
      }

      if (!loopStack.empty() && loopStack.back() == endIt->second) {
        loopStack.pop_back();
        continue;
      }

      for (auto it = loopStack.end(); it != loopStack.begin();) {
        --it;
        if (*it == endIt->second) {
          loopStack.erase(it);
          break;
        }
      }
    }

    int tempCounter = 0;
    std::vector<std::unique_ptr<ASTNode>> lowered;
    lowered.reserve(nodes.size() * 2);
    std::string sharedCondTmp;

    auto ensure_condition_tmp = [&]() -> const std::string& {
      if (sharedCondTmp.empty()) {
        sharedCondTmp = fresh_name("__lb_cond_tmp_", usedNames, tempCounter);
      }
      return sharedCondTmp;
    };

    for (auto& node : nodes) {
      auto* inst = dynamic_cast<Instruction*>(node.get());
      if (!inst) {
        lowered.push_back(std::move(node));
        continue;
      }

      if (auto* whileInst = dynamic_cast<WhileInstruction*>(inst)) {
        auto whileIt = whileIndex.find(whileInst);
        if (whileIt == whileIndex.end()) {
          lowered.push_back(std::move(node));
          continue;
        }
        const size_t idx = whileIt->second;
        lowered.push_back(std::make_unique<LabelInstruction>(
            std::make_unique<Label>(whiles[idx].condLabel)));

        const auto* cond = dynamic_cast<const CondOperator*>(&whileInst->get_condition());
        if (cond) {
          const std::string& tmpVar = ensure_condition_tmp();
          lowered.push_back(std::make_unique<NameDefInstruction>(
              std::make_unique<Type>("int64"), std::make_unique<Name>(tmpVar)));
          lowered.push_back(std::make_unique<OperatorAssignInstruction>(
              std::make_unique<Name>(tmpVar),
              std::make_unique<Operator>(clone_ast_node(*cond->left),
                                         clone_ast_node(*cond->right), cond->op)));
          lowered.push_back(std::make_unique<ConditionalBranchInstruction>(
              std::make_unique<Name>(tmpVar), clone_ast_node(whileInst->get_true_target()),
              clone_ast_node(whileInst->get_false_target())));
          continue;
        }

        lowered.push_back(std::move(node));
        continue;
      }

      if (auto* ifInst = dynamic_cast<ConditionalBranchInstruction*>(inst)) {
        const auto* cond = dynamic_cast<const CondOperator*>(&ifInst->get_condition());
        if (cond) {
          const std::string& tmpVar = ensure_condition_tmp();
          lowered.push_back(std::make_unique<NameDefInstruction>(
              std::make_unique<Type>("int64"), std::make_unique<Name>(tmpVar)));
          lowered.push_back(std::make_unique<OperatorAssignInstruction>(
              std::make_unique<Name>(tmpVar),
              std::make_unique<Operator>(clone_ast_node(*cond->left),
                                         clone_ast_node(*cond->right), cond->op)));
          lowered.push_back(std::make_unique<ConditionalBranchInstruction>(
              std::make_unique<Name>(tmpVar), clone_ast_node(ifInst->get_true_target()),
              clone_ast_node(ifInst->get_false_target())));
          continue;
        }

        lowered.push_back(std::move(node));
        continue;
      }

      if (dynamic_cast<ContinueInstruction*>(inst)) {
        auto loopIt = loopOf.find(inst);
        if (loopIt == loopOf.end()) {
          lowered.push_back(std::move(node));
          continue;
        }
        auto whileIt = whileIndex.find(loopIt->second);
        if (whileIt == whileIndex.end()) {
          lowered.push_back(std::move(node));
          continue;
        }
        const size_t idx = whileIt->second;
        lowered.push_back(std::make_unique<BranchInstruction>(
            std::make_unique<Label>(whiles[idx].condLabel)));
        continue;
      }

      if (dynamic_cast<BreakInstruction*>(inst)) {
        auto loopIt = loopOf.find(inst);
        if (loopIt == loopOf.end()) {
          lowered.push_back(std::move(node));
          continue;
        }
        auto whileIt = whileIndex.find(loopIt->second);
        if (whileIt == whileIndex.end()) {
          lowered.push_back(std::move(node));
          continue;
        }
        const size_t idx = whileIt->second;
        lowered.push_back(std::make_unique<BranchInstruction>(
            std::make_unique<Label>(whiles[idx].endLabel)));
        continue;
      }

      lowered.push_back(std::move(node));
    }

    nodes = std::move(lowered);
  }
}

void generate_code(const Program& p) {
  std::ofstream outputFile;
  outputFile.open("prog.a");
  p.generate_code(outputFile);
  outputFile.close();
}

}
