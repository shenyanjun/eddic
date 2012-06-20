//=======================================================================
// Copyright Baptiste Wicht 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#ifndef VALUE_GRAMMAR_H
#define VALUE_GRAMMAR_H

#include <boost/spirit/include/qi.hpp>
#include "lexer/SpiritLexer.hpp"

#include "ast/Value.hpp"
#include "parser/TypeGrammar.hpp"

namespace qi = boost::spirit::qi;

namespace eddic {

namespace parser {

/*!
 * \class ValueGrammar
 * \brief Grammar representing values in EDDI language.
 */
struct ValueGrammar : qi::grammar<lexer::Iterator, ast::Value()> {
    ValueGrammar(const lexer::Lexer& lexer, const lexer::pos_iterator_type& position_begin);
    
    qi::rule<lexer::Iterator, ast::Value()> value;
    qi::rule<lexer::Iterator, ast::Value()> primaryValue;
    qi::rule<lexer::Iterator, ast::Value()> unaryValue;
    qi::rule<lexer::Iterator, ast::Cast()> castValue;
    qi::rule<lexer::Iterator, ast::Minus()> negatedValue;
    qi::rule<lexer::Iterator, ast::Minus()> negatedConstantValue;
    qi::rule<lexer::Iterator, ast::Plus()> plusValue;
    qi::rule<lexer::Iterator, ast::Expression()> additiveValue;
    qi::rule<lexer::Iterator, ast::Expression()> multiplicativeValue;
    qi::rule<lexer::Iterator, ast::Expression()> relationalValue;
    qi::rule<lexer::Iterator, ast::Expression()> logicalAndValue;
    qi::rule<lexer::Iterator, ast::Expression()> logicalOrValue;
    qi::rule<lexer::Iterator, ast::Value()> constant;
    qi::rule<lexer::Iterator, ast::Integer()> integer;
    qi::rule<lexer::Iterator, ast::IntegerSuffix()> integer_suffix;
    qi::rule<lexer::Iterator, ast::Float()> float_;
    qi::rule<lexer::Iterator, ast::Litteral()> litteral;
    qi::rule<lexer::Iterator, ast::VariableValue()> variable_value;
    qi::rule<lexer::Iterator, ast::DereferenceVariableValue()> dereference_variable_value;
    qi::rule<lexer::Iterator, ast::ArrayValue()> arrayValue;
    qi::rule<lexer::Iterator, ast::BuiltinOperator()> builtin_operator;
    qi::rule<lexer::Iterator, ast::FunctionCall()> functionCall;
    qi::rule<lexer::Iterator, ast::Assignment()> assignment;
    qi::rule<lexer::Iterator, ast::DereferenceAssignment()> dereference_assignment;
    qi::rule<lexer::Iterator, ast::True()> true_;
    qi::rule<lexer::Iterator, ast::False()> false_;
    qi::rule<lexer::Iterator, ast::Null()> null;
    qi::rule<lexer::Iterator, ast::PrefixOperation()> prefix_operation;
    qi::rule<lexer::Iterator, ast::SuffixOperation()> suffix_operation;

    qi::symbols<char, ast::Operator> multiplicative_op;
    qi::symbols<char, ast::Operator> additive_op;
    qi::symbols<char, ast::Operator> relational_op;
    qi::symbols<char, ast::Operator> logical_and_op;
    qi::symbols<char, ast::Operator> logical_or_op;
    qi::symbols<char, ast::Operator> suffix_op;
    qi::symbols<char, ast::Operator> prefix_op;
    
    TypeGrammar type;

    qi::symbols<char, ast::BuiltinType> builtin_op;
    
    const lexer::pos_iterator_type& position_begin;
};

} //end of parser

} //end of eddic

#endif
