//=======================================================================
// Copyright Baptiste Wicht 2011-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#include "assert.hpp"
#include "Context.hpp"
#include "Variable.hpp"
#include "VisitorUtils.hpp"
#include "Type.hpp"
#include "GlobalContext.hpp"
#include "mangling.hpp"

#include "ast/GetTypeVisitor.hpp"
#include "ast/TypeTransformer.hpp"
#include "ast/Value.hpp"

using namespace eddic;

ASSIGN_INSIDE_CONST_CONST(ast::GetTypeVisitor, ast::Literal, STRING)
ASSIGN_INSIDE_CONST_CONST(ast::GetTypeVisitor, ast::CharLiteral, CHAR)

ASSIGN_INSIDE_CONST_CONST(ast::GetTypeVisitor, ast::Integer, INT)
ASSIGN_INSIDE_CONST_CONST(ast::GetTypeVisitor, ast::BuiltinOperator, INT) //At this time, all the builtin operators return an int
ASSIGN_INSIDE_CONST_CONST(ast::GetTypeVisitor, ast::IntegerSuffix, FLOAT) //For now, there is only a float (f) suffix

ASSIGN_INSIDE_CONST_CONST(ast::GetTypeVisitor, ast::Float, FLOAT)

ASSIGN_INSIDE_CONST_CONST(ast::GetTypeVisitor, ast::False, BOOL)
ASSIGN_INSIDE_CONST_CONST(ast::GetTypeVisitor, ast::True, BOOL)

std::shared_ptr<const Type> ast::GetTypeVisitor::operator()(const ast::Null& /*null*/) const {
    return new_pointer_type(INT);
}

std::shared_ptr<const Type> ast::GetTypeVisitor::operator()(const ast::New& value) const {
    return new_pointer_type(visit(ast::TypeTransformer(value.Content->context->global()), value.Content->type));
}

std::shared_ptr<const Type> ast::GetTypeVisitor::operator()(const ast::NewArray& value) const {
    return new_array_type(visit(ast::TypeTransformer(value.Content->context->global()), value.Content->type));
}

std::shared_ptr<const Type> ast::GetTypeVisitor::operator()(const ast::Ternary& ternary) const {
   return visit(*this, ternary.Content->true_value); 
}

std::shared_ptr<const Type> ast::GetTypeVisitor::operator()(const ast::Unary& unary) const {
   return visit(*this, unary.Content->value); 
}

std::shared_ptr<const Type> ast::GetTypeVisitor::operator()(const ast::Cast& cast) const {
   return visit(ast::TypeTransformer(cast.Content->context->global()), cast.Content->type); 
}

std::shared_ptr<const Type> ast::GetTypeVisitor::operator()(const ast::SuffixOperation& operation) const {
    return visit(*this, operation.Content->left_value);
}

std::shared_ptr<const Type> ast::GetTypeVisitor::operator()(const ast::PrefixOperation& operation) const {
    return visit(*this, operation.Content->left_value);
}

namespace {

std::shared_ptr<const Type> get_member_type(std::shared_ptr<GlobalContext> global_context, std::shared_ptr<const Type> type, const std::vector<std::string>& memberNames){
    auto struct_name = type->mangle();
    auto struct_type = global_context->get_struct(struct_name);

    for(std::size_t i = 0; i < memberNames.size(); ++i){
        auto member_type = (*struct_type)[memberNames[i]]->type;

        if(i == memberNames.size() - 1){
            return member_type;
        } else {
            if(member_type->is_pointer()){
                member_type = member_type->data_type();
            }

            struct_name = member_type->mangle();
            struct_type = global_context->get_struct(struct_name);
        }
    }

    eddic_unreachable("Problem with the type of members in nested struct values")
}

} //end of anonymous namespace

std::shared_ptr<const Type> ast::GetTypeVisitor::operator()(const ast::MemberValue& value) const {
    auto type = visit(*this, value.Content->location);

    if(type->is_pointer()){
        type = type->data_type();
    }

    return get_member_type(value.Content->context->global(), type, value.Content->memberNames);
}

std::shared_ptr<const Type> ast::GetTypeVisitor::operator()(const ast::VariableValue& variable) const {
    return variable.Content->var->type();
}

std::shared_ptr<const Type> ast::GetTypeVisitor::operator()(const ast::ArrayValue& array) const {
    auto array_type = array.Content->var->type();

    if(array_type == STRING){
        return CHAR;
    } 

    return array_type->data_type();
}

std::shared_ptr<const Type> ast::GetTypeVisitor::operator()(const ast::DereferenceValue& value) const {
    auto type = visit(*this, value.Content->ref);
    return type->data_type();
}

std::shared_ptr<const Type> ast::GetTypeVisitor::operator()(const ast::Assignment& assign) const {
    return visit(*this, assign.Content->left_value);
}

std::shared_ptr<const Type> ast::GetTypeVisitor::operator()(const ast::Expression& value) const {
    auto op = value.Content->operations[0].get<0>();

    if(op == ast::Operator::AND || op == ast::Operator::OR){
        return BOOL;
    } else if(op >= ast::Operator::EQUALS && op <= ast::Operator::GREATER_EQUALS){
        return BOOL;
    } else {
        //No need to recurse into operations because type are enforced in the check variables phase
        return visit(*this, value.Content->first);
    }
}

std::shared_ptr<const Type> ast::GetTypeVisitor::operator()(const ast::MemberFunctionCall& call) const {
    return call.Content->function->returnType;
}

std::shared_ptr<const Type> ast::GetTypeVisitor::operator()(const ast::FunctionCall& call) const {
    return call.Content->function->returnType;
}

std::shared_ptr<const Type> ast::GetTypeVisitor::operator()(const std::shared_ptr<Variable> value) const {
    return value->type();
}
