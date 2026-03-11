#pragma once

#include <stdexcept>
#include <string>

#include "program.h"

namespace IR {

const Var* as_var_or_null(const ASTNode&);
const Label* as_label_or_null(const ASTNode&);
const Type* as_type_or_null(const ASTNode&);
const Index* as_index_or_null(const ASTNode&);
const ArrayLength* as_array_length_or_null(const ASTNode&);
const TupleLength* as_tuple_length_or_null(const ASTNode&);
const NewArray* as_new_array_or_null(const ASTNode&);
const NewTuple* as_new_tuple_or_null(const ASTNode&);
const Parameter* as_parameter_or_null(const ASTNode&);
const BranchInstruction* as_branch_or_null(const Instruction&);
const ConditionalBranchInstruction* as_conditional_branch_or_null(const Instruction&);

const Var& expect_var(const ASTNode&, const std::string& message);
const Label& expect_label(const ASTNode&, const std::string& message);
const Type& expect_type(const ASTNode&, const std::string& message);
const Index& expect_index(const ASTNode&, const std::string& message);
const ArrayLength& expect_array_length(const ASTNode&, const std::string& message);
const TupleLength& expect_tuple_length(const ASTNode&, const std::string& message);
const NewArray& expect_new_array(const ASTNode&, const std::string& message);
const NewTuple& expect_new_tuple(const ASTNode&, const std::string& message);
const Parameter& expect_parameter(const ASTNode&, const std::string& message);

}  // namespace IR
