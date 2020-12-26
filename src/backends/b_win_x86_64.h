#pragma once
#include <backends/backend.h>
#include <util/robin_hood.h>

namespace gjs {
    struct w64_gen_ctx;
    class llvm_data;
    class win64_backend : public backend {
        public:
            win64_backend();
            ~win64_backend();

            virtual void init();
            void commit_bindings();

            virtual void generate(compilation_output& in);
            virtual u16 gp_count() const { return 0; }
            virtual u16 fp_count() const { return 0; }
            virtual bool perform_register_allocation() const { return false; }

            void log_ir(bool do_log_ir);

        protected:
            void generate_function(w64_gen_ctx& ctx);
            void gen_instruction(w64_gen_ctx& ctx);
            void add_bindings_to_module(w64_gen_ctx& ctx);
            virtual void call(script_function* func, void* ret, void** args);
            std::string internal_func_name(script_function* func);

            llvm_data* m_llvm;

            bool m_log_ir;
    };
};