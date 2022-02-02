#pragma once
#include <gjs/common/types.h>

namespace gjs {
    class script_module;
    class script_string;

    class script_dylib {
        public:
            script_dylib(u32 module_id);
            ~script_dylib();

            void try_load(const script_string& path);
            void try_import(const script_string& fname, u32 sig_type_id);

        protected:
            script_module* m_owner;

            #ifdef WIN32
                void* m_lib; // HMODULE
            #else
            #endif
    };
};

