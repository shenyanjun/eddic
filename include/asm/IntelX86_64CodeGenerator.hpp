//=======================================================================
// Copyright Baptiste Wicht 2011-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#ifndef INTEL_X86_64_CODE_GENERATOR_H
#define INTEL_X86_64_CODE_GENERATOR_H

#include "asm/IntelCodeGenerator.hpp"

namespace eddic {

namespace as {

/*!
 * \class IntelX86_64CodeGenerator
 * \brief Code generator for Intel X86_64 platform. 
 */
class IntelX86_64CodeGenerator : public IntelCodeGenerator {
    public:
        IntelX86_64CodeGenerator(AssemblyFileWriter& writer, std::shared_ptr<GlobalContext> context);
        
    protected:        
        void writeRuntimeSupport();
        void addStandardFunctions();
        void compile(mtac::function_p function);
        
        /* Functions for global variables */
        void defineDataSection();
        void declareIntArray(const std::string& name, unsigned int size);
        void declareStringArray(const std::string& name, unsigned int size);
        void declareFloatArray(const std::string& name, unsigned int size);

        void declareIntVariable(const std::string& name, int value);
        void declareStringVariable(const std::string& name, const std::string& label, int size);
        void declareString(const std::string& label, const std::string& value);
        void declareFloat(const std::string& label, double value);
};

} //end of as

} //end of eddic

#endif
