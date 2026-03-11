#pragma once

#include <cstdint>
#include <fstream>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "program.h"
#include "visitor.h"

namespace IR {

class CodegenVisitor : public ConstVisitor {
 public:
  explicit CodegenVisitor(std::ofstream& output_file);

  void emit(const ASTNode& node);
  void emit_basic_block(const BasicBlock& block, const Label* next_label);

  void visit(const Var&) override;
  void visit(const Label&) override;
  void visit(const Type&) override;
  void visit(const Number&) override;
  void visit(const Callee&) override;
  void visit(const Operator&) override;
  void visit(const Index&) override;
  void visit(const ArrayLength&) override;
  void visit(const TupleLength&) override;
  void visit(const FunctionCall&) override;
  void visit(const NewArray&) override;
  void visit(const NewTuple&) override;
  void visit(const Parameter&) override;

  void visit(const VarDefInstruction&) override;
  void visit(const VarNumLabelAssignInstruction&) override;
  void visit(const OperatorAssignInstruction&) override;
  void visit(const IndexAssignInstruction&) override;
  void visit(const LengthAssignInstruction&) override;
  void visit(const CallInstruction&) override;
  void visit(const CallAssignInstruction&) override;
  void visit(const NewArrayAssignInstruction&) override;
  void visit(const NewTupleAssignInstruction&) override;
  void visit(const BranchInstruction&) override;
  void visit(const ConditionalBranchInstruction&) override;
  void visit(const ReturnInstruction&) override;
  void visit(const ReturnValueInstruction&) override;

  void visit(const BasicBlock&) override;
  void visit(const Function&) override;
  void visit(const Program&) override;

 private:
  struct CacheEntry {
    std::string value;
    std::unordered_set<std::string> dependencies;
  };

  void clear_codegen_caches();
  void invalidate_codegen_caches_for_var(const std::string& var_name);
  std::unordered_set<std::string> collect_var_dependencies(const ASTNode&) const;
  std::string build_expr_key(const ASTNode&) const;
  std::string get_or_emit_decoded_dim(const std::string& base_var, int64_t dim_index);
  std::string get_or_emit_stride(const std::string& base_var, int64_t start_dim,
                                 int64_t dim_count);
  std::string emit_index_address(const Index& index);

  std::ofstream& output_file;
  const Label* next_label = nullptr;
  std::unordered_map<std::string, CacheEntry> decoded_dim_cache;
  std::unordered_map<std::string, CacheEntry> stride_cache;
  std::unordered_map<std::string, CacheEntry> index_address_cache;
};

}  // namespace IR
