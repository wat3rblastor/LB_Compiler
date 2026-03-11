#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "cfg_graph.h"
#include "program.h"

namespace IR {

class ConstantPropagationAnalysis {
 public:
  enum class CPKind { UNDEF, CONST, NAC };

  struct CPValue {
    CPKind kind = CPKind::UNDEF;
    int64_t constant = 0;

    static CPValue undef();
    static CPValue constant_value(int64_t value);
    static CPValue nac();

    bool operator==(const CPValue& other) const;
    bool operator!=(const CPValue& other) const;
  };

  using FactMap = std::unordered_map<std::string, CPValue>;

  struct InstructionGenKill {
    std::optional<std::string> def;
    std::unordered_set<std::string> uses;
  };

  struct FunctionInOut {
    std::vector<FactMap> block_in;
    std::vector<FactMap> block_out;
    std::vector<std::vector<FactMap>> instruction_in;
    std::vector<std::vector<FactMap>> instruction_out;
    FactMap entry_seed;
  };

  void generate_pred_graph(const CFGGraphs&);
  void generate_GEN_KILL(const Program&);
  void generate_IN_OUT(const Program&);
  void data_flow_analysis(const Program&);
  bool apply_transformations(Program&);

 private:
  CPValue meet_value(const CPValue&, const CPValue&) const;
  FactMap merge_fact_maps(const FactMap&, const FactMap&) const;
  CPValue lookup_value(const FactMap&, const std::string&) const;

  CFGGraphs cfg_graphs_;
  std::unordered_map<const Function*, std::vector<std::vector<InstructionGenKill>>> gen_kill_;
  std::unordered_map<const Function*, FunctionInOut> in_out_;
};

}  // namespace IR
