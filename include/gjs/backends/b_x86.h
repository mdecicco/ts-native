#pragma once
#include <gjs/backends/backend.h>
#include <gjs/util/robin_hood.h>

namespace asmjit {
    class JitRuntime;
    struct FuncSignature;
    class Label;
    namespace x86 {
        class Compiler;
    };
};

namespace gjs {
    class x86_backend : public backend {
        public:
            x86_backend();
            ~x86_backend();

            virtual void generate(compilation_output& in);
            virtual u16 gp_count() const;
            virtual u16 fp_count() const;
            virtual bool needs_register_allocation() const;
            virtual void call(script_function* func, void* ret, void** args);

        protected:

            typedef robin_hood::unordered_map<script_function*, asmjit::Label*> func_label_map;
            void gen_function(compilation_output& in, u16 fidx, asmjit::x86::Compiler& cc, asmjit::FuncSignature& fs, func_label_map& flabels);

            asmjit::JitRuntime* m_rt;
    };
};