#include "code_generator.h"

#include <fstream>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "instruction.h"

namespace LA {

namespace {

int tempCounter = 0;
std::unordered_set<std::string> reservedCodegenNames;

void reserve_codegen_name(const std::string& name) {
  if (name.empty()) {
    return;
  }
  if (name[0] == ':') {
    reservedCodegenNames.insert(name.substr(1));
    return;
  }
  reservedCodegenNames.insert(name);
}

std::string fresh_codegen_var() {
  while (true) {
    const std::string candidate = "new_var_" + std::to_string(tempCounter++);
    if (reservedCodegenNames.find(candidate) != reservedCodegenNames.end()) {
      continue;
    }
    reserve_codegen_name(candidate);
    return candidate;
  }
}

int64_t encode_number(int64_t value) {
  return value * 2 + 1;
}

bool is_int64_type(const std::string& typeName) {
  return typeName == "int64";
}

bool is_array_like_type(const Type& type) {
  return type.name == "tuple" || type.name == "code" || type.name.rfind("int64[", 0) == 0;
}

std::string node_name(const ASTNode& node) {
  if (const auto* name = dynamic_cast<const Name*>(&node)) {
    return name->name;
  }
  if (const auto* label = dynamic_cast<const Label*>(&node)) {
    return label->name;
  }
  return "";
}

std::unique_ptr<ASTNode> clone_ast(const ASTNode& node) {
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
    return std::make_unique<Operator>(clone_ast(*op->left), clone_ast(*op->right), op->op);
  }
  if (const auto* index = dynamic_cast<const Index*>(&node)) {
    std::vector<std::unique_ptr<ASTNode>> indices;
    for (const auto& term : index->indices) {
      indices.push_back(clone_ast(*term));
    }
    return std::make_unique<Index>(index->lineNumber, clone_ast(*index->name), std::move(indices));
  }
  if (const auto* length = dynamic_cast<const Length*>(&node)) {
    if (length->hasIndex) {
      return std::make_unique<Length>(length->lineNumber, clone_ast(*length->name), clone_ast(*length->index));
    }
    return std::make_unique<Length>(length->lineNumber, clone_ast(*length->name));
  }
  if (const auto* call = dynamic_cast<const FunctionCall*>(&node)) {
    std::vector<std::unique_ptr<ASTNode>> args;
    for (const auto& arg : call->args) {
      args.push_back(clone_ast(*arg));
    }
    return std::make_unique<FunctionCall>(clone_ast(*call->name), std::move(args));
  }
  if (const auto* newArray = dynamic_cast<const NewArray*>(&node)) {
    std::vector<std::unique_ptr<ASTNode>> args;
    for (const auto& arg : newArray->args) {
      args.push_back(clone_ast(*arg));
    }
    return std::make_unique<NewArray>(std::move(args));
  }
  if (const auto* newTuple = dynamic_cast<const NewTuple*>(&node)) {
    return std::make_unique<NewTuple>(clone_ast(*newTuple->length));
  }
  return nullptr;
}

std::string lookup_type(const std::unordered_map<std::string, std::string>& varTypes,
                        const std::string& name) {
  auto it = varTypes.find(name);
  if (it == varTypes.end()) {
    return "";
  }
  return it->second;
}

bool node_is_encoded_int(const ASTNode& node,
                         const std::unordered_map<std::string, std::string>& varTypes) {
  if (dynamic_cast<const Number*>(&node)) {
    return true;
  }
  if (const auto* name = dynamic_cast<const Name*>(&node)) {
    return is_int64_type(lookup_type(varTypes, name->name));
  }
  return false;
}

std::unique_ptr<ASTNode> encode_term_if_needed(const ASTNode& node, bool needEncoded) {
  if (!needEncoded) {
    return clone_ast(node);
  }
  if (const auto* number = dynamic_cast<const Number*>(&node)) {
    return std::make_unique<Number>(encode_number(number->value));
  }
  return clone_ast(node);
}

std::unique_ptr<ASTNode> decode_term_to_decoded(
    std::vector<std::unique_ptr<Instruction>>& emitted,
    const ASTNode& node,
    const std::unordered_map<std::string, std::string>& varTypes,
    std::unordered_map<std::string, std::string>& decodedCache) {
  if (const auto* number = dynamic_cast<const Number*>(&node)) {
    return std::make_unique<Number>(number->value);
  }

  const auto* name = dynamic_cast<const Name*>(&node);
  if (!name || !is_int64_type(lookup_type(varTypes, name->name))) {
    return clone_ast(node);
  }

  auto cacheIt = decodedCache.find(name->name);
  if (cacheIt != decodedCache.end()) {
    return std::make_unique<Name>(cacheIt->second);
  }

  const std::string decodedTmp = fresh_codegen_var();
  emitted.push_back(std::make_unique<NameDefInstruction>(
      std::make_unique<Type>("int64"), std::make_unique<Name>(decodedTmp)));
  emitted.push_back(std::make_unique<OperatorAssignInstruction>(
      std::make_unique<Name>(decodedTmp),
      std::make_unique<Operator>(clone_ast(node), std::make_unique<Number>(1), OP::RSHIFT)));
  decodedCache[name->name] = decodedTmp;
  return std::make_unique<Name>(decodedTmp);
}

std::unique_ptr<Index> decode_index_for_access(
    std::vector<std::unique_ptr<Instruction>>& emitted,
    const Index& index,
    const std::unordered_map<std::string, std::string>& varTypes,
    std::unordered_map<std::string, std::string>& decodedCache) {
  std::vector<std::unique_ptr<ASTNode>> newIndices;
  for (const auto& term : index.indices) {
    newIndices.push_back(decode_term_to_decoded(emitted, *term, varTypes, decodedCache));
  }
  return std::make_unique<Index>(index.lineNumber, clone_ast(*index.name), std::move(newIndices));
}

struct FunctionSignature {
  std::string returnType;
  std::vector<std::string> paramTypes;
};

FunctionSignature collect_signature(const Function& function) {
  FunctionSignature sig;
  if (const auto* type = dynamic_cast<const Type*>(&function.getType())) {
    sig.returnType = type->name;
  }
  for (const auto& parameterNode : function.getParameters()) {
    const auto* parameter = dynamic_cast<const Parameter*>(parameterNode.get());
    if (!parameter) {
      continue;
    }
    const auto* type = dynamic_cast<const Type*>(parameter->type.get());
    if (!type) {
      continue;
    }
    sig.paramTypes.push_back(type->name);
  }
  return sig;
}

std::unique_ptr<FunctionCall> encode_function_call_args(
    const FunctionCall& call,
    const std::unordered_map<std::string, FunctionSignature>& signatures,
    const std::unordered_map<std::string, std::string>& varTypes) {
  const std::string calleeName = node_name(*call.name);

  std::vector<std::unique_ptr<ASTNode>> args;
  for (size_t i = 0; i < call.args.size(); ++i) {
    bool needEncoded = false;

    auto sigIt = signatures.find(calleeName);
    if (sigIt != signatures.end() && i < sigIt->second.paramTypes.size()) {
      needEncoded = is_int64_type(sigIt->second.paramTypes[i]);
    } else if (calleeName == "print") {
      if (const auto* argName = dynamic_cast<const Name*>(call.args[i].get())) {
        needEncoded = is_int64_type(lookup_type(varTypes, argName->name));
      } else {
        needEncoded = dynamic_cast<const Number*>(call.args[i].get()) != nullptr;
      }
    } else if (calleeName == "tensor-error") {
      needEncoded = true;
    }

    args.push_back(encode_term_if_needed(*call.args[i], needEncoded));
  }

  return std::make_unique<FunctionCall>(clone_ast(*call.name), std::move(args));
}

bool get_declared_array_like_var(const Instruction& instruction, std::string& outVar) {
  const auto* def = dynamic_cast<const NameDefInstruction*>(&instruction);
  if (!def) {
    return false;
  }

  const auto* type = dynamic_cast<const Type*>(&def->get_type());
  const auto* name = dynamic_cast<const Name*>(&def->get_name());
  if (!type || !name) {
    return false;
  }

  if (!is_array_like_type(*type)) {
    return false;
  }

  outVar = name->name;
  return true;
}

bool get_accessed_array_like_var_and_line(const Instruction& instruction,
                                          std::string& outVar,
                                          int64_t& outLine) {
  if (const auto* indexAssign = dynamic_cast<const IndexAssignInstruction*>(&instruction)) {
    const Index* index = dynamic_cast<const Index*>(&indexAssign->get_dst());
    if (!index) {
      index = dynamic_cast<const Index*>(&indexAssign->get_src());
    }
    if (!index) {
      return false;
    }
    const auto* name = dynamic_cast<const Name*>(index->name.get());
    if (!name) {
      return false;
    }
    outVar = name->name;
    outLine = index->lineNumber;
    return true;
  }

  if (const auto* lengthAssign = dynamic_cast<const LengthAssignInstruction*>(&instruction)) {
    const auto* length = dynamic_cast<const Length*>(&lengthAssign->get_src());
    if (!length) {
      return false;
    }
    const auto* name = dynamic_cast<const Name*>(length->name.get());
    if (!name) {
      return false;
    }
    outVar = name->name;
    outLine = length->lineNumber;
    return true;
  }

  return false;
}

void collect_names_from_ast(const ASTNode& node, std::unordered_set<std::string>& names) {
  if (const auto* name = dynamic_cast<const Name*>(&node)) {
    names.insert(name->name);
    return;
  }
  if (const auto* label = dynamic_cast<const Label*>(&node)) {
    names.insert(label->name);
    return;
  }
  if (const auto* op = dynamic_cast<const Operator*>(&node)) {
    collect_names_from_ast(*op->left, names);
    collect_names_from_ast(*op->right, names);
    return;
  }
  if (const auto* index = dynamic_cast<const Index*>(&node)) {
    collect_names_from_ast(*index->name, names);
    for (const auto& term : index->indices) {
      collect_names_from_ast(*term, names);
    }
    return;
  }
  if (const auto* length = dynamic_cast<const Length*>(&node)) {
    collect_names_from_ast(*length->name, names);
    if (length->hasIndex) {
      collect_names_from_ast(*length->index, names);
    }
    return;
  }
  if (const auto* call = dynamic_cast<const FunctionCall*>(&node)) {
    collect_names_from_ast(*call->name, names);
    for (const auto& arg : call->args) {
      collect_names_from_ast(*arg, names);
    }
    return;
  }
  if (const auto* newArray = dynamic_cast<const NewArray*>(&node)) {
    for (const auto& arg : newArray->args) {
      collect_names_from_ast(*arg, names);
    }
    return;
  }
  if (const auto* newTuple = dynamic_cast<const NewTuple*>(&node)) {
    collect_names_from_ast(*newTuple->length, names);
  }
}

void collect_names_from_instruction(const Instruction& instruction,
                                    std::unordered_set<std::string>& names) {
  if (const auto* def = dynamic_cast<const NameDefInstruction*>(&instruction)) {
    collect_names_from_ast(def->get_name(), names);
    return;
  }
  if (const auto* assign = dynamic_cast<const NameNumAssignInstruction*>(&instruction)) {
    collect_names_from_ast(assign->get_dst(), names);
    collect_names_from_ast(assign->get_src(), names);
    return;
  }
  if (const auto* opAssign = dynamic_cast<const OperatorAssignInstruction*>(&instruction)) {
    collect_names_from_ast(opAssign->get_dst(), names);
    collect_names_from_ast(opAssign->get_src(), names);
    return;
  }
  if (const auto* labelInst = dynamic_cast<const LabelInstruction*>(&instruction)) {
    collect_names_from_ast(labelInst->get_label(), names);
    return;
  }
  if (const auto* branch = dynamic_cast<const BranchInstruction*>(&instruction)) {
    collect_names_from_ast(branch->get_target(), names);
    return;
  }
  if (const auto* condBranch = dynamic_cast<const ConditionalBranchInstruction*>(&instruction)) {
    collect_names_from_ast(condBranch->get_condition(), names);
    collect_names_from_ast(condBranch->get_true_target(), names);
    collect_names_from_ast(condBranch->get_false_target(), names);
    return;
  }
  if (const auto* ret = dynamic_cast<const ReturnValueInstruction*>(&instruction)) {
    collect_names_from_ast(ret->get_return_value(), names);
    return;
  }
  if (const auto* idxAssign = dynamic_cast<const IndexAssignInstruction*>(&instruction)) {
    collect_names_from_ast(idxAssign->get_dst(), names);
    collect_names_from_ast(idxAssign->get_src(), names);
    return;
  }
  if (const auto* lenAssign = dynamic_cast<const LengthAssignInstruction*>(&instruction)) {
    collect_names_from_ast(lenAssign->get_dst(), names);
    collect_names_from_ast(lenAssign->get_src(), names);
    return;
  }
  if (const auto* call = dynamic_cast<const CallInstruction*>(&instruction)) {
    collect_names_from_ast(call->get_function_call(), names);
    return;
  }
  if (const auto* callAssign = dynamic_cast<const CallAssignInstruction*>(&instruction)) {
    collect_names_from_ast(callAssign->get_dst(), names);
    collect_names_from_ast(callAssign->get_function_call(), names);
    return;
  }
  if (const auto* newArrayAssign = dynamic_cast<const NewArrayAssignInstruction*>(&instruction)) {
    collect_names_from_ast(newArrayAssign->get_dst(), names);
    collect_names_from_ast(newArrayAssign->get_new_array(), names);
    return;
  }
  if (const auto* newTupleAssign = dynamic_cast<const NewTupleAssignInstruction*>(&instruction)) {
    collect_names_from_ast(newTupleAssign->get_dst(), names);
    collect_names_from_ast(newTupleAssign->get_new_tuple(), names);
  }
}

void initialize_codegen_name_state(Program& program) {
  tempCounter = 0;
  reservedCodegenNames.clear();

  for (const auto& function : program.getFunctions()) {
    collect_names_from_ast(function->getFunctionName(), reservedCodegenNames);
    for (const auto& parameter : function->getParameters()) {
      const auto* typedParam = dynamic_cast<const Parameter*>(parameter.get());
      if (!typedParam) {
        continue;
      }
      collect_names_from_ast(*typedParam->name, reservedCodegenNames);
    }
    for (const auto& instruction : function->getInstructions()) {
      collect_names_from_instruction(*instruction, reservedCodegenNames);
    }
  }
}

std::string array_full_check_key(const std::string& arrayName, const Index& index) {
  std::string key = arrayName + "|";
  for (size_t i = 0; i < index.indices.size(); ++i) {
    if (i) {
      key += "|";
    }
    key += index.indices[i]->to_string();
  }
  return key;
}

std::string tuple_check_key(const std::string& tupleName, const ASTNode& decodedIndex) {
  return tupleName + "|" + decodedIndex.to_string();
}

void invalidate_cached_checks_for_write(
    const std::string& writtenName,
    std::unordered_set<std::string>& knownNonNullArrays,
    std::unordered_set<std::string>& cachedArrayChecks,
    std::unordered_set<std::string>& cachedTupleChecks,
    std::unordered_map<std::string, std::unordered_set<std::string>>& cacheDeps) {
  if (writtenName.empty()) {
    return;
  }

  knownNonNullArrays.erase(writtenName);

  auto invalidate_if_depends = [&](std::unordered_set<std::string>& cache) {
    std::vector<std::string> toErase;
    for (const auto& key : cache) {
      auto depIt = cacheDeps.find(key);
      if (depIt == cacheDeps.end()) {
        continue;
      }
      if (depIt->second.find(writtenName) != depIt->second.end()) {
        toErase.push_back(key);
      }
    }
    for (const auto& key : toErase) {
      cache.erase(key);
      cacheDeps.erase(key);
    }
  };

  invalidate_if_depends(cachedArrayChecks);
  invalidate_if_depends(cachedTupleChecks);

  // If array variable itself is reassigned, invalidate all checks keyed by it.
  auto invalidate_by_prefix = [&](std::unordered_set<std::string>& cache) {
    std::vector<std::string> toErase;
    const std::string prefix = writtenName + "|";
    for (const auto& key : cache) {
      if (key.rfind(prefix, 0) == 0) {
        toErase.push_back(key);
      }
    }
    for (const auto& key : toErase) {
      cache.erase(key);
      cacheDeps.erase(key);
    }
  };
  invalidate_by_prefix(cachedArrayChecks);
  invalidate_by_prefix(cachedTupleChecks);
}

std::string written_name_of_instruction(const Instruction& instruction) {
  if (const auto* def = dynamic_cast<const NameDefInstruction*>(&instruction)) {
    return node_name(def->get_name());
  }
  if (const auto* assign = dynamic_cast<const NameNumAssignInstruction*>(&instruction)) {
    return node_name(assign->get_dst());
  }
  if (const auto* opAssign = dynamic_cast<const OperatorAssignInstruction*>(&instruction)) {
    return node_name(opAssign->get_dst());
  }
  if (const auto* lenAssign = dynamic_cast<const LengthAssignInstruction*>(&instruction)) {
    return node_name(lenAssign->get_dst());
  }
  if (const auto* callAssign = dynamic_cast<const CallAssignInstruction*>(&instruction)) {
    return node_name(callAssign->get_dst());
  }
  if (const auto* newArrayAssign = dynamic_cast<const NewArrayAssignInstruction*>(&instruction)) {
    return node_name(newArrayAssign->get_dst());
  }
  if (const auto* newTupleAssign = dynamic_cast<const NewTupleAssignInstruction*>(&instruction)) {
    return node_name(newTupleAssign->get_dst());
  }
  if (const auto* idxAssign = dynamic_cast<const IndexAssignInstruction*>(&instruction)) {
    const auto* dstName = dynamic_cast<const Name*>(&idxAssign->get_dst());
    if (dstName) {
      return dstName->name;
    }
  }
  return "";
}

struct SharedFailureBlocks {
  std::string tensorErr1LineVar = fresh_codegen_var();
  std::string tensorErr1Label = ":" + fresh_codegen_var();
  bool useTensorErr1 = false;

