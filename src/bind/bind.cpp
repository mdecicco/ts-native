#include <gjs/bind/bind.h>

namespace gjs {
    namespace ffi {
        void trivial_copy(void* dest, void* src, size_t sz) {
            memcpy(dest, src, sz);
        }

        wrapped_function::wrapped_function(script_type* ret, std::vector<script_type*> args, const std::string& _name, bool anonymous) {
            return_type = ret;
            arg_types = args;
            name = _name;
            is_static_method = false;
            func_ptr = nullptr;
            ret_is_ptr = false;
            cdecl_wrapper_func = nullptr;
            srv_wrapper_func = nullptr;
            call_method_func = nullptr;
            pass_this = false;
            is_anonymous = anonymous;
        }

        wrapped_class::property::property(wrapped_function* g, wrapped_function* s, script_type* t, u64 o, u8 f) {
            getter = g;
            setter = s;
            type = t;
            offset = o;
            flags = f;
        }

        wrapped_class::wrapped_class(const std::string& _name, const std::string& _internal_name, size_t _size) {
            name = _name;
            internal_name = _internal_name;
            size = _size;
            dtor = nullptr;
            trivially_copyable = true;
            type = nullptr;
            is_pod = false;
        }

        wrapped_class::~wrapped_class() {
            for (auto i = properties.begin();i != properties.end();++i) {
                delete i->getSecond();
            }
            properties.clear();
        }

        #define dc_func_simp(tp, cfunc) template <> void do_call<tp>(DCCallVM* call, tp* ret, void* func) { (*(tp*)ret) = (tp)cfunc(call, func); }
        dc_func_simp(f32, dcCallFloat);
        dc_func_simp(f64, dcCallDouble);
        dc_func_simp(bool, dcCallBool);
        dc_func_simp(char, dcCallChar);
        dc_func_simp(u8, dcCallChar);
        dc_func_simp(i8, dcCallChar);
        dc_func_simp(u16, dcCallShort);
        dc_func_simp(i16, dcCallShort);
        dc_func_simp(u32, dcCallInt);
        dc_func_simp(i32, dcCallInt);
        dc_func_simp(u64, dcCallLongLong);
        dc_func_simp(i64, dcCallLongLong);
        #undef dc_func_simp

        template <>
        void do_call<void>(DCCallVM* call, void* ret, void* func) {
            dcCallVoid(call, func);
        }
    };
};