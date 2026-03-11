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
#include "parser.h"

#include <assert.h>
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
#include <climits>

namespace pegtl = TAO_PEGTL_NAMESPACE;

using namespace pegtl;

namespace L3 {

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
struct N : pegtl::seq<pegtl::opt<pegtl::sor<pegtl::one<'-'>, pegtl::one<'+'>>>,
                      pegtl::plus<pegtl::digit>> {};

/*
 * Operators
 */
struct op_add : TAO_PEGTL_STRING("+") {};
struct op_sub : TAO_PEGTL_STRING("-") {};
struct op_mul : TAO_PEGTL_STRING("*") {};
struct op_and : TAO_PEGTL_STRING("&") {};
struct op_lshift : TAO_PEGTL_STRING("<<") {};
struct op_rshift : TAO_PEGTL_STRING(">>") {};

struct op_less : TAO_PEGTL_STRING("<") {};
struct op_less_equal : TAO_PEGTL_STRING("<=") {};
struct op_equal : TAO_PEGTL_STRING("=") {};
struct op_greater_equal : TAO_PEGTL_STRING(">=") {};
struct op_greater : TAO_PEGTL_STRING(">") {};

struct op_arrow : TAO_PEGTL_STRING("\x3c\x2D") {};

/*
 * Callee strings
 */
struct str_print : TAO_PEGTL_STRING("print") {};
struct str_allocate : TAO_PEGTL_STRING("allocate") {};
struct str_input : TAO_PEGTL_STRING("input") {};
struct str_tuple_error : TAO_PEGTL_STRING("tuple-error") {};
struct str_tensor_error : TAO_PEGTL_STRING("tensor-error") {};

/*
 * Nonterminals
 */
struct s : pegtl::sor<t, label, l> {};
struct t : pegtl::sor<var, N> {};
struct u : pegtl::sor<var, l> {};
struct op : pegtl::sor<op_add, op_sub, op_mul, op_and, op_lshift, op_rshift> {};
struct cmp : pegtl::sor<op_less_equal, op_greater_equal, op_equal, op_less,
                        op_greater> {};

/*
 * Terminals
 */
struct str_load : TAO_PEGTL_STRING("load") {};
struct str_store : TAO_PEGTL_STRING("store") {};
struct str_return : TAO_PEGTL_STRING("return") {};
struct str_br : TAO_PEGTL_STRING("br") {};
struct str_call : TAO_PEGTL_STRING("call") {};
struct str_define : TAO_PEGTL_STRING("define") {};

/*
 * Instruction Dependencies
 */
struct callee : pegtl::sor<u, str_print, str_allocate, str_input,
                           str_tuple_error, str_tensor_error> {};

struct comma_sep : pegtl::seq<spaces, pegtl::one<','>, spaces> {};
struct vars_list : pegtl::list<var, comma_sep> {};
struct vars : pegtl::opt<vars_list> {};
struct args_list : pegtl::list<t, comma_sep> {};
struct args : pegtl::opt<args_list> {};

/*
 * Instructions
 */
struct inst_assign : pegtl::seq<var, spaces, op_arrow, spaces, s> {};
struct inst_assign_arithmetic
    : pegtl::seq<var, spaces, op_arrow, spaces, t, spaces, op, spaces, t> {};
struct inst_assign_comp
    : pegtl::seq<var, spaces, op_arrow, spaces, t, spaces, cmp, spaces, t> {};
struct inst_load
    : pegtl::seq<var, spaces, op_arrow, spaces, str_load, spaces, var> {};
struct inst_store
    : pegtl::seq<str_store, spaces, var, spaces, op_arrow, spaces, s> {};
struct inst_return : pegtl::seq<str_return> {};
struct inst_return_val : pegtl::seq<str_return, spaces, t> {};
struct inst_label : pegtl::seq<label> {};
struct inst_branch : pegtl::seq<str_br, spaces, label> {};
struct inst_conditional_branch : pegtl::seq<str_br, spaces, t, spaces, label> {
};
struct inst_call : pegtl::seq<str_call, spaces, callee, spaces, pegtl::one<'('>,
                              spaces, args, spaces, pegtl::one<')'>> {};
struct inst_assign_call : pegtl::seq<var, spaces, op_arrow, spaces, str_call,
                                     spaces, callee, spaces, pegtl::one<'('>,
                                     spaces, args, spaces, pegtl::one<')'>> {};

/*
 * Semantic terminals
 */
struct function_name : l {};
struct function_vars : vars {};

/*
 * High-level grammar rules
 */
struct inst : pegtl::sor<inst_assign_arithmetic, inst_assign_comp,
                         inst_assign_call, inst_assign, inst_load, inst_store,
                         inst_return_val, inst_return, inst_label,
                         inst_conditional_branch, inst_branch, inst_call> {};

struct inst_list : pegtl::plus<pegtl::seq<seps_with_comments, spaces, inst, spaces,
                               seps_with_comments>> {};

struct function
    : pegtl::seq<str_define, spaces, function_name, spaces, pegtl::one<'('>,
                 spaces, function_vars, spaces, pegtl::one<')'>, spaces,
                 seps_with_comments, pegtl::one<'{'>, spaces, seps_with_comments, inst_list,
                 seps_with_comments, spaces, seps_with_comments, pegtl::one<'}'>, spaces> {};

struct function_list
    : pegtl::plus<seps_with_comments, spaces, function, spaces, seps_with_comments> {};

struct program : function_list {};

struct grammar : pegtl::must<program> {};

/*
 * Actions
 */
using Item = std::variant<Var, Number, Label>;

std::vector<Item> parsed_items;
std::vector<EnumArithmeticOp> arithmetic_ops;
std::vector<EnumCompareOp> compare_ops;
std::vector<EnumCallee> enum_callee;

template <typename Rule>
struct action : pegtl::nothing<Rule> {};

template <>
struct action<label> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    parsed_items.push_back(Label{input.string()});
  }
};

