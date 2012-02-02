//=======================================================================
// Copyright Baptiste Wicht 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#include <iostream>
#include <fstream>

#include "DependenciesResolver.hpp"

#include "ast/SourceFile.hpp"

#include "parser/SpiritParser.hpp"

#include "SemanticalException.hpp"
#include "VisitorUtils.hpp"
#include "ASTVisitor.hpp"

using namespace eddic;

bool exists(const std::string& file){
   std::ifstream ifile(file.c_str());
   return ifile; 
}

DependenciesResolver::DependenciesResolver(parser::SpiritParser& p) : parser(p) {}

void includeDependencies(ast::SourceFile& program, parser::SpiritParser& parser);

class DependencyVisitor : public boost::static_visitor<> {
    private:
        parser::SpiritParser& parser;
        ast::SourceFile& source;

    public:
        DependencyVisitor(parser::SpiritParser& p, ast::SourceFile& s) : parser(p), source(s) {}

        AUTO_RECURSE_PROGRAM()
    
        void operator()(ast::StandardImport& import){
            auto headerFile = "stdlib/" + import.header + ".eddi";
            
            if(!exists(headerFile)){
                throw SemanticalException("The header " + import.header + " does not exist");
            }
           
            ast::SourceFile dependency; 
            if(parser.parse(headerFile, dependency)){
                includeDependencies(dependency, parser); 

                for(ast::FirstLevelBlock& block : dependency.Content->blocks){
                    source.Content->blocks.push_back(block);
                }
            } else {
                throw SemanticalException("The header " + import.header + " cannot be imported");
            }
        }
    
        void operator()(ast::Import& import){
            auto file = import.file;
            file.erase(0, 1);
            file.resize(file.size() - 1);

            if(!exists(file)){
                throw SemanticalException("The file " + file + " does not exist");
            }
           
            ast::SourceFile dependency; 
            if(parser.parse(file, dependency)){
                includeDependencies(dependency, parser); 

                for(ast::FirstLevelBlock& block : dependency.Content->blocks){
                    source.Content->blocks.push_back(block);
                }
            } else {
                throw SemanticalException("The file " + file + " cannot be imported");
            }
        }

        template<typename T>
        void operator()(T&){
            //Nothing to include there
        }
};

void includeDependencies(ast::SourceFile& program, parser::SpiritParser& parser){
    DependencyVisitor visitor(parser, program);
    visitor(program);
}

void DependenciesResolver::resolve(ast::SourceFile& program) const {
    includeDependencies(program, parser);
}
