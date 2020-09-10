#pragma once
#include <common/types.h>
#include <bind/builtin.h>
#include <util/template_utils.hpp>
#include <util/robin_hood.h>

#include <vector>
#include <typeindex>
#include <tuple>
#include <string>


namespace gjs {
    class vm_backend;
    class vm_function;
    class vm_type;
    class type_manager;

    class bind_exception : public std::exception {
        public:
            bind_exception(const std::string& _text) : text(_text) { }
            ~bind_exception() { }

            virtual char const* what() const { return text.c_str(); }

            std::string text;
    };

    namespace bind {
        struct wrapped_function {
            wrapped_function(std::type_index ret, std::vector<std::type_index> args, const std::string& _name)
            : return_type(ret), arg_types(args), name(_name), is_static_method(false), address(0), ret_is_ptr(false) { }

            std::string name;
            std::type_index return_type;
            std::vector<std::type_index> arg_types;
            std::vector<bool> arg_is_ptr;
            bool is_static_method;

            bool ret_is_ptr;
            u64 address;
        };

        struct wrapped_class {
            struct property {
                property(wrapped_function* g, wrapped_function* s, std::type_index t, u64 o, u8 f) :
                    getter(g), setter(s), type(t), offset(o), flags(f) { }

                wrapped_function* getter;
                wrapped_function* setter;
                std::type_index type;
                u64 offset;
                u8 flags;
            };

            wrapped_class(const std::string& _name, const std::string& _internal_name, size_t _size) :
                name(_name), internal_name(_internal_name), size(_size), dtor(nullptr), requires_subtype(false)
            {
            }

            ~wrapped_class();

            std::string name;
            std::string internal_name;
            bool requires_subtype;
            std::vector<wrapped_function*> methods;
            robin_hood::unordered_map<std::string, property*> properties;
            wrapped_function* dtor;
            size_t size;
        };

        // Call class method on object
        // Only visible to class methods with non-void return
        template <typename Ret, typename Cls, typename... Args>
        typename std::enable_if<!std::is_same<Ret, void>::value, Ret>::type
        call_class_method(Ret(Cls::*method)(Args...), Cls* self, Args... args) {
            return (*self.*method)(args...);
        }

        // Call class method on object
        // Only visible to class methods with void return
        template <typename Ret, typename Cls, typename... Args>
        typename std::enable_if<std::is_same<Ret, void>::value, Ret>::type
        call_class_method(Ret(Cls::*method)(Args...), Cls* self, Args... args) {
            (*self.*method)(args...);
        }

        template <typename Ret, typename... Args>
        struct global_function : wrapped_function {
            typedef Ret (*func_type)(Args...);
            typedef std::tuple_size<std::tuple<Args...>> arg_count;

            global_function(type_manager* tpm, func_type f, const std::string& name) :
                wrapped_function(
                    typeid(remove_all<Ret>::type),
                    { typeid(remove_all<Args>::type)... },
                    name
                ),
                original_func(f)
            {
                if (!tpm->get<Ret>()) {
                    throw bind_exception(format("Return type '%s' of function '%s' has not been bound yet", base_type_name<Ret>(), name.c_str()));
                }
                // describe the function for the wrapped_function interface
                ret_is_ptr = std::is_reference_v<Ret> || std::is_pointer_v<Ret>;
                arg_is_ptr = { (std::is_reference_v<Args> || std::is_pointer_v<Args>)... };
                address = u64(reinterpret_cast<void*>(f));
            }

            func_type original_func;
            void (*func)(void*, void**);
        };

        template <typename Ret, typename Cls, typename... Args>
        struct class_method : wrapped_function {
            public:
                typedef Ret (Cls::*method_type)(Args...);
                typedef Ret (*func_type)(method_type, Cls*, Args...);
                typedef std::tuple_size<std::tuple<Args...>> ac;

                class_method(type_manager* tpm, method_type f, const std::string& name) :
                    wrapped_function(
                        typeid(remove_all<Ret>::type),
                        { typeid(remove_all<Cls>::type), typeid(remove_all<Args>::type)... },
                        name
                    ),
                    original_func(call_class_method<Ret, Cls, Args...>)
                {
                    if (!tpm->get<Cls>()) {
                        throw bind_exception(format("Binding method '%s' of class '%s' that has not been bound yet", name.c_str(), typeid(remove_all<Cls>::type).name()));
                    }
                    if (!tpm->get<Ret>()) {
                        throw bind_exception(format("Return type '%s' of method '%s' of class '%s' has not been bound yet", base_type_name<Ret>(), name.c_str(), typeid(remove_all<Cls>::type).name()));
                    }

                    // describe the function for the wrapped_function interface
                    ret_is_ptr = std::is_reference_v<Ret> || std::is_pointer_v<Ret>;
                    arg_is_ptr = { true, (std::is_reference_v<Args> || std::is_pointer_v<Args>)... };
                    address = *(u64*)reinterpret_cast<void*>(&f);
                }

                func_type original_func;
                void (*func)(void*, void**);
        };

        /*
         * Function wrapping helper
         */
        template <typename Ret, typename... Args>
        wrapped_function* wrap(type_manager* tpm, const std::string& name, Ret(*func)(Args...)) {
            return new global_function<Ret, Args...>(tpm, func, name);
        };

        template <typename Ret, typename Cls, typename... Args>
        wrapped_function* wrap(type_manager* tpm, const std::string& name, Ret(Cls::*func)(Args...)) {
            return new class_method<Ret, Cls, Args...>(tpm, func, name);
        };


