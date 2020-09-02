#pragma once

namespace gjs {
	template<class T> struct remove_all { typedef T type; };
	template<class T> struct remove_all<T*> : remove_all<T> {};
	template<class T> struct remove_all<T&> : remove_all<T> {};
	template<class T> struct remove_all<T&&> : remove_all<T> {};
	template<class T> struct remove_all<T const> : remove_all<T> {};
	template<class T> struct remove_all<T volatile> : remove_all<T> {};
	template<class T> struct remove_all<T const volatile> : remove_all<T> {};
	template<class T> struct remove_all<T[]> : remove_all<T> {};

	template <typename T>
	inline const char* base_type_name() {
		return typeid(remove_all<T>::type).name();
	}
};