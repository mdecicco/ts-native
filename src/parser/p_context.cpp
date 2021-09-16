#include <gjs/parser/context.h>
#include <gjs/common/script_context.h>
#include <gjs/common/script_type.h>
#include <gjs/common/type_manager.h>
#include <gjs/common/script_module.h>

namespace gjs {
    using tt = lex::token_type;

    namespace parse {
        context::context(script_context* ctx) : env(ctx), cur_token(0), root(nullptr) {
            std::vector<script_type*> types = ctx->global()->types()->all();
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

        bool context::is_typename() const {
            lex::token t = current();
            if (is_typename(t.text)) return true;
            
            if (pattern({ tt::identifier, tt::member_accessor })) {
                bool is_type = false;
                for (u16 m = 0;m < named_imports.size() && !is_type;m++) {
                    if (named_imports[m].first == t.text) {
                        for (u16 i = 0;i < named_imports[m].second.size() && !is_type;i++) {
                            if (pattern_s({ t.text, ".", named_imports[m].second[i] })) return true;
                        }
                    }
                }
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

        void context::backup() {
            cur_stack.push(cur_token);
        }

        void context::restore() {
            cur_token = cur_stack.top();
            cur_stack.pop();
        }

        void context::commit() {
            cur_stack.pop();
        }


        bool context::match(const std::vector<std::string>& any) const {
            lex::token cur = current();
            for (u32 i = 0;i < any.size();i++) {
                if (cur == any[i]) return true;
            }

            return false;
        }

        bool context::match_s(const std::vector<std::string>& any) const {
            lex::token cur = current();
            for (u32 i = 0;i < any.size();i++) {
                if (cur == any[i]) return true;
            }

            return false;
        }

        bool context::match(const std::vector<lex::token_type>& any) const {
            lex::token cur = current();
            for (u32 i = 0;i < any.size();i++) {
                if (cur == any[i]) return true;
            }

            return false;
        }

        bool context::pattern(const std::vector<std::string>& sequence) const {
            if (cur_token + sequence.size() >= tokens.size()) return false;
            for (u8 i = 0;i < sequence.size();i++) {
                if (sequence[i].length() == 0) continue;
                if (tokens[cur_token + i].text != sequence[i]) return false;
            }

            return true;
        }

        bool context::pattern_s(const std::vector<std::string>& sequence) const {
            if (cur_token + sequence.size() >= tokens.size()) return false;
            for (u8 i = 0;i < sequence.size();i++) {
                if (sequence[i].length() == 0) continue;
                if (tokens[cur_token + i].text != sequence[i]) return false;
            }

            return true;
        }

        bool context::pattern(const std::vector<lex::token_type>& sequence) const {
            if (cur_token + sequence.size() >= tokens.size()) return false;
            for (u8 i = 0;i < sequence.size();i++) {
                if (sequence[i] == lex::token_type::any) continue;
                if (tokens[cur_token + i].type != sequence[i]) return false;
            }

            return true;
        }
    };
};