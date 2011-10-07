//=======================================================================
// Copyright Baptiste Wicht 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#ifndef MANGLING_H
#define MANGLING_H

#include <vector>
#include <string>
#include <sstream>
#include <memory>

#include "Types.hpp"

namespace eddic {

template<typename T>
std::string mangle(const std::string& functionName, const std::vector<std::shared_ptr<T>>& typed){
    if(functionName == "main"){
        return functionName;
    }

    std::ostringstream ss;

    ss << "_F";
    ss << functionName.length();
    ss << functionName;

    for(const std::shared_ptr<T>& t : typed){
        ss << mangle(t->type());
    }

    return ss.str();
}

std::string mangle(Type type);

} //end of eddic

#endif