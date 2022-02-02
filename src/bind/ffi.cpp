#include <gjs/bind/ffi.h>
#include <gjs/common/function_signature.h>
#include <gjs/common/script_type.h>
#include <gjs/common/errors.h>

namespace gjs {
    namespace ffi {
        cdecl_pointer::cdecl_pointer(type_manager* tpm, void* func, const std::string& name, function_signature* signature) :
            wrapped_function(signature->return_type, signature->arg_types(), name, false)
        {
            func_ptr = func;
            ret_is_ptr = signature->returns_pointer;

            if (signature->return_type->size > 0 && !signature->return_type->is_primitive && !ret_is_ptr) {
                throw error::bind_exception(error::ecode::b_external_struct_return);
            }

            for (u8 a = 0;a < signature->args.size();a++) {
                arg_is_ptr.push_back(signature->args[a].is_ptr);
            }
        }

        void cdecl_pointer::call(DCCallVM* cvm, void* ret, void** args) {
            dcMode(cvm, DC_CALL_C_DEFAULT);
            dcReset(cvm);

            for (u8 a = 0;a < arg_types.size();a++) {
                script_type* tp = arg_types[a];
                if (tp->is_primitive) {
                    if (tp->is_floating_point) {
                        if (tp->size == sizeof(f64)) dcArgDouble(cvm, *(f64*)&args[a]);
                        else dcArgFloat(cvm, *(f32*)&args[a]);
                    } else {
                        switch (tp->size) {
                            case sizeof(i8): {
                                if (tp->name == "bool") dcArgBool(cvm, *(bool*)&args[a]);
                                else dcArgChar(cvm, *(i8*)&args[a]);
                                break;
                            }
                            case sizeof(i16): { dcArgShort(cvm, *(i16*)&args[a]); break; }
                            case sizeof(i32): {
                                if (tp->is_unsigned) dcArgLong(cvm, *(u32*)&args[a]);
                                else dcArgInt(cvm, *(i32*)&args[a]);
                                break;
                            }
                            case sizeof(i64): { dcArgLongLong(cvm, *(i64*)&args[a]); break; }
                        }
                    }
                } else {
                    dcArgPointer(cvm, args[a]);
                }
            }

            if (return_type->size == 0) dcCallVoid(cvm, func_ptr);
            else {
                if (return_type->is_primitive) {
                    if (return_type->is_floating_point) {
                        if (return_type->size == sizeof(f64)) *(f64*)ret = dcCallDouble(cvm, func_ptr);
                        else *(f32*)ret = dcCallFloat(cvm, func_ptr);
                    } else {
                        switch (return_type->size) {
                            case sizeof(i8): {
                                if (return_type->name == "bool") *(bool*)ret = dcCallBool(cvm, func_ptr);
                                else *(i8*)ret = dcCallChar(cvm, func_ptr);
                                break;
                            }
                            case sizeof(i16): { *(i16*)ret = dcCallShort(cvm, func_ptr); break; }
                            case sizeof(i32): { *(i32*)ret = dcCallInt(cvm, func_ptr); break; }
                            case sizeof(i64): { *(i64*)ret = dcCallLongLong(cvm, func_ptr); break; }
                        }
                    }
                } else {
                    *(void**)ret = dcCallPointer(cvm, func_ptr);
                }
            }
        }
    };
};