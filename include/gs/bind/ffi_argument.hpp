#pragma once
#include <gs/common/types.h>
#include <gs/utils/remove_all.h>
#include <type_traits>

namespace gs {
    namespace ffi {
        /**
         * @brief Converts an rvalue reference of a function argument to a (void*)T* or a
         *        (void*)T**, whichever is expected by the callee. This function is intended
         *        for use with libffi. Libffi expects callee arguments to be specified in an
         *        array of type void*[].
         * 
         * @tparam T         Argument type
         * @param arg        rvalue reference to argument of type T
         * @param ptrStorage pointer to void* where a pointer to the argument would be stored if
         *                   the callee expects the argument to be a pointer
         * @param argType    An arg_type that defines what implicit value or value type the callee
         *                   expects to receive for this argument. This is used to understand whether
         *                   the callee expects a pointer or a primitive value for this argument.
         * @return If the callee expects a primitive value, a pointer to the argument is returned.
         *         If the callee expects a pointer to some type, a pointer to a pointer to the argument
         *         is returned.
         */
        template <typename T>
        void* getArg(T&& arg, void** ptrStorage, const arg_type& argType) {
            // If function expects value:
            //     output must be T*
            // If function expects a pointer:
            //     output must be T**


            // If this logic doesn't work then it's because of one or both of the following:
            // 1. The application programmer passed the wrong argument type to a script
            //    function.
            // 2. The script programmer used the wrong argument type for a function that
            //    the host programmer expected to be correct.


            //
            // When arg is wrapped in a gs::Object
            //

            if constexpr (std::is_same_v<Object, typename remove_all<T>::type>) {
                Object* o = nullptr;
                if constexpr (std::is_pointer_v<T>) o = arg;
                else if constexpr (std::is_reference_v<T>) {
                    if constexpr (std::is_pointer_v<std::remove_reference_t<T>>) o = arg;
                    else o = &arg;
                } else if constexpr (std::is_same_v<Object, T>) o = &arg;
                else {
                    throw BindException("void* getArg(T&& arg, etc...): Failed to get argument for FFI call. T is some form of 'gs::Object' that is unsupported");
                }

                const type_meta& t = o->getType()->getInfo();
                if (argType == arg_type::value) {
                    // Expected type is a primitive
                    return o->getPtr();
                } else {
                    // Expected type is a pointer
                    *ptrStorage = o->getPtr();
                    return (void*)ptrStorage;
                }
            }



            //
            // When arg is not wrapped in a gs::Object
            //
            // You might think this next block of code is longer than the above one,
            // but it's actually only either 2 or 3 operations long. The above
            // one is 8.
            //

            if constexpr (std::is_fundamental_v<typename remove_all<T>::type>) {
                // Currently every arg_type other than arg_type::value is a pointer
                if (argType == arg_type::value) {
                    // Expected type is a primitive

                    if constexpr (std::is_pointer_v<T>) {
                        // argument is pointer to primitive
                        if constexpr (std::is_const_v<T>) return (void*)const_cast<T*>(arg);
                        else return (void*)arg;
                    } else if constexpr (std::is_reference_v<T> && std::is_pointer_v<std::remove_reference_t<T>>) {
                        // argument is reference to pointer to primitive
                        if constexpr (std::is_const_v<T>) return (void*)const_cast<T*>(arg);
                        else return (void*)arg;
                    } else if constexpr (std::is_reference_v<T>) {
                        // argument is reference to primitive
                        if constexpr (std::is_const_v<T>) return (void*)const_cast<T*>(&arg);
                        else return (void*)&arg;
                    } else {
                        // argument is primitive
                        if constexpr (std::is_const_v<T>) return (void*)const_cast<T*>(&arg);
                        else return (void*)&arg;
                    }
                } else {
                    // Expected type is a pointer to a primitive
                    
                    if constexpr (std::is_pointer_v<T>) {
                        // argument is pointer to primitive
                        if constexpr (std::is_const_v<T>) {
                            *ptrStorage = (void*)const_cast<T*>(arg);
                            return (void*)ptrStorage;
                        } else {
                            *ptrStorage = (void*)arg;
                            return (void*)ptrStorage;
                        }
                    } else if constexpr (std::is_reference_v<T> && std::is_pointer_v<std::remove_reference_t<T>>) {
                        // argument is reference to pointer to primitive
                        if constexpr (std::is_const_v<T>) {
                            *ptrStorage = (void*)const_cast<T*>(arg);
                            return (void*)ptrStorage;
                        } else {
                            *ptrStorage = (void*)arg;
                            return (void*)ptrStorage;
                        }
                    } else if constexpr (std::is_reference_v<T>) {
                        // argument is reference to primitive
                        if constexpr (std::is_const_v<T>) {
                            *ptrStorage = (void*)const_cast<T*>(&arg);
                            return (void*)ptrStorage;
                        } else {
                            *ptrStorage = (void*)&arg;
                            return (void*)ptrStorage;
                        }
                    } else {
                        // argument is primitive
                        if constexpr (std::is_const_v<T>) {
                            *ptrStorage = (void*)const_cast<T*>(&arg);
                            return (void*)ptrStorage;
                        } else {
                            *ptrStorage = (void*)&arg;
                            return (void*)ptrStorage;
                        }
                    }
                }
            } else {
                // Expected type is a pointer to an object
                //
                // If it's not, it's user error. Functions that take
                // non-primitives by value as parameters can't be bound
                // Therefore: If the actual param type is not a pointer
                // to an object then it must be a primitive, and the host
                // is attempting to pass a non-primitive.

                if constexpr (std::is_pointer_v<T>) {
                    // argument is pointer to object
                    if constexpr (std::is_const_v<T>) {
                        *ptrStorage = (void*)const_cast<T*>(arg);
                        return (void*)ptrStorage;
                    } else {
                        *ptrStorage = (void*)arg;
                        return (void*)ptrStorage;
                    }
                } else if constexpr (std::is_reference_v<T> && std::is_pointer_v<std::remove_reference_t<T>>) {
                    // argument is reference to pointer to object
                    if constexpr (std::is_const_v<T>) {
                        *ptrStorage = (void*)const_cast<T*>(arg);
                        return (void*)ptrStorage;
                    } else {
                        *ptrStorage = (void*)arg;
                        return (void*)ptrStorage;
                    }
                } else if constexpr(std::is_reference_v<T>) {
                    // argument is reference to object
                    if constexpr (std::is_const_v<T>) {
                        *ptrStorage = (void*)const_cast<T*>(&arg);
                        return (void*)ptrStorage;
                    } else {
                        *ptrStorage = (void*)&arg;
                        return (void*)ptrStorage;
                    }
                } else {
                    // argument is object passed by value
                    if constexpr (std::is_const_v<T>) {
                        *ptrStorage = (void*)const_cast<T*>(&arg);
                        return (void*)ptrStorage;
                    } else {
                        *ptrStorage = (void*)&arg;
                        return (void*)ptrStorage;
                    }
                }
            }
        }
    };
};