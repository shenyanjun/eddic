//=======================================================================
// Copyright Baptiste Wicht 2011-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#include "variant.hpp"
#include "StringPool.hpp"

#include "ast/StringChecker.hpp"
#include "ast/SourceFile.hpp"
#include "ast/ASTVisitor.hpp"

using namespace eddic;

class StringCheckerVisitor : public boost::static_visitor<> {
    private:
        StringPool& pool;

    public:
        StringCheckerVisitor(StringPool& p) : pool(p) {}

        AUTO_RECURSE_PROGRAM()
        AUTO_RECURSE_STRUCT()
        AUTO_RECURSE_FUNCTION_DECLARATION() 
        AUTO_RECURSE_CONSTRUCTOR()
        AUTO_RECURSE_DESTRUCTOR()
        AUTO_RECURSE_GLOBAL_DECLARATION() 
        AUTO_RECURSE_FUNCTION_CALLS()
        AUTO_RECURSE_MEMBER_FUNCTION_CALLS()
        AUTO_RECURSE_BUILTIN_OPERATORS()
        AUTO_RECURSE_SIMPLE_LOOPS()
        AUTO_RECURSE_FOREACH()
        AUTO_RECURSE_BRANCHES()
        AUTO_RECURSE_COMPOSED_VALUES()
        AUTO_RECURSE_RETURN_VALUES()
        AUTO_RECURSE_ARRAY_VALUES()
        AUTO_RECURSE_VARIABLE_OPERATIONS()
        AUTO_RECURSE_STRUCT_DECLARATION()
        AUTO_RECURSE_TERNARY()
        AUTO_RECURSE_SWITCH()
        AUTO_RECURSE_SWITCH_CASE()
        AUTO_RECURSE_DEFAULT_CASE()
        AUTO_RECURSE_NEW()

        void operator()(ast::Literal& literal){
            literal.label = pool.label(literal.value);
        }

        AUTO_IGNORE_OTHERS()
};

void ast::StringCollectionPass::apply_program(ast::SourceFile& program, bool){
   StringCheckerVisitor visitor(*pool);
   visitor(program); 
}

bool ast::StringCollectionPass::is_simple(){
    return true;
}
