#pragma once

#include "program.h"

namespace L2 {

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
struct E;
struct F;
struct M;
struct N;

/*
 * Registers
 */
struct reg_rdi;
struct reg_rsi;
struct reg_rdx;
struct reg_rcx;
struct reg_r8;
struct reg_r9;
struct reg_rax;
struct reg_rsp;

/*
 * Operators
 */
struct op_add_equal;
struct op_sub_equal;
struct op_mul_equal;
struct op_and_equal;
struct op_arrow;
struct op_lshift_equal;
struct op_rshift_equal;
struct op_less;
struct op_less_equal;
struct op_equal;

/*
 * Operations
 */
struct aop;
struct sop;
struct cmp;

/*
 * Nonterminals
 */
struct w;
struct sx;
struct a;
struct sx;
struct s;
struct t;
struct u;
struct x;

/*
 * Terminals
 */
struct str_mem;
struct str_stack_arg;
struct str_cjump;
struct str_goto;
struct str_return;
struct str_call;
struct str_print;
struct str_input;
struct str_allocate;
struct str_tuple_error;
struct str_tensor_error;

/*
 * Instruction Dependencies
 */
struct mem_addr;
struct shift_amount;
struct at_label;

/*
 * Instructions
 */
struct inst_assign;

struct inst_mem_load;
struct inst_mem_store;

struct inst_stack_assign;

struct inst_arithmetic;
struct inst_shift;

struct inst_mem_add_equal;
struct inst_mem_sub_equal;
struct inst_add_equal_from_mem;
struct inst_sub_equal_from_mem;

struct inst_compare;
struct inst_label;
struct inst_cjump;
struct inst_goto;
struct inst_return;

struct inst_call;
struct inst_call_print;
struct inst_call_input;
struct inst_call_allocate;
struct inst_call_tuple_error;
struct inst_call_tensor_error;

struct inst_inc;
struct inst_dec;
struct inst_lea;

/*
 * Semantic terminals
 */
struct entry_function_name;
struct function_name;
struct num_function_args;
struct operand_literal; 
struct num_call_args;

/*
 * High-level grammar rules
 */
struct program;
struct function;
struct function_list;

struct instruction;
struct instruction_list;

struct spill_test;

struct grammar;

}  // namespace L2