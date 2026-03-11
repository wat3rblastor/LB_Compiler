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

#include "LA.h"
#include "program.h"
#include "utils.h"

namespace pegtl = TAO_PEGTL_NAMESPACE;

using namespace pegtl;

namespace LA {

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
struct var_name : name {};
struct label_name : name {};
struct label : pegtl::seq<pegtl::one<':'>, label_name> {};

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
 * Type strings
 */
struct str_int64 : TAO_PEGTL_STRING("int64") {};
struct str_tuple : TAO_PEGTL_STRING("tuple") {};
struct str_code : TAO_PEGTL_STRING("code") {};

/*
 * Nonterminals
 */
struct t : pegtl::sor<var_name, N> {};
struct T : pegtl::sor<type, str_void> {};
struct op : pegtl::sor<op_add, op_sub, op_mul, op_and, op_lshift, op_rshift,
                       op_less_equal, op_greater_equal, op_equal, op_less,
                       op_greater> {};
struct index : t {};
struct length_index : t {};

/*
 * Terminals
 */
struct str_brackets : TAO_PEGTL_STRING("[]") {};
struct str_void : TAO_PEGTL_STRING("void") {};
struct str_length : TAO_PEGTL_STRING("length") {};
struct str_return : TAO_PEGTL_STRING("return") {};
struct str_br : TAO_PEGTL_STRING("br") {};
struct str_define : TAO_PEGTL_STRING("define") {};
struct str_new : TAO_PEGTL_STRING("new") {};
struct str_Array : TAO_PEGTL_STRING("Array") {};
struct str_Tuple : TAO_PEGTL_STRING("Tuple") {};

/*
 * Instruction Dependencies
 */
struct oper : pegtl::seq<t, spaces, op, spaces, t> {};
struct name_index : pegtl::seq<var_name, spaces,
                               pegtl::plus<pegtl::one<'['>, spaces, index,
                                           spaces, pegtl::one<']'>>> {};
struct function_call : pegtl::seq<var_name, spaces, pegtl::one<'('>, spaces, args,
                                  spaces, pegtl::one<')'>> {};
struct new_array
    : pegtl::seq<str_new, spaces, str_Array, spaces, pegtl::one<'('>, spaces,
                 args, spaces, pegtl::one<')'>> {};
struct new_tuple
    : pegtl::seq<str_new, spaces, str_Tuple, spaces, pegtl::one<'('>, spaces, t,
                 spaces, pegtl::one<')'>> {};
struct type : pegtl::sor<pegtl::seq<str_int64, pegtl::star<str_brackets>>,
                         str_tuple, str_code> {};
struct length : pegtl::seq<str_length, spaces, var_name, spaces, pegtl::opt<length_index>> {};
struct comma_sep : pegtl::seq<spaces, pegtl::one<','>, spaces> {};
struct arg : t {};
struct args_list : pegtl::list<arg, comma_sep> {};
struct args : pegtl::opt<args_list> {};
struct param : pegtl::seq<type, spaces, var_name> {};
struct param_list : pegtl::list<param, comma_sep> {};
struct params : pegtl::opt<param_list> {};

/*
 * Instructions
 */
struct inst_name_def : pegtl::seq<type, spaces, var_name> {};
struct inst_name_num_assign : pegtl::seq<var_name, spaces, op_arrow, spaces, t> {};
struct inst_op_assign : pegtl::seq<var_name, spaces, op_arrow, spaces, oper> {};
struct inst_label : pegtl::seq<label> {};
struct inst_branch : pegtl::seq<str_br, spaces, label> {};
struct inst_cond_branch
    : pegtl::seq<str_br, spaces, t, spaces, label, spaces, label> {};
struct inst_return : pegtl::seq<str_return> {};
struct inst_return_val : pegtl::seq<str_return, spaces, t> {};
struct inst_index_load
    : pegtl::seq<var_name, spaces, op_arrow, spaces, name_index> {};
struct inst_index_store : pegtl::seq<name_index, spaces, op_arrow, spaces, t> {
};
struct inst_length : pegtl::seq<var_name, spaces, op_arrow, spaces, length> {};
struct inst_call : pegtl::seq<function_call> {};
struct inst_call_assign
    : pegtl::seq<var_name, spaces, op_arrow, spaces, function_call> {};
struct inst_new_array_assign
    : pegtl::seq<var_name, spaces, op_arrow, spaces, new_array> {};
struct inst_new_tuple_assign
    : pegtl::seq<var_name, spaces, op_arrow, spaces, new_tuple> {};

/*
 * Semantic terminals
 */
struct function_name : var_name {};
struct function_params : params {};

/*
 * High-level grammar rules
 */
struct inst
    : pegtl::sor<inst_name_def, inst_op_assign, inst_index_load,
                 inst_index_store, inst_length, inst_call,
                 inst_call_assign, inst_new_array_assign, inst_new_tuple_assign, inst_name_num_assign,
                 inst_cond_branch, inst_branch, inst_return_val, inst_return, inst_label> {
};
struct inst_list : pegtl::star<pegtl::seq<seps_with_comments, spaces, inst,
                                          spaces, seps_with_comments>> {};
struct function
    : pegtl::seq<T, spaces, function_name, spaces,
                 pegtl::one<'('>, spaces, function_params, spaces,
                 pegtl::one<')'>, spaces, seps_with_comments, pegtl::one<'{'>,
                 inst_list, seps_with_comments, spaces, pegtl::one<'}'>> {};

struct function_list : pegtl::plus<seps_with_comments, spaces, function, spaces,
                                   seps_with_comments> {};
struct program : function_list {};
struct grammar : pegtl::must<program> {};

/*
 * Actions
 */
std::vector<std::unique_ptr<ASTNode>> parsed_items;
std::vector<OP> enum_op;
int indexCounter = 0;
int argCounter = 0;
int paramCounter = 0;
bool existsLengthIndex = false;

template <typename Rule>
struct action : pegtl::nothing<Rule> {};

template <>
struct action<label> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    parsed_items.push_back(std::make_unique<Label>(input.string()));
  }
};

