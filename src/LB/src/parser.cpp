#include "parser.h"

#include <assert.h>
#include <sched.h>
#include <stdint.h>

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

#include "LB.h"
#include "program.h"
#include "utils.h"

namespace pegtl = TAO_PEGTL_NAMESPACE;

using namespace pegtl;

namespace LB {

/*
 * Helper grammar rules (lexing / whitespace / comments)
 */
struct comment
    : pegtl::disable<TAO_PEGTL_STRING("//"), pegtl::until<pegtl::eolf>> {};
struct ws_unit : pegtl::sor<pegtl::ascii::space, comment> {};
struct ws : pegtl::star<ws_unit> {};
struct spaces : pegtl::plus<pegtl::ascii::space> {};
struct comma_sep : pegtl::seq<ws, pegtl::one<','>, ws> {};

/*
 * Identifiers / labels
 */
struct name_first : pegtl::sor<pegtl::alpha, pegtl::one<'_'>> {};
struct name_rest : pegtl::sor<pegtl::alpha, pegtl::digit, pegtl::one<'_'>> {};
struct identifier : pegtl::seq<name_first, pegtl::star<name_rest>> {};
struct value_name : identifier {};
struct decl_name : identifier {};
struct param_name : identifier {};
struct call_name : identifier {};
struct function_name_tok : identifier {};
struct label : pegtl::seq<pegtl::one<':'>, identifier> {};

/*
 * Immediate
 */
struct N : pegtl::seq<pegtl::opt<pegtl::sor<pegtl::one<'-'>, pegtl::one<'+'>>>,
                      pegtl::plus<pegtl::digit>> {};

/*
 * Terminals
 */
struct str_void : TAO_PEGTL_STRING("void") {};
struct str_int64 : TAO_PEGTL_STRING("int64") {};
struct str_tuple : TAO_PEGTL_STRING("tuple") {};
struct str_code : TAO_PEGTL_STRING("code") {};
struct str_brackets : TAO_PEGTL_STRING("[]") {};
struct str_if : TAO_PEGTL_STRING("if") {};
struct str_while : TAO_PEGTL_STRING("while") {};
struct str_goto : TAO_PEGTL_STRING("goto") {};
struct str_return : TAO_PEGTL_STRING("return") {};
struct str_continue : TAO_PEGTL_STRING("continue") {};
struct str_break : TAO_PEGTL_STRING("break") {};
struct str_new : TAO_PEGTL_STRING("new") {};
struct str_Array : TAO_PEGTL_STRING("Array") {};
struct str_Tuple : TAO_PEGTL_STRING("Tuple") {};
struct str_length : TAO_PEGTL_STRING("length") {};
struct op_arrow : TAO_PEGTL_STRING("\x3c\x2D") {};

/*
 * Operators
 */
struct op_add : TAO_PEGTL_STRING("+") {};
struct op_sub : TAO_PEGTL_STRING("-") {};
struct op_mul : TAO_PEGTL_STRING("*") {};
struct op_and : TAO_PEGTL_STRING("&") {};
struct op_lshift : TAO_PEGTL_STRING("<<") {};
struct op_rshift : TAO_PEGTL_STRING(">>") {};
struct op_less_equal : TAO_PEGTL_STRING("<=") {};
struct op_greater_equal : TAO_PEGTL_STRING(">=") {};
struct op_less : TAO_PEGTL_STRING("<") {};
struct op_equal : TAO_PEGTL_STRING("=") {};
struct op_greater : TAO_PEGTL_STRING(">") {};
struct cmp : pegtl::sor<op_less_equal, op_greater_equal, op_equal, op_less,
                        op_greater> {};
struct op : pegtl::sor<op_add, op_sub, op_mul, op_and, op_lshift, op_rshift,
                       cmp> {};

/*
 * Nonterminals
 */
struct type : pegtl::sor<pegtl::seq<str_int64, pegtl::star<str_brackets>>,
                         str_tuple, str_code> {};
struct T : pegtl::sor<type, str_void> {};
struct t : pegtl::sor<value_name, N> {};
struct cond : pegtl::seq<t, ws, cmp, ws, t> {};
struct oper : pegtl::seq<t, ws, op, ws, t> {};
struct names : pegtl::list_must<decl_name, comma_sep> {};
struct arg : t {};
struct args_list : pegtl::list<arg, comma_sep> {};
struct args : pegtl::opt<args_list> {};
struct params_entry : pegtl::seq<type, spaces, param_name> {};
struct params_list : pegtl::list<params_entry, comma_sep> {};
struct params : pegtl::opt<params_list> {};
struct index_item : t {};
struct name_index
    : pegtl::seq<value_name, pegtl::plus<ws, pegtl::one<'['>, ws, index_item,
                                         ws, pegtl::one<']'>>> {};
struct length_index : t {};
struct length : pegtl::seq<str_length, spaces, value_name,
                           pegtl::opt<spaces, length_index>> {};
struct function_call : pegtl::seq<call_name, ws, pegtl::one<'('>, ws, args, ws,
                                  pegtl::one<')'>> {};
struct new_array
    : pegtl::seq<str_new, spaces, str_Array, ws, pegtl::one<'('>, ws, args, ws,
                 pegtl::one<')'>> {};
struct new_tuple
    : pegtl::seq<str_new, spaces, str_Tuple, ws, pegtl::one<'('>, ws, t, ws,
                 pegtl::one<')'>> {};

/*
 * Instructions and scopes
 */
struct scope_open : pegtl::one<'{'> {};
struct scope_close : pegtl::one<'}'> {};
struct inst;
struct scope : pegtl::seq<scope_open, ws, pegtl::star<inst, ws>, scope_close> {
};
struct inst_name_def : pegtl::seq<type, spaces, names> {};
struct inst_op_assign : pegtl::seq<value_name, ws, op_arrow, ws, oper> {};
struct inst_name_num_assign : pegtl::seq<value_name, ws, op_arrow, ws, t> {};
struct inst_label : pegtl::seq<label> {};
struct inst_if
    : pegtl::seq<str_if, ws, pegtl::one<'('>, ws, cond, ws, pegtl::one<')'>,
                 ws, label, ws, label> {};
struct inst_goto : pegtl::seq<str_goto, spaces, label> {};
struct inst_return_val : pegtl::seq<str_return, spaces, t> {};
struct inst_return : pegtl::seq<str_return> {};
struct inst_while
    : pegtl::seq<str_while, ws, pegtl::one<'('>, ws, cond, ws,
                 pegtl::one<')'>, ws, label, ws, label> {};
struct inst_continue : pegtl::seq<str_continue> {};
struct inst_break : pegtl::seq<str_break> {};
struct inst_index_load : pegtl::seq<value_name, ws, op_arrow, ws, name_index> {
};
struct inst_index_store : pegtl::seq<name_index, ws, op_arrow, ws, t> {};
struct inst_length : pegtl::seq<value_name, ws, op_arrow, ws, length> {};
struct inst_call : pegtl::seq<function_call> {};
struct inst_call_assign
    : pegtl::seq<value_name, ws, op_arrow, ws, function_call> {};
struct inst_new_array_assign
    : pegtl::seq<value_name, ws, op_arrow, ws, new_array> {};
struct inst_new_tuple_assign
    : pegtl::seq<value_name, ws, op_arrow, ws, new_tuple> {};
struct inst_scope : scope {};
struct inst
    : pegtl::sor<inst_name_def, inst_new_array_assign, inst_new_tuple_assign,
                 inst_call_assign, inst_length, inst_index_load,
                 inst_index_store, inst_op_assign, inst_name_num_assign,
                 inst_if, inst_goto, inst_return_val, inst_return, inst_while,
                 inst_continue, inst_break, inst_call, inst_label, inst_scope> {
};

/*
 * Functions and program
 */
struct function_params : params {};
struct function_scope : scope {};
struct function
    : pegtl::seq<T, ws, function_name_tok, ws, pegtl::one<'('>, ws,
                 function_params, ws, pegtl::one<')'>, ws, function_scope> {};
struct function_list : pegtl::plus<ws, function, ws> {};
struct program : function_list {};
struct grammar : pegtl::must<program, pegtl::eof> {};

/*
 * Actions
 */
std::vector<std::unique_ptr<ASTNode>> parsed_items;
std::vector<OP> enum_op;
std::vector<std::vector<std::unique_ptr<ASTNode>>> scope_instruction_stack;
int indexCounter = 0;
int argCounter = 0;
int paramCounter = 0;
int declCounter = 0;
bool existsLengthIndex = false;

template <typename Rule>
struct action : pegtl::nothing<Rule> {};

static std::string extract_name(ASTNode* node) {
  if (auto* n = dynamic_cast<Name*>(node)) {
    return n->name;
  }
  return "";
}

static std::string extract_type(ASTNode* node) {
  if (auto* t = dynamic_cast<Type*>(node)) {
    return t->name;
  }
  return "";
}

static void add_instruction(std::unique_ptr<Instruction> inst) {
  if (!scope_instruction_stack.empty()) {
    scope_instruction_stack.back().push_back(std::move(inst));
  }
}

template <>
struct action<value_name> {
  template <typename Input>
  static void apply(const Input& input, Program&) {
    parsed_items.push_back(std::make_unique<Name>(input.string()));
  }
};

template <>
struct action<decl_name> {
  template <typename Input>
  static void apply(const Input& input, Program&) {
    parsed_items.push_back(std::make_unique<Name>(input.string()));
    ++declCounter;
  }
};

template <>
struct action<param_name> {
  template <typename Input>
  static void apply(const Input& input, Program&) {
    parsed_items.push_back(std::make_unique<Name>(input.string()));
  }
};

template <>
struct action<call_name> {
  template <typename Input>
  static void apply(const Input& input, Program&) {
    parsed_items.push_back(std::make_unique<Name>(input.string()));
  }
};

template <>
struct action<label> {
  template <typename Input>
  static void apply(const Input& input, Program&) {
    parsed_items.push_back(std::make_unique<Label>(input.string()));
  }
};

template <>
struct action<type> {
  template <typename Input>
  static void apply(const Input& input, Program&) {
    parsed_items.push_back(std::make_unique<Type>(input.string()));
  }
};

template <>
struct action<str_void> {
  template <typename Input>
  static void apply(const Input& input, Program&) {
    parsed_items.push_back(std::make_unique<Type>(input.string()));
  }
};

template <>
struct action<N> {
  template <typename Input>
  static void apply(const Input& input, Program&) {
    parsed_items.push_back(std::make_unique<Number>(std::stoll(input.string())));
  }
};

template <>
struct action<op_add> {
  template <typename Input>
  static void apply(const Input&, Program&) {
    enum_op.push_back(OP::PLUS);
  }
};

template <>
struct action<op_sub> {
  template <typename Input>
  static void apply(const Input&, Program&) {
    enum_op.push_back(OP::SUB);
  }
};

template <>
struct action<op_mul> {
  template <typename Input>
  static void apply(const Input&, Program&) {
    enum_op.push_back(OP::MUL);
  }
};

template <>
struct action<op_and> {
  template <typename Input>
  static void apply(const Input&, Program&) {
    enum_op.push_back(OP::AND);
  }
};

template <>
struct action<op_lshift> {
  template <typename Input>
  static void apply(const Input&, Program&) {
    enum_op.push_back(OP::LSHIFT);
  }
};

template <>
struct action<op_rshift> {
  template <typename Input>
  static void apply(const Input&, Program&) {
    enum_op.push_back(OP::RSHIFT);
  }
};

template <>
struct action<op_less> {
  template <typename Input>
  static void apply(const Input&, Program&) {
    enum_op.push_back(OP::LT);
  }
};

template <>
struct action<op_less_equal> {
  template <typename Input>
  static void apply(const Input&, Program&) {
    enum_op.push_back(OP::LE);
  }
};

template <>
struct action<op_equal> {
  template <typename Input>
  static void apply(const Input&, Program&) {
    enum_op.push_back(OP::EQ);
  }
};

template <>
struct action<op_greater_equal> {
  template <typename Input>
  static void apply(const Input&, Program&) {
    enum_op.push_back(OP::GE);
  }
};

template <>
struct action<op_greater> {
  template <typename Input>
  static void apply(const Input&, Program&) {
    enum_op.push_back(OP::GT);
  }
};

template <>
struct action<oper> {
  template <typename Input>
  static void apply(const Input&, Program&) {
    std::unique_ptr<ASTNode> rightItem = std::move(parsed_items.back());
    parsed_items.pop_back();
    OP opCode = enum_op.back();
    enum_op.pop_back();
    std::unique_ptr<ASTNode> leftItem = std::move(parsed_items.back());
    parsed_items.pop_back();
    parsed_items.push_back(std::make_unique<Operator>(
        std::move(leftItem), std::move(rightItem), opCode));
  }
};

template <>
struct action<cond> {
  template <typename Input>
  static void apply(const Input&, Program&) {
    std::unique_ptr<ASTNode> rightItem = std::move(parsed_items.back());
    parsed_items.pop_back();
    OP opCode = enum_op.back();
    enum_op.pop_back();
    std::unique_ptr<ASTNode> leftItem = std::move(parsed_items.back());
    parsed_items.pop_back();
    parsed_items.push_back(std::make_unique<CondOperator>(
        std::move(leftItem), std::move(rightItem), opCode));
  }
};

template <>
struct action<index_item> {
  template <typename Input>
  static void apply(const Input&, Program&) {
    ++indexCounter;
  }
};

template <>
struct action<arg> {
  template <typename Input>
  static void apply(const Input&, Program&) {
    ++argCounter;
  }
};

template <>
struct action<length_index> {
  template <typename Input>
  static void apply(const Input&, Program&) {
    existsLengthIndex = true;
  }
};

template <>
struct action<name_index> {
  template <typename Input>
  static void apply(const Input& input, Program&) {
    int64_t lineNumber = input.position().line;
    std::vector<std::unique_ptr<ASTNode>> indices;
    for (int i = 0; i < indexCounter; ++i) {
      indices.push_back(std::move(parsed_items.back()));
      parsed_items.pop_back();
    }
    std::reverse(indices.begin(), indices.end());
    indexCounter = 0;
    std::unique_ptr<ASTNode> nameItem = std::move(parsed_items.back());
    parsed_items.pop_back();
    parsed_items.push_back(
        std::make_unique<Index>(lineNumber, std::move(nameItem), std::move(indices)));
  }
};

template <>
struct action<function_call> {
  template <typename Input>
  static void apply(const Input&, Program&) {
    std::vector<std::unique_ptr<ASTNode>> args;
    for (int i = 0; i < argCounter; ++i) {
      args.push_back(std::move(parsed_items.back()));
      parsed_items.pop_back();
    }
    std::reverse(args.begin(), args.end());
    argCounter = 0;
    std::unique_ptr<ASTNode> callee = std::move(parsed_items.back());
    parsed_items.pop_back();
    parsed_items.push_back(
        std::make_unique<FunctionCall>(std::move(callee), std::move(args)));
  }
};

template <>
struct action<new_array> {
  template <typename Input>
  static void apply(const Input&, Program&) {
    std::vector<std::unique_ptr<ASTNode>> args;
    for (int i = 0; i < argCounter; ++i) {
      args.push_back(std::move(parsed_items.back()));
      parsed_items.pop_back();
    }
    std::reverse(args.begin(), args.end());
    argCounter = 0;
    parsed_items.push_back(std::make_unique<NewArray>(std::move(args)));
  }
};

template <>
struct action<new_tuple> {
  template <typename Input>
  static void apply(const Input&, Program&) {
    std::unique_ptr<ASTNode> lengthItem = std::move(parsed_items.back());
    parsed_items.pop_back();
    parsed_items.push_back(std::make_unique<NewTuple>(std::move(lengthItem)));
  }
};

template <>
struct action<length> {
  template <typename Input>
  static void apply(const Input& input, Program&) {
    int64_t lineNumber = input.position().line;
    std::unique_ptr<ASTNode> indexItem;
    if (existsLengthIndex) {
      indexItem = std::move(parsed_items.back());
      parsed_items.pop_back();
    }
    std::unique_ptr<ASTNode> nameItem = std::move(parsed_items.back());
    parsed_items.pop_back();
    if (existsLengthIndex) {
      parsed_items.push_back(std::make_unique<Length>(
          lineNumber, std::move(nameItem), std::move(indexItem)));
      existsLengthIndex = false;
    } else {
      parsed_items.push_back(std::make_unique<Length>(lineNumber, std::move(nameItem)));
    }
  }
};

template <>
struct action<params_entry> {
  template <typename Input>
  static void apply(const Input&, Program&) {
    ++paramCounter;
    std::unique_ptr<ASTNode> nameItem = std::move(parsed_items.back());
    parsed_items.pop_back();
    std::unique_ptr<ASTNode> typeItem = std::move(parsed_items.back());
    parsed_items.pop_back();
    parsed_items.push_back(
        std::make_unique<Parameter>(std::move(typeItem), std::move(nameItem)));
  }
};

template <>
struct action<inst_name_def> {
  template <typename Input>
  static void apply(const Input&, Program&) {
    std::vector<std::string> declaredNames;
    for (int i = 0; i < declCounter; ++i) {
      declaredNames.push_back(extract_name(parsed_items.back().get()));
      parsed_items.pop_back();
    }
    std::reverse(declaredNames.begin(), declaredNames.end());
    std::string typeName = extract_type(parsed_items.back().get());
    parsed_items.pop_back();
    for (const auto& name : declaredNames) {
      add_instruction(std::make_unique<NameDefInstruction>(
          std::make_unique<Type>(typeName), std::make_unique<Name>(name)));
    }
    declCounter = 0;
  }
};

template <>
struct action<inst_name_num_assign> {
  template <typename Input>
  static void apply(const Input&, Program&) {
    std::unique_ptr<ASTNode> srcItem = std::move(parsed_items.back());
    parsed_items.pop_back();
    std::unique_ptr<ASTNode> dstItem = std::move(parsed_items.back());
    parsed_items.pop_back();
    add_instruction(std::make_unique<NameNumAssignInstruction>(
        std::move(dstItem), std::move(srcItem)));
  }
};

template <>
struct action<inst_op_assign> {
  template <typename Input>
  static void apply(const Input&, Program&) {
    std::unique_ptr<ASTNode> srcItem = std::move(parsed_items.back());
    parsed_items.pop_back();
    std::unique_ptr<ASTNode> dstItem = std::move(parsed_items.back());
    parsed_items.pop_back();
    add_instruction(std::make_unique<OperatorAssignInstruction>(
        std::move(dstItem), std::move(srcItem)));
  }
};

template <>
struct action<inst_label> {
  template <typename Input>
  static void apply(const Input&, Program&) {
    std::unique_ptr<ASTNode> labelItem = std::move(parsed_items.back());
    parsed_items.pop_back();
    add_instruction(std::make_unique<LabelInstruction>(std::move(labelItem)));
  }
};

template <>
struct action<inst_if> {
  template <typename Input>
  static void apply(const Input&, Program&) {
    std::unique_ptr<ASTNode> dst2Item = std::move(parsed_items.back());
    parsed_items.pop_back();
    std::unique_ptr<ASTNode> dst1Item = std::move(parsed_items.back());
    parsed_items.pop_back();
    std::unique_ptr<ASTNode> conditionItem = std::move(parsed_items.back());
    parsed_items.pop_back();
    add_instruction(std::make_unique<ConditionalBranchInstruction>(
        std::move(conditionItem), std::move(dst1Item), std::move(dst2Item)));
  }
};

template <>
struct action<inst_goto> {
  template <typename Input>
  static void apply(const Input&, Program&) {
    std::unique_ptr<ASTNode> targetItem = std::move(parsed_items.back());
    parsed_items.pop_back();
    add_instruction(std::make_unique<BranchInstruction>(std::move(targetItem)));
  }
};

template <>
struct action<inst_return> {
  template <typename Input>
  static void apply(const Input&, Program&) {
    add_instruction(std::make_unique<ReturnInstruction>());
  }
};

template <>
struct action<inst_return_val> {
  template <typename Input>
  static void apply(const Input&, Program&) {
    std::unique_ptr<ASTNode> returnValueItem = std::move(parsed_items.back());
    parsed_items.pop_back();
    add_instruction(
        std::make_unique<ReturnValueInstruction>(std::move(returnValueItem)));
  }
};

template <>
struct action<inst_while> {
  template <typename Input>
  static void apply(const Input&, Program&) {
    std::unique_ptr<ASTNode> dst2Item = std::move(parsed_items.back());
    parsed_items.pop_back();
    std::unique_ptr<ASTNode> dst1Item = std::move(parsed_items.back());
    parsed_items.pop_back();
    std::unique_ptr<ASTNode> conditionItem = std::move(parsed_items.back());
    parsed_items.pop_back();
    add_instruction(std::make_unique<WhileInstruction>(
        std::move(conditionItem), std::move(dst1Item), std::move(dst2Item)));
  }
};

template <>
struct action<inst_continue> {
  template <typename Input>
  static void apply(const Input&, Program&) {
    add_instruction(std::make_unique<ContinueInstruction>());
  }
};

template <>
struct action<inst_break> {
  template <typename Input>
  static void apply(const Input&, Program&) {
    add_instruction(std::make_unique<BreakInstruction>());
  }
};

template <>
struct action<inst_index_load> {
  template <typename Input>
  static void apply(const Input&, Program&) {
    std::unique_ptr<ASTNode> srcItem = std::move(parsed_items.back());
    parsed_items.pop_back();
    std::unique_ptr<ASTNode> dstItem = std::move(parsed_items.back());
    parsed_items.pop_back();
    add_instruction(std::make_unique<IndexAssignInstruction>(
        std::move(dstItem), std::move(srcItem)));
  }
};

template <>
struct action<inst_index_store> {
  template <typename Input>
  static void apply(const Input&, Program&) {
    std::unique_ptr<ASTNode> srcItem = std::move(parsed_items.back());
    parsed_items.pop_back();
    std::unique_ptr<ASTNode> dstItem = std::move(parsed_items.back());
    parsed_items.pop_back();
    add_instruction(std::make_unique<IndexAssignInstruction>(
        std::move(dstItem), std::move(srcItem)));
  }
};

template <>
struct action<inst_length> {
  template <typename Input>
  static void apply(const Input&, Program&) {
    std::unique_ptr<ASTNode> srcItem = std::move(parsed_items.back());
    parsed_items.pop_back();
    std::unique_ptr<ASTNode> dstItem = std::move(parsed_items.back());
    parsed_items.pop_back();
    add_instruction(std::make_unique<LengthAssignInstruction>(
        std::move(dstItem), std::move(srcItem)));
  }
};

template <>
struct action<inst_call> {
  template <typename Input>
  static void apply(const Input&, Program&) {
    std::unique_ptr<ASTNode> functionCall = std::move(parsed_items.back());
    parsed_items.pop_back();
    add_instruction(std::make_unique<CallInstruction>(std::move(functionCall)));
  }
};

template <>
struct action<inst_call_assign> {
  template <typename Input>
  static void apply(const Input&, Program&) {
    std::unique_ptr<ASTNode> functionCall = std::move(parsed_items.back());
    parsed_items.pop_back();
    std::unique_ptr<ASTNode> dstItem = std::move(parsed_items.back());
    parsed_items.pop_back();
    add_instruction(std::make_unique<CallAssignInstruction>(
        std::move(dstItem), std::move(functionCall)));
  }
};

template <>
struct action<inst_new_array_assign> {
  template <typename Input>
  static void apply(const Input&, Program&) {
    std::unique_ptr<ASTNode> newArrayItem = std::move(parsed_items.back());
    parsed_items.pop_back();
    std::unique_ptr<ASTNode> dstItem = std::move(parsed_items.back());
    parsed_items.pop_back();
    add_instruction(std::make_unique<NewArrayAssignInstruction>(
        std::move(dstItem), std::move(newArrayItem)));
  }
};

template <>
struct action<inst_new_tuple_assign> {
  template <typename Input>
  static void apply(const Input&, Program&) {
    std::unique_ptr<ASTNode> newTupleItem = std::move(parsed_items.back());
    parsed_items.pop_back();
    std::unique_ptr<ASTNode> dstItem = std::move(parsed_items.back());
    parsed_items.pop_back();
    add_instruction(std::make_unique<NewTupleAssignInstruction>(
        std::move(dstItem), std::move(newTupleItem)));
  }
};

template <>
struct action<function_name_tok> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    std::unique_ptr<ASTNode> typeItem = std::move(parsed_items.back());
    parsed_items.pop_back();
    std::unique_ptr<ASTNode> functionNameItem =
        std::make_unique<Name>(input.string());
    program.addFunction(
        std::make_unique<Function>(std::move(typeItem), std::move(functionNameItem)));
  }
};

