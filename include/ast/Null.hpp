//=======================================================================
// Copyright Baptiste Wicht 2011-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#ifndef AST_NULL_H
#define AST_NULL_H

#include <ostream>

namespace eddic {

namespace ast {

/*!
 * \class Null
 * \brief Represent a null pointer. 
 */
struct Null {
    
};

std::ostream& operator<< (std::ostream& stream, Null);

} //end of ast

} //end of eddic

#endif
