//=======================================================================
// Copyright Baptiste Wicht 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#include "mtac/Function.hpp"

using namespace eddic;

mtac::BasicBlock::BasicBlock(int i) : index(i), label("") {}

void mtac::BasicBlock::add(mtac::Statement statement){
    statements.push_back(statement);
}

std::ostream& mtac::operator<<(std::ostream& stream, BasicBlock& block){
    if(block.index == -1){
        return stream << "ENTRY";
    } else if(block.index == -2){
        return stream << "EXIT";
    } else {
        return stream << "B" << block.index;
    }
}