template <>
struct action<var_name> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    parsed_items.push_back(std::make_unique<Name>(input.string()));
  }
};

template <>
struct action<type> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    parsed_items.push_back(std::make_unique<Type>(input.string()));
  }
};

template <>
struct action<str_void> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    parsed_items.push_back(std::make_unique<Type>(input.string()));
  }
};

template <>
struct action<N> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    int64_t value = std::stoll(input.string());
    parsed_items.push_back(std::make_unique<Number>(value));
  }
};

template <>
struct action<op_add> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    enum_op.push_back(OP::PLUS);
  }
};

template <>
struct action<op_sub> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    enum_op.push_back(OP::SUB);
  }
};

template <>
struct action<op_mul> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    enum_op.push_back(OP::MUL);
  }
};

template <>
struct action<op_and> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    enum_op.push_back(OP::AND);
  }
};

template <>
struct action<op_lshift> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    enum_op.push_back(OP::LSHIFT);
  }
};

template <>
struct action<op_rshift> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    enum_op.push_back(OP::RSHIFT);
  }
};

template <>
struct action<op_less> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    enum_op.push_back(OP::LT);
  }
};

template <>
struct action<op_less_equal> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    enum_op.push_back(OP::LE);
  }
};

template <>
struct action<op_equal> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    enum_op.push_back(OP::EQ);
  }
};

template <>
struct action<op_greater_equal> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    enum_op.push_back(OP::GE);
  }
};

template <>
struct action<op_greater> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    enum_op.push_back(OP::GT);
  }
};

template <>
struct action<oper> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    std::unique_ptr<ASTNode> rightItem = std::move(parsed_items.back());
    parsed_items.pop_back();
    OP op = enum_op.back();
    enum_op.pop_back();
    std::unique_ptr<ASTNode> leftItem = std::move(parsed_items.back());
    parsed_items.pop_back();
    parsed_items.push_back(std::make_unique<Operator>(
        std::move(leftItem), std::move(rightItem), op));
  }
};

