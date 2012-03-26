//=======================================================================
// Copyright Baptiste Wicht 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#ifndef FUNCTIONS_ANNOTATOR_H
#define FUNCTIONS_ANNOTATOR_H

#include "ast/source_def.hpp"

namespace eddic {

class FunctionTable;

namespace ast {
    
void defineFunctions(ast::SourceFile& program, FunctionTable& functionTable);

} //end of ast

} //end of eddic

#endif