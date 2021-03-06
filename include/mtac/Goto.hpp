//=======================================================================
// Copyright Baptiste Wicht 2011-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#ifndef MTAC_GOTO_H
#define MTAC_GOTO_H

#include <memory>

#include "mtac/forward.hpp"

namespace eddic {

namespace mtac {

struct Goto {
    std::string label;
    unsigned int depth;
    
    //Filled only in later phase replacing the label
    basic_block_p block;

    Goto();
    Goto(const std::string& label);
};

} //end of mtac

} //end of eddic

#endif
