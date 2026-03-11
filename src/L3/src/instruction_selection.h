#pragma once

#include <functional>

#include "L3.h"

namespace L3 {

enum class NodeType {
  VAR,
  NUMBER,
  LABEL,
  ADD,
  SUB,
  MUL,
  AND,
  LSHIFT,
  RSHIFT,
  LT,
  LE,
  EQ,
  GE,
  GT,
  ARROW,
  LOAD,
  STORE,
  RETURN,
  BR,
};

struct TreeNode {
  NodeType type;
  std::string name;
  long long value;
  std::vector<std::unique_ptr<TreeNode>> children;
};

class TreeNodeFactory {
 public:
  static std::unique_ptr<TreeNode> make_var(Var);
  static std::unique_ptr<TreeNode> make_number(Number);
  static std::unique_ptr<TreeNode> make_label(Label);
  static std::unique_ptr<TreeNode> make_arithmetic_op(ArithmeticOp);
  static std::unique_ptr<TreeNode> make_compare_op(CompareOp);
  static std::unique_ptr<TreeNode> make_arrow();
  static std::unique_ptr<TreeNode> make_load();
  static std::unique_ptr<TreeNode> make_store();
  static std::unique_ptr<TreeNode> make_return();
  static std::unique_ptr<TreeNode> make_break();
};

struct TreeTile {
  int numInstructions;
  int numNodes;
  std::function<bool(TreeNode*)> match;
  std::function<std::vector<std::string>(TreeNode*)> generate_L2_code;
  std::function<std::vector<TreeNode*>(TreeNode*)> find_children;
};

struct Context {
  std::vector<Instruction> instructions;
  std::vector<std::unique_ptr<TreeNode>> trees;
  std::vector<std::string> L2Code;
};

struct FunctionContext {
  std::string functionName;
  int numArgs;
  std::vector<Var> vars;
  std::vector<std::variant<Context, LabelInstruction, CallInstruction,
                           AssignCallInstruction>>
      partitions;
};

class InstructionSelector {
 public:
  void load_tiles();
  void load_and_identify_contexts(const std::vector<Instruction>& instructions);
  void generate_trees();
  void merge_trees();
  void tile_trees();
  std::string emit_L2_instructions();

  std::vector<FunctionContext> functionContexts;
 
 private:
  std::string get_defined_var(TreeNode*);
  std::vector<std::string> get_used_vars(TreeNode*);
  bool involves_memory(TreeNode*);

  std::unique_ptr<TreeNode> generate_tree(const Instruction&);
  bool can_merge_trees(std::vector<std::unique_ptr<TreeNode>>&, int, int, std::string);
  void merge_tree(std::unique_ptr<TreeNode>&, std::unique_ptr<TreeNode>&, std::string);

  // Assumed to be sorted by preference
  std::vector<TreeTile> treeTiles;
  TreeNodeFactory factory;
};

}  // namespace L3
