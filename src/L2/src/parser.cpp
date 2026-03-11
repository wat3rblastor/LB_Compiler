/*
 * SUGGESTION FROM THE CC TEAM:
 * double check the order of actions that are fired.
 * You can do this in (at least) two ways:
 * 1) by using gdb adding breakpoints to actions
 * 2) by adding printing statements in each action
 *
 * For 2), we suggest writing the code to make it straightforward to
 * enable/disable all of them (e.g., assuming shouldIPrint is a global variable
 *    if (shouldIPrint) std::cerr << "MY OUTPUT" << std::endl;
 * )
 */
#include <L2.h>
#include <assert.h>
#include <parser.h>
#include <sched.h>
#include <stdint.h>
#include <utils.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <iterator>
#include <memory>
#include <set>
#include <string>
#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/analyze.hpp>
#include <tao/pegtl/contrib/raw_string.hpp>
#include <utility>
#include <vector>

namespace pegtl = TAO_PEGTL_NAMESPACE;

using namespace pegtl;

namespace L2 {

/*
 * Helper grammar rules (lexing / whitespace / comments)
 */
struct spaces : pegtl::star<pegtl::sor<pegtl::one<' '>, pegtl::one<'\t'>>> {};
struct seps : pegtl::star<pegtl::seq<spaces, pegtl::eol>> {};
struct comment
    : pegtl::disable<TAO_PEGTL_STRING("//"), pegtl::until<pegtl::eolf>> {};
struct seps_with_comments
    : pegtl::star<pegtl::seq<spaces, pegtl::sor<pegtl::eol, comment>>> {};

/*
 * Identifiers / labels
 */
struct name_first : pegtl::sor<pegtl::alpha, pegtl::one<'_'>> {};
struct name_rest : pegtl::sor<pegtl::alpha, pegtl::one<'_'>, pegtl::digit> {};
struct name : pegtl::seq<name_first, pegtl::star<name_rest>> {};
struct l : pegtl::seq<pegtl::one<'@'>, name> {};
struct label : pegtl::seq<pegtl::one<':'>, name> {};
struct var : pegtl::seq<pegtl::one<'%'>, name> {};

/*
 * Immediate
 */
struct E : pegtl::one<'1', '2', '4', '8'> {};
struct F : pegtl::one<'1', '3', '4'> {};
struct M : pegtl::seq<pegtl::opt<pegtl::sor<pegtl::one<'-'>, pegtl::one<'+'>>>,
                      pegtl::plus<pegtl::digit>> {};
struct N : pegtl::seq<pegtl::opt<pegtl::sor<pegtl::one<'-'>, pegtl::one<'+'>>>,
                      pegtl::plus<pegtl::digit>> {};

/*
 * Registers
 */
struct reg_rdi : TAO_PEGTL_STRING("rdi") {};
struct reg_rsi : TAO_PEGTL_STRING("rsi") {};
struct reg_rdx : TAO_PEGTL_STRING("rdx") {};
struct reg_rcx : TAO_PEGTL_STRING("rcx") {};
struct reg_r8 : TAO_PEGTL_STRING("r8") {};
struct reg_r9 : TAO_PEGTL_STRING("r9") {};
struct reg_rax : TAO_PEGTL_STRING("rax") {};
struct reg_rsp : TAO_PEGTL_STRING("rsp") {};

/*
 * Operators
 */
struct op_add_equal : TAO_PEGTL_STRING("+=") {};
struct op_sub_equal : TAO_PEGTL_STRING("-=") {};
struct op_mul_equal : TAO_PEGTL_STRING("*=") {};
struct op_and_equal : TAO_PEGTL_STRING("&=") {};
struct op_arrow : TAO_PEGTL_STRING("\x3c\x2D") {};
struct op_lshift_equal : TAO_PEGTL_STRING("\x3c\x3c=") {};
struct op_rshift_equal : TAO_PEGTL_STRING(">>=") {};
struct op_less : TAO_PEGTL_STRING("\x3c") {};
struct op_less_equal : TAO_PEGTL_STRING("\x3c=") {};
struct op_equal : TAO_PEGTL_STRING("=") {};
struct op_inc : TAO_PEGTL_STRING("++") {};
struct op_dec : TAO_PEGTL_STRING("--") {};

/*
 * Operations
 */
struct aop
    : pegtl::sor<op_add_equal, op_sub_equal, op_mul_equal, op_and_equal> {};
struct sop : pegtl::sor<op_lshift_equal, op_rshift_equal> {};
struct cmp : pegtl::sor<op_less_equal, op_less, op_equal> {};

/*
 * Nonterminals
 */
struct w : pegtl::sor<a, reg_rax> {};
struct a : pegtl::sor<reg_rdi, reg_rsi, reg_rdx, sx, reg_r8, reg_r9> {};
struct sx : pegtl::sor<reg_rcx, var> {};
struct s : pegtl::sor<t, label, at_label> {};
struct t : pegtl::sor<x, operand_literal> {};
struct u : pegtl::sor<w, at_label> {};
struct x : pegtl::sor<w, reg_rsp> {};

/*
 * Terminals
 */
struct str_mem : TAO_PEGTL_STRING("mem") {};
struct str_stack_arg : TAO_PEGTL_STRING("stack-arg") {};
struct str_cjump : TAO_PEGTL_STRING("cjump") {};
struct str_goto : TAO_PEGTL_STRING("goto") {};
struct str_return : TAO_PEGTL_STRING("return") {};
struct str_call : TAO_PEGTL_STRING("call") {};
struct str_print : TAO_PEGTL_STRING("print") {};
struct str_input : TAO_PEGTL_STRING("input") {};
struct str_allocate : TAO_PEGTL_STRING("allocate") {};
struct str_tuple_error : TAO_PEGTL_STRING("tuple-error") {};
struct str_tensor_error : TAO_PEGTL_STRING("tensor-error") {};

/*
 * Instruction Dependencies
 */
struct mem_addr : pegtl::seq<str_mem, spaces, x, spaces, M> {};
struct shift_amount : pegtl::sor<sx, operand_literal> {};
struct at_label : l {};

/*
 * Instructions
 */
struct inst_assign : pegtl::seq<w, spaces, op_arrow, spaces, s> {};

struct inst_mem_load : pegtl::seq<w, spaces, op_arrow, spaces, mem_addr> {};

struct inst_mem_store : pegtl::seq<mem_addr, spaces, op_arrow, spaces, s> {};

struct inst_stack_arg : pegtl::seq<w, spaces, op_arrow, spaces, str_stack_arg, spaces, M> {};

struct inst_arithmetic : pegtl::seq<w, spaces, aop, spaces, t> {};

struct inst_shift : pegtl::seq<w, spaces, sop, spaces, shift_amount> {};

struct inst_mem_add_equal
    : pegtl::seq<mem_addr, spaces, op_add_equal, spaces, t> {};

struct inst_mem_sub_equal
    : pegtl::seq<mem_addr, spaces, op_sub_equal, spaces, t> {};

struct inst_add_equal_from_mem
    : pegtl::seq<w, spaces, op_add_equal, spaces, mem_addr> {};

struct inst_sub_equal_from_mem
    : pegtl::seq<w, spaces, op_sub_equal, spaces, mem_addr> {};

struct inst_compare
    : pegtl::seq<w, spaces, op_arrow, spaces, t, spaces, cmp, spaces, t> {};

struct inst_label : label {};

struct inst_cjump
    : pegtl::seq<str_cjump, spaces, t, spaces, cmp, spaces, t, spaces, label> {
};

struct inst_goto : pegtl::seq<str_goto, spaces, label> {};

struct inst_return : pegtl::seq<str_return> {};

struct inst_call : pegtl::seq<str_call, spaces, u, spaces, num_call_args> {};

struct inst_call_print
    : pegtl::seq<str_call, spaces, str_print, spaces, pegtl::one<'1'>> {};

struct inst_call_input
    : pegtl::seq<str_call, spaces, str_input, spaces, pegtl::one<'0'>> {};

struct inst_call_allocate
    : pegtl::seq<str_call, spaces, str_allocate, spaces, pegtl::one<'2'>> {};

struct inst_call_tuple_error
    : pegtl::seq<str_call, spaces, str_tuple_error, spaces, pegtl::one<'3'>> {};

struct inst_call_tensor_error
    : pegtl::seq<str_call, spaces, str_tensor_error, spaces, F> {};

struct inst_inc : pegtl::seq<w, spaces, op_inc> {};

struct inst_dec : pegtl::seq<w, spaces, op_dec> {};

struct inst_lea
    : pegtl::seq<w, spaces, pegtl::one<'@'>, spaces, w, spaces, w, spaces, E> {
};

/*
 * Semantic terminals
 */
struct entry_function_name : l {};
struct function_name : l {};
struct num_function_args : N {};
struct operand_literal : N {};
struct num_call_args : N {};

/*
 * High-level grammar rules
 */
struct program
    : pegtl::seq<seps_with_comments, pegtl::seq<spaces, pegtl::one<'('>>,
                 seps_with_comments, entry_function_name, seps_with_comments,
                 function_list, seps_with_comments,
                 pegtl::seq<spaces, pegtl::one<')'>>, seps_with_comments> {};

struct function
    : pegtl::seq<pegtl::seq<spaces, pegtl::one<'('>>, seps_with_comments,
                 pegtl::seq<spaces, function_name>, seps_with_comments,
                 pegtl::seq<spaces, num_function_args>, seps_with_comments,
                 seps_with_comments, instruction_list, seps_with_comments,
                 pegtl::seq<spaces, pegtl::one<')'>>> {};

struct function_list
    : pegtl::plus<seps_with_comments, function, seps_with_comments> {};

struct instruction : pegtl::sor<
  inst_return, 
  inst_compare, inst_assign, inst_mem_load, inst_mem_store, inst_stack_arg,
  inst_arithmetic, inst_shift,
  inst_mem_add_equal, inst_mem_sub_equal,
  inst_add_equal_from_mem, inst_sub_equal_from_mem,
  inst_cjump, inst_label, inst_goto,
  inst_call_print, inst_call_input, inst_call_allocate,
  inst_call_tuple_error, inst_call_tensor_error,
  inst_call,
  inst_inc, inst_dec, inst_lea
> {};

struct instruction_list
    : pegtl::star<
          pegtl::seq<seps_with_comments, pegtl::seq<spaces, instruction>,
                     seps_with_comments>> {};

struct spill_test : pegtl::seq<function, seps_with_comments, var, seps_with_comments, var> {};

struct grammar : pegtl::must<pegtl::sor<program, spill_test, function>> {};

/*
 * Actions
 */
std::vector<std::unique_ptr<ASTNode>> parsed_items;
std::vector<EnumArithmeticOps> arithmetic_ops;
std::vector<EnumShiftOps> shift_ops;
std::vector<EnumCompareOps> compare_ops;

template <typename Rule>
struct action : pegtl::nothing<Rule> {};

template <>
struct action<label> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    parsed_items.push_back(std::make_unique<Label>(input.string()));
  }
};

