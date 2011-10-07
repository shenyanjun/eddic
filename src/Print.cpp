//=======================================================================
// Copyright Baptiste Wicht 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#include <iostream>

#include "Print.hpp"

#include "AssemblyFileWriter.hpp"
#include "Value.hpp"

using namespace eddic;

void Print::write(AssemblyFileWriter& writer) {
    value->write(writer);

    switch (value->type()) {
        case Type::INT:
            writer.stream() << "call print_integer" << std::endl;
            writer.stream() << "addl $4, %esp" << std::endl;

            break;
        case Type::STRING:
            writer.stream() << "call print_string" << std::endl;
            writer.stream() << "addl $8, %esp" << std::endl;

            break;
        default:
            throw CompilerException("Variable of invalid type");
    }
}

void Print::checkStrings(StringPool& pool) {
    value->checkStrings(pool);
}

void Print::checkVariables() {
    value->checkVariables();
}