  void append_declarations(std::vector<std::unique_ptr<Instruction>>& out) const {
    if (useTensorErr1) {
      out.push_back(std::make_unique<NameDefInstruction>(
          std::make_unique<Type>("int64"),
          std::make_unique<Name>(tensorErr1LineVar)));
    }
  }

  void append_blocks(std::vector<std::unique_ptr<Instruction>>& out) const {
    if (useTensorErr1) {
      out.push_back(
          std::make_unique<LabelInstruction>(std::make_unique<Label>(tensorErr1Label)));
      std::vector<std::unique_ptr<ASTNode>> args;
      args.push_back(std::make_unique<Name>(tensorErr1LineVar));
      out.push_back(std::make_unique<CallInstruction>(std::make_unique<FunctionCall>(
          std::make_unique<Name>("tensor-error"), std::move(args))));
      out.push_back(
          std::make_unique<BranchInstruction>(std::make_unique<Label>(tensorErr1Label)));
    }
  }
};

void append_allocation_check(std::vector<std::unique_ptr<Instruction>>& out,
                             const std::string& varName,
                             int64_t lineNumber,
                             SharedFailureBlocks& sharedFailBlocks) {
  const std::string condVar = fresh_codegen_var();
  const std::string okLabel = ":" + fresh_codegen_var();
  sharedFailBlocks.useTensorErr1 = true;
  out.push_back(std::make_unique<NameDefInstruction>(
      std::make_unique<Type>("int64"), std::make_unique<Name>(condVar)));

  out.push_back(std::make_unique<NameNumAssignInstruction>(
      std::make_unique<Name>(sharedFailBlocks.tensorErr1LineVar),
      std::make_unique<Number>(encode_number(lineNumber))));

  out.push_back(std::make_unique<OperatorAssignInstruction>(
      std::make_unique<Name>(condVar),
      std::make_unique<Operator>(std::make_unique<Name>(varName),
                                 std::make_unique<Number>(0), OP::EQ)));

  out.push_back(std::make_unique<ConditionalBranchInstruction>(
      std::make_unique<Name>(condVar),
      std::make_unique<Label>(sharedFailBlocks.tensorErr1Label),
      std::make_unique<Label>(okLabel)));
  out.push_back(std::make_unique<LabelInstruction>(std::make_unique<Label>(okLabel)));
}

int64_t array_rank_from_type(const std::string& typeName) {
  int64_t rank = 0;
  for (size_t i = 0; i + 1 < typeName.size(); ++i) {
    if (typeName[i] == '[' && typeName[i + 1] == ']') {
      ++rank;
    }
  }
  return rank;
}

std::unordered_map<std::string, std::string> collect_var_types(const Function& function) {
  std::unordered_map<std::string, std::string> varTypes;
  for (const auto& parameterNode : function.getParameters()) {
    const auto* parameter = dynamic_cast<const Parameter*>(parameterNode.get());
    if (!parameter) {
      continue;
    }
    const auto* type = dynamic_cast<const Type*>(parameter->type.get());
    const auto* name = dynamic_cast<const Name*>(parameter->name.get());
    if (type && name) {
      varTypes[name->name] = type->name;
    }
  }
  for (const auto& instruction : function.getInstructions()) {
    const auto* def = dynamic_cast<const NameDefInstruction*>(instruction.get());
    if (!def) {
      continue;
    }
    const auto* type = dynamic_cast<const Type*>(&def->get_type());
    const auto* name = dynamic_cast<const Name*>(&def->get_name());
    if (type && name) {
      varTypes[name->name] = type->name;
    }
  }
  return varTypes;
}

std::unique_ptr<ASTNode> encode_term_from_decoded(
    std::vector<std::unique_ptr<Instruction>>& out, const ASTNode& decodedTerm) {
  if (const auto* number = dynamic_cast<const Number*>(&decodedTerm)) {
    return std::make_unique<Number>(encode_number(number->value));
  }
  const auto* name = dynamic_cast<const Name*>(&decodedTerm);
  if (!name) {
    return clone_ast(decodedTerm);
  }

  const std::string encVar = fresh_codegen_var();
  out.push_back(std::make_unique<NameDefInstruction>(
      std::make_unique<Type>("int64"), std::make_unique<Name>(encVar)));
  out.push_back(std::make_unique<OperatorAssignInstruction>(
      std::make_unique<Name>(encVar),
      std::make_unique<Operator>(std::make_unique<Name>(name->name), std::make_unique<Number>(1), OP::LSHIFT)));
  out.push_back(std::make_unique<OperatorAssignInstruction>(
      std::make_unique<Name>(encVar),
      std::make_unique<Operator>(std::make_unique<Name>(encVar), std::make_unique<Number>(1), OP::PLUS)));
  return std::make_unique<Name>(encVar);
}

void append_tuple_bounds_check(std::vector<std::unique_ptr<Instruction>>& out,
                               const std::string& tupleName,
                               const ASTNode& decodedIndex,
                               int64_t lineNumber) {
  const std::string lineVar = fresh_codegen_var();
  const std::string lenEnc = fresh_codegen_var();
  const std::string lenDec = fresh_codegen_var();
  const std::string nonNeg = fresh_codegen_var();
  const std::string lessThan = fresh_codegen_var();
  const std::string checkLtLabel = ":" + fresh_codegen_var();
  const std::string errLabel = ":" + fresh_codegen_var();
  const std::string okLabel = ":" + fresh_codegen_var();

  out.push_back(std::make_unique<NameDefInstruction>(
      std::make_unique<Type>("int64"), std::make_unique<Name>(lineVar)));
  out.push_back(std::make_unique<NameNumAssignInstruction>(
      std::make_unique<Name>(lineVar), std::make_unique<Number>(encode_number(lineNumber))));

  out.push_back(std::make_unique<NameDefInstruction>(
      std::make_unique<Type>("int64"), std::make_unique<Name>(lenEnc)));
  out.push_back(std::make_unique<NameDefInstruction>(
      std::make_unique<Type>("int64"), std::make_unique<Name>(lenDec)));
  out.push_back(std::make_unique<NameDefInstruction>(
      std::make_unique<Type>("int64"), std::make_unique<Name>(nonNeg)));
  out.push_back(std::make_unique<NameDefInstruction>(
      std::make_unique<Type>("int64"), std::make_unique<Name>(lessThan)));

  out.push_back(std::make_unique<LengthAssignInstruction>(
      std::make_unique<Name>(lenEnc),
      std::make_unique<Length>(lineNumber, std::make_unique<Name>(tupleName))));
  out.push_back(std::make_unique<OperatorAssignInstruction>(
      std::make_unique<Name>(lenDec),
      std::make_unique<Operator>(std::make_unique<Name>(lenEnc), std::make_unique<Number>(1), OP::RSHIFT)));

  out.push_back(std::make_unique<OperatorAssignInstruction>(
      std::make_unique<Name>(nonNeg),
      std::make_unique<Operator>(clone_ast(decodedIndex), std::make_unique<Number>(0), OP::GE)));
  out.push_back(std::make_unique<ConditionalBranchInstruction>(
      std::make_unique<Name>(nonNeg), std::make_unique<Label>(checkLtLabel),
      std::make_unique<Label>(errLabel)));

  out.push_back(std::make_unique<LabelInstruction>(std::make_unique<Label>(checkLtLabel)));
  out.push_back(std::make_unique<OperatorAssignInstruction>(
      std::make_unique<Name>(lessThan),
      std::make_unique<Operator>(clone_ast(decodedIndex), std::make_unique<Name>(lenDec), OP::LT)));
  out.push_back(std::make_unique<ConditionalBranchInstruction>(
      std::make_unique<Name>(lessThan), std::make_unique<Label>(okLabel),
      std::make_unique<Label>(errLabel)));

  out.push_back(std::make_unique<LabelInstruction>(std::make_unique<Label>(errLabel)));
  std::vector<std::unique_ptr<ASTNode>> args;
  args.push_back(std::make_unique<Name>(lineVar));
  args.push_back(std::make_unique<Name>(lenEnc));
  args.push_back(encode_term_from_decoded(out, decodedIndex));
  out.push_back(std::make_unique<CallInstruction>(std::make_unique<FunctionCall>(
      std::make_unique<Name>("tuple-error"), std::move(args))));

  out.push_back(std::make_unique<LabelInstruction>(std::make_unique<Label>(okLabel)));
}

void append_array_tensor_bounds_check(std::vector<std::unique_ptr<Instruction>>& out,
                                      const std::string& arrayName,
                                      const Index& index,
                                      int64_t rank) {
  const std::string lineVar = fresh_codegen_var();
  out.push_back(std::make_unique<NameDefInstruction>(
      std::make_unique<Type>("int64"), std::make_unique<Name>(lineVar)));
  out.push_back(std::make_unique<NameNumAssignInstruction>(
      std::make_unique<Name>(lineVar), std::make_unique<Number>(encode_number(index.lineNumber))));

  const int64_t dimsToCheck = static_cast<int64_t>(index.indices.size());
  for (int64_t d = 0; d < dimsToCheck; ++d) {
    const std::string lenEnc = fresh_codegen_var();
    const std::string lenDec = fresh_codegen_var();
    const std::string nonNeg = fresh_codegen_var();
    const std::string lessThan = fresh_codegen_var();
    const std::string checkLtLabel = ":" + fresh_codegen_var();
    const std::string errLabel = ":" + fresh_codegen_var();
    const std::string okLabel = ":" + fresh_codegen_var();

    out.push_back(std::make_unique<NameDefInstruction>(
        std::make_unique<Type>("int64"), std::make_unique<Name>(lenEnc)));
    out.push_back(std::make_unique<NameDefInstruction>(
        std::make_unique<Type>("int64"), std::make_unique<Name>(lenDec)));
    out.push_back(std::make_unique<NameDefInstruction>(
        std::make_unique<Type>("int64"), std::make_unique<Name>(nonNeg)));
    out.push_back(std::make_unique<NameDefInstruction>(
        std::make_unique<Type>("int64"), std::make_unique<Name>(lessThan)));

    out.push_back(std::make_unique<LengthAssignInstruction>(
        std::make_unique<Name>(lenEnc),
        std::make_unique<Length>(index.lineNumber, std::make_unique<Name>(arrayName),
                                 std::make_unique<Number>(d))));
    out.push_back(std::make_unique<OperatorAssignInstruction>(
        std::make_unique<Name>(lenDec),
        std::make_unique<Operator>(std::make_unique<Name>(lenEnc), std::make_unique<Number>(1), OP::RSHIFT)));

    out.push_back(std::make_unique<OperatorAssignInstruction>(
        std::make_unique<Name>(nonNeg),
        std::make_unique<Operator>(clone_ast(*index.indices[d]), std::make_unique<Number>(0), OP::GE)));
    out.push_back(std::make_unique<ConditionalBranchInstruction>(
        std::make_unique<Name>(nonNeg), std::make_unique<Label>(checkLtLabel),
        std::make_unique<Label>(errLabel)));

    out.push_back(std::make_unique<LabelInstruction>(std::make_unique<Label>(checkLtLabel)));
    out.push_back(std::make_unique<OperatorAssignInstruction>(
        std::make_unique<Name>(lessThan),
        std::make_unique<Operator>(clone_ast(*index.indices[d]), std::make_unique<Name>(lenDec), OP::LT)));
    out.push_back(std::make_unique<ConditionalBranchInstruction>(
        std::make_unique<Name>(lessThan), std::make_unique<Label>(okLabel),
        std::make_unique<Label>(errLabel)));

    out.push_back(std::make_unique<LabelInstruction>(std::make_unique<Label>(errLabel)));
    std::vector<std::unique_ptr<ASTNode>> args;
    args.push_back(std::make_unique<Name>(lineVar));
    if (rank <= 1) {
      args.push_back(std::make_unique<Name>(lenEnc));
      args.push_back(encode_term_from_decoded(out, *index.indices[d]));
    } else {
      args.push_back(std::make_unique<Number>(encode_number(d)));
      args.push_back(std::make_unique<Name>(lenEnc));
      args.push_back(encode_term_from_decoded(out, *index.indices[d]));
    }
    out.push_back(std::make_unique<CallInstruction>(std::make_unique<FunctionCall>(
        std::make_unique<Name>("tensor-error"), std::move(args))));

    out.push_back(std::make_unique<LabelInstruction>(std::make_unique<Label>(okLabel)));
  }
}

}  // namespace

