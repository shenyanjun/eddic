//=======================================================================
// Copyright Baptiste Wicht 2011-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#ifndef MTAC_ARITHMETIC_IDENTITIES_H
#define MTAC_ARITHMETIC_IDENTIES_H

#include <memory>

#include "variant.hpp"

#include "mtac/forward.hpp"
#include "mtac/pass_traits.hpp"

namespace eddic {

namespace mtac {

struct ArithmeticIdentities : public boost::static_visitor<void> {
    bool optimized = false;

    void operator()(std::shared_ptr<mtac::Quadruple> quadruple);

    template<typename T>
    void operator()(T&) const { 
        //Nothing to optimize
    }
};

template<>
struct pass_traits<ArithmeticIdentities> {
    STATIC_CONSTANT(pass_type, type, pass_type::LOCAL);
    STATIC_STRING(name, "arithmetic_identities");
    STATIC_CONSTANT(unsigned int, property_flags, 0);
    STATIC_CONSTANT(unsigned int, todo_after_flags, 0);
};

} //end of mtac

} //end of eddic

#endif
