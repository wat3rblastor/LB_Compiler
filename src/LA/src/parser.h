#pragma once

#include "program.h"

namespace LA {

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
struct var_name;
struct label_name;
struct label;

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
 * Type strings
 */
struct str_int64;
struct str_tuple;
struct str_code;

/*
 * Nonterminals
 */
struct t;
struct T;
struct op;
struct index;
struct length_index;

/*
 * Terminals
 */
struct str_brackets;
struct str_void;
struct str_length;
struct str_return;
struct str_br;
struct str_new;
struct str_Array;
struct str_Tuple;

/*
 * Instruction Dependencies
 */
struct oper;
struct name_index;
struct function_call;
struct new_array;
struct new_tuple;
struct type;
struct length;
struct comma_sep;
struct arg;
struct args_list;
struct args;
struct param;
struct param_list;
struct params;

/*
 * Instructions
 */
struct inst_name_def;
struct inst_name_num_assign;
struct inst_op_assign;
struct inst_label;
struct inst_branch;
struct inst_cond_branch;
struct inst_return;
struct inst_return_val;
struct inst_index_load;
struct inst_index_store;
struct inst_length;
struct inst_call;
struct inst_call_assign;
struct inst_new_array_assign;
struct inst_new_tuple_assign;

/*
 * Semantic terminals
 */
struct function_name;
struct function_params;

/*
 * High-level grammar rules
 */
struct inst;
struct inst_list;
struct function;
struct function_list;
struct program;

}