template<>
struct action<var> {
    template <typename Input>
    static void apply(const Input& input, Program& program) {
      parsed_items.push_back(std::make_unique<Var>(input.string()));
    }
};

template <>
struct action<M> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    int64_t value = std::stoll(input.string());
    if (value % 8 != 0) {
      throw std::runtime_error("action<M>: M is not a multiple of 8");
    }
    parsed_items.push_back(std::make_unique<Number>(value));
  }
};

template <>
struct action<F> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    int64_t value = std::stoll(input.string());
    if (value != 1 && value != 3 && value != 4) {
      throw std::runtime_error("action<F>: F is not valid");
    }
    parsed_items.push_back(std::make_unique<Number>(value));
  }
};

template <>
struct action<reg_rdi> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    parsed_items.push_back(std::make_unique<Register>(RegisterID::rdi));
  }
};

template <>
struct action<reg_rsi> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    parsed_items.push_back(std::make_unique<Register>(RegisterID::rsi));
  }
};

template <>
struct action<reg_rdx> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    parsed_items.push_back(std::make_unique<Register>(RegisterID::rdx));
  }
};

template <>
struct action<reg_rcx> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    parsed_items.push_back(std::make_unique<Register>(RegisterID::rcx));
  }
};

template <>
struct action<reg_r8> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    parsed_items.push_back(std::make_unique<Register>(RegisterID::r8));
  }
};

