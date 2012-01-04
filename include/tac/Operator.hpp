//=======================================================================
// Copyright Baptiste Wicht 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#ifndef TAC_OPERATOR_H
#define TAC_OPERATOR_H

namespace eddic {

namespace tac {

enum class Operator : unsigned int {
    ADD,
    SUB,
    MUL,
    DIV,
    MOD,

    MINUS,          //result = -arg1
    
    DOT,            //result = (arg1)+arg2
    DOT_ASSIGN,     //result+arg1=arg2
    
    ARRAY,          //result=arg1[arg2]
    ARRAY_ASSIGN,   //result[arg1]=arg2

    PARAM           //push a single value
};

tac::Operator toOperator(char op);

} //end of tac

} //end of eddic

#endif