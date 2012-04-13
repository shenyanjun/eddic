//=======================================================================
// Copyright Baptiste Wicht 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#include <boost/assert.hpp>

#include "Type.hpp"

using namespace eddic;

Type::Type(BaseType base, bool a, unsigned int size, bool constant) : array(a), const_(constant), custom(false), baseType(base), m_size(size){}
Type::Type(const std::string& type) : array(false), const_(false), custom(true), m_type(type) {}

BaseType Type::base() const {
    return baseType;
}

bool Type::isArray() const {
    return array;
}

bool Type::isConst() const {
    return const_;
}

unsigned int Type::size() const {
    return m_size;
}
        
bool Type::is_custom_type() const {
    return custom;
}

bool Type::is_standard_type() const {
    return !is_custom_type();
}

std::string Type::type() const {
    BOOST_ASSERT_MSG(is_custom_type(), "Only custom type have a type");

    return m_type;
}

bool eddic::operator==(const Type& lhs, const Type& rhs){
    return lhs.baseType == rhs.baseType && 
           lhs.array == rhs.array &&
           lhs.const_ == rhs.const_ &&
           lhs.custom == rhs.custom &&
           lhs.m_size == rhs.m_size && 
           lhs.m_type == rhs.m_type; 
}

bool eddic::operator!=(const Type& lhs, const Type& rhs){
    return !(lhs == rhs); 
}

bool eddic::operator==(const Type& lhs, const BaseType& rhs){
    return lhs.baseType == rhs && !lhs.array && !lhs.custom; 
}

bool eddic::operator!=(const Type& lhs, const BaseType& rhs){
    return !(lhs == rhs); 
}