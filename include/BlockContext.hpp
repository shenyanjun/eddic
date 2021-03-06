//=======================================================================
// Copyright Baptiste Wicht 2011-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#ifndef BLOCK_CONTEXT_H
#define BLOCK_CONTEXT_H

#include "Context.hpp"

namespace eddic {

class FunctionContext;
struct GlobalContext;

/*!
 * \class BlockContext
 * \brief A symbol table for the block level. 
 */
class BlockContext final : public Context {
    private:
        std::shared_ptr<FunctionContext> m_functionContext;

    public:
        BlockContext(std::shared_ptr<Context> parent, std::shared_ptr<FunctionContext> functionContext, std::shared_ptr<GlobalContext> global_context);
        
        std::shared_ptr<Variable> addVariable(const std::string& a, std::shared_ptr<const Type> type);
        std::shared_ptr<Variable> addVariable(const std::string& a, std::shared_ptr<const Type> type, ast::Value& value);
        
        std::shared_ptr<Variable> generate_variable(const std::string& prefix, std::shared_ptr<const Type> type) override;

        std::shared_ptr<Variable> new_temporary(std::shared_ptr<const Type> type);
        
        std::shared_ptr<FunctionContext> function();
};

} //end of eddic

#endif
