#pragma once

#include "instructions.h"

namespace L2 {

// This should take f as input and output f without variables and with registers
// This should exist on the program level
class RegisterAllocator {
 public:
  void generate_GEN_KILL(const std::vector<std::unique_ptr<Instruction>>&);
  void generate_succGraph(const std::vector<std::unique_ptr<Instruction>>&);
  void generate_IN_OUT();
  std::string IN_OUT_to_string() const;
  void generate_interference_graph(
      const std::vector<std::unique_ptr<Instruction>>&);
  std::string interference_graph_to_string() const;
  void color_graph();
  // The below two functions modify modifiedInstructions
  void spill(std::vector<std::unique_ptr<Instruction>>&, const std::string&, const std::string&);
  void implement_coloring(std::vector<std::unique_ptr<Instruction>>&);
  void replace_stack_args(std::vector<std::unique_ptr<Instruction>>&);

  static LocationSet set_union(const LocationSet& a, const LocationSet& b);
  static LocationSet set_difference(const LocationSet& a, const LocationSet& b);
  // Returns true if the set changed
  static bool set_assignment(LocationSet& dst, const LocationSet& src);
  static std::string display_location(const Location& l);
  static void instruction_replace_var(std::unique_ptr<Instruction>&, const std::string&, const Location&);
  

  int numLocals = 0;
  int spilledVarIdx = 0;
  std::vector<GenKill> genKill;
  std::vector<std::vector<size_t>> succGraph;
  std::vector<InOut> inOut;
  std::unordered_map<Location, std::unordered_set<Location, LocationHash>,
                     LocationHash>
      interfGraph;
  std::unordered_map<std::string, RegisterID> coloring;
  std::vector<std::string> toSpillVars;
};

// The copying of all the sets is really inefficient here
// If there is time, clean this up
class Function : public ASTNode {
 public:
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
  std::unique_ptr<ASTNode> clone() const override;

  const std::vector<std::unique_ptr<Instruction>>& get_instructions() const {
    return instructions;
  }

  void add_instruction(std::unique_ptr<Instruction> instruction) {
    instructions.push_back(std::move(instruction));
  }

  std::string test_liveness();
  std::string test_interference();
  std::string test_spill();
  void allocate();

  // Need to clean this up by adding setters
  Label name;
  int64_t numArgs;

  std::string testSpillVarName;
  std::string testSpillPrefixName;

 private:
  std::vector<std::unique_ptr<Instruction>> instructions;
  std::vector<std::unique_ptr<Instruction>> modifiedInstructions;
  std::vector<std::vector<size_t>> succGraph;
  RegisterAllocator regAlloc;
};

class Program : public ASTNode {
 public:
  std::string to_string() const override;
  void generate_code(std::ofstream&) const override;
  std::unique_ptr<ASTNode> clone() const override;

  Program() {
    entryPoint = Label("");
    functions = std::vector<std::unique_ptr<Function>>();
  };

  Label entryPoint;
  std::vector<std::unique_ptr<Function>> functions;
};

}  // namespace L2