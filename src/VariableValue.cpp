//=======================================================================
// Copyright Baptiste Wicht 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#include "VariableValue.hpp"

#include "AssemblyFileWriter.hpp"
#include "Context.hpp"
#include "Value.hpp"
#include "Variable.hpp"

using namespace eddic;

void VariableValue::checkVariables() {
    if (!context()->exists(m_variable)) {
        throw CompilerException("Variable has not been declared", token());
    }

    m_var = context()->getVariable(m_variable);
    m_type = m_var->type();
}

void VariableValue::write(AssemblyFileWriter& writer) {
    m_var->pushToStack(writer);
}

bool VariableValue::isConstant() {
    return false;
}