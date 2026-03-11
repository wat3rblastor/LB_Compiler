#pragma once

#include "program.h"

#include <fstream>
#include <iostream>

namespace LB {

void translate_variable_decl(Program&);
void flatten_nested_scopes(Program&);
void translate_if_while(Program&);
void generate_code(const Program&);

}  // namespace LB
