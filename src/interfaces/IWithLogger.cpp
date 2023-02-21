#include <tsn/interfaces/IWithLogger.h>

namespace tsn {
    namespace compiler {
        IWithLogger::IWithLogger(Logger* l) {
            m_logger = l;
        }
    
        void IWithLogger::beginLoggerTransaction() {
            m_logger->begin();
        }

        void IWithLogger::commitLoggerTransaction() {
            m_logger->commit();
        }

        void IWithLogger::revertLoggerTransaction() {
            m_logger->revert();
        }

        log_message& IWithLogger::submitLog(log_type type, log_message_code code, const utils::String& msg, const SourceLocation& src, ParseNode* ast) {
            return m_logger->submit(type, code, msg, src, ast);
        }
    };
};