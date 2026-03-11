#pragma once

#include "program.h"

#include <fstream>
#include <iostream>

namespace LA {

void encode_decode_values(Program&);
void check_allocation_access(Program&);
void check_access(Program&);
void generate_basic_blocks(Program&);
void generate_code(const Program& p);
}  // namespace LA
