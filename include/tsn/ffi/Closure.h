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

        class CaptureData;
        class Closure {
            public:
                Closure();
                Closure(const Closure& closure);
                Closure(CaptureData* closure);
                ~Closure();

                ffi::Function* getTarget() const;
                CaptureData* getCaptures() const;
                void* getSelf() const;

                void operator=(const Closure& rhs);
                void operator=(CaptureData* rhs);
                
            protected:
                friend class compiler::Compiler;
                void* m_captureData;
                CaptureData* m_ref;
        };

        class CaptureData {
            public:
                CaptureData(ExecutionContext* ectx, function_id targetId, void* captureData);
                CaptureData(const CaptureData& c);
                ~CaptureData();

                void bind(void* self);
                ffi::Function* getTarget() const;
                Context* getContext() const;
                void* getSelf() const;

                void operator=(const CaptureData& rhs) = delete;

            protected:
                friend class Closure;
                friend class compiler::Compiler;

                void* m_self;
                void* m_captureData;
                ffi::Function* m_target;
                u32 m_refCount;
                Context* m_ctx;
        };

        template <typename Ret, typename ...Args>
        class Callable : public Closure {
            public:
                Object operator()(Args&&... args) const;
        };
    };
};
