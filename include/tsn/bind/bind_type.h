#pragma once
#include <tsn/common/types.h>
#include <utils/Array.h>
#include <utils/String.h>

namespace tsn {
    class Module;

    namespace compiler {
        struct InlineCodeGenContext;
        typedef void (*InlineCodeGenFunc)(compiler::InlineCodeGenContext* ctx);
    };

    namespace ffi {
        struct type_property;
        struct type_base;
        struct function_argument;
        class Function;
        class Method;
        class DataType;
        class FunctionRegistry;
        class DataTypeRegistry;

        class DataTypeBinder {
            public:
                DataTypeBinder(Module* mod, FunctionRegistry* freg, DataTypeRegistry* treg, const utils::String& name, const utils::String& fullyQualifiedName, type_meta&& meta);
                ~DataTypeBinder();

                type_meta& info();
                DataType* finalize();

            protected:
                void setAccessModifier(access_modifier access);
                void setDestructor(Function* dtor);
                void addMethod(Function* method);
                void addProperty(type_property&& prop);
                bool propNameExists(const utils::String& name) const;
                
                bool m_isFinal;
                FunctionRegistry* funcRegistry;
                DataTypeRegistry* typeRegistry;
                DataType* m_type;
                Module* m_mod;
        };

        class DataTypeExtender {
            public:
                DataTypeExtender(Module* mod, FunctionRegistry* freg, DataTypeRegistry* treg, DataType* type);
                DataTypeExtender(Module* mod, FunctionRegistry* freg, DataTypeRegistry* treg, type_meta&& meta);
                ~DataTypeExtender();

                void addMethod(Function* method);
                void setDestructor(Function* dtor);
                void setDestructor(compiler::InlineCodeGenFunc genFn, access_modifier access = public_access);
                void addProperty(type_property&& prop);
                type_property& addProperty(const utils::String& name, DataType* type, u64 offset, value_flag_mask flags = vf_rw, access_modifier access = public_access);
                type_property& addProperty(const utils::String& name, DataType* type, Function* getter, Function* setter, access_modifier access = public_access);
                type_property& addProperty(const utils::String& name, DataType* type, compiler::InlineCodeGenFunc getterGenFn, compiler::InlineCodeGenFunc setterGenFn, access_modifier access = public_access);
                bool propNameExists(const utils::String& name) const;


                Method* addMethod(const utils::String& name, DataType* retTp, bool returnsPointer, const utils::Array<function_argument>& args, compiler::InlineCodeGenFunc genFn, access_modifier access = public_access);
                Function* addStaticMethod(const utils::String& name, DataType* retTp, bool returnsPointer, const utils::Array<function_argument>& args, compiler::InlineCodeGenFunc genFn, access_modifier access = public_access);
                
                template <typename Ret, typename... Args>
                Function* addMethod(const utils::String& name, compiler::InlineCodeGenFunc genFn, access_modifier access = public_access);
                template <typename Ret, typename... Args>
                Function* addStaticMethod(const utils::String& name, compiler::InlineCodeGenFunc genFn, access_modifier access = public_access);

                type_meta& info();

            protected:
                FunctionRegistry* funcRegistry;
                DataTypeRegistry* typeRegistry;
                DataType* m_type;
                Module* m_mod;
        };

        template <typename Cls>
        class PrimitiveTypeBinder : public DataTypeBinder {
            public:
                PrimitiveTypeBinder(Module* mod, FunctionRegistry* freg, DataTypeRegistry* treg, const utils::String& name, const utils::String& fullyQualifiedName);
                ~PrimitiveTypeBinder();

                PrimitiveTypeBinder<Cls>& access(access_modifier access);

                // fake instance method
                template <typename Ret, typename... Args>
                PrimitiveTypeBinder<Cls>& method(const utils::String& name, Ret (*method)(Cls*, Args...), access_modifier access = public_access);
                template <typename Ret, typename... Args>
                PrimitiveTypeBinder<Cls>& method(const utils::String& name, compiler::InlineCodeGenFunc genFn, access_modifier access = public_access);

                // static method
                template <typename Ret, typename... Args>
                PrimitiveTypeBinder<Cls>& staticMethod(const utils::String& name, Ret (*method)(Args...), access_modifier access = public_access);
                template <typename Ret, typename... Args>
                PrimitiveTypeBinder<Cls>& staticMethod(const utils::String& name, compiler::InlineCodeGenFunc genFn, access_modifier access = public_access);

