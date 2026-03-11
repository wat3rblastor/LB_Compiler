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

#include "instruction_selection.h"
#include "label_globalization.h"
#include "parser.h"
#include "program.h"

void print_help(char* progName) {
  std::cerr << "Usage: " << progName << " [-v] [-g 0|1] [-O 0|1|2] SOURCE"
            << std::endl;
  return;
}

int main(int argc, char** argv) {
  auto enable_code_generator = false;
  int32_t optLevel = 0;
  bool verbose = false;

  /*
   * Check the compiler arguments.
   */
  if (argc < 2) {
    print_help(argv[0]);
    return 1;
  }
  int32_t opt;
  while ((opt = getopt(argc, argv, "vg:lisO:")) != -1) {
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

  /*
   * Parse the input file.
   */
  auto p = L3::parse_file(argv[optind]);

  L3::globalize_labels(p);
  L3::select_instructions(p);

  /*
   * Print the source program.
   */
  if (verbose) {
    std::cout << p.to_string();
  }

  /*
   * Generate L3 code.
   */
  L3::generate_code(p);

  return 0;
}
