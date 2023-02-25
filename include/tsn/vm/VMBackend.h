#pragma once
#include <tsn/common/types.h>
#include <tsn/compiler/types.h>
#include <tsn/vm/types.h>
#include <tsn/interfaces/IBackend.h>
#include <tsn/interfaces/IFunctionHolder.h>
#include <tsn/utils/SourceMap.h>

#include <utils/Array.h>
#include <utils/robin_hood.h>

namespace utils {
    template <typename T>
    class PerThreadObject;
};

namespace tsn {
    namespace compiler {
        class FunctionDef;
        class Instruction;
        class CodeHolder;
    };

    namespace ffi {
        class FunctionType;
    };

    namespace vm {
        class Instruction;
        class VM;

        struct arg_location {
            vm_register reg_id;
            u32 stack_offset;
            u32 stack_size;
        };

        struct function_data {
            u32 begin;
            u32 end;
        };

        class Backend : public backend::IBackend, public IFunctionHolder {
            public:
                Backend(Context* ctx, u32 stackSize);
                ~Backend();
                
                virtual void beforeCompile(Pipeline* input);
                virtual void generate(Pipeline* input);
                void generate(compiler::CodeHolder* ch);
                
                virtual void call(ffi::Function* func, ffi::ExecutionContext* ctx, void* retPtr, void** args);

                VM* getVM();
                const utils::Array<Instruction>& getCode() const;
                const function_data* getFunctionData(ffi::Function* func) const;
                const SourceMap& getSourceMap() const;
            
            protected:
                void calcArgLifetimesAndBackupRegs(
                    const utils::Array<arg_location>& argLocs,
                    compiler::CodeHolder* ch,
                    robin_hood::unordered_flat_set<vm_register>& assignedNonVolatile,
                    robin_hood::unordered_map<vm_register, address>& argLifetimeEnds
                );

                utils::Array<Instruction> m_instructions;
                utils::PerThreadObject<VM>* m_vms;
                robin_hood::unordered_map<ffi::Function*, function_data> m_funcData;
                SourceMap m_map;
        };

        void getArgRegisters(ffi::FunctionType* sig, utils::Array<arg_location>& out);
    };
};