//=======================================================================
// Copyright Baptiste Wicht 2011-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#ifndef AST_TEMPLATE_STRUCT_H
#define AST_TEMPLATE_STRUCT_H

#include <vector>

#include <boost/fusion/include/adapt_struct.hpp>

#include "ast/Deferred.hpp"
#include "ast/Position.hpp"
#include "ast/MemberDeclaration.hpp"
#include "ast/Constructor.hpp"
#include "ast/Destructor.hpp"
#include "ast/FunctionDeclaration.hpp"
#include "ast/TemplateFunctionDeclaration.hpp"

namespace eddic {

namespace ast {

struct ASTTemplateStruct {
    Position position;
    std::vector<std::string> template_types;
    std::string name;
    std::vector<MemberDeclaration> members;
    std::vector<Constructor> constructors;
    std::vector<Destructor> destructors;
    std::vector<FunctionDeclaration> functions;
    std::vector<TemplateFunctionDeclaration> template_functions;

    mutable long references = 0;
};

typedef Deferred<ASTTemplateStruct> TemplateStruct;

} //end of ast

} //end of eddic

//Adapt the struct for the AST
BOOST_FUSION_ADAPT_STRUCT(
    eddic::ast::TemplateStruct,
    (eddic::ast::Position, Content->position)
    (std::vector<std::string>, Content->template_types)
    (std::string, Content->name)
    (std::vector<eddic::ast::MemberDeclaration>, Content->members)
    (std::vector<eddic::ast::Constructor>, Content->constructors)
    (std::vector<eddic::ast::Destructor>, Content->destructors)
    (std::vector<eddic::ast::FunctionDeclaration>, Content->functions)
    (std::vector<eddic::ast::TemplateFunctionDeclaration>, Content->template_functions)
)

#endif