template <>
struct action<reg_r9> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    parsed_items.push_back(std::make_unique<Register>(RegisterID::r9));
  }
};

template <>
struct action<reg_rax> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    parsed_items.push_back(std::make_unique<Register>(RegisterID::rax));
  }
};

template <>
struct action<reg_rsp> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    parsed_items.push_back(std::make_unique<Register>(RegisterID::rsp));
  }
};

template <>
struct action<op_add_equal> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    arithmetic_ops.push_back(EnumArithmeticOps::ADD_EQUAL);
  }
};

template <>
struct action<op_sub_equal> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    arithmetic_ops.push_back(EnumArithmeticOps::SUB_EQUAL);
  }
};

template <>
struct action<op_mul_equal> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    arithmetic_ops.push_back(EnumArithmeticOps::MUL_EQUAL);
  }
};

template <>
struct action<op_and_equal> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    arithmetic_ops.push_back(EnumArithmeticOps::AND_EQUAL);
  }
};

template <>
struct action<op_lshift_equal> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    shift_ops.push_back(EnumShiftOps::LEFT_SHIFT);
  }
};

template <>
struct action<op_rshift_equal> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    shift_ops.push_back(EnumShiftOps::RIGHT_SHIFT);
  }
};

template <>
struct action<op_less> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    compare_ops.push_back(EnumCompareOps::LESS_THAN);
  }
};

