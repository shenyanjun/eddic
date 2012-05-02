//=======================================================================
// Copyright Baptiste Wicht 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#ifndef LTAC_ARGUMENT_H
#define LTAC_ARGUMENT_H

#include <memory>
#include <string>

#include <boost/variant.hpp>

#include "ltac/Register.hpp"
#include "ltac/FloatRegister.hpp"
#include "ltac/Address.hpp"

#define LTAC_CUSTOM_STRONG_TYPEDEF(T, D)                                    \
struct D {                                                                  \
    T t;                                                                    \
    explicit D(const T t_) : t(t_) {};                                      \
                                                                            \
    D(){};                                                                  \
    D(const D & t_) : t(t_.t){}                                             \
                                                                            \
    D(Register t_) : t(t_) {}                                               \
    D(FloatRegister t_) : t(t_) {}                                          \
    D(Address t_) : t(t_) {}                                                \
    D(int t_) : t(t_) {}                                                    \
    D(unsigned int t_) : t((int) t_) {}                                     \
    D(double t_) : t(t_) {}                                                 \
    D(const std::string& t_) : t(t_) {}                                     \
                                                                            \
    D& operator=(const D & rhs) { t = rhs.t; return *this;}                 \
    D& operator=(const T & rhs) { t = rhs; return *this;}                   \
                                                                            \
    D& operator=(Register rhs) { t = rhs; return *this; }                   \
    D& operator=(FloatRegister rhs) { t = rhs; return *this; }              \
    D& operator=(Address rhs) { t = rhs; return *this; }                    \
    D& operator=(int rhs) { t = rhs; return *this; }                        \
    D& operator=(unsigned int rhs) { t = (int) rhs; return *this; }         \
    D& operator=(double rhs) { t = rhs; return *this; }                     \
    D& operator=(const std::string& rhs) { t = rhs; return *this; }         \
                                                                            \
    operator const T &() const {return t; }                                 \
    operator T &() { return t; }                                            \
                                                                            \
    T* int_ptr() {return &t; }                                              \
    const T* int_ptr() const  {return &t; }                                 \
                                                                            \
    T& int_ref() {return t; }                                               \
    const T& int_ref() const {return t; }                                   \
                                                                            \
    template<typename Visitor>                                              \
    typename Visitor::result_type apply_visitor(Visitor& visitor)           \
    { return int_ref().apply_visitor(visitor); }                            \
};

namespace eddic {
class Variable;
} //end of eddic

namespace eddi_detail {
    typedef boost::variant<double, int, eddic::ltac::FloatRegister, eddic::ltac::Register, eddic::ltac::Address, std::string> ltac_variant_t;

    struct ltac_equals_visitor : boost::static_visitor<bool> {
        template <typename T>
        bool operator()(const T& a, const T& b) const
        { return a == b; }

        template<typename T, typename U>
        bool operator()(const T&, const U&) const
        { return false; }
    };

    struct ltac_variant_equals {
        ltac_equals_visitor _helper;

        bool operator()(const ltac_variant_t& a, const ltac_variant_t& b) const
        { return boost::apply_visitor(_helper, a, b); }
    };
} //end of eddi_detail

namespace eddic {

namespace ltac {

LTAC_CUSTOM_STRONG_TYPEDEF(eddi_detail::ltac_variant_t, Argument)

bool operator==(const Argument& a, const Argument& b);
bool operator==(const Argument& a, Register b);
bool operator==(const Argument& a, FloatRegister b);
bool operator==(const Argument& a, Address b);
bool operator==(const Argument& a, int b);
bool operator==(const Argument& a, double b);
bool operator==(const Argument& a, std::shared_ptr<Variable> b);
bool operator==(const Argument& a, const std::string& b);

} //end of ltac

} //end of eddic

namespace boost {

template<typename T>
inline T* get(eddic::ltac::Argument* argument){
    return boost::get<T>(argument->int_ptr());
}

template<typename T>
inline const T* get(const eddic::ltac::Argument* argument){
    return boost::get<T>(argument->int_ptr());
}

template<typename T>
inline T& get(eddic::ltac::Argument& argument){
    return boost::get<T>(argument.int_ref());
}

template<typename T>
inline const T& get(const eddic::ltac::Argument& argument){
    return boost::get<T>(argument.int_ref());
}

} //end of boost

#endif