                // fake instance member proxy
                template <typename T>
                PrimitiveTypeBinder<Cls>& prop(const utils::String& name, T (*getter)(Cls*), T (*setter)(Cls*, const T&), access_modifier access = public_access);
                template <typename T>
                PrimitiveTypeBinder<Cls>& prop(const utils::String& name, T (*getter)(Cls*), T (*setter)(Cls*, T), access_modifier access = public_access);
                template <typename T>
                PrimitiveTypeBinder<Cls>& prop(const utils::String& name, compiler::InlineCodeGenFunc getterGenFn, compiler::InlineCodeGenFunc setterGenFn, access_modifier access = public_access);

                // static member
                template <typename T>
                PrimitiveTypeBinder<Cls>& staticProp(const utils::String& name, T* member, access_modifier access = public_access, value_flag_mask flags = vf_rw);

                // static member proxy
                template <typename T>
                PrimitiveTypeBinder<Cls>& staticProp(const utils::String& name, T (*getter)(), T (*setter)(const T&), access_modifier access = public_access);
                template <typename T>
                PrimitiveTypeBinder<Cls>& staticProp(const utils::String& name, T (*getter)(), T (*setter)(T), access_modifier access = public_access);
                template <typename T>
                PrimitiveTypeBinder<Cls>& staticProp(const utils::String& name, compiler::InlineCodeGenFunc getterGenFn, compiler::InlineCodeGenFunc setterGenFn, access_modifier access = public_access);

            private:
                template <typename T>
                bool checkNewProp(const utils::String& name) const;
        };

        template <typename Cls>
        class PrimitiveTypeExtender : public DataTypeExtender {
            public:
                PrimitiveTypeExtender(Module* mod, FunctionRegistry* freg, DataTypeRegistry* treg);
                ~PrimitiveTypeExtender();

                // fake instance method
                template <typename Ret, typename... Args>
                PrimitiveTypeExtender<Cls>& method(const utils::String& name, Ret (*method)(Cls*, Args...), access_modifier access = public_access);
                template <typename Ret, typename... Args>
                PrimitiveTypeExtender<Cls>& method(const utils::String& name, compiler::InlineCodeGenFunc genFn, access_modifier access = public_access);

                // static method
                template <typename Ret, typename... Args>
                PrimitiveTypeExtender<Cls>& staticMethod(const utils::String& name, Ret (*method)(Args...), access_modifier access = public_access);
                template <typename Ret, typename... Args>
                PrimitiveTypeExtender<Cls>& staticMethod(const utils::String& name, compiler::InlineCodeGenFunc genFn, access_modifier access = public_access);

                // fake instance member proxy
                template <typename T>
                PrimitiveTypeExtender<Cls>& prop(const utils::String& name, T (*getter)(Cls*), T (*setter)(Cls*, const T&), access_modifier access = public_access);
                template <typename T>
                PrimitiveTypeExtender<Cls>& prop(const utils::String& name, T (*getter)(Cls*), T (*setter)(Cls*, T), access_modifier access = public_access);
                template <typename T>
                PrimitiveTypeExtender<Cls>& prop(const utils::String& name, compiler::InlineCodeGenFunc getterGenFn, compiler::InlineCodeGenFunc setterGenFn, access_modifier access = public_access);

                // static member
                template <typename T>
                PrimitiveTypeExtender<Cls>& staticProp(const utils::String& name, T* member, access_modifier access = public_access, value_flag_mask flags = vf_rw);

                // static member proxy
                template <typename T>
                PrimitiveTypeExtender<Cls>& staticProp(const utils::String& name, T (*getter)(), T (*setter)(const T&), access_modifier access = public_access);
                template <typename T>
                PrimitiveTypeExtender<Cls>& staticProp(const utils::String& name, T (*getter)(), T (*setter)(T), access_modifier access = public_access);
                template <typename T>
                PrimitiveTypeExtender<Cls>& staticProp(const utils::String& name, compiler::InlineCodeGenFunc getterGenFn, compiler::InlineCodeGenFunc setterGenFn, access_modifier access = public_access);
                
            private:
                template <typename T>
                bool checkNewProp(const utils::String& name) const;
        };

        template <typename Cls>
        class ObjectTypeBinder : public DataTypeBinder {
            public:
                ObjectTypeBinder(Module* mod, FunctionRegistry* freg, DataTypeRegistry* treg, const utils::String& name, const utils::String& fullyQualifiedName);
                ~ObjectTypeBinder();

                ObjectTypeBinder<Cls>& access(access_modifier access);

                template <typename... Args>
                ObjectTypeBinder<Cls>& ctor(access_modifier access = public_access);
                template <typename... Args>
                ObjectTypeBinder<Cls>& ctor(compiler::InlineCodeGenFunc genFn, access_modifier access = public_access);