template <>
struct action<op_less_equal> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    compare_ops.push_back(EnumCompareOps::LESS_EQUAL);
  }
};

template <>
struct action<op_equal> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    compare_ops.push_back(EnumCompareOps::EQUAL);
  }
};

template <>
struct action<mem_addr> {
  template <typename Input>
  static void apply(const Input&, Program&) {
    auto offset = std::move(parsed_items.back());
    parsed_items.pop_back();
    auto location = std::move(parsed_items.back());
    parsed_items.pop_back();

    if (!dynamic_cast<Number*>(offset.get())) {
      throw std::runtime_error("action<mem_addr>: expected number");
    }

    if (!dynamic_cast<Register*>(location.get()) && !dynamic_cast<Var*>(location.get())) {
      throw std::runtime_error("action<mem_addr>: expected register or var");
    }

    parsed_items.push_back(
      std::make_unique<MemoryAddress>(std::move(location), std::move(offset))
    );
  }
};

template<>
struct action<at_label> {
  template<typename Input>
  static void apply(const Input& input, Program& program) {
    parsed_items.push_back(std::make_unique<Label>(input.string()));
  }
};

template <>
struct action<inst_assign> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    Function* curFunction = program.functions.back().get();

    auto src = std::move(parsed_items.back());
    parsed_items.pop_back();
    auto dst = std::move(parsed_items.back());
    parsed_items.pop_back();

    std::unique_ptr<Instruction> instruction =
        std::make_unique<AssignmentInstruction>(std::move(dst), std::move(src));
    curFunction->add_instruction(std::move(instruction));
  }
};

/*
 *
 * If I have time, keep adding type checking from here on forwards
 * 
 */

template <>
struct action<inst_mem_load> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    Function* curFunction = program.functions.back().get();
    auto memoryAddr = std::move(parsed_items.back());
    parsed_items.pop_back();
    auto location = std::move(parsed_items.back());
    parsed_items.pop_back();

    std::unique_ptr<Instruction> instruction =
        std::make_unique<AssignmentInstruction>(std::move(location),
                                                std::move(memoryAddr));

    curFunction->add_instruction(std::move(instruction));
  }
};

template <>
struct action<inst_mem_store> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    Function* curFunction = program.functions.back().get();

    auto location = std::move(parsed_items.back());
    parsed_items.pop_back();
    auto memoryAddr = std::move(parsed_items.back());
    parsed_items.pop_back();

    std::unique_ptr<Instruction> instruction =
        std::make_unique<AssignmentInstruction>(std::move(memoryAddr),
                                                std::move(location));

    curFunction->add_instruction(std::move(instruction));
  }
};

