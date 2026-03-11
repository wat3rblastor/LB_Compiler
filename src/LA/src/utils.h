#pragma once

#define TODO(s) std::cerr << "TODO: " << s; throw 1
#define UNREACHABLE(s) std::cerr << "UNREACHABLE: " << s; throw 2
#define UNIMPLEMENTED(s) std::cerr << "UNIMPLEMENTED: " << s; throw 3