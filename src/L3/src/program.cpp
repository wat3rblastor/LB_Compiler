#include "program.h"

namespace L3 {

std::string Program::to_string() const {
  std::ostringstream out;
  for (const Function& function : functions) {
    out << function.to_string();
    out << "\n";
  }
  return out.str();
}

void Program::addFunction(Function&& function) {
  functions.push_back(std::move(function));
}

std::vector<Function>& Program::getFunctions() { return functions; }

void select_instructions(Program& program) {
  program.selector.load_tiles();
  std::vector<Function>& functions = program.getFunctions();
  for (Function& function : functions) {
    FunctionContext functionContext;
    Label label = function.getLabel();
    std::vector<Var> vars = function.getVars();
    functionContext.functionName = label.name;
    functionContext.numArgs = vars.size();
    functionContext.vars = vars;
    program.selector.functionContexts.push_back(std::move(functionContext));

    std::vector<Instruction>& instructions = function.getInstructions();
    program.selector.load_and_identify_contexts(instructions);
    program.selector.generate_trees();
    program.selector.merge_trees();
    program.selector.tile_trees();
  }
}

void generate_code(Program& program) {
  std::ofstream outputFile;
  outputFile.open("prog.L2");
  outputFile << program.selector.emit_L2_instructions();
  outputFile.close();
}

}  // namespace L3