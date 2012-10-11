//=======================================================================
// Copyright Baptiste Wicht 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#ifndef MTAC_BASIC_BLOCK_H
#define MTAC_BASIC_BLOCK_H

#include "variant.hpp"

#include "mtac/Statement.hpp"

namespace eddic {

class FunctionContext;
class Context;

namespace mtac {

class BasicBlock {
    public:
        int index;
        std::string label;

        std::vector<mtac::Statement> statements;
        std::shared_ptr<FunctionContext> context = nullptr;

        std::shared_ptr<BasicBlock> next = nullptr;
        std::shared_ptr<BasicBlock> prev = nullptr;

        BasicBlock(int index);

        void add(mtac::Statement statement);
};

std::ostream& operator<<(std::ostream& stream, BasicBlock& basic_block);

} //end of mtac

} //end of eddic

#endif
