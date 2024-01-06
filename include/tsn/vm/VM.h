#include <tsn/common/types.h>
#include <tsn/vm/types.h>
#include <tsn/vm/State.h>
#include <tsn/interfaces/IContextual.h>

namespace utils {
    template <typename T>
    class Array;
}

namespace tsn {
    namespace ffi {
        class Function;
        class Closure;
    };

    namespace vm {
        class Instruction;

        class VM : public IContextual {
            public:
                VM(Context* ctx, u32 stackSize);
                ~VM();

                bool isExecuting() const;

                void prepareState();
                void execute(const utils::Array<Instruction>& code, address entry);

                State state;

            protected:
                void executeInternal(const utils::Array<Instruction>& code, address entry);
                void call_external(ffi::Function* fn);
                
                u32 m_stackSize;
                u32 m_executionNestLevel;
        };
    };
};