                ObjectTypeBinder<Cls>& dtor(access_modifier access = public_access);
                ObjectTypeBinder<Cls>& dtor(compiler::InlineCodeGenFunc genFn, access_modifier access = public_access);

                // fake instance method
                template <typename Ret, typename... Args>
                ObjectTypeBinder<Cls>& method(const utils::String& name, Ret (*method)(Cls*, Args...), access_modifier access = public_access);

                template <typename Ret, typename... Args>
                ObjectTypeBinder<Cls>& method(const utils::String& name, Ret (Cls::*method)(Args...), access_modifier access = public_access);

                template <typename Ret, typename... Args>
                ObjectTypeBinder<Cls>& method(const utils::String& name, Ret (Cls::*method)(Args...) const, access_modifier access = public_access);

                template <typename Ret, typename... Args>
                ObjectTypeBinder<Cls>& method(const utils::String& name, compiler::InlineCodeGenFunc genFn, access_modifier access = public_access);

                template <typename Ret, typename... Args>
                ObjectTypeBinder<Cls>& staticMethod(const utils::String& name, Ret (*method)(Args...), access_modifier access = public_access);

                template <typename Ret, typename... Args>
                ObjectTypeBinder<Cls>& staticMethod(const utils::String& name, compiler::InlineCodeGenFunc genFn, access_modifier access = public_access);

                // normal member
                template <typename T>
                ObjectTypeBinder<Cls>& prop(const utils::String& name, T Cls::*member, access_modifier access = public_access, value_flag_mask flags = vf_rw);
                
                // member proxy
                template <typename T>
                ObjectTypeBinder<Cls>& prop(const utils::String& name, T (Cls::*getter)(), T (Cls::*setter)(const T&), access_modifier access = public_access);
                template <typename T>
                ObjectTypeBinder<Cls>& prop(const utils::String& name, T (Cls::*getter)() const, T (Cls::*setter)(const T&), access_modifier access = public_access);
                template <typename T>
                ObjectTypeBinder<Cls>& prop(const utils::String& name, T (Cls::*getter)(), T (Cls::*setter)(const T&) const, access_modifier access = public_access);
                template <typename T>
                ObjectTypeBinder<Cls>& prop(const utils::String& name, T (Cls::*getter)() const, T (Cls::*setter)(const T&) const, access_modifier access = public_access);
                template <typename T>
                ObjectTypeBinder<Cls>& prop(const utils::String& name, T (Cls::*getter)(), T (Cls::*setter)(T), access_modifier access = public_access);
                template <typename T>
                ObjectTypeBinder<Cls>& prop(const utils::String& name, T (Cls::*getter)() const, T (Cls::*setter)(T), access_modifier access = public_access);
                template <typename T>
                ObjectTypeBinder<Cls>& prop(const utils::String& name, T (Cls::*getter)(), T (Cls::*setter)(T) const, access_modifier access = public_access);
                template <typename T>
                ObjectTypeBinder<Cls>& prop(const utils::String& name, T (Cls::*getter)() const, T (Cls::*setter)(T) const, access_modifier access = public_access);
                template <typename T>
                ObjectTypeBinder<Cls>& prop(const utils::String& name, T (Cls::*getter)(), access_modifier access = public_access);
                template <typename T>
                ObjectTypeBinder<Cls>& prop(const utils::String& name, T (Cls::*getter)() const, access_modifier access = public_access);
                template <typename T>
                ObjectTypeBinder<Cls>& prop(const utils::String& name, compiler::InlineCodeGenFunc getterGenFn, compiler::InlineCodeGenFunc setterGenFn, access_modifier access = public_access);
            
                // static member
                template <typename T>
                ObjectTypeBinder<Cls>& staticProp(const utils::String& name, T* member, access_modifier access = public_access, value_flag_mask flags = vf_rw);

                // static member proxy
                template <typename T>
                ObjectTypeBinder<Cls>& staticProp(const utils::String& name, T (*getter)(), T (*setter)(const T&), access_modifier access = public_access);
                template <typename T>
                ObjectTypeBinder<Cls>& staticProp(const utils::String& name, T (*getter)(), T (*setter)(T), access_modifier access = public_access);
                template <typename T>
                ObjectTypeBinder<Cls>& staticProp(const utils::String& name, T (*getter)(), access_modifier access = public_access);
                template <typename T>
                ObjectTypeBinder<Cls>& staticProp(const utils::String& name, compiler::InlineCodeGenFunc getterGenFn, compiler::InlineCodeGenFunc setterGenFn, access_modifier access = public_access);

            private:
                template <typename T>
                bool checkNewProp(const utils::String& name) const;
        };
    