void encode_decode_values(Program& program) {
  initialize_codegen_name_state(program);
  std::unordered_map<std::string, FunctionSignature> signatures;
  for (const auto& function : program.getFunctions()) {
    signatures[node_name(function->getFunctionName())] = collect_signature(*function);
  }

  for (auto& function : program.getFunctions()) {
    std::unordered_map<std::string, std::string> varTypes;
    for (const auto& parameterNode : function->getParameters()) {
      const auto* parameter = dynamic_cast<const Parameter*>(parameterNode.get());
      if (!parameter) {
        continue;
      }
      const auto* type = dynamic_cast<const Type*>(parameter->type.get());
      const auto* name = dynamic_cast<const Name*>(parameter->name.get());
      if (type && name) {
        varTypes[name->name] = type->name;
      }
    }

    std::vector<std::unique_ptr<Instruction>> rewritten;
    std::unordered_map<std::string, std::string> decodedCache;
    auto invalidate_written_name = [&](const Instruction& inst) {
      const std::string writtenName = written_name_of_instruction(inst);
      if (!writtenName.empty()) {
        decodedCache.erase(writtenName);
      }
    };

    for (auto& instruction : function->getInstructions()) {
      if (dynamic_cast<const LabelInstruction*>(instruction.get())) {
        decodedCache.clear();
      }

      if (const auto* def = dynamic_cast<const NameDefInstruction*>(instruction.get())) {
        const auto* type = dynamic_cast<const Type*>(&def->get_type());
        const auto* name = dynamic_cast<const Name*>(&def->get_name());
        if (type && name) {
          varTypes[name->name] = type->name;
        }
        rewritten.push_back(std::make_unique<NameDefInstruction>(
            clone_ast(def->get_type()), clone_ast(def->get_name())));
        if (type && name) {
          // LA locals are implicitly initialized:
          // int64 -> 0 (encoded as 1), others -> 0/null.
          const int64_t initValue = is_int64_type(type->name) ? 1 : 0;
          rewritten.push_back(std::make_unique<NameNumAssignInstruction>(
              std::make_unique<Name>(name->name), std::make_unique<Number>(initValue)));
        }
        invalidate_written_name(*instruction);
        continue;
      }

      if (const auto* assign = dynamic_cast<const NameNumAssignInstruction*>(instruction.get())) {
        const auto* dstName = dynamic_cast<const Name*>(&assign->get_dst());
        bool dstIsInt = dstName && is_int64_type(lookup_type(varTypes, dstName->name));
        std::string srcDecodedAlias;
        const auto* srcName = dynamic_cast<const Name*>(&assign->get_src());
        if (dstIsInt && dstName && srcName &&
            is_int64_type(lookup_type(varTypes, srcName->name))) {
          auto srcIt = decodedCache.find(srcName->name);
          if (srcIt != decodedCache.end()) {
            srcDecodedAlias = srcIt->second;
          }
        }

        rewritten.push_back(std::make_unique<NameNumAssignInstruction>(
            clone_ast(assign->get_dst()), encode_term_if_needed(assign->get_src(), dstIsInt)));

        invalidate_written_name(*instruction);
        if (!srcDecodedAlias.empty()) {
          decodedCache[dstName->name] = srcDecodedAlias;
        }
        continue;
      }

      if (const auto* opAssign = dynamic_cast<const OperatorAssignInstruction*>(instruction.get())) {
        const auto* dstName = dynamic_cast<const Name*>(&opAssign->get_dst());
        bool dstIsInt = dstName && is_int64_type(lookup_type(varTypes, dstName->name));
        const auto* opNode = dynamic_cast<const Operator*>(&opAssign->get_src());
        if (!opNode) {
          rewritten.push_back(std::make_unique<OperatorAssignInstruction>(
              clone_ast(opAssign->get_dst()), clone_ast(opAssign->get_src())));
          invalidate_written_name(*instruction);
          continue;
        }

        std::unique_ptr<ASTNode> leftDecoded =
            decode_term_to_decoded(rewritten, *opNode->left, varTypes, decodedCache);
        std::unique_ptr<ASTNode> rightDecoded =
            decode_term_to_decoded(rewritten, *opNode->right, varTypes, decodedCache);

        if (!dstIsInt) {
          rewritten.push_back(std::make_unique<OperatorAssignInstruction>(
              clone_ast(opAssign->get_dst()),
              std::make_unique<Operator>(std::move(leftDecoded), std::move(rightDecoded), opNode->op)));
          invalidate_written_name(*instruction);
          continue;
        }

        const std::string decodedTmp = fresh_codegen_var();
        rewritten.push_back(std::make_unique<NameDefInstruction>(
            std::make_unique<Type>("int64"), std::make_unique<Name>(decodedTmp)));
        rewritten.push_back(std::make_unique<OperatorAssignInstruction>(
            std::make_unique<Name>(decodedTmp),
            std::make_unique<Operator>(std::move(leftDecoded), std::move(rightDecoded), opNode->op)));

        const auto* dst = dynamic_cast<const Name*>(&opAssign->get_dst());
        rewritten.push_back(std::make_unique<OperatorAssignInstruction>(
            std::make_unique<Name>(dst->name),
            std::make_unique<Operator>(std::make_unique<Name>(decodedTmp), std::make_unique<Number>(1), OP::LSHIFT)));
        rewritten.push_back(std::make_unique<OperatorAssignInstruction>(
            std::make_unique<Name>(dst->name),
            std::make_unique<Operator>(std::make_unique<Name>(dst->name), std::make_unique<Number>(1), OP::PLUS)));
        if (dst) {
          decodedCache[dst->name] = decodedTmp;
        }
        continue;
      }

      if (const auto* branch = dynamic_cast<const ConditionalBranchInstruction*>(instruction.get())) {
        std::unique_ptr<ASTNode> decodedCond =
            decode_term_to_decoded(rewritten, branch->get_condition(), varTypes, decodedCache);
        rewritten.push_back(std::make_unique<ConditionalBranchInstruction>(
            std::move(decodedCond), clone_ast(branch->get_true_target()), clone_ast(branch->get_false_target())));
        decodedCache.clear();
        continue;
      }

      if (const auto* ret = dynamic_cast<const ReturnValueInstruction*>(instruction.get())) {
        bool returnsInt = false;
        if (const auto* retType = dynamic_cast<const Type*>(&function->getType())) {
          returnsInt = is_int64_type(retType->name);
        }
        rewritten.push_back(std::make_unique<ReturnValueInstruction>(
            encode_term_if_needed(ret->get_return_value(), returnsInt)));
        decodedCache.clear();
        continue;
      }

      if (const auto* idxAssign = dynamic_cast<const IndexAssignInstruction*>(instruction.get())) {
        const auto* dstIndex = dynamic_cast<const Index*>(&idxAssign->get_dst());
        const auto* srcIndex = dynamic_cast<const Index*>(&idxAssign->get_src());

        std::unique_ptr<ASTNode> dst = clone_ast(idxAssign->get_dst());
        std::unique_ptr<ASTNode> src = clone_ast(idxAssign->get_src());

        if (dstIndex) {
          dst = decode_index_for_access(rewritten, *dstIndex, varTypes, decodedCache);
          src = encode_term_if_needed(idxAssign->get_src(), dynamic_cast<const Number*>(&idxAssign->get_src()) != nullptr);
        }
        if (srcIndex) {
          src = decode_index_for_access(rewritten, *srcIndex, varTypes, decodedCache);
        }

        rewritten.push_back(std::make_unique<IndexAssignInstruction>(std::move(dst), std::move(src)));
        invalidate_written_name(*instruction);
        continue;
      }

      if (const auto* lenAssign = dynamic_cast<const LengthAssignInstruction*>(instruction.get())) {
        const auto* len = dynamic_cast<const Length*>(&lenAssign->get_src());
        if (len && len->hasIndex) {
          std::unique_ptr<ASTNode> decIndex =
              decode_term_to_decoded(rewritten, *len->index, varTypes, decodedCache);
          rewritten.push_back(std::make_unique<LengthAssignInstruction>(
              clone_ast(lenAssign->get_dst()),
              std::make_unique<Length>(len->lineNumber, clone_ast(*len->name), std::move(decIndex))));
        } else {
          rewritten.push_back(std::make_unique<LengthAssignInstruction>(
              clone_ast(lenAssign->get_dst()), clone_ast(lenAssign->get_src())));
        }
        invalidate_written_name(*instruction);
        continue;
      }

      if (const auto* callInst = dynamic_cast<const CallInstruction*>(instruction.get())) {
        const auto* call = dynamic_cast<const FunctionCall*>(&callInst->get_function_call());
        if (call) {
          rewritten.push_back(std::make_unique<CallInstruction>(
              encode_function_call_args(*call, signatures, varTypes)));
        } else {
          rewritten.push_back(std::make_unique<CallInstruction>(clone_ast(callInst->get_function_call())));
        }
        decodedCache.clear();
        continue;
      }

      if (const auto* callAssign = dynamic_cast<const CallAssignInstruction*>(instruction.get())) {
        const auto* call = dynamic_cast<const FunctionCall*>(&callAssign->get_function_call());
        if (call) {
          rewritten.push_back(std::make_unique<CallAssignInstruction>(
              clone_ast(callAssign->get_dst()),
              encode_function_call_args(*call, signatures, varTypes)));
        } else {
          rewritten.push_back(std::make_unique<CallAssignInstruction>(
              clone_ast(callAssign->get_dst()), clone_ast(callAssign->get_function_call())));
        }
        invalidate_written_name(*instruction);
        decodedCache.clear();
        continue;
      }

      if (const auto* newArrayAssign = dynamic_cast<const NewArrayAssignInstruction*>(instruction.get())) {
        const auto* newArray = dynamic_cast<const NewArray*>(&newArrayAssign->get_new_array());
        if (newArray) {
          std::vector<std::unique_ptr<ASTNode>> args;
          for (const auto& arg : newArray->args) {
            args.push_back(encode_term_if_needed(*arg, true));
          }
          rewritten.push_back(std::make_unique<NewArrayAssignInstruction>(
              clone_ast(newArrayAssign->get_dst()), std::make_unique<NewArray>(std::move(args))));
        } else {
          rewritten.push_back(std::make_unique<NewArrayAssignInstruction>(
              clone_ast(newArrayAssign->get_dst()), clone_ast(newArrayAssign->get_new_array())));
        }
        invalidate_written_name(*instruction);
        continue;
      }

      if (const auto* newTupleAssign = dynamic_cast<const NewTupleAssignInstruction*>(instruction.get())) {
        const auto* newTuple = dynamic_cast<const NewTuple*>(&newTupleAssign->get_new_tuple());
        if (newTuple) {
          rewritten.push_back(std::make_unique<NewTupleAssignInstruction>(
              clone_ast(newTupleAssign->get_dst()),
              std::make_unique<NewTuple>(encode_term_if_needed(*newTuple->length, true))));
        } else {
          rewritten.push_back(std::make_unique<NewTupleAssignInstruction>(
              clone_ast(newTupleAssign->get_dst()), clone_ast(newTupleAssign->get_new_tuple())));
        }
        invalidate_written_name(*instruction);
        continue;
      }

      if (dynamic_cast<const LabelInstruction*>(instruction.get())) {
        const auto* labelInstruction = dynamic_cast<const LabelInstruction*>(instruction.get());
        rewritten.push_back(std::make_unique<LabelInstruction>(clone_ast(labelInstruction->get_label())));
        continue;
      }
      if (dynamic_cast<const BranchInstruction*>(instruction.get())) {
        const auto* branch = dynamic_cast<const BranchInstruction*>(instruction.get());
        rewritten.push_back(std::make_unique<BranchInstruction>(clone_ast(branch->get_target())));
        decodedCache.clear();
        continue;
      }
      if (dynamic_cast<const ReturnInstruction*>(instruction.get())) {
        rewritten.push_back(std::make_unique<ReturnInstruction>());
        decodedCache.clear();
        continue;
      }

      invalidate_written_name(*instruction);
      rewritten.push_back(std::move(instruction));
    }

    function->setInstructions(std::move(rewritten));
  }
}