template <>
struct action<index> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    ++indexCounter;
  }
};

template <>
struct action<length_index> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    existsLengthIndex = true;
  }
};

template <>
struct action<name_index> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
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
struct action<arg> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    ++argCounter;
  }
};


template <>
struct action<function_call> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
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
  static void apply(const Input& input, Program& program) {
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
  static void apply(const Input& input, Program& program) {
    std::unique_ptr<ASTNode> lengthItem = std::move(parsed_items.back());
    parsed_items.pop_back();
    parsed_items.push_back(std::make_unique<NewTuple>(std::move(lengthItem)));
  }
};

template <>
struct action<length> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
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
struct action<param> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
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
  static void apply(const Input& input, Program& program) {
    Function* curFunction = program.getFunctions().back().get();
    std::unique_ptr<ASTNode> nameItem = std::move(parsed_items.back());
    parsed_items.pop_back();
    std::unique_ptr<ASTNode> typeItem = std::move(parsed_items.back());
    parsed_items.pop_back();
    curFunction->addInstruction(std::make_unique<NameDefInstruction>(
        std::move(typeItem), std::move(nameItem)));
  }
};

template <>
struct action<inst_name_num_assign> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    Function* curFunction = program.getFunctions().back().get();
    std::unique_ptr<ASTNode> srcItem = std::move(parsed_items.back());
    parsed_items.pop_back();
    std::unique_ptr<ASTNode> dstItem = std::move(parsed_items.back());
    parsed_items.pop_back();
    curFunction->addInstruction(std::make_unique<NameNumAssignInstruction>(
        std::move(dstItem), std::move(srcItem)));
  }
};

template <>
struct action<inst_op_assign> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    Function* curFunction = program.getFunctions().back().get();
    std::unique_ptr<ASTNode> srcItem = std::move(parsed_items.back());
    parsed_items.pop_back();
    std::unique_ptr<ASTNode> dstItem = std::move(parsed_items.back());
    parsed_items.pop_back();
    curFunction->addInstruction(std::make_unique<OperatorAssignInstruction>(
        std::move(dstItem), std::move(srcItem)));
  }
};

template <> 
struct action<inst_label> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    Function* curFunction = program.getFunctions().back().get();
    std::unique_ptr<ASTNode> labelItem = std::move(parsed_items.back());
    parsed_items.pop_back();
    curFunction->addInstruction(std::make_unique<LabelInstruction>(std::move(labelItem)));
  }
};

template <>
struct action<inst_branch> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    Function* curFunction = program.getFunctions().back().get();
    std::unique_ptr<ASTNode> targetItem = std::move(parsed_items.back());
    parsed_items.pop_back();
    curFunction->addInstruction(std::make_unique<BranchInstruction>(std::move(targetItem)));
  }
};

template <>
struct action<inst_cond_branch> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    Function* curFunction = program.getFunctions().back().get();
    std::unique_ptr<ASTNode> dst2Item = std::move(parsed_items.back());
    parsed_items.pop_back();
    std::unique_ptr<ASTNode> dst1Item = std::move(parsed_items.back());
    parsed_items.pop_back();
    std::unique_ptr<ASTNode> conditionItem = std::move(parsed_items.back());
    parsed_items.pop_back();
    curFunction->addInstruction(std::make_unique<ConditionalBranchInstruction>(
        std::move(conditionItem), std::move(dst1Item), std::move(dst2Item)));
  }
};

template <>
struct action<inst_return> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    Function* curFunction = program.getFunctions().back().get();
    curFunction->addInstruction(std::make_unique<ReturnInstruction>());
  }
};