        template <typename Cls>
        class ObjectTypeExtender : public DataTypeExtender {
            public:
                ObjectTypeExtender(Module* mod, FunctionRegistry* freg, DataTypeRegistry* treg);
                ~ObjectTypeExtender();
                
                // fake instance method
                template <typename Ret, typename... Args>
                ObjectTypeExtender<Cls>& method(const utils::String& name, Ret (*method)(Cls*, Args...), access_modifier access = public_access);
                
                template <typename Ret, typename... Args>
                ObjectTypeExtender<Cls>& method(const utils::String& name, Ret (Cls::*method)(Args...), access_modifier access = public_access);

                template <typename Ret, typename... Args>
                ObjectTypeExtender<Cls>& method(const utils::String& name, Ret (Cls::*method)(Args...) const, access_modifier access = public_access);

                template <typename Ret, typename... Args>
                ObjectTypeExtender<Cls>& method(const utils::String& name, compiler::InlineCodeGenFunc genFn, access_modifier access = public_access);

                template <typename Ret, typename... Args>
                ObjectTypeExtender<Cls>& staticMethod(const utils::String& name, Ret (*method)(Args...), access_modifier access = public_access);

                template <typename Ret, typename... Args>
                ObjectTypeExtender<Cls>& staticMethod(const utils::String& name, compiler::InlineCodeGenFunc genFn, access_modifier access = public_access);
                
            
                // normal member
                template <typename T>
                ObjectTypeExtender<Cls>& prop(const utils::String& name, T Cls::*member, access_modifier access = public_access, value_flag_mask flags = vf_rw);
                
                // member proxy
                template <typename T>
                ObjectTypeExtender<Cls>& prop(const utils::String& name, T (Cls::*getter)(), T (Cls::*setter)(const T&), access_modifier access = public_access);
                template <typename T>
                ObjectTypeExtender<Cls>& prop(const utils::String& name, T (Cls::*getter)() const, T (Cls::*setter)(const T&), access_modifier access = public_access);
                template <typename T>
                ObjectTypeExtender<Cls>& prop(const utils::String& name, T (Cls::*getter)(), T (Cls::*setter)(const T&) const, access_modifier access = public_access);
                template <typename T>
                ObjectTypeExtender<Cls>& prop(const utils::String& name, T (Cls::*getter)() const, T (Cls::*setter)(const T&) const, access_modifier access = public_access);
                template <typename T>
                ObjectTypeExtender<Cls>& prop(const utils::String& name, T (Cls::*getter)(), T (Cls::*setter)(T), access_modifier access = public_access);
                template <typename T>
                ObjectTypeExtender<Cls>& prop(const utils::String& name, T (Cls::*getter)() const, T (Cls::*setter)(T), access_modifier access = public_access);
                template <typename T>
                ObjectTypeExtender<Cls>& prop(const utils::String& name, T (Cls::*getter)(), T (Cls::*setter)(T) const, access_modifier access = public_access);
                template <typename T>
                ObjectTypeExtender<Cls>& prop(const utils::String& name, T (Cls::*getter)() const, T (Cls::*setter)(T) const, access_modifier access = public_access);
                template <typename T>
                ObjectTypeExtender<Cls>& prop(const utils::String& name, T (Cls::*getter)(), access_modifier access = public_access);
                template <typename T>
                ObjectTypeExtender<Cls>& prop(const utils::String& name, T (Cls::*getter)() const, access_modifier access = public_access);
                template <typename T>
                ObjectTypeExtender<Cls>& prop(const utils::String& name, compiler::InlineCodeGenFunc getterGenFn, compiler::InlineCodeGenFunc setterGenFn, access_modifier access = public_access);
            
                // static member
                template <typename T>
                ObjectTypeExtender<Cls>& staticProp(const utils::String& name, T* member, access_modifier access = public_access, value_flag_mask flags = vf_rw);

                // static member proxy
                template <typename T>
                ObjectTypeExtender<Cls>& staticProp(const utils::String& name, T (*getter)(), T (*setter)(const T&), access_modifier access = public_access);
                template <typename T>
                ObjectTypeExtender<Cls>& staticProp(const utils::String& name, T (*getter)(), T (*setter)(T), access_modifier access = public_access);
                template <typename T>
                ObjectTypeExtender<Cls>& staticProp(const utils::String& name, T (*getter)(), access_modifier access = public_access);
                template <typename T>
                ObjectTypeExtender<Cls>& staticProp(const utils::String& name, compiler::InlineCodeGenFunc getterGenFn, compiler::InlineCodeGenFunc setterGenFn, access_modifier access = public_access);

            private:
                template <typename T>
                bool checkNewProp(const utils::String& name) const;
        };
    };
};