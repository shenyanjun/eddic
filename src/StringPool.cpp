//=======================================================================
// Copyright Baptiste Wicht 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#include <sstream>
#include <cassert>

#include "StringPool.hpp"

using namespace eddic;

StringPool::StringPool() : currentString(0) {
    label("`\\n`");   //Carriage return special label for println
    label("\"-\"");     //- special label for print_integer with negative number
    label("\"\"");      //- special label for default string value
    label("\".\"");     //- special label for printing floating point numbers
}

std::string StringPool::label(const std::string& value) {
    if (pool.find(value) == pool.end()) {
        std::stringstream ss;
        ss << "S";
        ss << ++currentString;
        pool[value] = ss.str();
    }

    return pool[value];
}

std::string StringPool::value(const std::string& label) {
    for (auto it : pool){
        if(it.second == label){
            return it.first;
        }
    }

    //This method should not be called on not-existing label
    assert(false);
}

std::unordered_map<std::string, std::string> StringPool::getPool() const {
    return pool;
}