        /*
         * Class wrapping helper
         */
        template <typename Cls, typename... Args>
        Cls* construct_object(void* mem, Args... args) {
            return new (mem) Cls(args...);
        }

        template <typename Cls>
        void destruct_object(Cls* obj) {
            obj->~Cls();
        }

        template <typename Cls, typename... Args>
        wrapped_function* wrap_constructor(type_manager* tpm, const std::string& name) {
            return wrap(tpm, name + "::constructor", construct_object<Cls, Args...>);
        }

        template <typename Cls>
        wrapped_function* wrap_destructor(type_manager* tpm, const std::string& name) {
            return wrap(tpm, name + "::destructor", destruct_object<Cls>);
        }

        enum property_flags {
            pf_none             = 0b00000000,
            pf_read_only        = 0b00000001,
            pf_write_only       = 0b00000010,
            pf_pointer          = 0b00000100,
            pf_static           = 0b00001000
        };

        template <typename Cls>
        struct wrap_class : wrapped_class {
            wrap_class(type_manager* tpm, const std::string& name) : wrapped_class(name, typeid(remove_all<Cls>::type).name(), sizeof(remove_all<Cls>::type)), types(tpm) {
                vm_type* tp = tpm->add(name, typeid(remove_all<Cls>::type).name());
                tp->is_host = true;
            }

            template <typename... Args, std::enable_if_t<sizeof...(Args) != 0, int> = 0>
            wrap_class& constructor() {
                requires_subtype = std::is_same_v<std::tuple_element_t<0, std::tuple<Args...>>, vm_type*>;
                methods.push_back(wrap_constructor<Cls, Args...>(types, name));
                if (!dtor) dtor = wrap_destructor<Cls>(types, name);
                return *this;
            }

            template <typename... Args, std::enable_if_t<sizeof...(Args) == 0, int> = 0>
            wrap_class& constructor() {
                methods.push_back(wrap_constructor<Cls, Args...>(types, name));
                if (!dtor) dtor = wrap_destructor<Cls>(types, name);
                return *this;
            }

            template <typename Ret, typename... Args>
            wrap_class& method(const std::string& _name, Ret(Cls::*func)(Args...)) {
                methods.push_back(wrap(types, name + "::" + _name, func));
                return *this;
            }

            template <typename Ret, typename... Args>
            wrap_class& method(const std::string& _name, Ret(*func)(Args...)) {
                methods.push_back(wrap(types, name + "::" + _name, func));
                methods[methods.size() - 1]->is_static_method = true;
                return *this;
            }

            template <typename T>
            wrap_class& prop(const std::string& _name, T Cls::*member, u8 flags = property_flags::pf_none) {
                if (properties.find(_name) != properties.end()) {
                    throw bind_exception(format("Property '%s' already bound to type '%s'", _name.c_str(), name.c_str()));
                }

                if (!types->get<T>()) {
                    throw bind_exception(format("Attempting to bind property of type '%s' that has not been bound itself", typeid(remove_all<T>::type).name()));
                }

                u32 offset = (u8*)&((Cls*)nullptr->*member) - (u8*)nullptr;
                properties[_name] = new property(nullptr, nullptr, typeid(remove_all<T>::type), offset, flags);
                return *this;
            }

            template <typename T>
            wrap_class& prop(const std::string& _name, T *member, u8 flags = property_flags::pf_none) {
                if (properties.find(_name) != properties.end()) {
                    throw bind_exception(format("Property '%s' already bound to type '%s'", _name.c_str(), name.c_str()));
                }

                if (!types->get<T>()) {
                    throw bind_exception(format("Attempting to bind property of type '%s' that has not been bound itself", typeid(remove_all<T>::type).name()));
                }

                properties[_name] = new property(nullptr, nullptr, typeid(remove_all<T>::type), (u64)member, flags | pf_static);
                return *this;
            }

            template <typename T>
            wrap_class& prop(const std::string& _name, T(Cls::*getter)(), T(Cls::*setter)(T), u8 flags = property_flags::pf_none) {
                if (properties.find(_name) != properties.end()) {
                    throw bind_exception(format("Property '%s' already bound to type '%s'", _name.c_str(), name.c_str()));
                }

                if (!types->get<T>()) {
                    throw bind_exception(format("Attempting to bind property of type '%s' that has not been bound itself", typeid(remove_all<T>::type).name()));
                }

                properties[_name] = new property(
                    wrap(types, name + "::get_" + _name, getter),
                    wrap(types, name + "::set_" + _name, setter),
                    typeid(remove_all<T>::type),
                    0,
                    flags
                );
                return *this;
            }


            template <typename T>
            wrap_class& prop(const std::string& _name, T(Cls::*getter)(), u8 flags = property_flags::pf_none) {
                if (properties.find(_name) != properties.end()) {
                    throw bind_exception(format("Property '%s' already bound to type '%s'", _name.c_str(), name.c_str()));
                }

                if (!types->get<T>()) {
                    throw bind_exception(format("Attempting to bind property of type '%s' that has not been bound itself", typeid(remove_all<T>::type).name()));
                }

                properties[_name] = new property(
                    wrap(types, name + "::get_" + _name, getter),
                    nullptr,
                    typeid(remove_all<T>::type),
                    0,
                    flags
                );
                return *this;
            }

            vm_type* finalize() {
                return types->finalize_class(this);
            }

            type_manager* types;
        };
    };
};
#undef fimm