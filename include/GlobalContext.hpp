//=======================================================================
// Copyright Baptiste Wicht 2011-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#ifndef GLOBAL_CONTEXT_H
#define GLOBAL_CONTEXT_H

#include <map>

#include "Context.hpp"
#include "Function.hpp"
#include "Struct.hpp"
#include "Platform.hpp"

namespace eddic {

/*!
 * \struct GlobalContext
 * \brief The global symbol table for the whole source. 
 *
 * There is always only one instance of this class in the application. This symbol table is responsible
 * of storing all the global variables. It is also responsible for storing the global functions and structures. 
 */
struct GlobalContext final : public Context {
    public: 
        typedef std::unordered_map<std::string, std::shared_ptr<Function>> FunctionMap;
        typedef std::unordered_map<std::string, std::shared_ptr<Struct>> StructMap;
    
    public:
        GlobalContext(Platform platform);

        void release_references();
        
        Variables getVariables();
        
        std::shared_ptr<Variable> addVariable(const std::string& a, std::shared_ptr<const Type> type);
        std::shared_ptr<Variable> addVariable(const std::string& a, std::shared_ptr<const Type> type, ast::Value& value);
        
        std::shared_ptr<Variable> generate_variable(const std::string& prefix, std::shared_ptr<const Type> type) override;
        
        /*!
         * Add the given function to the symbol table. 
         * \param function The function to add to the symbol table. 
         */
        void addFunction(std::shared_ptr<Function> function);
        
        /*!
         * Returns the function with the given name. 
         * \param function The function to search for. 
         * \return A pointer to the function with the given name. 
         */
        std::shared_ptr<Function> getFunction(const std::string& function);
        
        /*!
         * Indicates if a function with the given name exists. 
         * \param function The function to search for. 
         * \return true if a function with the given name exists, otherwise false. 
         */
        bool exists(const std::string& function);

        /*!
         * Add the given structure to the symbol table. 
         * \param struct_ The structure to add to the symbol table. 
         */
        void add_struct(std::shared_ptr<Struct> struct_);
        
        /*!
         * Returns the structure with the given name. 
         * \param struct_ The structure to search for. 
         * \return A pointer to the structure with the given name. 
         */
        std::shared_ptr<Struct> get_struct(const std::string& struct_);
        
        /*!
         * Indicates if a structure with the given name exists. 
         * \param struct_ The structure to search for. 
         * \return true if a structure with the given name exists, otherwise false. 
         */
        bool struct_exists(const std::string& struct_);
        
        std::shared_ptr<const Type> member_type(std::shared_ptr<Struct> struct_, int offset);
        int member_offset(std::shared_ptr<Struct> struct_, const std::string& member);
        int size_of_struct(const std::string& struct_);
        
        bool is_recursively_nested(const std::string& struct_);

        FunctionMap functions();

        /*!
         * Add a reference to the function with the given name. 
         * \param function The function to add a reference to. 
         */
        void addReference(const std::string& function);

        /*!
         * Remove a reference to the function with the given name. 
         * \param function The function to remove a reference from. 
         */
        void removeReference(const std::string& function);

        /*!
         * Get the reference counter of the given function. 
         * \param function The function to add a reference to. 
         * \return The reference counter of the given function. 
         */
        int referenceCount(const std::string& function);

        void add_reference(std::shared_ptr<Variable> variable) override;
        unsigned int reference_count(std::shared_ptr<Variable> variable) override;

        Platform target_platform();
    
    private:
        FunctionMap m_functions;
        StructMap m_structs;
        Platform platform;

        std::shared_ptr<std::map<std::shared_ptr<Variable>, unsigned int>> references;

        void addPrintFunction(const std::string& function, std::shared_ptr<const Type> parameterType);
        void defineStandardFunctions();
        
        bool is_recursively_nested(const std::string& struct_, unsigned int left);
};

} //end of eddic

#endif
