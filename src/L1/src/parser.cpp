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
#include <L1.h>
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

namespace L1 {

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
struct reg_rbx : TAO_PEGTL_STRING("rbx") {};
struct reg_rbp : TAO_PEGTL_STRING("rbp") {};
struct reg_r10 : TAO_PEGTL_STRING("r10") {};
struct reg_r11 : TAO_PEGTL_STRING("r11") {};
struct reg_r12 : TAO_PEGTL_STRING("r12") {};
struct reg_r13 : TAO_PEGTL_STRING("r13") {};
struct reg_r14 : TAO_PEGTL_STRING("r14") {};
struct reg_r15 : TAO_PEGTL_STRING("r15") {};
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
struct w : pegtl::sor<a, reg_rax, reg_rbx, reg_rbp, reg_r10, reg_r11, reg_r12,
                      reg_r13, reg_r14, reg_r15> {};

struct a : pegtl::sor<reg_rdi, reg_rsi, reg_rdx, reg_rcx, reg_r8, reg_r9> {};

struct s : pegtl::sor<t, label, at_label> {};
struct t : pegtl::sor<x, operand_literal> {};
struct u : pegtl::sor<w, at_label> {};
struct x : pegtl::sor<w, reg_rsp> {};

/*
 * Terminals
 */
struct str_mem : TAO_PEGTL_STRING("mem") {};
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
struct shift_amount : pegtl::sor<reg_rcx, operand_literal> {};
struct at_label : l {};

/*
 * Instructions
 */
struct inst_assign : pegtl::seq<w, spaces, op_arrow, spaces, s> {};

struct inst_mem_load : pegtl::seq<w, spaces, op_arrow, spaces, mem_addr> {};

struct inst_mem_store : pegtl::seq<mem_addr, spaces, op_arrow, spaces, s> {};

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
struct function_name : l {};
struct num_function_args : N {};
struct num_function_locals : N {};
struct operand_literal : N {};
struct num_call_args : N {};

/*
 * High-level grammar rules
 */
struct program
    : pegtl::seq<seps_with_comments, pegtl::seq<spaces, pegtl::one<'('>>,
                 seps_with_comments, function_name, seps_with_comments,
                 function_list, seps_with_comments,
                 pegtl::seq<spaces, pegtl::one<')'>>, seps_with_comments> {};

struct function
    : pegtl::seq<pegtl::seq<spaces, pegtl::one<'('>>, seps_with_comments,
                 pegtl::seq<spaces, function_name>, seps_with_comments,
                 pegtl::seq<spaces, num_function_args>, seps_with_comments,
                 pegtl::seq<spaces, num_function_locals>, seps_with_comments,
                 instruction_list, seps_with_comments,
                 pegtl::seq<spaces, pegtl::one<')'>>> {};

struct function_list
    : pegtl::plus<seps_with_comments, function, seps_with_comments> {};

struct instruction : pegtl::sor<
  inst_return, 
  inst_compare, inst_assign, inst_mem_load, inst_mem_store,
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

struct grammar : pegtl::must<program> {};

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
    std::unique_ptr<Label> aLabel = std::make_unique<Label>(input.string());
    parsed_items.push_back(std::move(aLabel));
  }
};

template <>
struct action<l> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    std::unique_ptr<Label> cLabel = std::make_unique<Label>(input.string());
    parsed_items.push_back(std::move(cLabel));
  }
};

template <>
struct action<M> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    int64_t value = std::stoll(input.string());
    if (value % 8 != 0) {
      throw std::runtime_error("M is not a multiple of 8");
    }
    std::unique_ptr<Number> num = std::make_unique<Number>(value);
    parsed_items.push_back(std::move(num));
  }
};

template <>
struct action<F> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    int64_t value = std::stoll(input.string());
    std::unique_ptr<Number> num = std::make_unique<Number>(value);
    parsed_items.push_back(std::move(num));
  }
};

template <>
struct action<reg_rdi> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    std::unique_ptr<Register> reg = std::make_unique<Register>(RegisterID::rdi);
    parsed_items.push_back(std::move(reg));
  }
};

template <>
struct action<reg_rsi> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    std::unique_ptr<Register> reg = std::make_unique<Register>(RegisterID::rsi);
    parsed_items.push_back(std::move(reg));
  }
};

