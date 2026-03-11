#include <assert.h>
#include <stdint.h>
#include <unistd.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <iterator>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "program.h"
#include "parser.h"
#include "algebraic_simplify.h"
#include "branch_simplify.h"
#include "cfg_graph.h"
#include "code_generator.h"
#include "const_prop.h"
#include "dce.h"
#include "gvn.h"
#include "linearize.h"
#include "loop_licm.h"
#include "to_string_visitor.h"

void print_help(char* progName) {
  std::cerr << "Usage: " << progName << " [-v] [-g 0|1] [-O 0|1|2] SOURCE"
            << std::endl;
  return;
}

int main(int argc, char** argv) {
  auto enable_code_generator = false;
  int32_t optLevel = 0;
  bool verbose = false;

  if (argc < 2) {
    print_help(argv[0]);
    return 1;
  }
  int32_t opt;
  while ((opt = getopt(argc, argv, "vg:O:")) != -1) {
    switch (opt) {
      case 'O':
        optLevel = strtoul(optarg, NULL, 0);
        break;
      case 'g':
        enable_code_generator = (strtoul(optarg, NULL, 0) == 0) ? false : true;
        break;
      case 'v':
        verbose = true;
        break;
      default:
        print_help(argv[0]);
        return 1;
    }
  }

  auto p = IR::parse_file(argv[optind]);

  IR::ConstantPropagationAnalysis cp_analysis;
  IR::AlgebraicSimplify algebraic_simplify;
  IR::BranchSimplify branch_simplify;
  IR::LoopInvariantCodeMotion loop_licm;
  IR::GlobalValueNumbering gvn;
  IR::DeadCodeElimination dce;
  bool changed = true;
  int iteration = 0;
  constexpr int kMaxIterations = 8;
  while (changed && iteration < kMaxIterations) {
    IR::CFGGraphs cfg_graphs = IR::build_cfg_graphs(p);
    cp_analysis.generate_pred_graph(cfg_graphs);
    cp_analysis.generate_GEN_KILL(p);
    cp_analysis.generate_IN_OUT(p);
    cp_analysis.data_flow_analysis(p);
    const bool cp_changed = cp_analysis.apply_transformations(p);
    const bool alg_changed = algebraic_simplify.run(p);
    const bool branch_changed = branch_simplify.run(p);
    const bool licm_changed = loop_licm.run(p);

    if (cp_changed || alg_changed || branch_changed) {
      cfg_graphs = IR::build_cfg_graphs(p);
    }
    const bool gvn_changed = false;
    const bool dce_changed = dce.run(p, cfg_graphs);
    changed = cp_changed || alg_changed || branch_changed || licm_changed ||
              gvn_changed || dce_changed;
    ++iteration;
  }

  IR::linearize_cfg(p);

  if (verbose) {
    std::cout << IR::to_string(p);
  }

  IR::generate_code(p);

  return 0;
}
