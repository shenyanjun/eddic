//=======================================================================
// Copyright Baptiste Wicht 2011-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#include "mtac/IfFalse.hpp"

using namespace eddic;

mtac::IfFalse::IfFalse(){}
mtac::IfFalse::IfFalse(Argument a1, const std::string& l) : arg1(a1), label(l) {}
mtac::IfFalse::IfFalse(BinaryOperator o, Argument a1, Argument a2, const std::string& l) : arg1(a1), op(o), arg2(a2), label(l) {}
