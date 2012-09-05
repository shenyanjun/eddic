//=======================================================================
// Copyright Baptiste Wicht 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#ifndef FUNCTION_CONTEXT_H
#define FUNCTION_CONTEXT_H

#include <memory>

#include "Context.hpp"
#include "Platform.hpp"

namespace eddic {

class Type;
class GlobalContext;

/*!
 * \class FunctionContext
 * \brief A symbol table for a function.
 */
class FunctionContext final : public Context, public std::enable_shared_from_this<FunctionContext> {
    private:
        int currentPosition;
        int currentParameter;
        int m_size;
        int temporary = 1;
        Platform platform;

        //Refer all variables that are stored, including temporary
        Variables storage;

        void reallocate_storage();

    public:
        FunctionContext(std::shared_ptr<Context> parent, std::shared_ptr<GlobalContext> global_context, Platform platform);
        
        int size() const;

        std::shared_ptr<Variable> addVariable(const std::string& a, std::shared_ptr<const Type> type);
        std::shared_ptr<Variable> addVariable(const std::string& a, std::shared_ptr<const Type> type, ast::Value& value);
        std::shared_ptr<Variable> addParameter(const std::string& a, std::shared_ptr<const Type> type);
        std::shared_ptr<Variable> newVariable(const std::string& a, std::shared_ptr<const Type> type);
        std::shared_ptr<Variable> newParameter(const std::string& a, std::shared_ptr<const Type> type);

        std::shared_ptr<Variable> newVariable(std::shared_ptr<Variable> source);
        
        std::shared_ptr<Variable> new_temporary(std::shared_ptr<const Type> type);
        
        void removeVariable(const std::string& variable) override;

        void storeTemporary(std::shared_ptr<Variable> temp);

        void allocate_in_register(std::shared_ptr<Variable> variable, unsigned int register_);
        void allocate_in_param_register(std::shared_ptr<Variable> variable, unsigned int register_);

        Variables stored_variables();

        std::shared_ptr<const Type> struct_type = nullptr;
        
        std::shared_ptr<FunctionContext> function();
};

} //end of eddic

#endif
