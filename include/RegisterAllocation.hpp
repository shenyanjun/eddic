//=======================================================================
// Copyright Baptiste Wicht 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#ifndef REGISTER_ALLOCATION_H
#define REGISTER_ALLOCATION_H

namespace eddic {

class FunctionTable;

void allocateParams(FunctionTable& functionTable);

} //end of eddic

#endif