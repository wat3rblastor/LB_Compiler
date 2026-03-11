CPP_FILES			:= $(wildcard src/*.cpp)
BUILD_DIR			= ../../build/$(PL_CLASS)
OBJ_DIR			= $(BUILD_DIR)/obj
BIN_DIR			= $(BUILD_DIR)/bin
OBJ_FILES			= $(addprefix $(OBJ_DIR)/,$(notdir $(CPP_FILES:.cpp=.o)))
CC_FLAGS			:= --std=c++17 -I./src -I../../lib/PEGTL/include -I../../lib -g3 -DDEBUG -pedantic -pedantic-errors -Werror=pedantic
LD_FLAGS			:= 
CC						:= g++
PL_CLASS  		:= LB
DST_PL_CLASS 	:= a
EXT_CLASS 		:= b
COMPILER			= $(BIN_DIR)/$(PL_CLASS)
OPT_LEVEL			:=
CC_CLASS			:= $(PL_CLASS)c

all: dirs $(COMPILER)

dirs:
	mkdir -p $(OBJ_DIR) $(BIN_DIR)

$(COMPILER): $(OBJ_FILES)
	$(CC) $(LD_FLAGS) -o $@ $^

$(OBJ_DIR)/%.o: src/%.cpp
	$(CC) $(CC_FLAGS) -c -o $@ $<

test: dirs $(COMPILER)
	$(MAKE) -C ../.. --no-print-directory L1_lang L2_lang L3_lang IR_lang LA_lang LB_lang
	../../scripts/test.sh $(EXT_CLASS) $(CC_CLASS) "../../tests"

clean:
	rm -fr $(BUILD_DIR) *.out *.o core.* `find ../../tests -iname *.tmp`
	rm -fr *.$(DST_PL_CLASS)

.PHONY: dirs $(COMPILER) test benchmark clean
# Runs competition correctness checks and reports per-test runtime.
benchmark: dirs $(COMPILER)
	$(MAKE) -C ../.. --no-print-directory L1_lang L2_lang L3_lang IR_lang LA_lang LB_lang
	../../scripts/benchmark.sh -c ./$(CC_CLASS) -r 7 -w 2 -o ../../benchmark_results.csv
