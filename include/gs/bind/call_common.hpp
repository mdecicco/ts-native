#pragma once
#include <ffi.h>
#include <gs/utils/remove_all.h>
#include <gs/common/Object.h>
#include <gs/common/DataType.h>
#include <gs/bind/bind.h>

namespace gs {
    namespace ffi {
        /**
         * @brief Gets a pointer to the ffi_type that corresponds to type T.
         *        Note: References count as pointers as well as any non-primitive
         *        type. Non-primitive types will universally be passed as pointers
         *        since passing objects by value is a nightmare for FFI.
         * 
         * @tparam T Host data type
         * @return Pointer to ffi_type that corresponds to type T
         */
        template <typename T>
        ffi_type* getType(T&& val) {
            if constexpr (std::is_same_v<void, T>) return &ffi_type_void;
            else if constexpr (std::is_same_v<u8, T>) return &ffi_type_uint8;
            else if constexpr (std::is_same_v<i8, T>) return &ffi_type_sint8;
            else if constexpr (std::is_same_v<u16, T>) return &ffi_type_uint16;
            else if constexpr (std::is_same_v<i16, T>) return &ffi_type_sint16;
            else if constexpr (std::is_same_v<u32, T>) return &ffi_type_uint32;
            else if constexpr (std::is_same_v<i32, T>) return &ffi_type_sint32;
            else if constexpr (std::is_same_v<u64, T>) return &ffi_type_uint64;
            else if constexpr (std::is_same_v<i64, T>) return &ffi_type_sint64;
            else if constexpr (std::is_same_v<f32, T>) return &ffi_type_float;
            else if constexpr (std::is_same_v<f64, T>) return &ffi_type_double;
            else if constexpr (std::is_pointer_v<T> || std::is_reference_v<T> || !std::is_fundamental_v<T>) {
                if constexpr (std::is_same_v<Object, remove_all<T>::type>) {
                    Object* o = nullptr;
                    if constexpr (std::is_pointer_v<T>) o = val;
                    else if constexpr (std::is_reference_v<T>) {
                        if constexpr (std::is_pointer_v<std::remove_reference_t<T>>) o = val;
                        else o = &val;
                    } else if constexpr (std::is_same_v<Object, T>) o = &val;
                    else {
                        throw BindException("ffi_type* getType(T&& val): Failed to get type. T is some form of 'gs::Object' that is unsupported");
                    }

                    const type_meta& t = o->getType()->getInfo();
                    if (t.size == 0) {
                        return &ffi_type_void;
                    } else if (t.is_primitive) {
                        if (t.is_floating_point) {
                            if (t.size == sizeof(f32)) return &ffi_type_float;
                            else return &ffi_type_double;
                        } else if (t.is_integral) {
                            if (t.is_unsigned) {
                                switch (t.size) {
                                    case 1: return &ffi_type_uint8;
                                    case 2: return &ffi_type_uint16;
                                    case 4: return &ffi_type_uint32;
                                    case 8: return &ffi_type_uint64;
                                }
                            } else {
                                switch (t.size) {
                                    case 1: return &ffi_type_sint8;
                                    case 2: return &ffi_type_sint16;
                                    case 4: return &ffi_type_sint32;
                                    case 8: return &ffi_type_sint64;
                                }
                            }
                        }
                    } else return &ffi_type_pointer;
                }

                return &ffi_type_pointer;
            }
        }
    };
};
