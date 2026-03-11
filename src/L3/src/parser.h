#pragma once

#include "L3.h"
#include "program.h"

namespace L3 {

Program parse_file(char* fileName);

/*
 * Helper grammar rules (lexing / whitespace / comments)
 */
struct spaces;
struct seps;
struct comment;
struct seps_with_comments;

/*
 * Identifiers / labels
 */
struct name_first;
struct name_rest;
struct name;
struct l;
struct label;
struct var;

/*
 * Immediate
 */
struct N;

/*
 * Operators
 */
struct op_add;
struct op_sub;
struct op_mul;
struct op_and;
struct op_lshift;
struct op_rshift;

struct op_less;
struct op_less_equal;
struct op_equal;
struct op_greater_equal;
struct op_greater;

struct op_arrow;

/*
 * Callee strings
 */
struct str_print;
struct str_allocate;
struct str_input;
struct str_tuple_error;
struct str_tensor_error;

/*
 * Nonterminals
 */
struct s;
struct t;
struct u;
struct op;
struct cmp;

/*
 * Terminals
 */
struct str_load;
struct str_store;
struct str_return;
struct str_br;
struct str_call;
struct str_define;

/*
 * Instruction Dependencies
 */
struct callee;
struct vars_list;
struct vars;
struct args_list;
struct args;

/*
 * Instructions
 */
struct inst_assign;
struct inst_assign_arithmetic;
struct inst_assign_comp;
struct inst_load;
struct inst_store;
struct inst_return;
struct inst_return_val;
struct inst_label;
struct inst_branch;
struct inst_conditional_branch;
struct inst_call;
struct inst_assign_call;

/*
 * Semantic terminals
 */
struct function_name;
struct function_vars;

/*
 * High-level grammar rules
 */
struct inst;
struct inst_list;
struct function;
struct function_list;
struct program;

}  // namespace L3