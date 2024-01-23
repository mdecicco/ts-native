#pragma once
#include <tsn/interfaces/IDataTypeHolder.h>
#include <tsn/bind/bind.h>
#include <tsn/ffi/DataType.h>
#include <tsn/ffi/Object.h>
#include <tsn/ffi/DataTypeRegistry.h>
#include <tsn/ffi/Closure.h>
#include <tsn/common/Context.h>
#include <tsn/common/types.hpp>
#include <tsn/utils/remove_all.h>
#include <tsn/pipeline/Pipeline.h>

namespace tsn {
    template <typename C> struct is_array : std::false_type {};
    template <typename T> struct is_array< utils::Array<T> > : std::true_type {};
    template <typename C> inline constexpr bool is_array_v = is_array<C>::value;
    template <class T> struct array_type {};
    template <typename ElemTp>
    struct array_type<utils::Array<ElemTp>> {
        using value_type = ElemTp;
    };


    template <typename C> struct is_callable : std::false_type {};
    template <typename Ret, typename... Args> struct is_callable<ffi::Callable<Ret, Args...>> : std::true_type {};
    template <typename C> inline constexpr bool is_callable_v = is_callable<C>::value;
    template <class T> struct callable_type {};
    template <typename Ret, typename... Args>
    struct callable_type<ffi::Callable<Ret, Args...>> {
        using value_type = Ret(Args...);
    };

    template<typename Fn> struct function_traits;

    // specialization for functions
    template<typename Ret, typename ...Args>
    struct function_traits<Ret (Args...)> {
        using result = Ret;

        static ffi::DataType* getRetType(ffi::DataTypeRegistry* reg) {
            return reg->getType<Ret>();
        }

        static utils::Array<ffi::function_argument> getArgTypes(ffi::DataTypeRegistry* reg) {
            ffi::DataType* voidt = reg->getVoid();
            ffi::DataType* voidp = reg->getVoidPtr();
            utils::Array<ffi::function_argument> args = {{ arg_type::context_ptr, voidp }};

            constexpr int argc = std::tuple_size_v<std::tuple<Args...>>;
            if constexpr (argc == 0) return args;
            args.reserve(argc);

            if constexpr (argc > 0) {
                ffi::DataType* argTypes[] = { reg->getType<typename ffi::remove_all<Args>::type>()... };
                const char* argTpNames[] = { type_name<Args>()... };
                constexpr bool argIsPtr[] = { (std::is_reference_v<Args> || std::is_pointer_v<Args>)... };
                constexpr bool argNeedsPtr[] = { !std::is_fundamental_v<Args>... };

                for (u8 a = 0;a < argc;a++) {
                    if (!argTypes[a]) {
                        throw BindException(utils::String::Format(
                            "Argument index %d of function signature is of type '%s', which has not been bound",
                            a + 1, argTpNames[a]
                        ));
                    }

                    if (argNeedsPtr[a] && !argIsPtr[a]) {
                        throw BindException(utils::String::Format(
                            "Argument index %d of function signature is an object of type '%s' which is passed by value. "
                            "Passing objects by value is unsupported, please use a pointer or reference type for this argument",
                            a + 1, argTpNames[a]
                        ));
                    }

                    bool isVoidPtr = false;
                    if (argIsPtr[a] && argTypes[a]->getId() == voidt->getId()) {
                        isVoidPtr = true;
                    }

                    args.push({
                        (!isVoidPtr && (argNeedsPtr[a] || argIsPtr[a])) ? arg_type::pointer : arg_type::value,
                        isVoidPtr ? voidp : argTypes[a]
                    });
                }
            }

            return args;
        }
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
                throw ffi::BindException("IDataTypeHolder::getType(): Failed to compile specialization for array type");
            }

            return out;
        } else if constexpr (is_callable_v<typename ffi::remove_all<T>::type>) {
            using traits = function_traits<typename callable_type<typename ffi::remove_all<T>::type>::value_type>;
            auto it = m_typeHashMap.find(type_hash<typename std::remove_pointer_t<T>>());
            if (it != m_typeHashMap.end()) return m_types[it->second];

            ffi::DataType* retTp = traits::getRetType(m_ctx->getTypes());
            if (!retTp) {
                throw ffi::BindException("IDataTypeHolder::getType(): Failed to get type. T is a function pointer with a return type that has not been bound");
            }

            utils::Array<ffi::function_argument> args = traits::getArgTypes(m_ctx->getTypes());
            ffi::FunctionType ftp(nullptr, retTp, args, std::is_pointer_v<typename traits::result> && !std::is_same_v<typename traits::result, void*>);

            ffi::DataType* existing = m_ctx->getTypes()->getType(ftp.getId());
            if (existing) return existing;

            ffi::FunctionType* out = new ffi::FunctionType(ftp);
            m_ctx->getTypes()->addHostType(type_hash<typename std::remove_pointer_t<T>>(), out);

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
                throw ffi::BindException("IDataTypeHolder::getType(T&& arg): Failed to compile specialization for array type");
            }

            return out;
        } else if constexpr (is_callable_v<typename ffi::remove_all<T>::type>) {
            using traits = function_traits<typename callable_type<typename ffi::remove_all<T>::type>::value_type>;
            auto it = m_typeHashMap.find(type_hash<typename std::remove_pointer_t<T>>());
            if (it != m_typeHashMap.end()) return m_types[it->second];

            ffi::DataType* retTp = traits::getRetType(m_ctx->getTypes());
            if (!retTp) {
                throw ffi::BindException("IDataTypeHolder::getType(): Failed to get type. T is a function pointer with a return type that has not been bound");
            }

            utils::Array<ffi::function_argument> args = traits::getArgTypes(m_ctx->getTypes());
            ffi::FunctionType ftp(nullptr, retTp, args, std::is_pointer_v<typename traits::result> && !std::is_same_v<typename traits::result, void*>);

            ffi::DataType* existing = m_ctx->getTypes()->getType(ftp.getId());
            if (existing) return existing;

            ffi::FunctionType* out = new ffi::FunctionType(ftp);
            m_ctx->getTypes()->addHostType(type_hash<typename std::remove_pointer_t<T>>(), out);

            return out;
        } else {
            auto it = m_typeHashMap.find(type_hash<typename ffi::remove_all_except_void_ptr<T>::type>());
            if (it == m_typeHashMap.end()) return nullptr;
            return m_types[it->second];
        }
    }
};