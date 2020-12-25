#pragma once
#include <backends/backend.h>
#include <util/robin_hood.h>

#include <llvm/ExecutionEngine/Orc/ThreadSafeModule.h>

namespace llvm {
    class Module;
    class Type;
    class InitLLVM;

    namespace orc {
        class LLJIT;
    };
};

namespace gjs {
    struct w64_gen_ctx;
    class win64_backend : public backend {
        public:
            win64_backend(int argc, char* argv[]);
            ~win64_backend();

            virtual void init();
            void commit_bindings();

            virtual void generate(compilation_output& in);
            virtual u16 gp_count() const { return 0; }
            virtual u16 fp_count() const { return 0; }
            virtual bool perform_register_allocation() const;

            void log_ir(bool do_log_ir);

        protected:
            void generate_function(w64_gen_ctx& ctx);
            void gen_instruction(w64_gen_ctx& ctx);
            void add_bindings_to_module(w64_gen_ctx& ctx);
            virtual void call(script_function* func, void* ret, void** args);
            std::string internal_func_name(script_function* func);

            llvm::InitLLVM* m_llvm_init;
            llvm::orc::ThreadSafeContext m_llvm;
            std::unique_ptr<llvm::orc::LLJIT> m_jit;
            robin_hood::unordered_map<std::string, llvm::orc::ThreadSafeModule> m_modules;
            robin_hood::unordered_map<u64, llvm::Type*> m_types;

            bool m_log_ir;
    };
};