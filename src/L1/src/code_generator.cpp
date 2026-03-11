#include <code_generator.h>

#include <exception>
#include <fstream>
#include <iostream>
#include <string>

namespace L1 {

void generate_code(const Program& p) {
  std::ofstream outputFile;
  outputFile.open("prog.S");
  p.generate_code(outputFile);
  outputFile.close();
}
}  // namespace L1
