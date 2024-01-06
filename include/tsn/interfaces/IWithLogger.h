#pragma once
#include <tsn/compiler/Logger.h>

namespace tsn {
    namespace compiler {
        class IWithLogger {
            public:
                IWithLogger(Logger* l);
                
                Logger* getLogger() const;
            
                void beginLoggerTransaction();
                void commitLoggerTransaction();
                void revertLoggerTransaction();
                log_message& submitLog(log_type type, log_message_code code, const utils::String& msg, const SourceLocation& src, ParseNode* ast);

            protected:
                Logger* m_logger;
        };
    };
};