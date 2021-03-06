//=======================================================================
// Copyright Baptiste Wicht 2011-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#ifndef LTAC_ARGUMENT_H
#define LTAC_ARGUMENT_H

#include <memory>
#include <string>

#include "variant.hpp"

#include "ltac/Register.hpp"
#include "ltac/FloatRegister.hpp"
#include "ltac/PseudoRegister.hpp"
#include "ltac/PseudoFloatRegister.hpp"
#include "ltac/Address.hpp"

namespace eddic {

namespace ltac {

typedef boost::variant<
        /* Hard Registers */
        eddic::ltac::FloatRegister, eddic::ltac::Register, 
        /* Pseudo Registers */
        eddic::ltac::PseudoFloatRegister, eddic::ltac::PseudoRegister, 
        /* Address */
        eddic::ltac::Address, 
        /* Literals */
        std::string, 
        /* Constants */
        double, int
    > Argument;

} //end of ltac

} //end of eddic

#endif