template <>
struct action<reg_rdx> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    std::unique_ptr<Register> reg = std::make_unique<Register>(RegisterID::rdx);
    parsed_items.push_back(std::move(reg));
  }
};

template <>
struct action<reg_rcx> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    std::unique_ptr<Register> reg = std::make_unique<Register>(RegisterID::rcx);
    parsed_items.push_back(std::move(reg));
  }
};

template <>
struct action<reg_r8> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    std::unique_ptr<Register> reg = std::make_unique<Register>(RegisterID::r8);
    parsed_items.push_back(std::move(reg));
  }
};

template <>
struct action<reg_r9> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    std::unique_ptr<Register> reg = std::make_unique<Register>(RegisterID::r9);
    parsed_items.push_back(std::move(reg));
  }
};

template <>
struct action<reg_rax> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    std::unique_ptr<Register> reg = std::make_unique<Register>(RegisterID::rax);
    parsed_items.push_back(std::move(reg));
  }
};

template <>
struct action<reg_rbx> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    std::unique_ptr<Register> reg = std::make_unique<Register>(RegisterID::rbx);
    parsed_items.push_back(std::move(reg));
  }
};

template <>
struct action<reg_rbp> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    std::unique_ptr<Register> reg = std::make_unique<Register>(RegisterID::rbp);
    parsed_items.push_back(std::move(reg));
  }
};

template <>
struct action<reg_rsp> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    std::unique_ptr<Register> reg = std::make_unique<Register>(RegisterID::rsp);
    parsed_items.push_back(std::move(reg));
  }
};

template <>
struct action<reg_r10> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    std::unique_ptr<Register> reg = std::make_unique<Register>(RegisterID::r10);
    parsed_items.push_back(std::move(reg));
  }
};

template <>
struct action<reg_r11> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    std::unique_ptr<Register> reg = std::make_unique<Register>(RegisterID::r11);
    parsed_items.push_back(std::move(reg));
  }
};

template <>
struct action<reg_r12> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    std::unique_ptr<Register> reg = std::make_unique<Register>(RegisterID::r12);
    parsed_items.push_back(std::move(reg));
  }
};

template <>
struct action<reg_r13> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    std::unique_ptr<Register> reg = std::make_unique<Register>(RegisterID::r13);
    parsed_items.push_back(std::move(reg));
  }
};

template <>
struct action<reg_r14> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    std::unique_ptr<Register> reg = std::make_unique<Register>(RegisterID::r14);
    parsed_items.push_back(std::move(reg));
  }
};

template <>
struct action<reg_r15> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    std::unique_ptr<Register> reg = std::make_unique<Register>(RegisterID::r15);
    parsed_items.push_back(std::move(reg));
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
  static void apply(const Input& input, Program& program) {
    auto offsetBase = std::move(parsed_items.back());
    parsed_items.pop_back();
    auto regBase = std::move(parsed_items.back());
    parsed_items.pop_back();
    auto* offsetRaw = dynamic_cast<Number*>(offsetBase.get());
    if (!offsetRaw) {
      throw std::runtime_error("action<mem_addr>: expected number");
    }
    auto* regRaw = dynamic_cast<Register*>(regBase.get());
    if (!regRaw) {
      throw std::runtime_error("action<mem_addr>: expected register");
    }
    std::unique_ptr<Number> offsetNode(
        static_cast<Number*>(offsetBase.release()));
    std::unique_ptr<Register> regNode(
        static_cast<Register*>(regBase.release()));
    parsed_items.push_back(std::make_unique<MemoryAddress>(
        std::move(regNode), std::move(offsetNode)));
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
    Function* cur = program.functions.back().get();

    auto src = std::move(parsed_items.back());
    parsed_items.pop_back();
    auto dst = std::move(parsed_items.back());
    parsed_items.pop_back();

    std::unique_ptr<Instruction> instruction =
        std::make_unique<AssignmentInstruction>(std::move(dst), std::move(src));
    cur->instructions.push_back(std::move(instruction));
  }
};

