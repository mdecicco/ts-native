#include <tsn/ffi/DataTypeRegistry.h>
#include <tsn/ffi/DataType.h>
#include <tsn/interfaces/IDataTypeHolder.hpp>
#include <tsn/bind/bind.h>
#include <utils/Array.hpp>

namespace tsn {
    namespace ffi {
        template <typename... Args>
        void DataTypeRegistry::validateAndGetArgs(utils::Array<function_argument>& args) {
            constexpr int argc = std::tuple_size_v<std::tuple<Args...>>;
            if constexpr (argc == 0) return;

            if constexpr (argc > 0) {
                DataType* argTypes[] = { getType<typename remove_all<Args>::type>()... };
                const char* argTpNames[] = { type_name<Args>()... };
                constexpr bool argIsPtr[] = { (std::is_reference_v<Args> || std::is_pointer_v<Args>)... };
                constexpr bool argNeedsPtr[] = { (!std::is_fundamental_v<Args> && !std::is_same_v<Args, null_t>)... };

                for (u8 a = 0;a < argc;a++) {
                    if (!argTypes[a]) {
                        throw BindException(utils::String::Format(
                            "Argument index %d of signature is of type '%s', which has not been bound",
                            a + 1, argTpNames[a]
                        ));
                    }

                    if (argNeedsPtr[a] && !argIsPtr[a]) {
                        throw BindException(utils::String::Format(
                            "Argument index %d of signature is an object of type '%s' which is passed by value. "
                            "Passing objects by value is unsupported, please use a pointer or reference type for this argument",
                            a + 1, argTpNames[a]
                        ));
                    }

                    bool isVoidPtr = false;
                    if (argIsPtr[a] && argTypes[a]->getId() == m_void->getId()) {
                        isVoidPtr = true;
                    }

                    args.push({
                        (!isVoidPtr && (argNeedsPtr[a] || argIsPtr[a])) ? arg_type::pointer : arg_type::value,
                        isVoidPtr ? m_voidPtr : argTypes[a]
                    });
                }
            }
        }

        template <typename Ret, typename ...Args>
        FunctionType* DataTypeRegistry::getSignatureType(DataType* selfType) {
            DataType* retTp = getType<Ret>();
            if (!retTp) {
                throw BindException(utils::String::Format("Return type '%s' of signature has not been bound yet", type_name<Ret>()));
            }

            if constexpr (std::is_pointer_v<Ret> && std::is_pointer_v<std::remove_pointer_t<Ret>>) {
                throw BindException("Return type of signature is a pointer to a pointer, which is not supported at this time");
            }

            utils::Array<function_argument> args;
            validateAndGetArgs<Args...>(args);

            return getSignatureType(selfType, retTp, std::is_pointer_v<Ret> && !std::is_same_v<Ret, void*>, args);
        }
        
        template <typename Cls, typename Ret, typename ...Args>
        FunctionType* DataTypeRegistry::getMethodSignatureType() {
            DataType* selfTp = getType<Cls>();
            if (!selfTp) {
                throw BindException(utils::String::Format("'this' type '%s' of signature has not been bound yet", type_name<Cls>()));
            }

            DataType* retTp = getType<Ret>();
            if (!retTp) {
                throw BindException(utils::String::Format("Return type '%s' of signature has not been bound yet", type_name<Ret>()));
            }

            if constexpr (std::is_pointer_v<Ret> && std::is_pointer_v<std::remove_pointer_t<Ret>>) {
                throw BindException("Return type of signature is a pointer to a pointer, which is not supported at this time");
            }

            utils::Array<function_argument> args;
            validateAndGetArgs<Args...>(args);

            return getSignatureType(selfTp, retTp, std::is_pointer_v<Ret> && !std::is_same_v<Ret, void*>, args);
        }
    };
};