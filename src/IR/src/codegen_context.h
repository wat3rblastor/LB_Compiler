#pragma once

#include <cstdint>
#include <string>

namespace IR {

void reset_codegen_context();
std::string fresh_codegen_var();

void set_var_type(const std::string& varName, const std::string& typeName);
std::string get_var_type(const std::string& varName);
bool is_tuple_type(const std::string& typeName);

}  // namespace IR
