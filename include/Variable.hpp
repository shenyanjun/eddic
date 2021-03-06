//=======================================================================
// Copyright Baptiste Wicht 2011-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#ifndef VARIABLE_H
#define VARIABLE_H

#include <utility>
#include <string>
#include <memory>
#include <iostream>

#include "variant.hpp"
#include "Position.hpp"

#include "ast/Position.hpp"

namespace eddic {

typedef boost::variant<int, double, std::pair<std::string, int>> Val;

class Type;

/*!
 * \class Variable
 * \brief An entry of the symbol table
 */
class Variable {
    private:
        const std::string m_name;
        std::shared_ptr<const Type> m_type;

        Position m_position;
        ast::Position m_source_position;
        Val v_value;

    public:
        Variable(const std::string& name, std::shared_ptr<const Type> type, Position position);
        Variable(const std::string& name, std::shared_ptr<const Type> type, Position position, Val value);

        std::string name() const ;
        std::shared_ptr<const Type> type() const ;
        Position position() const ;

        Val val() const ;
        
        const ast::Position& source_position() const ;
        void set_source_position(const ast::Position& position);

        void setPosition(Position position);
};

} //end of eddic

#endif
