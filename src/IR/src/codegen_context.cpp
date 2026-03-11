#include "codegen_context.h"

#include <unordered_map>

namespace IR {
namespace {

int64_t temp_counter = 0;
std::unordered_map<std::string, std::string> var_types;

}  // namespace

void reset_codegen_context() {
  temp_counter = 0;
  var_types.clear();
}

std::string fresh_codegen_var() {
  // This will be incorrect if any variable starts with "thisIsANewVar"
  // Need to fix this later
  return "%thisIsANewVar" + std::to_string(temp_counter++);
}

void set_var_type(const std::string& varName, const std::string& typeName) {
  var_types[varName] = typeName;
}

std::string get_var_type(const std::string& varName) {
  auto it = var_types.find(varName);
  if (it == var_types.end()) {
    return "";
  }
  return it->second;
}

bool is_tuple_type(const std::string& typeName) {
  return typeName == "tuple";
}

}  // namespace IR
