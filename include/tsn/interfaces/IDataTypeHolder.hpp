#pragma once
#include <tsn/interfaces/IDataTypeHolder.h>
#include <tsn/bind/bind.h>
#include <tsn/ffi/DataType.h>
#include <tsn/ffi/Object.h>
#include <tsn/ffi/DataTypeRegistry.h>
#include <tsn/common/Context.h>
#include <tsn/common/types.hpp>
#include <tsn/utils/remove_all.h>
#include <tsn/pipeline/Pipeline.h>


namespace tsn {
    template <typename C> struct is_array : std::false_type {};
    template <typename T> struct is_array< utils::Array<T> > : std::true_type {};
    template <typename C> inline constexpr bool is_array_v = is_array<C>::value;
    template<class T> class array_type {};
    template<typename ElemTp>
    class array_type<utils::Array<ElemTp>> {
        public:
            typedef ElemTp value_type;
    };

    template <typename T>
    ffi::DataType* IDataTypeHolder::getType() const {
        if constexpr (is_array_v<T>) {
            ffi::DataType* elemTp = getType<typename array_type<T>::value_type>();
            if (!elemTp) {
                throw ffi::BindException("IDataTypeHolder::getType(): Failed to get type. T is an Array<U>, where U is a type that has not been bound");
            }
            
            ffi::TemplateType* arrTp = m_ctx->getTypes()->getArray();
            utils::Array<ffi::DataType*> targs = { elemTp };
            ffi::DataType* existing = m_ctx->getTypes()->allTypes().find([arrTp, &targs](ffi::DataType* tp) {
                return tp->isSpecializationOf(arrTp, targs);
            });

            if (existing) return existing;

            ffi::DataType* out = m_ctx->getPipeline()->specializeTemplate(arrTp, { elemTp });
            if (!out) {
                throw ffi::BindException("IDataTypeHolder::getType(): Failed to compile instantiation for array type");
            }

            return out;
        } else {
            auto it = m_typeHashMap.find(type_hash<typename ffi::remove_all_except_void_ptr<T>::type>());
            if (it == m_typeHashMap.end()) return nullptr;
            return m_types[it->second];
        }
    }
    
    template <typename T>
    ffi::DataType* IDataTypeHolder::getType(T&& arg) const {
        if constexpr (std::is_same_v<Object, typename ffi::remove_all<T>::type>) {
            Object* o = nullptr;
            if constexpr (std::is_pointer_v<T>) o = arg;
            else if constexpr (std::is_reference_v<T>) {
                if constexpr (std::is_pointer_v<std::remove_reference_t<T>>) o = arg;
                else o = &arg;
            } else if constexpr (std::is_same_v<Object, T>) o = &arg;
            else {
                throw ffi::BindException("IDataTypeHolder::getType(T&& arg): Failed to get type. T is some form of 'gs::Object' that is unsupported");
            }

            return o->getType();
        } else if constexpr (is_array_v<T>) {
            ffi::DataType* elemTp = getType<array_type<T>::value_type>();
            if (!elemTp) {
                throw ffi::BindException("IDataTypeHolder::getType(T&& arg): Failed to get type. T is an Array<U>, where U is a type that has not been bound");
            }
            
            ffi::TemplateType* arrTp = m_ctx->getTypes()->getArray();
            utils::Array<ffi::DataType*> targs = { elemTp };
            ffi::DataType* existing = m_ctx->getTypes()->allTypes().find([arrTp, &targs](ffi::DataType* tp) {
                return tp->isSpecializationOf(arrTp, targs);
            });

            if (existing) return existing;

            ffi::DataType* out = m_ctx->getPipeline()->specializeTemplate(arrTp, { elemTp });
            if (!out) {
                throw ffi::BindException("IDataTypeHolder::getType(T&& arg): Failed to compile instantiation for array type");
            }

            return out;
        } else {
            auto it = m_typeHashMap.find(type_hash<typename ffi::remove_all_except_void_ptr<T>::type>());
            if (it == m_typeHashMap.end()) return nullptr;
            return m_types[it->second];
        }
    }
};