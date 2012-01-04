//=======================================================================
// Copyright Baptiste Wicht 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#ifndef BOOLEAN_GRAMMAR_H
#define BOOLEAN_GRAMMAR_H

#include <boost/spirit/include/qi.hpp>
#include "lexer/SpiritLexer.hpp"

#include "parser/ValueGrammar.hpp"

#include "ast/Condition.hpp"

namespace qi = boost::spirit::qi;

namespace eddic {

namespace parser {

typedef lexer::lexer_type::iterator_type Iterator;
typedef lexer::SimpleLexer<lexer::lexer_type> Lexer;

/*!
 * \class BooleanGrammar
 * \brief Grammar to represent boolean litterals and conditions. 
 */
struct BooleanGrammar : qi::grammar<Iterator, ast::Condition()> {
    BooleanGrammar(const Lexer& lexer);

    qi::rule<Iterator, ast::Condition()> condition;
    qi::rule<Iterator, ast::True()> true_;
    qi::rule<Iterator, ast::False()> false_;
    qi::rule<Iterator, ast::BinaryCondition()> binary_condition;
    
    ValueGrammar value;
};

} //end of parser

} //end of eddic

#endif