#include <gjs/vm/vm_state.h>
#include <gjs/vm/instruction_array.h>
#include <gjs/vm/vm.h>
#include <gjs/backends/backend.h>
#include <gjs/common/source_map.h>
#include <gjs/util/robin_hood.h>

namespace gjs {
    class script_context;
    class vm_allocator;

    class vm_backend : public backend {
        public:
            vm_backend(vm_allocator* alloc, u32 stack_size, u32 mem_size);
            ~vm_backend();

            inline vm_state*            state    () { return &m_vm.state; }
            inline instruction_array*   code     () { return &m_instructions; }
            inline source_map*          map      () { return &m_map; }
            inline gjs::vm*             vm       () { return &m_vm; }
            inline vm_allocator*        allocator() { return m_alloc; }

            inline bool is_executing    () const { return m_is_executing; }
            inline bool log_instructions() const { return m_log_instructions; }
            inline void log_instructions(bool doLog) { m_log_instructions = doLog; }

            void execute(address entry);

            virtual void generate(compilation_output& in);
            virtual u16 gp_count() const;
            virtual u16 fp_count() const;
            virtual bool perform_register_allocation() const;

        protected:
            virtual void call(script_function* func, void* ret, void** args);
            typedef robin_hood::unordered_map<u64, u64> tac_map;
            typedef robin_hood::unordered_map<u64, script_function*> jal_map;
            void gen_function(compilation_output& in, tac_map& tmap, jal_map& jmap, u16 fidx);

            vm_allocator* m_alloc;
            gjs::vm m_vm;
            instruction_array m_instructions;
            source_map m_map;
            bool m_is_executing;
            bool m_log_instructions;
    };

    class vm_exception : public std::exception {
        public:
            vm_exception(vm_backend* ctx, const std::string& text);
            vm_exception(const std::string& text);
            ~vm_exception();

            virtual char const* what() const { return text.c_str(); }

            bool raised_from_script;
            std::string module;
            std::string lineText;
            std::string text;
            u32 line;
            u32 col;
    };
};