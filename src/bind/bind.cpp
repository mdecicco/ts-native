#include <bind/bind.h>

namespace gjs {
    namespace bind {
        wrapped_class::~wrapped_class() {
            for (auto i = properties.begin();i != properties.end();++i) {
                delete i->getSecond();
            }
            properties.clear();
        }

        #define dc_func_simp(tp, cfunc) template <> void do_call<tp>(DCCallVM* call, tp* ret, void* func) { (*(tp*)ret) = (tp)cfunc(call, func); }
        dc_func_simp(f32, dcCallFloat);
        dc_func_simp(f64, dcCallDouble);
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