template <>
struct action<l> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    parsed_items.push_back(Label{input.string()});
  }
};

template <>
struct action<var> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    parsed_items.push_back(Var{input.string()});
  }
};

template <>
struct action<N> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    parsed_items.push_back(Number{std::stoll(input.string())});
  }
};

template <>
struct action<op_add> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    arithmetic_ops.push_back(EnumArithmeticOp::ADD);
  }
};

template <>
struct action<op_sub> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    arithmetic_ops.push_back(EnumArithmeticOp::SUB);
  }
};

template <>
struct action<op_mul> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    arithmetic_ops.push_back(EnumArithmeticOp::MUL);
  }
};

template <>
struct action<op_and> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    arithmetic_ops.push_back(EnumArithmeticOp::AND);
  }
};

template <>
struct action<op_lshift> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    arithmetic_ops.push_back(EnumArithmeticOp::LSHIFT);
  }
};

template <>
struct action<op_rshift> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    arithmetic_ops.push_back(EnumArithmeticOp::RSHIFT);
  }
};

template <>
struct action<op_less> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    compare_ops.push_back(EnumCompareOp::LT);
  }
};

template <>
struct action<op_less_equal> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    compare_ops.push_back(EnumCompareOp::LE);
  }
};

template <>
struct action<op_equal> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    compare_ops.push_back(EnumCompareOp::EQ);
  }
};

template <>
struct action<op_greater_equal> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    compare_ops.push_back(EnumCompareOp::GE);
  }
};

template <>
struct action<op_greater> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    compare_ops.push_back(EnumCompareOp::GT);
  }
};

template <>
struct action<str_print> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    enum_callee.push_back(EnumCallee::PRINT);
  }
};

template <>
struct action<str_allocate> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    enum_callee.push_back(EnumCallee::ALLOCATE);
  }
};

template <>
struct action<str_input> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    enum_callee.push_back(EnumCallee::INPUT);
  }
};

template <>
struct action<str_tuple_error> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    enum_callee.push_back(EnumCallee::TUPLE_ERROR);
  }
};

template <>
struct action<str_tensor_error> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    enum_callee.push_back(EnumCallee::TENSOR_ERROR);
  }
};