template <>
struct action<inst_stack_arg> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    Function* curFunction = program.functions.back().get();

    auto offset = std::move(parsed_items.back());
    parsed_items.pop_back();
    auto dst = std::move(parsed_items.back());
    parsed_items.pop_back();

    // if (!dynamic_cast<Number*>(offset.get())) {
    //   throw std::runtime_error("action<inst_stack_arg>: Expected number");
    // }
    // if (!dynamic_cast<Register*>(dst.get()) && !dynamic_cast<Var*>(dst.get())) {
    //   throw std::runtime_error("action<inst_stack_arg>: Expected register or var");
    // }

    std::unique_ptr<ASTNode> stackArg = std::make_unique<StackArg>(std::move(offset));
    std::unique_ptr<Instruction> instruction =
        std::make_unique<AssignmentInstruction>(std::move(dst), std::move(stackArg));

    curFunction->add_instruction(std::move(instruction));
  }
};

template <>
struct action<inst_arithmetic> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    Function* curFunction = program.functions.back().get();

    auto src = std::move(parsed_items.back());
    parsed_items.pop_back();
    auto dst = std::move(parsed_items.back());
    parsed_items.pop_back();
    EnumArithmeticOps op = arithmetic_ops.back();
    arithmetic_ops.pop_back();

    std::unique_ptr<ArithmeticOp> arithmeticOp =
        std::make_unique<ArithmeticOp>(std::move(dst), std::move(src), op);
    std::unique_ptr<Instruction> instruction =
        std::make_unique<ArithmeticInstruction>(std::move(arithmeticOp));
    curFunction->add_instruction(std::move(instruction));
  }
};

template <>
struct action<inst_shift> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    Function* curFunction = program.functions.back().get();
    auto src = std::move(parsed_items.back());
    parsed_items.pop_back();
    auto dst = std::move(parsed_items.back());
    parsed_items.pop_back();
    EnumShiftOps op = shift_ops.back();
    shift_ops.pop_back();
    std::unique_ptr<ShiftOp> shiftOp =
        std::make_unique<ShiftOp>(std::move(dst), std::move(src), op);
    std::unique_ptr<Instruction> instruction =
        std::make_unique<ShiftInstruction>(std::move(shiftOp));
    curFunction->add_instruction(std::move(instruction));
  }
};

template <>
struct action<inst_mem_add_equal> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    Function* curFunction = program.functions.back().get();
    auto src = std::move(parsed_items.back());
    parsed_items.pop_back();
    auto dst = std::move(parsed_items.back());
    parsed_items.pop_back();
    EnumArithmeticOps op = arithmetic_ops.back();
    arithmetic_ops.pop_back();
    std::unique_ptr<ArithmeticOp> arithmeticOp =
        std::make_unique<ArithmeticOp>(std::move(dst), std::move(src), op);
    std::unique_ptr<Instruction> instruction =
        std::make_unique<ArithmeticInstruction>(std::move(arithmeticOp));
    curFunction->add_instruction(std::move(instruction));
  }
};

template <>
struct action<inst_mem_sub_equal> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    Function* curFunction = program.functions.back().get();
    auto src = std::move(parsed_items.back());
    parsed_items.pop_back();
    auto dst = std::move(parsed_items.back());
    parsed_items.pop_back();
    EnumArithmeticOps op = arithmetic_ops.back();
    arithmetic_ops.pop_back();
    std::unique_ptr<ArithmeticOp> arithmeticOp =
        std::make_unique<ArithmeticOp>(std::move(dst), std::move(src), op);
    std::unique_ptr<Instruction> instruction =
        std::make_unique<ArithmeticInstruction>(std::move(arithmeticOp));
    curFunction->add_instruction(std::move(instruction));
  }
};

template <>
struct action<inst_add_equal_from_mem> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    Function* curFunction = program.functions.back().get();
    auto src = std::move(parsed_items.back());
    parsed_items.pop_back();
    auto dst = std::move(parsed_items.back());
    parsed_items.pop_back();
    EnumArithmeticOps op = arithmetic_ops.back();
    arithmetic_ops.pop_back();
    std::unique_ptr<ArithmeticOp> arithmeticOp =
        std::make_unique<ArithmeticOp>(std::move(dst), std::move(src), op);
    std::unique_ptr<Instruction> instruction =
        std::make_unique<ArithmeticInstruction>(std::move(arithmeticOp));
    curFunction->add_instruction(std::move(instruction));
  }
};

