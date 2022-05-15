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

                void setDestructor(Function* dtor);
                void addMethod(Function* method);
                void addProperty(type_property&& prop);
                bool propNameExists(const utils::String& name) const;

                DataType* finalize(Module* mod = nullptr);

            protected:
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

                // static member
                template <typename T>
                PrimitiveTypeBinder<Cls>& prop(const utils::String& name, T* member, value_flag_mask flags = vf_rw);

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
                ObjectTypeBinder<Cls>& prop(const utils::String& name, T Cls::*member, value_flag_mask flags = vf_rw);

                // static member
                template <typename T>
                ObjectTypeBinder<Cls>& prop(const utils::String& name, T* member, value_flag_mask flags = vf_rw);

            private:
                template <typename T>
                bool checkNewProp(const utils::String& name) const;
        };
    };
};