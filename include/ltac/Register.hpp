//=======================================================================
// Copyright Baptiste Wicht 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#ifndef LTAC_REGISTER_H
#define LTAC_REGISTER_H

namespace eddic {

namespace ltac {

/*!
 * \struct Register
 * Represents a symbolic register in the LTAC Representation. 
 */
struct Register {
    unsigned short reg;

    Register();
    Register(unsigned short);
    
    operator int();

    bool operator==(const Register& rhs) const;
    bool operator!=(const Register& rhs) const;
};

/*!
 * Represent the stack pointer. 
 */
static const Register SP(1000);

/*!
 * Represent the base pointer. 
 */
static const Register BP(1001);

} //end of ltac

} //end of eddic

#endif