void check_allocation_access(Program& program) {
  initialize_codegen_name_state(program);
  std::vector<std::unique_ptr<Function>>& functions = program.getFunctions();
  for (auto& function : functions) {
    const std::unordered_map<std::string, std::string> varTypes = collect_var_types(*function);
    std::vector<std::unique_ptr<Instruction>> newInstructions;
    SharedFailureBlocks sharedFailBlocks;
    std::unordered_set<std::string> knownNonNullArrays;
    std::unordered_set<std::string> cachedArrayChecks;
    std::unordered_set<std::string> cachedTupleChecks;
    std::unordered_map<std::string, std::unordered_set<std::string>> cacheDeps;

    auto& instructions = function->getInstructions();
    for (std::unique_ptr<Instruction>& instruction : instructions) {
      // Keep cache local to straight-line regions.
      if (dynamic_cast<LabelInstruction*>(instruction.get()) ||
          dynamic_cast<BranchInstruction*>(instruction.get()) ||
          dynamic_cast<ConditionalBranchInstruction*>(instruction.get()) ||
          dynamic_cast<CallInstruction*>(instruction.get()) ||
          dynamic_cast<CallAssignInstruction*>(instruction.get()) ||
          dynamic_cast<ReturnInstruction*>(instruction.get()) ||
          dynamic_cast<ReturnValueInstruction*>(instruction.get())) {
        knownNonNullArrays.clear();
        cachedArrayChecks.clear();
        cachedTupleChecks.clear();
        cacheDeps.clear();
      }

      if (const auto* indexAssign = dynamic_cast<const IndexAssignInstruction*>(instruction.get())) {
        const Index* index = dynamic_cast<const Index*>(&indexAssign->get_dst());
        if (!index) {
          index = dynamic_cast<const Index*>(&indexAssign->get_src());
        }
        if (index) {
          const auto* name = dynamic_cast<const Name*>(index->name.get());
          if (name) {
            if (knownNonNullArrays.find(name->name) == knownNonNullArrays.end()) {
              append_allocation_check(newInstructions, name->name, index->lineNumber,
                                      sharedFailBlocks);
              knownNonNullArrays.insert(name->name);
            }
            const std::string typeName = lookup_type(varTypes, name->name);
            if (typeName == "tuple") {
              if (!index->indices.empty()) {
                const std::string key = tuple_check_key(name->name, *index->indices[0]);
                if (cachedTupleChecks.find(key) == cachedTupleChecks.end()) {
                  append_tuple_bounds_check(newInstructions, name->name, *index->indices[0],
                                           index->lineNumber);
                  cachedTupleChecks.insert(key);
                  auto& deps = cacheDeps[key];
                  deps.insert(name->name);
                  collect_names_from_ast(*index->indices[0], deps);
                }
              }
            } else if (typeName.rfind("int64[", 0) == 0) {
              const std::string key = array_full_check_key(name->name, *index);
              if (cachedArrayChecks.find(key) == cachedArrayChecks.end()) {
                append_array_tensor_bounds_check(newInstructions, name->name, *index,
                                                 array_rank_from_type(typeName));
                cachedArrayChecks.insert(key);
                auto& deps = cacheDeps[key];
                deps.insert(name->name);
                for (const auto& term : index->indices) {
                  collect_names_from_ast(*term, deps);
                }
              }
            }
          }
        }
      } else if (const auto* lengthAssign = dynamic_cast<const LengthAssignInstruction*>(instruction.get())) {
        const auto* length = dynamic_cast<const Length*>(&lengthAssign->get_src());
        if (length) {
          const auto* name = dynamic_cast<const Name*>(length->name.get());
          if (name) {
            if (knownNonNullArrays.find(name->name) == knownNonNullArrays.end()) {
              append_allocation_check(newInstructions, name->name, length->lineNumber,
                                      sharedFailBlocks);
              knownNonNullArrays.insert(name->name);
            }
          }
        }
      }

      const std::string writtenName = written_name_of_instruction(*instruction);
      invalidate_cached_checks_for_write(writtenName, knownNonNullArrays, cachedArrayChecks,
                                         cachedTupleChecks, cacheDeps);
      newInstructions.push_back(std::move(instruction));
    }

    std::vector<std::unique_ptr<Instruction>> finalInstructions;
    sharedFailBlocks.append_declarations(finalInstructions);
    for (auto& instruction : newInstructions) {
      finalInstructions.push_back(std::move(instruction));
    }
    sharedFailBlocks.append_blocks(finalInstructions);

    function->setInstructions(std::move(finalInstructions));
  }
}

