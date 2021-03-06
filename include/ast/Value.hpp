//=======================================================================
// Copyright Baptiste Wicht 2011-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#ifndef AST_VALUE_H
#define AST_VALUE_H

#include "variant.hpp"

#include "ast/values_def.hpp"
#include "ast/Integer.hpp"
#include "ast/IntegerSuffix.hpp"
#include "ast/Float.hpp"
#include "ast/CharLiteral.hpp"
#include "ast/Literal.hpp"
#include "ast/VariableValue.hpp"
#include "ast/DereferenceValue.hpp"
#include "ast/True.hpp"
#include "ast/False.hpp"
#include "ast/Null.hpp"

namespace eddic {

namespace ast {

typedef boost::mpl::vector<
            Integer, 
            IntegerSuffix, 
            Float,
            Literal, 
            CharLiteral, 
            VariableValue,
            DereferenceValue,
            Expression,
            Unary,
            Null,
            True,
            False,
            ArrayValue,
            FunctionCall,
            MemberFunctionCall,
            Cast,
            BuiltinOperator,
            Assignment,
            SuffixOperation,
            PrefixOperation
        > types_initial;

typedef boost::mpl::push_back<boost::mpl::push_back<boost::mpl::push_back<boost::mpl::push_back<types_initial, 
        Ternary>::type, 
        MemberValue>::type,
        New>::type,
        NewArray>::type 
    types;

typedef boost::make_variant_over<types>::type Value;

} //end of ast

} //end of eddic

#include "ast/Assignment.hpp"
#include "ast/MemberValue.hpp"
#include "ast/Ternary.hpp"
#include "ast/Expression.hpp"
#include "ast/ArrayValue.hpp"
#include "ast/MemberFunctionCall.hpp"
#include "ast/FunctionCall.hpp"
#include "ast/BuiltinOperator.hpp"
#include "ast/Unary.hpp"
#include "ast/Cast.hpp"
#include "ast/New.hpp"
#include "ast/NewArray.hpp"
#include "ast/SuffixOperation.hpp"
#include "ast/PrefixOperation.hpp"

#endif
