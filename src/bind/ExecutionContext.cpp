#include <tsn/bind/ExecutionContext.h>
#include <utils/Array.hpp>

namespace tsn {
    namespace ffi {
        ExecutionContext::thread_ctx_map* ExecutionContext::g_ectx = nullptr;

        ExecutionContext::ExecutionContext(Context* ctx) : IContextual(ctx) {
        }

        ExecutionContext::~ExecutionContext() {
        }

        void ExecutionContext::raiseException(const utils::String& message, const SourceLocation& src) {
        }

        bool ExecutionContext::hasException() const {
            return m_exceptionCaught;
        }

        const utils::String& ExecutionContext::getMessage() const {
            return m_exceptionMessage;
        }

        const utils::Array<SourceLocation>& ExecutionContext::getCallStack() const {
            return m_callStack;
        }

        ExecutionContext* ExecutionContext::Get() {
            if (!g_ectx) return nullptr;

            utils::thread_id tid = utils::Thread::Current();
            auto it = g_ectx->find(tid);
            if (it == g_ectx->end()) return nullptr;

            auto& stack = it->second;
            if (stack.size() == 0) return nullptr;

            return stack.last();
        }
        
        void ExecutionContext::Push(ExecutionContext* ectx) {
            if (!g_ectx) return;

            utils::thread_id tid = utils::Thread::Current();
            auto it = g_ectx->find(tid);
            if (it == g_ectx->end()) {
                g_ectx->insert(thread_ctx_map::value_type(tid, utils::Array<ExecutionContext*>({ ectx })));
                return;
            }

            it->second.push(ectx);
        }
        
        void ExecutionContext::Pop() {
            if (!g_ectx) return;

            utils::thread_id tid = utils::Thread::Current();
            auto it = g_ectx->find(tid);
            if (it == g_ectx->end()) return;
            
            it->second.pop();
        }

        void ExecutionContext::Init() {
            if (g_ectx) return;
            g_ectx = new ExecutionContext::thread_ctx_map();
        }

        void ExecutionContext::Shutdown() {
            if (!g_ectx) return;
            delete g_ectx;
        }
    };
};