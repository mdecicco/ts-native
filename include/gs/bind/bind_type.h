#pragma once
#include <gs/common/types.h>
#include <utils/Array.h>
#include <utils/String.h>

#include <xtr1common>

namespace gs {
    class Module;

    namespace ffi {
        struct type_property;
        struct type_base;
        class Function;
        class DataType;
        class FunctionRegistry;
        class DataTypeRegistry;

        class DataTypeBinder {
            public:
                DataTypeBinder(FunctionRegistry* freg, DataTypeRegistry* treg, const utils::String& name, const utils::String& fullyQualifiedName, type_meta&& meta);
                ~DataTypeBinder();

                DataType* finalize(Module* mod = nullptr);

            protected:
                void setDestructor(Function* dtor);
                void addMethod(Function* method);
                void addProperty(type_property&& prop);
                bool propNameExists(const utils::String& name) const;
                
                bool m_isFinal;
                FunctionRegistry* funcRegistry;
                DataTypeRegistry* typeRegistry;
                DataType* m_type;
        };

        template <typename Cls>
        class PrimitiveTypeBinder : public DataTypeBinder {
            public:
                PrimitiveTypeBinder(FunctionRegistry* freg, DataTypeRegistry* treg, const utils::String& name, const utils::String& fullyQualifiedName);
                ~PrimitiveTypeBinder();

                // fake instance method
                template <typename Ret, typename... Args>
                PrimitiveTypeBinder<Cls>& method(const utils::String& name, Ret (*method)(Cls*, Args...), access_modifier access = public_access);

                // static method
                template <typename Ret, typename... Args>
                PrimitiveTypeBinder<Cls>& method(const utils::String& name, Ret (*method)(Args...), access_modifier access = public_access);

                // static member
                template <typename T>
                PrimitiveTypeBinder<Cls>& prop(const utils::String& name, T* member, access_modifier access = public_access, value_flag_mask flags = vf_rw);

                // static member proxy
                template <typename T>
                PrimitiveTypeBinder<Cls>& prop(const utils::String& name, T (*getter)(), T (*setter)(const T&), access_modifier access = public_access);
                template <typename T>
                PrimitiveTypeBinder<Cls>& prop(const utils::String& name, T (*getter)(), T (*setter)(T), access_modifier access = public_access);

                // fake instance member proxy
                template <typename T>
                PrimitiveTypeBinder<Cls>& prop(const utils::String& name, T (*getter)(Cls*), T (*setter)(Cls*, const T&), access_modifier access = public_access);
                template <typename T>
                PrimitiveTypeBinder<Cls>& prop(const utils::String& name, T (*getter)(Cls*), T (*setter)(Cls*, T), access_modifier access = public_access);

            private:
                template <typename T>
                bool checkNewProp(const utils::String& name) const;
        };

        template <typename Cls>
        class ObjectTypeBinder : public DataTypeBinder {
            public:
                ObjectTypeBinder(FunctionRegistry* freg, DataTypeRegistry* treg, const utils::String& name, const utils::String& fullyQualifiedName);
                ~ObjectTypeBinder();

                template <typename... Args>
                ObjectTypeBinder<Cls>& ctor(access_modifier access = public_access);

                ObjectTypeBinder<Cls>& dtor(access_modifier access = public_access);

                template <typename Ret, typename... Args>
                ObjectTypeBinder<Cls>& method(const utils::String& name, Ret (Cls::*method)(Args...), access_modifier access = public_access);

                template <typename Ret, typename... Args>
                ObjectTypeBinder<Cls>& method(const utils::String& name, Ret (Cls::*method)(Args...) const, access_modifier access = public_access);

                template <typename Ret, typename... Args>
                ObjectTypeBinder<Cls>& method(const utils::String& name, Ret (*method)(Args...), access_modifier access = public_access);

                // normal member
                template <typename T>
                ObjectTypeBinder<Cls>& prop(const utils::String& name, T Cls::*member, access_modifier access = public_access, value_flag_mask flags = vf_rw);

                // static member
                template <typename T>
                ObjectTypeBinder<Cls>& prop(const utils::String& name, T* member, access_modifier access = public_access, value_flag_mask flags = vf_rw);

                // static member proxy
                template <typename T>
                ObjectTypeBinder<Cls>& prop(const utils::String& name, T (*getter)(), T (*setter)(const T&), access_modifier access = public_access);
                template <typename T>
                ObjectTypeBinder<Cls>& prop(const utils::String& name, T (*getter)(), T (*setter)(T), access_modifier access = public_access);

                // member proxy
                template <typename T>
                ObjectTypeBinder<Cls>& prop(const utils::String& name, T (Cls::*getter)(Cls*), T (Cls::*setter)(const T&), access_modifier access = public_access);
                template <typename T>
                ObjectTypeBinder<Cls>& prop(const utils::String& name, T (Cls::*getter)(Cls*), T (Cls::*setter)(T), access_modifier access = public_access);

            private:
                template <typename T>
                bool checkNewProp(const utils::String& name) const;
        };
    };
};