#include <parser/context.h>
#include <context.h>
#include <vm_type.h>

namespace gjs {
    namespace parse {
        context::context(vm_context* ctx) : env(ctx), cur_token(0), root(nullptr) {
            std::vector<vm_type*> types = ctx->all_types();
            for (u16 t = 0;t < types.size();t++) {
                type_names.push_back(types[t]->name);
            }
        }

        bool context::is_typename(const std::string& text) const {
            for (u16 t = 0;t < type_names.size();t++) {
                if (type_names[t] == text) return true;
            }

            return false;
        }

        const lex::token& context::current() const {
            static const lex::token null_tok;
            if (cur_token >= tokens.size()) return null_tok;
            return tokens[cur_token];
        }

        void context::consume() {
            cur_token++;
        }

        bool context::at_end() const {
            return cur_token == tokens.size();
        }

        bool context::match(const std::vector<std::string>& any) {
            lex::token cur = current();
            for (u32 i = 0;i < any.size();i++) {
                if (cur == any[i]) return true;
            }

            return false;
        }

        bool context::match_s(const std::vector<std::string>& any) {
            lex::token cur = current();
            for (u32 i = 0;i < any.size();i++) {
                if (cur == any[i]) return true;
            }

            return false;
        }

        bool context::match(const std::vector<lex::token_type>& any) {
            lex::token cur = current();
            for (u32 i = 0;i < any.size();i++) {
                if (cur == any[i]) return true;
            }

            return false;
        }

        bool context::pattern(const std::vector<std::string>& sequence) {
            if (cur_token + sequence.size() >= tokens.size()) return false;
            for (u8 i = 0;i < sequence.size();i++) {
                if (sequence[i].length() == 0) continue;
                if (tokens[cur_token + i].text != sequence[i]) return false;
            }

            return true;
        }

        bool context::pattern_s(const std::vector<std::string>& sequence) {
            if (cur_token + sequence.size() >= tokens.size()) return false;
            for (u8 i = 0;i < sequence.size();i++) {
                if (sequence[i].length() == 0) continue;
                if (tokens[cur_token + i].text != sequence[i]) return false;
            }

            return true;
        }

        bool context::pattern(const std::vector<lex::token_type>& sequence) {
            if (cur_token + sequence.size() >= tokens.size()) return false;
            for (u8 i = 0;i < sequence.size();i++) {
                if (sequence[i] == lex::token_type::any) continue;
                if (tokens[cur_token + i].type != sequence[i]) return false;
            }

            return true;
        }
    };
};