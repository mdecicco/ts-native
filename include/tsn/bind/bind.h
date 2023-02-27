#pragma once
#include <tsn/common/types.h>
#include <tsn/utils/remove_all.h>

#include <utils/String.h>
#include <exception>
#include <type_traits>

namespace tsn {
    class Context;
    class Module;

    namespace ffi {
        class BindException : public std::exception {
            public:
                BindException(const utils::String& message);
                ~BindException();

                virtual char const* what() const;

            private:
                utils::String m_message;
        };

        template <typename Cls>
        class PrimitiveTypeBinder;

        template <typename Cls>
        class PrimitiveTypeExtender;

        template <typename Cls>
        class ObjectTypeBinder;

        template <typename Cls>
        class ObjectTypeExtender;

        class Function;

        template <typename Cls>
        std::enable_if_t<std::is_fundamental_v<typename remove_all<Cls>::type>, PrimitiveTypeBinder<Cls>>
        bind(Context* ctx, const utils::String& name);

        template <typename Cls>
        std::enable_if_t<std::is_fundamental_v<typename remove_all<Cls>::type>, PrimitiveTypeBinder<Cls>>
        bind(Module* mod, const utils::String& name);

        template <typename Cls>
        std::enable_if_t<std::is_fundamental_v<typename remove_all<Cls>::type>, PrimitiveTypeExtender<Cls>>
        extend(Context* ctx);

        template <typename Cls>
        std::enable_if_t<std::is_fundamental_v<typename remove_all<Cls>::type>, PrimitiveTypeExtender<Cls>>
        extend(Module* mod);

        template <typename Cls>
        std::enable_if_t<!std::is_fundamental_v<typename remove_all<Cls>::type>, ObjectTypeBinder<Cls>>
        bind(Context* ctx, const utils::String& name);

        template <typename Cls>
        std::enable_if_t<!std::is_fundamental_v<typename remove_all<Cls>::type>, ObjectTypeBinder<Cls>>
        bind(Module* mod, const utils::String& name);

        template <typename Cls>
        std::enable_if_t<!std::is_fundamental_v<typename remove_all<Cls>::type>, ObjectTypeExtender<Cls>>
        extend(Context* ctx);

        template <typename Cls>
        std::enable_if_t<!std::is_fundamental_v<typename remove_all<Cls>::type>, ObjectTypeExtender<Cls>>
        extend(Module* mod);

        template <typename Ret, typename... Args>
        Function* bind(Context* ctx, const utils::String& name, Ret (*func)(Args...), access_modifier access = public_access);

        template <typename Ret, typename... Args>
        Function* bind(Module* mod, const utils::String& name, Ret (*func)(Args...), access_modifier access = public_access);
    };
};