template <>
struct action<inst_return_val> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    Function* curFunction = program.getFunctions().back().get();
    std::unique_ptr<ASTNode> returnValueItem = std::move(parsed_items.back());
    parsed_items.pop_back();
    curFunction->addInstruction(
        std::make_unique<ReturnValueInstruction>(std::move(returnValueItem)));
  }
};

template <>
struct action<inst_index_load> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    Function* curFunction = program.getFunctions().back().get();
    std::unique_ptr<ASTNode> srcItem = std::move(parsed_items.back());
    parsed_items.pop_back();
    std::unique_ptr<ASTNode> dstItem = std::move(parsed_items.back());
    parsed_items.pop_back();
    curFunction->addInstruction(std::make_unique<IndexAssignInstruction>(
        std::move(dstItem), std::move(srcItem)));
  }
};

template <>
struct action<inst_index_store> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    Function* curFunction = program.getFunctions().back().get();
    std::unique_ptr<ASTNode> srcItem = std::move(parsed_items.back());
    parsed_items.pop_back();
    std::unique_ptr<ASTNode> dstItem = std::move(parsed_items.back());
    parsed_items.pop_back();
    curFunction->addInstruction(std::make_unique<IndexAssignInstruction>(
        std::move(dstItem), std::move(srcItem)));
  }
};

template <>
struct action<inst_length> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    Function* curFunction = program.getFunctions().back().get();
    std::unique_ptr<ASTNode> srcItem = std::move(parsed_items.back());
    parsed_items.pop_back();
    std::unique_ptr<ASTNode> dstItem = std::move(parsed_items.back());
    parsed_items.pop_back();
    curFunction->addInstruction(std::make_unique<LengthAssignInstruction>(
        std::move(dstItem), std::move(srcItem)));
  }
};

template <>
struct action<inst_call> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    Function* curFunction = program.getFunctions().back().get();
    std::unique_ptr<ASTNode> functionCall = std::move(parsed_items.back());
    parsed_items.pop_back();
    curFunction->addInstruction(
        std::make_unique<CallInstruction>(std::move(functionCall)));
  }
};

template <>
struct action<inst_call_assign> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    Function* curFunction = program.getFunctions().back().get();
    std::unique_ptr<ASTNode> functionCall = std::move(parsed_items.back());
    parsed_items.pop_back();
    std::unique_ptr<ASTNode> dstItem = std::move(parsed_items.back());
    parsed_items.pop_back();
    curFunction->addInstruction(std::make_unique<CallAssignInstruction>(
        std::move(dstItem), std::move(functionCall)));
  }
};

template <>
struct action<inst_new_array_assign> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    Function* curFunction = program.getFunctions().back().get();
    std::unique_ptr<ASTNode> newArrayItem = std::move(parsed_items.back());
    parsed_items.pop_back();
    std::unique_ptr<ASTNode> dstItem = std::move(parsed_items.back());
    parsed_items.pop_back();
    curFunction->addInstruction(std::make_unique<NewArrayAssignInstruction>(
        std::move(dstItem), std::move(newArrayItem)));
  }
};

template <>
struct action<inst_new_tuple_assign> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    Function* curFunction = program.getFunctions().back().get();
    std::unique_ptr<ASTNode> newTupleItem = std::move(parsed_items.back());
    parsed_items.pop_back();
    std::unique_ptr<ASTNode> dstItem = std::move(parsed_items.back());
    parsed_items.pop_back();
    curFunction->addInstruction(std::make_unique<NewTupleAssignInstruction>(
        std::move(dstItem), std::move(newTupleItem)));
  }
};

template <>
struct action<function_name> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
    std::unique_ptr<ASTNode> typeItem = std::move(parsed_items.back());
    parsed_items.pop_back();
    std::unique_ptr<ASTNode> functionNameItem =
        std::make_unique<Label>(input.string());
    program.addFunction(
        std::make_unique<Function>(std::move(typeItem), std::move(functionNameItem)));
  }
};

template <>
struct action<function_params> {
  template <typename Input>
  static void apply(const Input& input, Program& program) {
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

}  // namespace LA
