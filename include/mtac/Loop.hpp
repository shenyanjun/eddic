//=======================================================================
// Copyright Baptiste Wicht 2011-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#ifndef MTAC_LOOP_H
#define MTAC_LOOP_H

#include <memory>
#include <set>

#include "mtac/forward.hpp"

namespace eddic {

namespace mtac {

class Loop {
    public:
        typedef std::set<mtac::basic_block_p>::iterator iterator;

        Loop(const std::set<mtac::basic_block_p>& blocks);

        iterator begin();
        iterator end();
        
        std::set<mtac::basic_block_p>& blocks();

        int estimate();
        void set_estimate(int estimate);

    private:
        std::set<mtac::basic_block_p> m_blocks;

        int m_estimate = -1;
};

Loop::iterator begin(std::shared_ptr<mtac::Loop> loop);
Loop::iterator end(std::shared_ptr<mtac::Loop> loop);

} //end of mtac

} //end of eddic

#endif
