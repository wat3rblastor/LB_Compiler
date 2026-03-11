#include "code_generator.h"

#include <fstream>

#include "codegen_visitor.h"

namespace IR {

void generate_code(const Program& p) {
  std::ofstream output_file("prog.L3");
  CodegenVisitor visitor(output_file);
  visitor.emit(p);
}

}  // namespace IR
