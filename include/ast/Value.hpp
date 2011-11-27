//=======================================================================
// Copyright Baptiste Wicht 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#ifndef AST_VALUE_H
#define AST_VALUE_H

#include <boost/variant/variant.hpp>

#include "ast/values_def.hpp"

#include "ast/Integer.hpp"
#include "ast/Litteral.hpp"
#include "ast/VariableValue.hpp"

namespace eddic {

namespace ast {

typedef boost::variant<
            Integer, 
            Litteral, 
            VariableValue,
            ComposedValue,
            ArrayValue,
            FunctionCall
        > Value;

} //end of ast

} //end of eddic

#include "ast/ComposedValue.hpp"
#include "ast/ArrayValue.hpp"
#include "ast/FunctionCall.hpp"

#endif
