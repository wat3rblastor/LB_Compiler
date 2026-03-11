#include "codegen_visitor.h"

#include <stdexcept>
#include <type_traits>

#include "codegen_context.h"
#include "node_utils.h"

namespace IR {
namespace {

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

void collect_var_dependencies_impl(const ASTNode& node,
                                   std::unordered_set<std::string>* deps) {
  class Collector : public VisitorBase {
   public:
    explicit Collector(std::unordered_set<std::string>* deps) : deps(deps) {}

    void collect(const ASTNode& n) { n.accept(*this); }

    void visit(const Var& node) override { deps->insert(node.name); }

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

   private:
    std::unordered_set<std::string>* deps;
  } collector(deps);

  collector.collect(node);
}

std::string build_expr_key_impl(const ASTNode& node) {
  class Builder : public VisitorBase {
   public:
    std::string build(const ASTNode& n) {
      n.accept(*this);
      return out;
    }

    void visit(const Var& node) override { out = "v:" + node.name; }
    void visit(const Label& node) override { out = "l:" + node.name; }
    void visit(const Type& node) override { out = "t:" + node.name; }
    void visit(const Number& node) override { out = "n:" + std::to_string(node.value); }

    void visit(const Callee& node) override {
      if (const auto* enum_value = std::get_if<EnumCallee>(&node.calleeValue)) {
        switch (*enum_value) {
          case EnumCallee::PRINT:
            out = "callee:print";
            break;
          case EnumCallee::INPUT:
            out = "callee:input";
            break;
          case EnumCallee::TUPLE_ERROR:
            out = "callee:tuple-error";
            break;
          case EnumCallee::TENSOR_ERROR:
            out = "callee:tensor-error";
            break;
        }
        return;
      }

      const auto* target = std::get_if<std::unique_ptr<ASTNode>>(&node.calleeValue);
      out = "callee(" + build(**target) + ")";
    }

    void visit(const Operator& node) override {
      out = "op(" + build(*node.left) + "," + op_to_string(node.op) + "," +
            build(*node.right) + ")";
    }

    void visit(const Index& node) override {
      out = "idx(" + build(*node.var);
      for (const auto& idx : node.indices) {
        out += "," + build(*idx);
      }
      out += ")";
    }

    void visit(const ArrayLength& node) override {
      out = "alen(" + build(*node.var) + "," + build(*node.index) + ")";
    }

    void visit(const TupleLength& node) override {
      out = "tlen(" + build(*node.var) + ")";
    }

    void visit(const FunctionCall& node) override {
      out = "call(" + build(*node.callee);
      for (const auto& arg : node.args) {
        out += "," + build(*arg);
      }
      out += ")";
    }

    void visit(const NewArray& node) override {
      out = "new_array(";
      for (size_t i = 0; i < node.args.size(); ++i) {
        out += build(*node.args[i]);
        if (i + 1 != node.args.size()) {
          out += ",";
        }
      }
      out += ")";
    }

    void visit(const NewTuple& node) override {
      out = "new_tuple(" + build(*node.length) + ")";
    }

    void visit(const Parameter& node) override {
      out = "param(" + build(*node.type) + "," + build(*node.var) + ")";
    }

   private:
    std::string out;
  } builder;

  return builder.build(node);
}

template <typename MapType>
void invalidate_cache_for_var(MapType* cache, const std::string& var_name) {
  for (auto it = cache->begin(); it != cache->end();) {
    if (it->second.dependencies.find(var_name) != it->second.dependencies.end()) {
      it = cache->erase(it);
    } else {
      ++it;
    }
  }
}

}  // namespace

CodegenVisitor::CodegenVisitor(std::ofstream& output_file)
    : output_file(output_file) {}

void CodegenVisitor::clear_codegen_caches() {
  decoded_dim_cache.clear();
  stride_cache.clear();
  index_address_cache.clear();
}

void CodegenVisitor::invalidate_codegen_caches_for_var(
    const std::string& var_name) {
  if (var_name.empty()) {
    return;
  }
  invalidate_cache_for_var(&decoded_dim_cache, var_name);
  invalidate_cache_for_var(&stride_cache, var_name);
  invalidate_cache_for_var(&index_address_cache, var_name);
}

std::unordered_set<std::string> CodegenVisitor::collect_var_dependencies(
    const ASTNode& node) const {
  std::unordered_set<std::string> deps;
  collect_var_dependencies_impl(node, &deps);
  return deps;
}

std::string CodegenVisitor::build_expr_key(const ASTNode& node) const {
  return build_expr_key_impl(node);
}

std::string CodegenVisitor::get_or_emit_decoded_dim(const std::string& base_var,
                                                    int64_t dim_index) {
  const std::string key = base_var + "#" + std::to_string(dim_index);
  auto it = decoded_dim_cache.find(key);
  if (it != decoded_dim_cache.end()) {
    return it->second.value;
  }

  const std::string dim_addr = fresh_codegen_var();
  output_file << " " << dim_addr << " <- " << base_var << " + "
              << (8 + dim_index * 8) << "\n";

  const std::string dim_encoded = fresh_codegen_var();
  output_file << " " << dim_encoded << " <- load " << dim_addr << "\n";

  const std::string dim_decoded = fresh_codegen_var();
  output_file << " " << dim_decoded << " <- " << dim_encoded << " >> 1\n";

  CacheEntry entry;
  entry.value = dim_decoded;
  entry.dependencies.insert(base_var);
  decoded_dim_cache.emplace(key, std::move(entry));
  return dim_decoded;
}

std::string CodegenVisitor::get_or_emit_stride(const std::string& base_var,
                                               int64_t start_dim,
                                               int64_t dim_count) {
  const std::string key = base_var + "#" + std::to_string(start_dim) + "#" +
                          std::to_string(dim_count);
  auto it = stride_cache.find(key);
  if (it != stride_cache.end()) {
    return it->second.value;
  }

  std::string stride;
  if (start_dim == dim_count - 1) {
    stride = get_or_emit_decoded_dim(base_var, start_dim);
  } else {
    const std::string current_dim = get_or_emit_decoded_dim(base_var, start_dim);
    const std::string tail_stride =
        get_or_emit_stride(base_var, start_dim + 1, dim_count);
    stride = fresh_codegen_var();
    output_file << " " << stride << " <- " << current_dim << " * " << tail_stride
                << "\n";
  }

  CacheEntry entry;
  entry.value = stride;
  entry.dependencies.insert(base_var);
  stride_cache.emplace(key, std::move(entry));
  return stride;
}

void CodegenVisitor::emit(const ASTNode& node) {
  node.accept(*this);
}

void CodegenVisitor::emit_basic_block(const BasicBlock& block,
                                      const Label* next_label) {
  const Label* saved = this->next_label;
  this->next_label = next_label;
  block.accept(*this);
  this->next_label = saved;
}

void CodegenVisitor::visit(const Var& node) { output_file << node.name; }

void CodegenVisitor::visit(const Label& node) { output_file << node.name; }

void CodegenVisitor::visit(const Type& node) { output_file << node.name; }

void CodegenVisitor::visit(const Number& node) { output_file << node.value; }

void CodegenVisitor::visit(const Callee& node) {
  std::visit(
      [this](auto&& calleeValue) {
        using T = std::decay_t<decltype(calleeValue)>;
        if constexpr (std::is_same_v<T, EnumCallee>) {
          switch (calleeValue) {
            case EnumCallee::PRINT:
              output_file << "print";
              break;
            case EnumCallee::INPUT:
              output_file << "input";
              break;
            case EnumCallee::TUPLE_ERROR:
              output_file << "tuple-error";
              break;
            case EnumCallee::TENSOR_ERROR:
              output_file << "tensor-error";
              break;
          }
        } else if constexpr (std::is_same_v<T, std::unique_ptr<ASTNode>>) {
          emit(*calleeValue);
        }
      },
      node.calleeValue);
}

void CodegenVisitor::visit(const Operator& node) {
  emit(*node.left);
  output_file << " " << op_to_string(node.op) << " ";
  emit(*node.right);
}

void CodegenVisitor::visit(const Index& node) {
  emit(*node.var);
  for (const auto& idx : node.indices) {
    output_file << "[";
    emit(*idx);
    output_file << "]";
  }
}

void CodegenVisitor::visit(const ArrayLength& node) {
  output_file << "length ";
  emit(*node.var);
  output_file << " ";
  emit(*node.index);
}

void CodegenVisitor::visit(const TupleLength& node) {
  output_file << "length ";
  emit(*node.var);
}

void CodegenVisitor::visit(const FunctionCall& node) {
  output_file << "call ";
  emit(*node.callee);
  output_file << " (";
  for (size_t i = 0; i < node.args.size(); ++i) {
    emit(*node.args[i]);
    if (i + 1 != node.args.size()) {
      output_file << ", ";
    }
  }
  output_file << ")";
}

void CodegenVisitor::visit(const NewArray& node) {
  for (size_t i = 0; i < node.args.size(); ++i) {
    output_file << "%p" << i << "D <- ";
    emit(*node.args[i]);
    output_file << " >> 1\n";
  }
  output_file << "%v0 <- 1\n";
  for (size_t i = 0; i < node.args.size(); ++i) {
    output_file << "v0 <- v0 * %p" << i << "\n";
  }
  output_file << "%v0 <- %v0 + " << node.args.size() << "\n";

  output_file << "%v0 <- %v0 << 1\n";
  output_file << "%v0 <- %v0 + 1\n";
  output_file << "%a <- call allocate(%v0, 1)\n";
}

void CodegenVisitor::visit(const NewTuple& node) {
  output_file << "new Tuple(";
  emit(*node.length);
  output_file << ")";
}

void CodegenVisitor::visit(const Parameter& node) { emit(*node.var); }

void CodegenVisitor::visit(const VarDefInstruction& node) {
  const Var& var_node = expect_var(node.get_var(), "Expected Var AST node");
  const Type& type_node = expect_type(node.get_type(), "Expected Type AST node");
  set_var_type(var_node.name, type_node.name);
  invalidate_codegen_caches_for_var(var_node.name);
}

void CodegenVisitor::visit(const VarNumLabelAssignInstruction& node) {
  output_file << " ";
  emit(node.get_dst());
  output_file << " <- ";
  emit(node.get_src());
  output_file << "\n";

  if (const Var* dst = as_var_or_null(node.get_dst()); dst != nullptr) {
    invalidate_codegen_caches_for_var(dst->name);
  }
}

void CodegenVisitor::visit(const OperatorAssignInstruction& node) {
  output_file << " ";
  emit(node.get_dst());
  output_file << " <- ";
  emit(node.get_src());
  output_file << "\n";

  if (const Var* dst = as_var_or_null(node.get_dst()); dst != nullptr) {
    invalidate_codegen_caches_for_var(dst->name);
  }
}

std::string CodegenVisitor::emit_index_address(const Index& index) {
  const Var& base_var = expect_var(*index.var, "Expected Var AST node");
  const std::string type_name = get_var_type(base_var.name);
  const bool is_tuple = is_tuple_type(type_name);

  std::unordered_set<std::string> deps;
  deps.insert(base_var.name);
  std::string address_key = is_tuple ? "tuple|" : "array|";
  address_key += base_var.name;
  for (const auto& idx : index.indices) {
    const std::unordered_set<std::string> idx_deps = collect_var_dependencies(*idx);
    deps.insert(idx_deps.begin(), idx_deps.end());
    address_key += "|" + build_expr_key(*idx);
  }

  auto addr_it = index_address_cache.find(address_key);
  if (addr_it != index_address_cache.end()) {
    return addr_it->second.value;
  }

  const std::string addr = fresh_codegen_var();
  if (is_tuple) {
    if (index.indices.size() != 1) {
      throw std::runtime_error("Tuple indexing expects exactly one index");
    }

    const std::string offset = fresh_codegen_var();
    output_file << " " << offset << " <- ";
    emit(*index.indices[0]);
    output_file << " * 8\n";
    output_file << " " << offset << " <- " << offset << " + 8\n";
    output_file << " " << addr << " <- " << base_var.name << " + " << offset
                << "\n";

    CacheEntry entry;
    entry.value = addr;
    entry.dependencies = std::move(deps);
    index_address_cache.emplace(address_key, std::move(entry));
    return addr;
  }

  const int64_t dim_count = static_cast<int64_t>(index.indices.size());
  if (dim_count <= 0) {
    throw std::runtime_error("Array indexing requires at least one index");
  }

  const std::string linear = fresh_codegen_var();
  output_file << " " << linear << " <- 0\n";

  for (int64_t i = 0; i + 1 < dim_count; ++i) {
    const std::string stride = get_or_emit_stride(base_var.name, i + 1, dim_count);
    const std::string term = fresh_codegen_var();
    output_file << " " << term << " <- ";
    emit(*index.indices[i]);
    output_file << " * " << stride << "\n";
    output_file << " " << linear << " <- " << term << " + " << linear << "\n";
  }

  output_file << " " << linear << " <- " << linear << " + ";
  emit(*index.indices[dim_count - 1]);
  output_file << "\n";

  const std::string body_offset = fresh_codegen_var();
  output_file << " " << body_offset << " <- " << linear << " * 8\n";
  output_file << " " << body_offset << " <- " << body_offset << " + "
              << (8 * (dim_count + 1)) << "\n";
  output_file << " " << addr << " <- " << base_var.name << " + " << body_offset
              << "\n";

  CacheEntry entry;
  entry.value = addr;
  entry.dependencies = std::move(deps);
  index_address_cache.emplace(address_key, std::move(entry));
  return addr;
}

void CodegenVisitor::visit(const IndexAssignInstruction& node) {
  const Index* dst_index = as_index_or_null(node.get_dst());
  const Index* src_index = as_index_or_null(node.get_src());

  if (dst_index != nullptr && src_index == nullptr) {
    const std::string addr = emit_index_address(*dst_index);
    output_file << " store " << addr << " <- ";
    emit(node.get_src());
    output_file << "\n";
    return;
  }

  if (src_index != nullptr && dst_index == nullptr) {
    const std::string addr = emit_index_address(*src_index);
    output_file << " ";
    emit(node.get_dst());
    output_file << " <- load " << addr << "\n";

    if (const Var* dst = as_var_or_null(node.get_dst()); dst != nullptr) {
      invalidate_codegen_caches_for_var(dst->name);
    }
    return;
  }

  throw std::runtime_error("Invalid IndexAssignInstruction shape");
}

void CodegenVisitor::visit(const LengthAssignInstruction& node) {
  if (const ArrayLength* array_length = as_array_length_or_null(node.get_src());
      array_length != nullptr) {
    const Var& var_node = expect_var(*array_length->var, "Expected Var AST node");
    const std::string offset = fresh_codegen_var();
    output_file << " " << offset << " <- ";
    emit(*array_length->index);
    output_file << " * 8\n";
    output_file << " " << offset << " <- " << offset << " + 8\n";
    const std::string addr = fresh_codegen_var();
    output_file << " " << addr << " <- " << var_node.name << " + " << offset
                << "\n";
    output_file << " ";
    emit(node.get_dst());
    output_file << " <- load " << addr << "\n";

    if (const Var* dst = as_var_or_null(node.get_dst()); dst != nullptr) {
      invalidate_codegen_caches_for_var(dst->name);
    }
    return;
  }

  if (const TupleLength* tuple_length = as_tuple_length_or_null(node.get_src());
      tuple_length != nullptr) {
    const Var& var_node = expect_var(*tuple_length->var, "Expected Var AST node");
    const std::string length_value = fresh_codegen_var();
    output_file << " " << length_value << " <- load " << var_node.name << "\n";
    output_file << " " << length_value << " <- " << length_value << " << 1\n";
    output_file << " " << length_value << " <- " << length_value << " + 1\n";
    output_file << " ";
    emit(node.get_dst());
    output_file << " <- " << length_value << "\n";

    if (const Var* dst = as_var_or_null(node.get_dst()); dst != nullptr) {
      invalidate_codegen_caches_for_var(dst->name);
    }
    return;
  }

  throw std::runtime_error("Invalid LengthAssignInstruction source");
}

void CodegenVisitor::visit(const CallInstruction& node) {
  output_file << " ";
  emit(node.get_call());
  output_file << "\n";
  clear_codegen_caches();
}

void CodegenVisitor::visit(const CallAssignInstruction& node) {
  output_file << " ";
  emit(node.get_dst());
  output_file << " <- ";
  emit(node.get_call());
  output_file << "\n";

  clear_codegen_caches();
  if (const Var* dst = as_var_or_null(node.get_dst()); dst != nullptr) {
    invalidate_codegen_caches_for_var(dst->name);
  }
}

void CodegenVisitor::visit(const NewArrayAssignInstruction& node) {
  const NewArray& new_array_node =
      expect_new_array(node.get_new_array(), "Expected NewArray AST node");
  if (new_array_node.args.empty()) {
    throw std::runtime_error("new Array() requires at least one dimension");
  }

  const std::string decoded_product = fresh_codegen_var();
  const std::string first_decoded = fresh_codegen_var();
  output_file << " " << first_decoded << " <- ";
  emit(*new_array_node.args[0]);
  output_file << " >> 1\n";
  output_file << " " << decoded_product << " <- " << first_decoded << "\n";

  for (size_t i = 1; i < new_array_node.args.size(); ++i) {
    const std::string decoded_dim = fresh_codegen_var();
    output_file << " " << decoded_dim << " <- ";
    emit(*new_array_node.args[i]);
    output_file << " >> 1\n";
    output_file << " " << decoded_product << " <- " << decoded_product << " * "
                << decoded_dim << "\n";
  }

  output_file << " " << decoded_product << " <- " << decoded_product << " << 1\n";
  output_file << " " << decoded_product << " <- " << decoded_product << " + 1\n";
  output_file << " " << decoded_product << " <- " << decoded_product << " + "
              << (2 * static_cast<int64_t>(new_array_node.args.size())) << "\n";

  output_file << " ";
  emit(node.get_dst());
  output_file << " <- call allocate (" << decoded_product << ", 1)\n";

  for (size_t i = 0; i < new_array_node.args.size(); ++i) {
    const std::string dim_addr = fresh_codegen_var();
    output_file << " " << dim_addr << " <- ";
    emit(node.get_dst());
    output_file << " + " << (8 + static_cast<int64_t>(i) * 8) << "\n";
    output_file << " store " << dim_addr << " <- ";
    emit(*new_array_node.args[i]);
    output_file << "\n";
  }

  clear_codegen_caches();
  if (const Var* dst = as_var_or_null(node.get_dst()); dst != nullptr) {
    invalidate_codegen_caches_for_var(dst->name);
  }
}

void CodegenVisitor::visit(const NewTupleAssignInstruction& node) {
  const NewTuple& new_tuple_node =
      expect_new_tuple(node.get_new_tuple(), "Expected NewTuple AST node");
  output_file << " ";
  emit(node.get_dst());
  output_file << " <- call allocate (";
  emit(*new_tuple_node.length);
  output_file << ", 1)\n";

  clear_codegen_caches();
  if (const Var* dst = as_var_or_null(node.get_dst()); dst != nullptr) {
    invalidate_codegen_caches_for_var(dst->name);
  }
}

void CodegenVisitor::visit(const BranchInstruction& node) {
  output_file << " br ";
  emit(node.get_target());
  output_file << "\n";
}

void CodegenVisitor::visit(const ConditionalBranchInstruction& node) {
  output_file << " br ";
  emit(node.get_condition());
  output_file << " ";
  emit(node.get_true_target());
  output_file << "\n";
  output_file << " br ";
  emit(node.get_false_target());
  output_file << "\n";
}

void CodegenVisitor::visit(const ReturnInstruction&) {
  output_file << " return\n";
}

void CodegenVisitor::visit(const ReturnValueInstruction& node) {
  output_file << " return ";
  emit(node.get_return_value());
  output_file << "\n";
}

void CodegenVisitor::visit(const BasicBlock& node) {
  clear_codegen_caches();

  output_file << " ";
  emit(node.get_label());
  output_file << "\n";

  for (const auto& instruction : node.get_instructions()) {
    emit(*instruction);
  }

  const Instruction& te = node.get_te();
  if (const BranchInstruction* branch = as_branch_or_null(te);
      branch != nullptr && next_label != nullptr) {
    const Label* target = as_label_or_null(branch->get_target());
    if (target != nullptr && target->name == next_label->name) {
      return;
    }
  }

  if (const ConditionalBranchInstruction* cond = as_conditional_branch_or_null(te);
      cond != nullptr) {
    output_file << " br ";
    emit(cond->get_condition());
    output_file << " ";
    emit(cond->get_true_target());
    output_file << "\n";

    if (next_label != nullptr) {
      const Label* false_target = as_label_or_null(cond->get_false_target());
      if (false_target != nullptr && false_target->name == next_label->name) {
        return;
      }
    }

    output_file << " br ";
    emit(cond->get_false_target());
    output_file << "\n";
    return;
  }

  emit(te);
}

void CodegenVisitor::visit(const Function& node) {
  reset_codegen_context();
  clear_codegen_caches();

  for (const auto& parameter : node.get_parameters()) {
    const Parameter& param =
        expect_parameter(*parameter, "Function parameter is not a Parameter node");
    const Type& type = expect_type(*param.type, "Malformed function parameter");
    const Var& var = expect_var(*param.var, "Malformed function parameter");
    set_var_type(var.name, type.name);
  }

  output_file << "define ";
  emit(node.get_name());
  output_file << " (";
  const auto& parameters = node.get_parameters();
  for (size_t i = 0; i < parameters.size(); ++i) {
    const Parameter& param =
        expect_parameter(*parameters[i], "Function parameter is not a Parameter node");
    emit(*param.var);
    if (i + 1 != parameters.size()) {
      output_file << ", ";
    }
  }
  output_file << ") {\n";

  const auto& blocks = node.get_basicBlocks();
  for (size_t i = 0; i < blocks.size(); ++i) {
    const Label* next = nullptr;
    if (i + 1 < blocks.size()) {
      next = &blocks[i + 1]->get_label();
    }
    emit_basic_block(*blocks[i], next);
    output_file << "\n";
  }

  output_file << "}\n";
}

void CodegenVisitor::visit(const Program& node) {
  const auto& functions = node.getFunctions();
  for (size_t i = 0; i < functions.size(); ++i) {
    emit(*functions[i]);
    if (i + 1 != functions.size()) {
      output_file << "\n";
    }
  }
}

}  // namespace IR
