#include <tsn/compiler/Logger.h>
#include <tsn/compiler/Parser.h>

#include <utils/Array.hpp>

namespace tsn {
    namespace compiler {
        Logger::Logger() {
        }

        Logger::~Logger() {
            m_messages.each([](log_message& m) {
                if (m.node) ParseNode::destroyDetachedAST(m.node);
            });
        }

        void Logger::begin() {
            m_logCountStack.push(m_messages.size());
        }

        void Logger::commit() {
            m_logCountStack.pop();

            if (m_logCountStack.size() > 0) {
                u32& cur = m_logCountStack.last();
                cur = m_messages.size();
            }
        }

        void Logger::revert() {
            m_logCountStack.pop();

            if (m_logCountStack.size() > 0 && m_logCountStack.last() < m_messages.size()) {
                m_messages.remove(m_logCountStack.last(), m_messages.size() - m_logCountStack.last());
            }
        }

        log_message& Logger::submit(log_type type, log_message_code code, const utils::String& msg, const SourceLocation& src, ParseNode* ast) {
            m_messages.push({
                type,
                code,
                msg,
                src,
                ast
            });
            return m_messages.last();
        }

        log_message& Logger::submit(log_type type, log_message_code code, const utils::String& msg) {
            m_messages.push({
                type,
                code,
                msg,
                SourceLocation(),
                nullptr
            });
            return m_messages.last();
        }

        bool Logger::hasErrors() const {
            return m_messages.some([](const log_message& l) {
                return l.type == lt_error;
            });
        }

        const utils::Array<log_message>& Logger::getMessages() {
            return m_messages;
        }
    };
};