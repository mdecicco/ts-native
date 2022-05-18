#pragma once
#include <gs/interfaces/IContextual.h>
#include <utils/String.h>
#include <utils/Array.h>

namespace gs {
    class SourceLocation;

    namespace ffi {
        class ExecutionContext : public IContextual {
            public:
                ExecutionContext(Context* ctx);
                ~ExecutionContext();

                void raiseException(const utils::String& message, const SourceLocation& src);
                bool hasException() const;
                const utils::String& getMessage() const;
                const utils::Array<SourceLocation>& getCallStack() const;
            
            private:
                bool m_exceptionCaught;
                utils::String m_exceptionMessage;
                utils::Array<SourceLocation> m_callStack;
        };
    };
};