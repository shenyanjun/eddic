//=======================================================================
// Copyright Baptiste Wicht 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#include <boost/variant/static_visitor.hpp>

#include "ast/StringChecker.hpp"
#include "ast/SourceFile.hpp"
#include "ast/ASTVisitor.hpp"

#include "StringPool.hpp"
#include "VisitorUtils.hpp"

using namespace eddic;

class StringCheckerVisitor : public boost::static_visitor<> {
    private:
        StringPool& pool;

    public:
        StringCheckerVisitor(StringPool& p) : pool(p) {}

        AUTO_RECURSE_PROGRAM()
        AUTO_RECURSE_FUNCTION_DECLARATION() 
        AUTO_RECURSE_GLOBAL_DECLARATION() 
        AUTO_RECURSE_FUNCTION_CALLS()
        AUTO_RECURSE_BUILTIN_OPERATORS()
        AUTO_RECURSE_SIMPLE_LOOPS()
        AUTO_RECURSE_FOREACH()
        AUTO_RECURSE_BRANCHES()
        AUTO_RECURSE_BINARY_CONDITION()
        AUTO_RECURSE_COMPOSED_VALUES()
        AUTO_RECURSE_RETURN_VALUES()
        AUTO_RECURSE_ARRAY_VALUES()
        AUTO_RECURSE_VARIABLE_OPERATIONS()
        AUTO_RECURSE_ARRAY_ASSIGNMENT()

        void operator()(ast::Litteral& litteral){
            litteral.label = pool.label(litteral.value);
        }

        template<typename T>        
        void operator()(T&){
            //No need for string checking in other types
        }
};

void ast::checkStrings(ast::SourceFile& program, StringPool& pool){
   StringCheckerVisitor visitor(pool);
   visitor(program); 
}