template <>
struct action<function_params> {
  template <typename Input>
  static void apply(const Input&, Program& program) {
    Function* curFunction = program.getFunctions().back().get();
    std::vector<std::unique_ptr<ASTNode>> params;
    for (int i = 0; i < paramCounter; ++i) {
      params.push_back(std::move(parsed_items.back()));
      parsed_items.pop_back();
    }
    std::reverse(params.begin(), params.end());
    for (int i = 0; i < paramCounter; ++i) {
      curFunction->addParameter(std::move(params[i]));
    }
    paramCounter = 0;
  }
};

template <>
struct action<scope_open> {
  template <typename Input>
  static void apply(const Input&, Program&) {
    scope_instruction_stack.emplace_back();
  }
};

template <>
struct action<scope_close> {
  template <typename Input>
  static void apply(const Input&, Program&) {
    std::vector<std::unique_ptr<ASTNode>> cur =
        std::move(scope_instruction_stack.back());
    scope_instruction_stack.pop_back();

    auto newScope = std::make_unique<Scope>();
    for (auto& node : cur) {
      newScope->addNode(std::move(node));
    }

    if (scope_instruction_stack.empty()) {
      parsed_items.push_back(std::move(newScope));
      return;
    }

    scope_instruction_stack.back().push_back(std::move(newScope));
  }
};

template <>
struct action<function_scope> {
  template <typename Input>
  static void apply(const Input&, Program& program) {
    Function* curFunction = program.getFunctions().back().get();
    std::unique_ptr<ASTNode> functionScope = std::move(parsed_items.back());
    parsed_items.pop_back();
    curFunction->setScope(std::move(functionScope));
  }
};

/*
 * Parse file
 */
Program parse_file(char* fileName) {
  parsed_items.clear();
  enum_op.clear();
  scope_instruction_stack.clear();
  indexCounter = 0;
  argCounter = 0;
  paramCounter = 0;
  declCounter = 0;
  existsLengthIndex = false;

  if (pegtl::analyze<grammar>() != 0) {
    std::cerr << "There are problems with the grammar" << std::endl;
    exit(1);
  }

  file_input<> fileInput(fileName);
  Program program;
  parse<grammar, action>(fileInput, program);
  return program;
}

}  // namespace LB
