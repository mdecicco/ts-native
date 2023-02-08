#pragma once

namespace tsn {
    namespace ffi {
        template<class T> struct remove_all { typedef T type; };
        template<class T> struct remove_all<T*> : remove_all<T> {};
        template<class T> struct remove_all<T&> : remove_all<T> {};
        template<class T> struct remove_all<T&&> : remove_all<T> {};
        template<class T> struct remove_all<T const> : remove_all<T> {};
        template<class T> struct remove_all<T volatile> : remove_all<T> {};
        template<class T> struct remove_all<T const volatile> : remove_all<T> {};
        template<class T> struct remove_all<T[]> : remove_all<T> {};
        
        template <typename T>
        struct remove_all_except_void_ptr { using type = typename ffi::remove_all<T>::type; };

        template <>
        struct remove_all_except_void_ptr<void*> { using type = void*; };
    };
};