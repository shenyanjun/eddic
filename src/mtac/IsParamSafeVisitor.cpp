//=======================================================================
// Copyright Baptiste Wicht 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#include "VisitorUtils.hpp"

#include "mtac/IsParamSafeVisitor.hpp"

using namespace eddic;

//TODO In some cases, it's possible that some of them can be param safe
//Typically when their subcomponents are safe or constants
    
ASSIGN_INSIDE_CONST_CONST(mtac::IsParamSafeVisitor, ast::VariableValue, true)
ASSIGN_INSIDE_CONST_CONST(mtac::IsParamSafeVisitor, ast::Integer, true)
ASSIGN_INSIDE_CONST_CONST(mtac::IsParamSafeVisitor, ast::Float, true)
ASSIGN_INSIDE_CONST_CONST(mtac::IsParamSafeVisitor, ast::True, true)
ASSIGN_INSIDE_CONST_CONST(mtac::IsParamSafeVisitor, ast::False, true)
ASSIGN_INSIDE_CONST_CONST(mtac::IsParamSafeVisitor, ast::Litteral, true)

ASSIGN_INSIDE_CONST_CONST(mtac::IsParamSafeVisitor, ast::ArrayValue, false)
ASSIGN_INSIDE_CONST_CONST(mtac::IsParamSafeVisitor, ast::Expression, false)
ASSIGN_INSIDE_CONST_CONST(mtac::IsParamSafeVisitor, ast::Minus, false)
ASSIGN_INSIDE_CONST_CONST(mtac::IsParamSafeVisitor, ast::Plus, false)
ASSIGN_INSIDE_CONST_CONST(mtac::IsParamSafeVisitor, ast::FunctionCall, false)
ASSIGN_INSIDE_CONST_CONST(mtac::IsParamSafeVisitor, ast::BuiltinOperator, false)
ASSIGN_INSIDE_CONST_CONST(mtac::IsParamSafeVisitor, ast::SuffixOperation, false)
ASSIGN_INSIDE_CONST_CONST(mtac::IsParamSafeVisitor, ast::PrefixOperation, false)
ASSIGN_INSIDE_CONST_CONST(mtac::IsParamSafeVisitor, ast::Assignment, false)