void check_access(Program& program) {
  check_allocation_access(program);
}

void generate_basic_blocks(Program& program) {
  initialize_codegen_name_state(program);
  std::vector<std::unique_ptr<Function>>& functions = program.getFunctions();
  for (auto& function : functions) {
    auto& instructions = function->getInstructions();
    std::vector<std::unique_ptr<Instruction>> newInstructions;
    int startIndex = 0;

    // Ensure every function starts from :beginning_label.
    if (!instructions.empty()) {
      const auto* firstLabelInst = dynamic_cast<LabelInstruction*>(instructions.front().get());
      const auto* firstLabel = firstLabelInst ? dynamic_cast<const Label*>(&firstLabelInst->get_label()) : nullptr;
      if (firstLabel && firstLabel->name == ":beginning_label") {
        newInstructions.push_back(std::move(instructions.front()));
        startIndex = 1;
      } else {
        newInstructions.push_back(
            std::make_unique<LabelInstruction>(std::make_unique<Label>(":beginning_label")));
      }
    } else {
      newInstructions.push_back(
          std::make_unique<LabelInstruction>(std::make_unique<Label>(":beginning_label")));
    }

    bool lastWasTerminator = false;
    bool needBlockLabel = false;
    for (int i = startIndex; i < static_cast<int>(instructions.size()); ++i) {
      std::unique_ptr<Instruction>& instruction = instructions[i];

      if (needBlockLabel && !dynamic_cast<LabelInstruction*>(instruction.get())) {
        newInstructions.push_back(std::make_unique<LabelInstruction>(
            std::make_unique<Label>(":" + fresh_codegen_var())));
        lastWasTerminator = false;
      }
      needBlockLabel = false;

      if (LabelInstruction* labelInstruction = dynamic_cast<LabelInstruction*>(instruction.get())) {
        if (!lastWasTerminator) {
          const auto* label = dynamic_cast<const Label*>(&labelInstruction->get_label());
          newInstructions.push_back(
              std::make_unique<BranchInstruction>(std::make_unique<Label>(label->name)));
        }
      }

      lastWasTerminator =
          dynamic_cast<ReturnInstruction*>(instruction.get()) != nullptr ||
          dynamic_cast<ReturnValueInstruction*>(instruction.get()) != nullptr ||
          dynamic_cast<BranchInstruction*>(instruction.get()) != nullptr ||
          dynamic_cast<ConditionalBranchInstruction*>(instruction.get()) != nullptr;

      if (lastWasTerminator) {
        needBlockLabel = true;
      }

      newInstructions.push_back(std::move(instruction));
    }

    if (!lastWasTerminator) {
      std::unique_ptr<Instruction> returnInstruction;
      const auto* type = dynamic_cast<const Type*>(&function->getType());
      if (type->name == "void") {
        returnInstruction = std::make_unique<ReturnInstruction>();
      } else {
        returnInstruction = std::make_unique<ReturnValueInstruction>(std::make_unique<Number>(0));
      }
      newInstructions.push_back(std::move(returnInstruction));
    }
    function->setInstructions(std::move(newInstructions));
  }
}

void generate_code(const Program& p) {
  std::ofstream outputFile;
  outputFile.open("prog.IR");
  p.generate_code(outputFile);
  outputFile.close();
}

}  // namespace LA