template <>
struct action<inst_sub_equal_from_mem> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    Function* curFunction = program.functions.back().get();
    auto src = std::move(parsed_items.back());
    parsed_items.pop_back();
    auto dst = std::move(parsed_items.back());
    parsed_items.pop_back();
    EnumArithmeticOps op = arithmetic_ops.back();
    arithmetic_ops.pop_back();
    std::unique_ptr<ArithmeticOp> arithmeticOp =
        std::make_unique<ArithmeticOp>(std::move(dst), std::move(src), op);
    std::unique_ptr<Instruction> instruction =
        std::make_unique<ArithmeticInstruction>(std::move(arithmeticOp));
    curFunction->add_instruction(std::move(instruction));
  }
};

template <>
struct action<inst_compare> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    Function* curFunction = program.functions.back().get();
    auto right = std::move(parsed_items.back());
    parsed_items.pop_back();
    auto left = std::move(parsed_items.back());
    parsed_items.pop_back();
    auto dst = std::move(parsed_items.back());
    parsed_items.pop_back();
    EnumCompareOps eOp = compare_ops.back();
    compare_ops.pop_back();
    std::unique_ptr<CompareOp> op =
        std::make_unique<CompareOp>(std::move(left), std::move(right), eOp);
    std::unique_ptr<Instruction> instruction =
        std::make_unique<CompareInstruction>(std::move(dst), std::move(op));
    curFunction->add_instruction(std::move(instruction));
  }
};

template <>
struct action<inst_label> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    Function* curFunction = program.functions.back().get();
    std::unique_ptr<ASTNode> label = std::make_unique<Label>(input.string());
    std::unique_ptr<Instruction> instruction =
        std::make_unique<LabelInstruction>(std::move(label));
    curFunction->add_instruction(std::move(instruction));
  }
};

template <>
struct action<inst_cjump> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    Function* curFunction = program.functions.back().get();
    auto label = std::move(parsed_items.back());
    parsed_items.pop_back();
    auto right = std::move(parsed_items.back());
    parsed_items.pop_back();
    auto left = std::move(parsed_items.back());
    parsed_items.pop_back();
    EnumCompareOps eOp = compare_ops.back();
    compare_ops.pop_back();
    std::unique_ptr<CompareOp> op =
        std::make_unique<CompareOp>(std::move(left), std::move(right), eOp);
    std::unique_ptr<Instruction> instruction =
        std::make_unique<ConditionalJumpInstruction>(std::move(op),
                                                     std::move(label));
    curFunction->add_instruction(std::move(instruction));
  }
};

template <>
struct action<inst_goto> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    Function* curFunction = program.functions.back().get();
    auto label = std::move(parsed_items.back());
    parsed_items.pop_back();
    std::unique_ptr<Instruction> instruction =
        std::make_unique<JumpInstruction>(std::move(label));
    curFunction->add_instruction(std::move(instruction));
  }
};

template <>
struct action<inst_return> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    Function* curFunction = program.functions.back().get();
    std::unique_ptr<Instruction> returnInstruction =
        std::make_unique<ReturnInstruction>();
    curFunction->add_instruction(std::move(returnInstruction));
  }
};

template <>
struct action<inst_call> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    Function* curFunction = program.functions.back().get();
    std::unique_ptr<ASTNode> numArgs = std::move(parsed_items.back());
    parsed_items.pop_back();
    std::unique_ptr<ASTNode> target = std::move(parsed_items.back());
    parsed_items.pop_back();
    std::unique_ptr<Instruction> instruction = std::make_unique<CallInstruction>(std::move(target), std::move(numArgs));
    curFunction->add_instruction(std::move(instruction));
  }
};

template <>
struct action<inst_call_print> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    Function* curFunction = program.functions.back().get();
    std::unique_ptr<Instruction> instruction = std::make_unique<CallPrintInstruction>();
    curFunction->add_instruction(std::move(instruction));
  }
};

template <>
struct action<inst_call_input> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    Function* curFunction = program.functions.back().get();
    std::unique_ptr<Instruction> instruction = std::make_unique<CallInputInstruction>();
    curFunction->add_instruction(std::move(instruction));
  }
};

template <>
struct action<inst_call_allocate> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    Function* curFunction = program.functions.back().get();
    std::unique_ptr<Instruction> instruction = std::make_unique<CallAllocateInstruction>();
    curFunction->add_instruction(std::move(instruction));
  }
};

