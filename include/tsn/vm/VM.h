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
        class ClosureRef;
    };

    namespace vm {
        class Instruction;

        class VM : public IContextual {
            public:
                VM(Context* ctx, u32 stackSize);
                ~VM();

                void execute(const utils::Array<Instruction>& code, address entry, bool nested);

                bool isExecuting() const;

                State state;

            protected:
                void call_external(ffi::Function* fn);
                
                u32 m_stackSize;
        };
    };
};