Operand itemToOperand(const Item& item) {
  return std::visit(
      [](auto&& val) -> Operand {
        using T = std::decay_t<decltype(val)>;

        if constexpr (std::is_same_v<T, Var>) {
          return val;
        } else if constexpr (std::is_same_v<T, Number>) {
          return val;
        } else {
          throw std::runtime_error("Invalid operand type");
        }
      },
      item);
}

template <>
struct action<inst_assign> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    Item right = std::move(parsed_items.back());
    parsed_items.pop_back();
    Item left = std::move(parsed_items.back());
    parsed_items.pop_back();

    Var dst = std::get<Var>(left);

    std::variant<Var, Number, Label, ArithmeticOp, CompareOp> src = std::visit(
        [](auto&& item)
            -> std::variant<Var, Number, Label, ArithmeticOp, CompareOp> {
          using T = std::decay_t<decltype(item)>;
          if constexpr (std::is_same_v<T, Var> || std::is_same_v<T, Number> ||
                        std::is_same_v<T, Label>) {
            return item;
          } else {
            throw std::runtime_error(
                "action<inst_assign>: Invalid operand type");
          }
        },
        right);

    AssignInstruction instruction(dst, src);
    program.getFunctions().back().addInstruction(std::move(instruction));
  }
};

template <>
struct action<inst_assign_arithmetic> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    Item rightItem = std::move(parsed_items.back());
    parsed_items.pop_back();
    Item leftItem = std::move(parsed_items.back());
    parsed_items.pop_back();
    Item dstItem = std::move(parsed_items.back());
    parsed_items.pop_back();

    Operand left = itemToOperand(leftItem);
    Operand right = itemToOperand(rightItem);

    EnumArithmeticOp op = arithmetic_ops.back();
    arithmetic_ops.pop_back();

    Var dst = std::get<Var>(dstItem);
    ArithmeticOp src(left, right, op);

    AssignInstruction instruction(dst, src);
    program.getFunctions().back().addInstruction(std::move(instruction));
  }
};

template <>
struct action<inst_assign_comp> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    Item rightItem = std::move(parsed_items.back());
    parsed_items.pop_back();
    Item leftItem = std::move(parsed_items.back());
    parsed_items.pop_back();
    Item dstItem = std::move(parsed_items.back());
    parsed_items.pop_back();

    Operand left = itemToOperand(leftItem);
    Operand right = itemToOperand(rightItem);

    EnumCompareOp op = compare_ops.back();
    compare_ops.pop_back();

    Var dst = std::get<Var>(dstItem);
    CompareOp src(left, right, op);

    AssignInstruction instruction(dst, src);
    program.getFunctions().back().addInstruction(std::move(instruction));
  }
};

template <>
struct action<inst_load> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    Item rightItem = std::move(parsed_items.back());
    parsed_items.pop_back();
    Item leftItem = std::move(parsed_items.back());
    parsed_items.pop_back();

    Var dst = std::get<Var>(leftItem);
    Var src = std::get<Var>(rightItem);

    LoadInstruction instruction(dst, src);
    program.getFunctions().back().addInstruction(std::move(instruction));
  }
};

template <>
struct action<inst_store> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    std::variant<Var, Number, Label> rightItem = std::move(parsed_items.back());
    parsed_items.pop_back();
    Item leftItem = std::move(parsed_items.back());
    parsed_items.pop_back();

    Var dst = std::get<Var>(leftItem);

    StoreInstruction instruction(dst, rightItem);
    program.getFunctions().back().addInstruction(std::move(instruction));
  }
};

template <>
struct action<inst_return> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    ReturnInstruction instruction;
    program.getFunctions().back().addInstruction(std::move(instruction));
  }
};

template <>
struct action<inst_return_val> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    Item item = std::move(parsed_items.back());
    parsed_items.pop_back();
    Operand value = itemToOperand(item);
    ReturnValInstruction instruction(value);
    program.getFunctions().back().addInstruction(std::move(instruction));
  }
};

