//=======================================================================
// Copyright Baptiste Wicht 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#ifndef SCANNER_H
#define SCANNER_H

#include <fstream>
#include <string>
#include <cctype>

namespace eddic {

class Scanner {
    private:
        std::ifstream stream;

        char currentChar;
        int line;
        int col;
    public:
        Scanner() : currentChar(0), line(1), col(1) {}

        /* File manipulations */
        void scan(std::string file);
        void close();

        /* Manage files */
        bool next();
        void pushBack();

        /* Getters */
        inline char current() const {
            return currentChar;
        };
        inline int currentLine() const {
            return line;
        };
        inline int currentCol() const {
            return col;
        };

        /* Utility */
        inline bool isAlpha() const {
            return isalpha(currentChar);
        };
        inline bool isSpace() const {
            return isspace(currentChar);
        };
        inline bool isDigit() const {
            return isdigit(currentChar);
        };
};

} //end of eddic

#endif
