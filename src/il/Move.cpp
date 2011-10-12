//=======================================================================
// Copyright Baptiste Wicht 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#include <iostream>

#include "il/Move.hpp"
#include "il/Operand.hpp"

#include "AssemblyFileWriter.hpp"

using namespace eddic;

Move::Move(std::shared_ptr<Operand> lhs, std::shared_ptr<Operand> rhs) : m_lhs(lhs), m_rhs(rhs) {}

void Move::write(AssemblyFileWriter& writer){
    //We can always put an immediate value everywhere
    writer.stream() << "movl " << m_lhs->getValue() << ", " << m_rhs->getValue() << std::endl;

    //TODO Improve ?
}
