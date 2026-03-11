#pragma once

#include "program.h"

namespace IR {

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
struct str_input;
struct str_tuple_error;
struct str_tensor_error;

/*
 * Type strings
 */
struct str_int64;
struct str_tuple;
struct str_code;

/*
 * Nonterminals
 */
struct s;
struct t;
struct u;
struct T;
struct op;

/*
 * Terminals
 */
struct str_brackets;
struct str_void;
struct str_length;
struct str_return;
struct str_br;
struct str_call;
struct str_define;
struct str_new;
struct str_Array;
struct str_Tuple;

/*
 * Instruction Dependencies
 */
struct callee;
struct oper;
struct var_index;
struct function_call;
struct new_array;
struct new_tuple;
struct type;
struct array_length;
struct tuple_length;
struct arg;
struct args_list;
struct args;
struct param;
struct param_list;
struct params;

/*
 * Instructions
 */
struct inst_var_def;
struct inst_var_num_label_assign;
struct inst_op_assign;
struct inst_index_load;
struct inst_index_store;
struct inst_array_length;
struct inst_tuple_length;
struct inst_call;
struct inst_call_assign;
struct inst_assign_new_array;
struct inst_assign_new_tuple;
struct inst_branch;
struct inst_cond_branch;
struct inst_return;
struct inst_return_val;

/*
 * Semantic terminals
 */
struct function_name;
struct function_params;
struct bb_name;
struct index;

/*
 * High-level grammar rules
 */

struct inst;
struct inst_list;
struct te;
struct bb;
struct bb_list;
struct function;
struct function_list;
struct program;


}  // namespace LR