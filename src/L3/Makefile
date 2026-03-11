CPP_FILES			:= $(wildcard src/*.cpp)
BUILD_DIR			= ../../build/$(PL_CLASS)
OBJ_DIR			= $(BUILD_DIR)/obj
BIN_DIR			= $(BUILD_DIR)/bin
OBJ_FILES			= $(addprefix $(OBJ_DIR)/,$(notdir $(CPP_FILES:.cpp=.o)))
CC_FLAGS			:= --std=c++17 -I./src -I../../lib/PEGTL/include -I../../lib -g3 -DDEBUG -pedantic -pedantic-errors -Werror=pedantic
LD_FLAGS			:= 
CC						:= g++
PL_CLASS  		:= L3
DST_PL_CLASS 	:= L2
EXT_CLASS			:= $(PL_CLASS)
COMPILER			= $(BIN_DIR)/$(PL_CLASS)
OPT_LEVEL			:=
CC_CLASS			:= $(PL_CLASS)c

all: dirs $(COMPILER)

compiler: dirs $(COMPILER)

dirs:
	mkdir -p $(OBJ_DIR) $(BIN_DIR)

$(COMPILER): $(OBJ_FILES)
	$(CC) $(LD_FLAGS) -o $@ $^

$(OBJ_DIR)/%.o: src/%.cpp
	$(CC) $(CC_FLAGS) -c -o $@ $<

clean:
	rm -fr $(BUILD_DIR) *.out *.o core.*
	rm -fr *.$(DST_PL_CLASS)

.PHONY: dirs $(COMPILER) clean
