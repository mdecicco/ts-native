#pragma once
#include <tsn/interfaces/IContextual.h>
#include <utils/String.h>
#include <utils/Array.h>
#include <utils/robin_hood.h>

namespace tsn {
    class SourceLocation;

    namespace ffi {
        class ExecutionContext : public IContextual {
            public:
                using thread_ctx_map = robin_hood::unordered_map<utils::thread_id, utils::Array<ExecutionContext*>>;
                
                ExecutionContext(Context* ctx);
                ~ExecutionContext();

                void raiseException(const utils::String& message, const SourceLocation& src);
                bool hasException() const;
                const utils::String& getMessage() const;
                const utils::Array<SourceLocation>& getCallStack() const;

                static ExecutionContext* Get();
                static void Push(ExecutionContext* ectx);
                static void Pop();
                static void Init();
                static void Shutdown();

            private:
                bool m_exceptionCaught;
                utils::String m_exceptionMessage;
                utils::Array<SourceLocation> m_callStack;

                static thread_ctx_map* g_ectx;
        };
    };
};