template <>
struct action<inst_call_tuple_error> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    Function* curFunction = program.functions.back().get();
    std::unique_ptr<Instruction> instruction = std::make_unique<CallTupleErrorInstruction>();
    curFunction->add_instruction(std::move(instruction));
  }
};

template <>
struct action<inst_call_tensor_error> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    Function* curFunction = program.functions.back().get();
    auto f = std::move(parsed_items.back());
    parsed_items.pop_back();
    std::unique_ptr<Instruction> instruction = std::make_unique<CallTensorErrorInstruction>(std::move(f));
    curFunction->add_instruction(std::move(instruction));
  }
};

template <>
struct action<inst_inc> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    Function* curFunction = program.functions.back().get();
    auto reg = std::move(parsed_items.back());
    parsed_items.pop_back();
    std::unique_ptr<Instruction> instruction =
        std::make_unique<IncrementInstruction>(std::move(reg));
    curFunction->add_instruction(std::move(instruction));
  }
};

template <>
struct action<inst_dec> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    Function* curFunction = program.functions.back().get();
    auto reg = std::move(parsed_items.back());
    parsed_items.pop_back();
    std::unique_ptr<Instruction> instruction =
        std::make_unique<DecrementInstruction>(std::move(reg));
    curFunction->add_instruction(std::move(instruction));
  }
};

template <>
struct action<E> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    std::unique_ptr<Number> e = std::make_unique<Number>(std::stoll(input.string()));
    parsed_items.push_back(std::move(e));
  }
};

template <>
struct action<inst_lea> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    Function* curFunction = program.functions.back().get();
    std::unique_ptr<ASTNode> e = std::move(parsed_items.back());
    parsed_items.pop_back();
    std::unique_ptr<ASTNode> reg3 = std::move(parsed_items.back());
    parsed_items.pop_back();
    std::unique_ptr<ASTNode> reg2 = std::move(parsed_items.back());
    parsed_items.pop_back();
    std::unique_ptr<ASTNode> reg1 = std::move(parsed_items.back());
    parsed_items.pop_back();
    std::unique_ptr<Instruction> instruction = std::make_unique<LeaInstruction>(
        std::move(reg1), std::move(reg2), std::move(reg3), std::move(e));
    curFunction->add_instruction(std::move(instruction));
  }
};

template <>
struct action<entry_function_name> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    program.entryPoint = input.string();
  }
};

template <>
struct action<function_name> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    std::unique_ptr<Function> function = std::make_unique<Function>();
    function->name = input.string();
    program.functions.push_back(std::move(function));
  }
};

template <>
struct action<num_function_args> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    if (program.functions.empty()) {
      throw std::runtime_error(
          "action<num_function_args>: program.functions is empty");
    }
    Function* curFunction = program.functions.back().get();
    curFunction->numArgs = std::stoll(input.string());
  }
};

template <>
struct action<spill_test> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    Function* curFunction = program.functions[0].get();
    auto prefix = std::move(parsed_items.back());
    parsed_items.pop_back();
    auto var = std::move(parsed_items.back());
    parsed_items.pop_back();
    std::unique_ptr<Var> convertedPrefix(static_cast<Var*>(prefix.release()));
    std::unique_ptr<Var> convertedVar(static_cast<Var*>(var.release()));
    curFunction->testSpillPrefixName = convertedPrefix->name;
    curFunction->testSpillVarName = convertedVar->name; 
  }
};


template <>
struct action<operand_literal> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    std::unique_ptr<Number> num =
        std::make_unique<Number>(std::stoll(input.string()));
    parsed_items.push_back(std::move(num));
  }
};

template <>
struct action<num_call_args> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    std::unique_ptr<Number> num = std::make_unique<Number>(std::stoll(input.string()));
    parsed_items.push_back(std::move(num));
  }
};

/*
 * Parse file
 */
Program parse_file(char* fileName) {

  parsed_items.clear();
  arithmetic_ops.clear();
  shift_ops.clear();
  compare_ops.clear();

  if (pegtl::analyze<grammar>() != 0) {
    std::cerr << "There are problems with the grammar" << std::endl;
    exit(1);
  }
  file_input<> fileInput(fileName);
  Program program;
  parse<grammar, action>(fileInput, program);
  return program;
}

}  // namespace L2
