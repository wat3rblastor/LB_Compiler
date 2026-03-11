
#include "program.h"

#include <functional>
#include <sstream>
#include <stack>
#include <string>
#include <variant>

namespace L2 {

void RegisterAllocator::generate_GEN_KILL(
    const std::vector<std::unique_ptr<Instruction>>& instructions) {
  genKill.clear();
  int numInstructions = instructions.size();
  for (const auto& instruction : instructions) {
    genKill.push_back(instruction->generate_GEN_KILL());
  }
}

void RegisterAllocator::generate_succGraph(
    const std::vector<std::unique_ptr<Instruction>>& instructions) {
  succGraph.clear();

  const size_t n = instructions.size();
  succGraph.assign(n, {});

  auto findLabelIndex = [&](const std::string& labelName) -> size_t {
    for (size_t k = 0; k < n; ++k) {
      if (auto* labelInstruction =
              dynamic_cast<LabelInstruction*>(instructions[k].get())) {
        if (auto* label = dynamic_cast<Label*>(labelInstruction->label.get())) {
          if (label->name == labelName) return k;
        } else {
          throw std::runtime_error(
              "Label instruction label is not of type Label");
        }
      }
    }
    throw std::runtime_error("Label not found: " + labelName);
  };

  for (size_t i = 0; i < n; ++i) {
    if (dynamic_cast<ReturnInstruction*>(instructions[i].get()) ||
        dynamic_cast<CallTupleErrorInstruction*>(instructions[i].get()) ||
        dynamic_cast<CallTensorErrorInstruction*>(instructions[i].get())) {
      continue;
    }

    if (auto* uj = dynamic_cast<JumpInstruction*>(instructions[i].get())) {
      const std::string labelName = dynamic_cast<Label*>(uj->label.get())->name;
      succGraph[i].push_back(findLabelIndex(labelName));
      continue;
    }

    if (auto* cj =
            dynamic_cast<ConditionalJumpInstruction*>(instructions[i].get())) {
      const std::string labelName = dynamic_cast<Label*>(cj->label.get())->name;
      succGraph[i].push_back(findLabelIndex(labelName));
      if (i + 1 < n) succGraph[i].push_back(i + 1);
      continue;
    }

    if (i + 1 < n) succGraph[i].push_back(i + 1);
  }
}

void RegisterAllocator::generate_IN_OUT() {
  int numInstructions = genKill.size();
  inOut.clear();
  inOut.assign(numInstructions, {});
  bool changeToInOut = true;
  while (changeToInOut) {
    changeToInOut = false;
    for (int i = numInstructions - 1; i >= 0; --i) {
      LocationSet new_out_locations;
      for (size_t succ : succGraph[i]) {
        new_out_locations =
            set_union(new_out_locations, inOut[succ].in_locations);
      }
      changeToInOut |=
          set_assignment(inOut[i].out_locations, new_out_locations);
      LocationSet locationSetDifference =
          set_difference(inOut[i].out_locations, genKill[i].kill_locations);
      LocationSet locationGenUnion =
          set_union(genKill[i].gen_locations, locationSetDifference);
      changeToInOut |= set_assignment(inOut[i].in_locations, locationGenUnion);
    }
  }
}

std::string RegisterAllocator::IN_OUT_to_string() const {
  int numInstructions = genKill.size();

  std::ostringstream out;
  out << "(" << std::endl;
  out << "(in" << std::endl;

  for (int i = 0; i < numInstructions; ++i) {
    out << "(";
    for (Location l : inOut[i].in_locations) {
      out << display_location(l);
      out << " ";
    }
    out << ")" << std::endl;
  }
  out << ")" << std::endl << std::endl;
  out << "(out" << std::endl;

  for (int i = 0; i < numInstructions; ++i) {
    out << "(";
    for (Location l : inOut[i].out_locations) {
      out << display_location(l);
      out << " ";
    }
    out << ")" << std::endl;
  }
  out << ")" << std::endl << std::endl;
  out << ")";

  return out.str();
}

void RegisterAllocator::generate_interference_graph(
    const std::vector<std::unique_ptr<Instruction>>& instructions) {
  interfGraph.clear();

  // Generate graph nodes that appear anywhere in liveness
  auto ensure_node = [&](const Location& l) {
    if (std::holds_alternative<RegisterID>(l) &&
        std::get<RegisterID>(l) == RegisterID::rsp)
      return;
    interfGraph[l];  // create empty adjacency if missing
  };

  for (size_t i = 0; i < inOut.size(); ++i) {
    for (const auto& l : genKill[i].gen_locations) ensure_node(l);
    for (const auto& l : genKill[i].kill_locations) ensure_node(l);
    for (const auto& l : inOut[i].in_locations) ensure_node(l);
    for (const auto& l : inOut[i].out_locations) ensure_node(l);
  }

  // Connect each pair of variables that belong to the same IN or OUT set
  for (size_t i = 0; i < inOut.size(); ++i) {
    for (auto l1 : inOut[i].in_locations) {
      for (auto l2 : inOut[i].in_locations) {
        if (l1 == l2) continue;
        if (std::holds_alternative<RegisterID>(l1) &&
            std::get<RegisterID>(l1) == RegisterID::rsp)
          continue;
        if (std::holds_alternative<RegisterID>(l2) &&
            std::get<RegisterID>(l2) == RegisterID::rsp)
          continue;
        interfGraph[l1].insert(l2);
      }
    }
    for (auto l1 : inOut[i].out_locations) {
      for (auto l2 : inOut[i].out_locations) {
        if (l1 == l2) continue;
        if (std::holds_alternative<RegisterID>(l1) &&
            std::get<RegisterID>(l1) == RegisterID::rsp)
          continue;
        if (std::holds_alternative<RegisterID>(l2) &&
            std::get<RegisterID>(l2) == RegisterID::rsp)
          continue;
        interfGraph[l1].insert(l2);
      }
    }
  }

  // Connect GP register to all other registers
  std::vector<RegisterID> registerIDs = {
      RegisterID::rdi, RegisterID::rsi, RegisterID::rdx, RegisterID::rcx,
      RegisterID::r8,  RegisterID::r9,  RegisterID::rax, RegisterID::rbx,
      RegisterID::rbp, RegisterID::r10, RegisterID::r11, RegisterID::r12,
      RegisterID::r13, RegisterID::r14, RegisterID::r15,
  };

  for (size_t i = 0; i < registerIDs.size(); ++i) {
    for (size_t j = 0; j < registerIDs.size(); ++j) {
      if (i == j) continue;
      interfGraph[registerIDs[i]].insert(registerIDs[j]);
    }
  }

  // Connect variables in KILL[i] with those in OUT[i]
  for (size_t i = 0; i < inOut.size(); ++i) {
    for (auto l1 : genKill[i].kill_locations) {
      for (auto l2 : inOut[i].out_locations) {
        if (l1 == l2) continue;
        if (std::holds_alternative<RegisterID>(l1) &&
            std::get<RegisterID>(l1) == RegisterID::rsp)
          continue;
        if (std::holds_alternative<RegisterID>(l2) &&
            std::get<RegisterID>(l2) == RegisterID::rsp)
          continue;
        interfGraph[l1].insert(l2);
        interfGraph[l2].insert(l1);
      }
    }
  }

  // Add contraints of L1, or x sop wsx
  int numInstructions = instructions.size();
  for (size_t i = 0; i < numInstructions; ++i) {
    if (ShiftInstruction* sInst =
            dynamic_cast<ShiftInstruction*>(instructions[i].get())) {
      ShiftOp* op = sInst->op.get();
      if (Var* v = dynamic_cast<Var*>(op->src.get())) {
        for (const RegisterID& reg : registerIDs) {
          if (reg == RegisterID::rcx) continue;
          interfGraph[v->name].insert(reg);
          interfGraph[reg].insert(v->name);
        }
      }
    }
  }
}

std::string RegisterAllocator::interference_graph_to_string() const {
  std::ostringstream out;

  for (const auto& kv : interfGraph) {
    const Location& node = kv.first;
    const auto& value = kv.second;
    out << display_location(node);
    for (const Location& neighbor : value) {
      out << " " << display_location(neighbor);
    }
    out << std::endl;
  }
  return out.str();
}

// After this, spilledVars contains spilled vars (if any)
void RegisterAllocator::color_graph() {
  coloring.clear();
  toSpillVars.clear();

  // Removing nodes from graph and adding to stack
  std::stack<std::string> varStack;
  std::unordered_map<Location, int, LocationHash> degree;

  for (const auto& kv : interfGraph) {
    degree[kv.first] = kv.second.size();
  }

  // Remove nodes with less than 15 edges
  while (true) {
    std::string removeVar;
    for (auto it = degree.begin(); it != degree.end(); ++it) {
      if (it->second <= -1) continue;
      if (std::holds_alternative<RegisterID>(it->first)) continue;
      if (it->second < 15) {
        if (removeVar.empty()) {
          removeVar = std::get<std::string>(it->first);
        } else if (it->second > degree[removeVar]) {
          removeVar = std::get<std::string>(it->first);
        }
      }
    }
    if (removeVar.empty()) break;
    for (const auto& neighbor : interfGraph[removeVar]) {
      degree[neighbor]--;
    }
    degree[removeVar] = -1;
    varStack.push(removeVar);
  }

  // Remove nodes in descending order of edges
  while (true) {
    std::string removeVar;
    for (auto it = degree.begin(); it != degree.end(); ++it) {
      if (it->second <= -1) continue;
      if (std::holds_alternative<RegisterID>(it->first)) continue;
      if (removeVar.empty()) {
        removeVar = std::get<std::string>(it->first);
      } else if (it->second > degree[removeVar]) {
        removeVar = std::get<std::string>(it->first);
      }
    }
    if (removeVar.empty()) break;
    for (const auto& neighbor : interfGraph[removeVar]) {
      degree[neighbor]--;
    }
    degree[removeVar] = -1;
    varStack.push(removeVar);
  }

  // Color the variables
  std::vector<RegisterID> coloringOrdering = {
      RegisterID::r10, RegisterID::r11, RegisterID::r8,  RegisterID::r9,
      RegisterID::rax, RegisterID::rcx, RegisterID::rdi, RegisterID::rdx,
      RegisterID::rsi, RegisterID::r12, RegisterID::r13, RegisterID::r14,
      RegisterID::r15, RegisterID::rbp, RegisterID::rbx};

  // vars to unordered_set<registerID>
  std::unordered_map<Location, std::unordered_set<Location, LocationHash>,
                     LocationHash>
      unavailableColors;
  toSpillVars.clear();
  while (!varStack.empty()) {
    std::string var = varStack.top();
    varStack.pop();
    bool toSpill = true;
    for (const auto& id : coloringOrdering) {
      if (interfGraph[var].find(id) == interfGraph[var].end() &&
          (unavailableColors.find(var) == unavailableColors.end() ||
           unavailableColors[var].find(id) == unavailableColors[var].end())) {
        // Var is chosen as color id
        coloring[var] = id;
        for (const auto& neighbor : interfGraph[var]) {
          unavailableColors[neighbor].insert(id);
        }
        toSpill = false;
        break;
      }
    }
    if (toSpill) {
      toSpillVars.push_back(var);
    }
  }
}

void RegisterAllocator::spill(
    std::vector<std::unique_ptr<Instruction>>& modifiedInstructions,
    const std::string& var, const std::string& prefix) {
  std::vector<std::unique_ptr<Instruction>> newInstructions;
  if (genKill.size() != modifiedInstructions.size()) {
    throw std::runtime_error(
        "genKil.size() != modifiedInstructions.size() for "
        "RegisterAllocator::spill");
  }
  int numInstructions = genKill.size();
  bool spilledVar = false;
  for (int i = 0; i < numInstructions; ++i) {
    // Push back memLoad if exist in GEN
    std::string sVar = prefix + std::to_string(spilledVarIdx);
    bool incrementSpilledVarIdx = false;
    if (genKill[i].gen_locations.find(var) != genKill[i].gen_locations.end()) {
      std::unique_ptr<MemoryAddress> memAddr = std::make_unique<MemoryAddress>(
          std::make_unique<Register>(RegisterID::rsp),
          std::make_unique<Number>(numLocals * 8));
      std::unique_ptr<Instruction> memLoad =
          std::make_unique<AssignmentInstruction>(std::make_unique<Var>(sVar),
                                                  std::move(memAddr));
      newInstructions.push_back(std::move(memLoad));
      incrementSpilledVarIdx = true;
      spilledVar = true;
    }
    // Push back copiedInstruction (but need to replace with sVar)
    std::unique_ptr<Instruction> copiedInstruction(
        static_cast<Instruction*>(modifiedInstructions[i]->clone().release()));
    instruction_replace_var(copiedInstruction, var, sVar);
    newInstructions.push_back(std::move(copiedInstruction));
    // Push back memStore if exist in KILL
    if (genKill[i].kill_locations.find(var) !=
        genKill[i].kill_locations.end()) {
      std::unique_ptr<MemoryAddress> memAddr = std::make_unique<MemoryAddress>(
          std::make_unique<Register>(RegisterID::rsp),
          std::make_unique<Number>(numLocals * 8));
      std::unique_ptr<Instruction> memStore =
          std::make_unique<AssignmentInstruction>(std::move(memAddr),
                                                  std::make_unique<Var>(sVar));
      newInstructions.push_back(std::move(memStore));
      incrementSpilledVarIdx = true;
      spilledVar = true;
    }
    if (incrementSpilledVarIdx) spilledVarIdx++;
  }
  if (spilledVar) numLocals++;
  modifiedInstructions = std::move(newInstructions);
}

void RegisterAllocator::implement_coloring(
    std::vector<std::unique_ptr<Instruction>>& modifiedInstructions) {
  for (auto& instruction : modifiedInstructions) {
    for (const auto& kv : coloring) {
      std::string varName = kv.first;
      RegisterID id = kv.second;
      instruction_replace_var(instruction, varName, id);
    }
  }
}

LocationSet RegisterAllocator::set_union(const LocationSet& a,
                                         const LocationSet& b) {
  LocationSet unionSet = a;
  for (const auto& location : b) {
    unionSet.insert(location);
  }
  return unionSet;
}

LocationSet RegisterAllocator::set_difference(const LocationSet& a,
                                              const LocationSet& b) {
  LocationSet difference = a;
  for (const auto& location : b) {
    difference.erase(location);
  }
  return difference;
}

// Returns true if the set changed
bool RegisterAllocator::set_assignment(LocationSet& dst,
                                       const LocationSet& src) {
  bool change = !(dst == src);
  dst = src;
  return change;
}

std::string RegisterAllocator::display_location(const Location& l) {
  std::ostringstream out;
  std::visit(
      [&out](auto&& x) {
        using T = std::decay_t<decltype(x)>;
        if constexpr (std::is_same_v<T, RegisterID>) {
          switch (x) {
            case RegisterID::rdi:
              out << "rdi";
              break;
            case RegisterID::rsi:
              out << "rsi";
              break;
            case RegisterID::rdx:
              out << "rdx";
              break;
            case RegisterID::rcx:
              out << "rcx";
              break;
            case RegisterID::r8:
              out << "r8";
              break;
            case RegisterID::r9:
              out << "r9";
              break;
            case RegisterID::rax:
              out << "rax";
              break;
            case RegisterID::rbx:
              out << "rbx";
              break;
            case RegisterID::rbp:
              out << "rbp";
              break;
            case RegisterID::r10:
              out << "r10";
              break;
            case RegisterID::r11:
              out << "r11";
              break;
            case RegisterID::r12:
              out << "r12";
              break;
            case RegisterID::r13:
              out << "r13";
              break;
            case RegisterID::r14:
              out << "r14";
              break;
            case RegisterID::r15:
              out << "r15";
              break;
            case RegisterID::rsp:
              // Look into this logic
              break;
            default:
              throw std::runtime_error(
                  "Invalid RegisterId for generate_IN_OUT");
              break;
          }
        } else if constexpr (std::is_same_v<T, std::string>) {
          out << x;
        } else {
          throw std::runtime_error(
              "generate_in_out location visit has an invalid type");
        }
      },
      l);

  return out.str();
}

void RegisterAllocator::instruction_replace_var(
    std::unique_ptr<Instruction>& instruction, const std::string& oldVar,
    const Location& newLoc) {
  if (!instruction) return;

  std::function<void(std::unique_ptr<ASTNode>&)> replace_node =
      [&](std::unique_ptr<ASTNode>& node) {
        if (!node) return;

        // Var leaf
        if (auto* v = dynamic_cast<Var*>(node.get())) {
          if (v->name != oldVar) return;

          if (std::holds_alternative<std::string>(newLoc)) {
            v->name = std::get<std::string>(newLoc);
          } else {
            node = std::make_unique<Register>(std::get<RegisterID>(newLoc));
          }
          return;
        }

        // MemoryAddress: recurse into reg + offset
        if (auto* mem = dynamic_cast<MemoryAddress*>(node.get())) {
          replace_node(mem->reg);
          return;
        }

        // Operators
        if (auto* aop = dynamic_cast<ArithmeticOp*>(node.get())) {
          replace_node(aop->dst);
          replace_node(aop->src);
          return;
        }
        if (auto* sop = dynamic_cast<ShiftOp*>(node.get())) {
          replace_node(sop->dst);
          replace_node(sop->src);
          return;
        }
        if (auto* cop = dynamic_cast<CompareOp*>(node.get())) {
          replace_node(cop->left);
          replace_node(cop->right);
          return;
        }

        // Label/Number/Register: nothing to do
      };

  if (auto* i = dynamic_cast<AssignmentInstruction*>(instruction.get())) {
    replace_node(i->dst);
    replace_node(i->src);
    return;
  }

  if (auto* i = dynamic_cast<ArithmeticInstruction*>(instruction.get())) {
    if (i->op) {
      replace_node(i->op->dst);
      replace_node(i->op->src);
    }
    return;
  }

  if (auto* i = dynamic_cast<CompareInstruction*>(instruction.get())) {
    replace_node(i->dst);
    if (i->op) {
      replace_node(i->op->left);
      replace_node(i->op->right);
    }
    return;
  }

  if (auto* i = dynamic_cast<ShiftInstruction*>(instruction.get())) {
    if (i->op) {
      replace_node(i->op->dst);
      replace_node(i->op->src);
    }
    return;
  }

  if (auto* i = dynamic_cast<ConditionalJumpInstruction*>(instruction.get())) {
    if (i->op) {
      replace_node(i->op->left);
      replace_node(i->op->right);
    }
    replace_node(i->label);
    return;
  }

  if (auto* i = dynamic_cast<CallInstruction*>(instruction.get())) {
    replace_node(i->target);
    return;
  }

  if (auto* i = dynamic_cast<IncrementInstruction*>(instruction.get())) {
    replace_node(i->reg);
    return;
  }

  if (auto* i = dynamic_cast<DecrementInstruction*>(instruction.get())) {
    replace_node(i->reg);
    return;
  }

  if (auto* i = dynamic_cast<LeaInstruction*>(instruction.get())) {
    replace_node(i->reg1);
    replace_node(i->reg2);
    replace_node(i->reg3);
    return;
  }
}

void RegisterAllocator::replace_stack_args(
    std::vector<std::unique_ptr<Instruction>>& instructions) {
  for (auto& instruction : instructions) {
    auto* assignInst = dynamic_cast<AssignmentInstruction*>(instruction.get());
    if (!assignInst) continue;
    auto* stackArg = dynamic_cast<StackArg*>(assignInst->src.get());
    if (!stackArg) continue;
    auto* offsetPtr = dynamic_cast<Number*>(stackArg->offset.get());
    int64_t offset = offsetPtr->num;
    int64_t modifiedOffset = static_cast<int64_t>(numLocals) * 8 + offset;
    assignInst->src = std::make_unique<MemoryAddress>(
        std::make_unique<Register>(RegisterID::rsp),
        std::make_unique<Number>(modifiedOffset));
  }
}

std::string Function::to_string() const {
  std::ostringstream out;
  out << "(" << name.to_string() << "\n";
  out << "\t" << numArgs << " " << regAlloc.numLocals << "\n";
  for (const auto& instruction : modifiedInstructions) {
    out << "\t" << instruction->to_string() << "\n";
  }
  out << ")\n";
  return out.str();
}

void Function::generate_code(std::ofstream& outputFile) const {
  outputFile << "\t(";
  name.generate_code(outputFile);
  outputFile << "\n";
  outputFile << "\t\t" << numArgs << " " << regAlloc.numLocals << "\n";
  for (const auto& instruction : modifiedInstructions) {
    outputFile << "\t\t";
    instruction->generate_code(outputFile);
    outputFile << "\n";
  }
  outputFile << "\t)\n";
}

std::unique_ptr<ASTNode> Function::clone() const { TODO("Function::clone()"); }

std::string Function::test_liveness() {
  regAlloc.generate_GEN_KILL(instructions);
  regAlloc.generate_succGraph(instructions);
  regAlloc.generate_IN_OUT();
  return regAlloc.IN_OUT_to_string();
}

std::string Function::test_interference() {
  regAlloc.generate_GEN_KILL(instructions);
  regAlloc.generate_succGraph(instructions);
  regAlloc.generate_IN_OUT();
  regAlloc.generate_interference_graph(instructions);
  return regAlloc.interference_graph_to_string();
}

std::string Function::test_spill() {
  modifiedInstructions.clear();
  for (const auto& instruction : instructions) {
    std::unique_ptr<Instruction> copiedInstruction(
        static_cast<Instruction*>(instruction->clone().release()));
    modifiedInstructions.push_back(std::move(copiedInstruction));
  }
  regAlloc.generate_GEN_KILL(modifiedInstructions);
  regAlloc.spill(modifiedInstructions, testSpillVarName, testSpillPrefixName);
  return to_string();
}

void Function::allocate() {
  regAlloc.numLocals = 0;
  regAlloc.spilledVarIdx = 0;
  regAlloc.coloring.clear();
  regAlloc.toSpillVars.clear();
  modifiedInstructions.clear();
  for (const auto& instruction : instructions) {
    std::unique_ptr<Instruction> copiedInstruction(
        static_cast<Instruction*>(instruction->clone().release()));
    modifiedInstructions.push_back(std::move(copiedInstruction));
  }
  while (true) {
    regAlloc.generate_GEN_KILL(modifiedInstructions);
    regAlloc.generate_succGraph(modifiedInstructions);
    regAlloc.generate_IN_OUT();
    regAlloc.generate_interference_graph(modifiedInstructions);
    regAlloc.color_graph();
    if (regAlloc.toSpillVars.empty()) break;
    for (const auto& var : regAlloc.toSpillVars) {
      regAlloc.generate_GEN_KILL(modifiedInstructions);
      regAlloc.spill(modifiedInstructions, var, "%S");
    }
  }
  regAlloc.implement_coloring(modifiedInstructions);
  regAlloc.replace_stack_args(modifiedInstructions);
}

std::string Program::to_string() const {
  std::ostringstream out;
  out << "(" << entryPoint.to_string() << "\n";
  for (const auto& function : functions) {
    function->allocate();
    out << function->to_string() << "\n";
  }
  out << ")" << "\n";
  return out.str();
}

void Program::generate_code(std::ofstream& outputFile) const {
  outputFile << "(";
  entryPoint.generate_code(outputFile);
  outputFile << "\n";
  for (const auto& function : functions) {
    function->allocate();
    function->generate_code(outputFile);
    outputFile << "\n";
  }
  outputFile << ")\n";
}

std::unique_ptr<ASTNode> Program::clone() const { TODO("Program::clone"); }

}  // namespace L2