template <>
struct action<inst_label> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    Item item = std::move(parsed_items.back());
    parsed_items.pop_back();

    Label label = std::get<Label>(item);
    LabelInstruction instruction(label);
    program.getFunctions().back().addInstruction(std::move(instruction));
  }
};

template <>
struct action<inst_branch> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    Item item = std::move(parsed_items.back());
    parsed_items.pop_back();

    Label target = std::get<Label>(item);
    BranchInstruction instruction(target);
    program.getFunctions().back().addInstruction(std::move(instruction));
  }
};

template <>
struct action<inst_conditional_branch> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    Item targetItem = std::move(parsed_items.back());
    parsed_items.pop_back();
    Item conditionItem = std::move(parsed_items.back());
    parsed_items.pop_back();

    Operand condition = itemToOperand(conditionItem);
    Label target = std::get<Label>(targetItem);

    ConditionalBranchInstruction instruction(condition, target);
    program.getFunctions().back().addInstruction(std::move(instruction));
  }
};

int argCount = 0;

template <>
struct action<args_list> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    std::string str = input.string();
    argCount = 1 + std::count(str.begin(), str.end(), ',');
  }
};

int varCount = 0;

template <>
struct action<vars_list> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    std::string str = input.string();
    varCount = 1 + std::count(str.begin(), str.end(), ',');
  }
};

template <>
struct action<inst_call> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    std::vector<Operand> args;

    for (int i = 0; i < argCount; ++i) {
      Item item = parsed_items.back();
      parsed_items.pop_back();
      Operand arg = itemToOperand(item);
      args.push_back(arg);
    }

    std::reverse(args.begin(), args.end());

    CallInstruction instruction;

    if (!enum_callee.empty()) {
      EnumCallee callee = enum_callee.back();
      enum_callee.pop_back();
      instruction = CallInstruction(callee, args);
    } else {
      Item item = parsed_items.back();
      parsed_items.pop_back();
      if (std::holds_alternative<Var>(item)) {
        Var callee = std::get<Var>(item);
        instruction = CallInstruction(callee, args);
      } else if (std::holds_alternative<Label>(item)) {
        Label callee = std::get<Label>(item);
        instruction = CallInstruction(callee, args);
      } else {
        throw std::runtime_error("action<inst_call>: Not a valid callee");
      }
    }

    program.getFunctions().back().addInstruction(std::move(instruction));
    argCount = 0;
  }
};

template <>
struct action<inst_assign_call> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    std::vector<Operand> args;

    for (int i = 0; i < argCount; ++i) {
      Item item = parsed_items.back();
      parsed_items.pop_back();
      Operand arg = itemToOperand(item);
      args.push_back(arg);
    }

    std::reverse(args.begin(), args.end());

    Callee callee;

    if (!enum_callee.empty()) {
      callee = enum_callee.back();
      enum_callee.pop_back();
    } else {
      Item item = parsed_items.back();
      parsed_items.pop_back();
      if (std::holds_alternative<Var>(item)) {
        callee = std::get<Var>(item);
      } else if (std::holds_alternative<Label>(item)) {
        callee = std::get<Label>(item);
      } else {
        throw std::runtime_error(
            "action<inst_assign_call>: Not a valid callee");
      }
    }

    Item dstItem = parsed_items.back();
    parsed_items.pop_back();
    Var dst = std::get<Var>(dstItem);

    AssignCallInstruction instruction(dst, callee, args);
    program.getFunctions().back().addInstruction(std::move(instruction));
    argCount = 0;
  }
};

template <>
struct action<function_name> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    Function function(Label{input.string()});
    program.addFunction(std::move(function));
  }
};

template <>
struct action<function_vars> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    Function& curFunction = program.getFunctions().back();
    for (int i = 0; i < varCount; ++i) {
      Item item = parsed_items.back();
      parsed_items.pop_back();
      Var var = std::get<Var>(item);
      curFunction.addVar(var);
    }

    varCount = 0;
  }
};

/*
 * Parse file
 */
Program parse_file(char* fileName) {
  parsed_items.clear();
  arithmetic_ops.clear();
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

}  // namespace L3
