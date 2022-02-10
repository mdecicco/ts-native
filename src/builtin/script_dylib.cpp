#include <gjs/builtin/script_dylib.h>
#include <gjs/builtin/script_string.h>
#include <gjs/common/script_context.h>
#include <gjs/common/type_manager.h>
#include <gjs/common/script_type.h>
#include <gjs/common/function_signature.h>
#include <gjs/common/script_function.h>
#include <gjs/common/script_module.h>
#include <gjs/common/errors.h>
#include <gjs/bind/ffi.h>

#ifdef WIN32
    #include <Windows.h>
#else
#endif

using namespace std;

namespace gjs {
    script_dylib::script_dylib(u32 module_id) : m_owner(script_context::current()->module(module_id)) {
        #ifdef WIN32
            m_lib = nullptr;
        #else
        #endif
    }

    script_dylib::~script_dylib() {
        #ifdef WIN32
            if (m_lib) {
                FreeLibrary((HMODULE)m_lib);
                m_lib = nullptr;
            }
        #else
        #endif
    }

    void script_dylib::try_load(const script_string& path) {
        #ifdef WIN32
        m_lib = (void*)LoadLibrary(path.c_str());
        if (!m_lib) {
            u32 err = GetLastError();
            char* buf = nullptr;
            size_t size = FormatMessageA(
                FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                err,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (char*)&buf,
                0,
                NULL
            );
            std::string code(buf, size);
            LocalFree(buf);

            throw error::runtime_exception(error::ecode::r_failed_to_load_dylib, path.c_str(), code.c_str());
        }
        #else
        #endif
    }

    void script_dylib::try_import(const script_string& fname, u32 sig_type_id) {
        script_type* sig = script_context::current()->types()->get(sig_type_id);
        if (!sig) {
            throw error::runtime_exception(error::ecode::r_dylib_function_invalid_signature_id, fname.c_str(), "malformed module-type id");
        }

        if (!sig->signature) {
            throw error::runtime_exception(error::ecode::r_dylib_function_invalid_signature_id, fname.c_str(), "not a function signature type");
        }

        #ifdef WIN32
            HMODULE mod = (HMODULE)m_lib;
            void* addr = GetProcAddress(mod, fname.c_str());
            if (!addr) {
                u32 err = GetLastError();
                char* buf = nullptr;
                size_t size = FormatMessageA(
                    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                    NULL,
                    err,
                    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                    (char*)&buf,
                    0,
                    NULL
                );
                std::string code(buf, size);
                LocalFree(buf);

                throw error::runtime_exception(error::ecode::r_dylib_function_not_found, sig->signature->to_string(fname).c_str(), code.c_str());
            }
            
            auto matches = m_owner->functions();
            string fnstr = fname;
            for (auto& f : matches) {
                if (f->name != fnstr) continue;
                if (f->type != sig) continue;
                if (!f->is_external) continue;

                f->access.wrapped = new ffi::cdecl_pointer(m_owner->types(), addr, fname, sig->signature);
                return;
            }

            throw error::runtime_exception(error::ecode::r_dylib_function_not_found, sig->signature->to_string(fname).c_str());
        #else
        #endif
    }
};