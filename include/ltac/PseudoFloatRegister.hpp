//=======================================================================
// Copyright Baptiste Wicht 2011-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#ifndef LTAC_PSEUDO_FLOAT_REGISTER_H
#define LTAC_PSEUDO_FLOAT_REGISTER_H

#include <ostream>

namespace eddic {

namespace ltac {

/*!
 * \struct PseudFloatRegister
 * Represents a pseudo float register in the LTAC Representation. 
 */
struct PseudoFloatRegister {
    unsigned short reg;
    bool bound;
    unsigned short binding;

    PseudoFloatRegister();
    PseudoFloatRegister(unsigned short);
    PseudoFloatRegister(unsigned short, unsigned short);

    explicit operator int();

    bool operator<(const PseudoFloatRegister& rhs) const;
    bool operator>(const PseudoFloatRegister& rhs) const;

    bool operator==(const PseudoFloatRegister& rhs) const;
    bool operator!=(const PseudoFloatRegister& rhs) const;
};

std::ostream& operator<<(std::ostream& out, const PseudoFloatRegister& reg);

} //end of ltac

} //end of eddic

namespace std {
    template<>
    class hash<eddic::ltac::PseudoFloatRegister> {
    public:
        size_t operator()(const eddic::ltac::PseudoFloatRegister& val) const {
            return val.reg;
        }
    };
}

#endif