template <>
struct action<inst_mem_load> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    Function* curFunction = program.functions.back().get();
    auto memoryAddr = std::move(parsed_items.back());
    parsed_items.pop_back();
    auto reg = std::move(parsed_items.back());
    parsed_items.pop_back();
    auto memoryAddrRaw = dynamic_cast<MemoryAddress*>(memoryAddr.get());
    if (!memoryAddrRaw) {
      throw std::runtime_error("action<inst_mem_load>: expected MemoryAddress");
    }
    auto regRaw = dynamic_cast<Register*>(reg.get());
    if (!regRaw) {
      throw std::runtime_error("action<inst_mem_load>: expected Register");
    }
    std::unique_ptr<Instruction> instruction =
        std::make_unique<AssignmentInstruction>(std::move(reg),
                                                std::move(memoryAddr));
    curFunction->instructions.push_back(std::move(instruction));
  }
};

template <>
struct action<inst_mem_store> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    Function* curFunction = program.functions.back().get();
    auto src = std::move(parsed_items.back());
    parsed_items.pop_back();
    auto memoryAddr = std::move(parsed_items.back());
    parsed_items.pop_back();
    auto memoryAddrRaw = dynamic_cast<MemoryAddress*>(memoryAddr.get());
    if (!memoryAddrRaw) {
      throw std::runtime_error(
          "action<inst_mem_store>: expected MemoryAddress");
    }
    std::unique_ptr<Instruction> instruction =
        std::make_unique<AssignmentInstruction>(std::move(memoryAddr),
                                                std::move(src));
    curFunction->instructions.push_back(std::move(instruction));
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
    curFunction->instructions.push_back(std::move(instruction));
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
    curFunction->instructions.push_back(std::move(instruction));
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
    curFunction->instructions.push_back(std::move(instruction));
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
    curFunction->instructions.push_back(std::move(instruction));
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
    curFunction->instructions.push_back(std::move(instruction));
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
    curFunction->instructions.push_back(std::move(instruction));
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
    curFunction->instructions.push_back(std::move(instruction));
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
    curFunction->instructions.push_back(std::move(instruction));
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
    curFunction->instructions.push_back(std::move(instruction));
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
    curFunction->instructions.push_back(std::move(instruction));
  }
};

template <>
struct action<inst_return> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    Function* curFunction = program.functions.back().get();
    std::unique_ptr<Instruction> returnInstruction =
        std::make_unique<ReturnInstruction>();
    curFunction->instructions.push_back(std::move(returnInstruction));
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
    curFunction->instructions.push_back(std::move(instruction));
  }
};

template <>
struct action<inst_call_print> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    Function* curFunction = program.functions.back().get();
    std::unique_ptr<Instruction> instruction = std::make_unique<CallPrintInstruction>();
    curFunction->instructions.push_back(std::move(instruction));
  }
};

template <>
struct action<inst_call_input> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    Function* curFunction = program.functions.back().get();
    std::unique_ptr<Instruction> instruction = std::make_unique<CallInputInstruction>();
    curFunction->instructions.push_back(std::move(instruction));
  }
};

template <>
struct action<inst_call_allocate> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    Function* curFunction = program.functions.back().get();
    std::unique_ptr<Instruction> instruction = std::make_unique<CallAllocateInstruction>();
    curFunction->instructions.push_back(std::move(instruction));
  }
};

template <>
struct action<inst_call_tuple_error> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    Function* curFunction = program.functions.back().get();
    std::unique_ptr<Instruction> instruction = std::make_unique<CallTupleErrorInstruction>();
    curFunction->instructions.push_back(std::move(instruction));
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
    curFunction->instructions.push_back(std::move(instruction));
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
    curFunction->instructions.push_back(std::move(instruction));
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
    curFunction->instructions.push_back(std::move(instruction));
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
    curFunction->instructions.push_back(std::move(instruction));
  }
};

template <>
struct action<function_name> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    if (program.entryPoint.name.empty()) {
      program.entryPoint = input.string();
    } else {
      std::unique_ptr<Function> function = std::make_unique<Function>();
      function->name = input.string();
      program.functions.push_back(std::move(function));
    }
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
struct action<num_function_locals> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    Function* curFunction = program.functions.back().get();
    curFunction->numLocal = std::stoll(input.string());
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
  if (pegtl::analyze<grammar>() != 0) {
    std::cerr << "There are problems with the grammar" << std::endl;
    exit(1);
  }
  file_input<> fileInput(fileName);
  Program program;
  parse<grammar, action>(fileInput, program);
  return program;
}

}  // namespace L1
