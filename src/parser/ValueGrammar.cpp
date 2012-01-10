//=======================================================================
// Copyright Baptiste Wicht 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#include "parser/ValueGrammar.hpp"

using namespace eddic;

parser::ValueGrammar::ValueGrammar(const lexer::Lexer& lexer) : ValueGrammar::base_type(value, "Value Grammar") {
    /* Match operators into symbols */
    additive_op.add
        ("+", ast::Operator::ADD)
        ("-", ast::Operator::SUB)
        ;

    multiplicative_op.add
        ("/", ast::Operator::DIV)
        ("*", ast::Operator::MUL)
        ("%", ast::Operator::MOD)
        ;

    value = additiveValue.alias();
    
    additiveValue %=
            multiplicativeValue
        >>  *(
                (lexer.addition > multiplicativeValue)
            |   (lexer.subtraction > multiplicativeValue)
            );
   
    multiplicativeValue %=
            unaryValue
        >>  *(
                (lexer.multiplication > unaryValue)
            |   (lexer.division > unaryValue)
            |   (lexer.modulo > unaryValue)
            );
    
    unaryValue %= 
            negatedValue
        |   plusValue
        |   primaryValue;
   
    negatedValue = 
            lexer.subtraction
         >> primaryValue;
  
    plusValue %=
            lexer.addition
         >> primaryValue;
    
    primaryValue = 
            constant 
        |   functionCall
        |   arrayValue
        |   variable 
        |   (lexer.left_parenth >> value > lexer.right_parenth);

    integer %= 
            qi::eps 
        >>  lexer.integer;
   
    variable %= 
            qi::eps
        >>  lexer.word;
   
    arrayValue %=
            lexer.word
        >>  lexer.left_bracket
        >>  value
        >>  lexer.right_bracket;
    
    litteral %= 
            qi::eps 
        >> lexer.litteral;

    constant %= 
            integer 
        |   litteral;
    
    functionCall %=
            lexer.word
        >>  lexer.left_parenth
        >>  -( value >> *( lexer.comma > value))
        >>  lexer.right_parenth;
}
