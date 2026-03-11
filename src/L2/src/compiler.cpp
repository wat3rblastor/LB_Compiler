#include <assert.h>
#include <code_generator.h>
#include <parser.h>
#include <stdint.h>
#include <unistd.h>

#include <memory>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <iterator>
#include <set>
#include <string>
#include <utility>
#include <vector>

void print_help(char* progName) {
  std::cerr << "Usage: " << progName << " [-v] [-g 0|1] [-l] [-i] [-O 0|1|2] SOURCE"
            << std::endl;
  return;
}

int main(int argc, char** argv) {

  auto enable_code_generator = false;
  int32_t optLevel = 0;
  bool verbose = false;
  bool livenessTest = false;
  bool interferenceTest = false;
  bool spillTest = false;

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
      case 'l':
        livenessTest = true;
        break;
      case 'i':
        interferenceTest = true;
        break;
      case 's':
        spillTest = true;
        break;
      default:
        print_help(argv[0]);
        return 1;
    }
  }

  /*
   * Liveness test
   */

  /*
   * Parse the input file.
   */

  auto p = L2::parse_file(argv[optind]);

  /*
   * Code optimizations (optional)
   */

  /*
   * Print the source program.
   */
  if (verbose) {
  std::cout << p.to_string();
  }

  if (livenessTest) {
    std::cout << p.functions[0]->test_liveness() << std::endl;
  }

  if (interferenceTest) {
    std::cout << p.functions[0]->test_interference() << std::endl;
  }

  if (spillTest) {
    std::cout << p.functions[0]->test_spill() << std::endl;
  }

  /*
   * Generate L1 code.
   */
  // if (enable_code_generator) {
  L2::generate_code(p);
  // }

  return 0;
}
