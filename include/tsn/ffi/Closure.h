#pragma once
#include <tsn/common/types.h>
#include <tsn/interfaces/IContextual.h>
#include <tsn/ffi/Object.h>

namespace tsn {
    namespace compiler {
        class Compiler;
    };

    namespace ffi {
        class Function;
        class ExecutionContext;

        class Closure;
        class ClosureRef {
            public:
                ClosureRef();
                ClosureRef(const ClosureRef& closure);
                ClosureRef(Closure* closure);
                ~ClosureRef();

                ffi::Function* getTarget() const;
                void* getSelf() const;

                void operator=(const ClosureRef& rhs);
                void operator=(Closure* rhs);

                template <typename ...Args>
                Object call(Args&&... args);
                
            protected:
                friend class compiler::Compiler;
                void* m_captureData;
                Closure* m_ref;
        };

        class Closure : public IContextual {
            public:
                Closure(ExecutionContext* ectx, function_id targetId, void* captureData);
                Closure(const Closure& c);
                ~Closure();

                void bind(void* self);
                ffi::Function* getTarget() const;
                void* getSelf() const;

                void operator=(const Closure& rhs) = delete;

            protected:
                friend class ClosureRef;
                friend class compiler::Compiler;

                void* m_self;
                void* m_captureData;
                ffi::Function* m_target;
                u32 m_refCount;
        };
    };
};