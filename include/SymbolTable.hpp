//=======================================================================
// Copyright Baptiste Wicht 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <memory>
#include <string>
#include <unordered_map>

#include "Variable.hpp"
#include "Function.hpp"
#include "Struct.hpp"

namespace eddic {

typedef std::unordered_map<std::string, std::shared_ptr<Function>> FunctionMap;
typedef std::unordered_map<std::string, std::shared_ptr<Struct>> StructMap;

/*!
 * \class SymbolTable
 * \brief The global symbol table. 
 * 
 * This class is responsible for managing all the functions and the structs declarations of the current program. 
 * It's also responsible of managing the reference count for the functions.  
 */
class SymbolTable {
    private:
        FunctionMap functions;
        StructMap structs;

        void addPrintFunction(const std::string& function, BaseType parameterType);
        void defineStandardFunctions();

    public:
        SymbolTable();
        SymbolTable(const SymbolTable& rhs) = delete;

        /* Functions management */
        void addFunction(std::shared_ptr<Function> function);
        std::shared_ptr<Function> getFunction(const std::string& function);
        bool exists(const std::string& function);

        /* Struct management */
        void add_struct(std::shared_ptr<Struct> struct_);
        std::shared_ptr<Struct> get_struct(const std::string& struct_);
        bool struct_exists(const std::string& struct_);
        int member_offset(std::shared_ptr<Struct> struct_, const std::string& member);
        int member_offset_reverse(std::shared_ptr<Struct> struct_, const std::string& member);
        int size_of_struct(const std::string& struct_);

        //For now the symbol table is only iterable on functions
        FunctionMap::const_iterator begin();
        FunctionMap::const_iterator end();

        void addReference(const std::string& function);
        int referenceCount(const std::string& function);
};

extern SymbolTable symbols;

} //end of eddic

#endif