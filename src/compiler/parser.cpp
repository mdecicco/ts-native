#include <tsn/compiler/Parser.h>
#include <tsn/compiler/Lexer.h>
#include <tsn/utils/ModuleSource.h>
#include <utils/Array.hpp>
#include <utils/Buffer.hpp>

#include <stdarg.h>
#include <iostream>

namespace tsn {
    namespace compiler {
        //
        // ParseNode
        //
        
        utils::String ParseNode::str() const {
            if (str_len == 0) return utils::String();
            return utils::String(const_cast<char*>(value.s), str_len);
        }
        
        void iprintf(u32 indent, const char* fmt, ...) {
            va_list l;
            va_start(l, fmt);
            for (u32 i = 0;i < indent;i++) printf("    ");
            vprintf(fmt, l);
            va_end(l);    
        }
        
        void getNodeEndLoc(const ParseNode* n, SourceLocation* out, bool ignoreNext = true) {
            if (!n) return;

            if (n->tok.src.getLine() > out->getLine()) {
                *out = SourceLocation(
                    n->tok.src.getSource(),
                    n->tok.src.getLine(),
                    n->tok.src.getCol() + n->tok.text.size()
                );
            } else if (n->tok.src.getLine() == out->getLine() && n->tok.src.getCol() + n->tok.text.size() > out->getCol()) {
                *out = SourceLocation(
                    n->tok.src.getSource(),
                    n->tok.src.getLine(),
                    n->tok.src.getCol() + n->tok.text.size()
                );
            }
            
            getNodeEndLoc(n->data_type, out, false);
            getNodeEndLoc(n->lvalue, out, false);
            getNodeEndLoc(n->rvalue, out, false);
            getNodeEndLoc(n->cond, out, false);
            getNodeEndLoc(n->body, out, false);
            getNodeEndLoc(n->else_body, out, false);
            getNodeEndLoc(n->initializer, out, false);
            getNodeEndLoc(n->parameters, out, false);
            getNodeEndLoc(n->template_parameters, out, false);
            getNodeEndLoc(n->modifier, out, false);
            getNodeEndLoc(n->alias, out, false);
            getNodeEndLoc(n->inheritance, out, false);


            if (!ignoreNext) getNodeEndLoc(n->next, out, false);
        }


        
        ParseNode::ParseNode() {
            tok.tp = tt_unknown;
            tok.src = SourceLocation();

            value = { 0 };

            if (data_type) data_type = nullptr;
            if (lvalue) lvalue = nullptr;
            if (rvalue) rvalue = nullptr;
            if (cond) cond = nullptr;
            if (body) body = nullptr;
            if (else_body) else_body = nullptr;
            if (initializer) initializer = nullptr;
            if (parameters) parameters = nullptr;
            if (template_parameters) template_parameters = nullptr;
            if (modifier) modifier = nullptr;
            if (alias) alias = nullptr;
            if (inheritance) inheritance = nullptr;
            if (next) next = nullptr;
        }

        void ParseNode::computeSourceLocationRange() {
            if (tok.src.m_length > 0) return;

            SourceLocation end = tok.src;
            getNodeEndLoc(this, &end);
            
            tok.src.m_length = end.getOffset() - tok.src.getOffset();
            tok.src.m_endLine = end.m_line;
            tok.src.m_endCol = end.m_col;
        }
        
        void ParseNode::manuallySpecifyRange(const token& _end) {
            SourceLocation end = _end.src.getEndLocation();
            tok.src.m_length = end.getOffset() - tok.src.getOffset();
            tok.src.m_endLine = end.m_line;
            tok.src.m_endCol = end.m_col;
        }
        
        void ParseNode::offsetSourceLocations(const SourceLocation& offset) {
            tok.src.offset(offset);
            tok.text = utils::String::View(tok.src.getPointer() + tok.src.getCol(), tok.len);
            if (str_len > 0) value.s = tok.text.c_str();

            if (data_type) data_type->offsetSourceLocations(offset);
            if (lvalue) lvalue->offsetSourceLocations(offset);
            if (rvalue) rvalue->offsetSourceLocations(offset);
            if (cond) cond->offsetSourceLocations(offset);
            if (body) body->offsetSourceLocations(offset);
            if (else_body) else_body->offsetSourceLocations(offset);
            if (initializer) initializer->offsetSourceLocations(offset);
            if (parameters) parameters->offsetSourceLocations(offset);
            if (template_parameters) template_parameters->offsetSourceLocations(offset);
            if (modifier) modifier->offsetSourceLocations(offset);
            if (alias) alias->offsetSourceLocations(offset);
            if (inheritance) inheritance->offsetSourceLocations(offset);
            if (next) next->offsetSourceLocations(offset);
        }
        
        void ParseNode::rehydrateSourceRefs(ModuleSource* src) {
            tok.src.setSource(src);
            tok.text = utils::String::View(tok.src.getPointer() + tok.src.getCol(), tok.len);
            
            if (str_len > 0 && !value.s && str_len <= tok.len) {
                // If this function is being called then the node is detached.
                // value.s must be dynamically allocated, because it will be deleted later.
                char* s = new char[str_len + 1];
                memcpy(s, tok.text.c_str(), str_len);
                s[str_len] = 0;
                value.s = s;
            }

            if (data_type) data_type->rehydrateSourceRefs(src);
            if (lvalue) lvalue->rehydrateSourceRefs(src);
            if (rvalue) rvalue->rehydrateSourceRefs(src);
            if (cond) cond->rehydrateSourceRefs(src);
            if (body) body->rehydrateSourceRefs(src);
            if (else_body) else_body->rehydrateSourceRefs(src);
            if (initializer) initializer->rehydrateSourceRefs(src);
            if (parameters) parameters->rehydrateSourceRefs(src);
            if (template_parameters) template_parameters->rehydrateSourceRefs(src);
            if (modifier) modifier->rehydrateSourceRefs(src);
            if (alias) alias->rehydrateSourceRefs(src);
            if (inheritance) inheritance->rehydrateSourceRefs(src);
            if (next) next->rehydrateSourceRefs(src);
        }

        ParseNode* ParseNode::clone(bool copyNext) {
            ParseNode* c = new ParseNode();
            c->tok = tok;
            c->tp = tp;
            c->value_tp = value_tp;
            c->op = op;
            c->str_len = str_len;
            c->value = value;
            c->flags = flags;
            c->flags.is_detached = 1;

            if (str_len > 0) {
                char* ns = new char[str_len + 1];
                memcpy(ns, value.s, str_len);
                ns[str_len] = 0;
                c->value.s = ns;
            }
            
            if (data_type) c->data_type = data_type->clone(true);
            else c->data_type = nullptr;

            if (lvalue) c->lvalue = lvalue->clone(true);
            else c->lvalue = nullptr;

            if (rvalue) c->rvalue = rvalue->clone(true);
            else c->rvalue = nullptr;

            if (cond) c->cond = cond->clone(true);
            else c->cond = nullptr;

            if (body) c->body = body->clone(true);
            else c->body = nullptr;

            if (else_body) c->else_body = else_body->clone(true);
            else c->else_body = nullptr;

            if (initializer) c->initializer = initializer->clone(true);
            else c->initializer = nullptr;

            if (parameters) c->parameters = parameters->clone(true);
            else c->parameters = nullptr;

            if (template_parameters) c->template_parameters = template_parameters->clone(true);
            else c->template_parameters = nullptr;

            if (modifier) c->modifier = modifier->clone(true);
            else c->modifier = nullptr;

            if (alias) c->alias = alias->clone(true);
            else c->alias = nullptr;

            if (inheritance) c->inheritance = inheritance->clone(true);
            else c->inheritance = nullptr;

            if (copyNext) {
                if (next) c->next = next->clone(true);
                else c->next = nullptr;
            } else c->next = nullptr;

            return c;
        }

        bool ParseNode::serialize(utils::Buffer* out, Context* ctx) const {
            if (!out->write(tok.tp)) return false;
            if (!out->write(tok.len)) return false;
            if ((tok.tp == tt_string || tok.tp == tt_template_string) && !out->write(tok.text)) return false;
            if (!tok.src.serialize(out, ctx)) return false;

            if (!out->write(tp)) return false;
            if (!out->write(value_tp)) return false;
            if (!out->write(op)) return false;
            if (!out->write(str_len)) return false;
            if (str_len == 0 && !out->write(value.u)) return false;
            else if (str_len != 0) {
                if (tok.text != utils::String((void*)value.s, str_len)) {
                    if (!out->write(true)) return false;
                    if (!out->write(value.s, str_len)) return false;
                } else {
                    if (!out->write(false)) return false;
                }
            }
            if (!out->write(flags)) return false;

            u16 nodeFlags = 0;
            u8 idx = 0;
            if (data_type)           nodeFlags |= (1 << idx++); else idx++;
            if (lvalue)              nodeFlags |= (1 << idx++); else idx++;
            if (rvalue)              nodeFlags |= (1 << idx++); else idx++;
            if (cond)                nodeFlags |= (1 << idx++); else idx++;
            if (body)                nodeFlags |= (1 << idx++); else idx++;
            if (else_body)           nodeFlags |= (1 << idx++); else idx++;
            if (initializer)         nodeFlags |= (1 << idx++); else idx++;
            if (parameters)          nodeFlags |= (1 << idx++); else idx++;
            if (template_parameters) nodeFlags |= (1 << idx++); else idx++;
            if (modifier)            nodeFlags |= (1 << idx++); else idx++;
            if (alias)               nodeFlags |= (1 << idx++); else idx++;
            if (inheritance)         nodeFlags |= (1 << idx++); else idx++;
            if (next)                nodeFlags |= (1 << idx++); else idx++;

            if (!out->write(nodeFlags)) return false;
            
            if (data_type && !data_type->serialize(out, ctx)) return false;
            if (lvalue && !lvalue->serialize(out, ctx)) return false;
            if (rvalue && !rvalue->serialize(out, ctx)) return false;
            if (cond && !cond->serialize(out, ctx)) return false;
            if (body && !body->serialize(out, ctx)) return false;
            if (else_body && !else_body->serialize(out, ctx)) return false;
            if (initializer && !initializer->serialize(out, ctx)) return false;
            if (parameters && !parameters->serialize(out, ctx)) return false;
            if (template_parameters && !template_parameters->serialize(out, ctx)) return false;
            if (modifier && !modifier->serialize(out, ctx)) return false;
            if (alias && !alias->serialize(out, ctx)) return false;
            if (inheritance && !inheritance->serialize(out, ctx)) return false;
            if (next && !next->serialize(out, ctx)) return false;

            return true;
        }

        bool ParseNode::deserialize(utils::Buffer* in, Context* ctx) {
            auto readStr = [in](utils::String& str) {
                str = in->readStr();
                if (str.size() == 0) return false;
                return true;
            };
            if (!in->read(tok.tp)) return false;
            if (!in->read(tok.len)) return false;
            if ((tok.tp == tt_string || tok.tp == tt_template_string) && !readStr(tok.text)) return false;
            if (!tok.src.deserialize(in, ctx)) return false;

            if (!in->read(tp)) return false;
            if (!in->read(value_tp)) return false;
            if (!in->read(op)) return false;
            if (!in->read(str_len)) return false;
            if (str_len == 0 && !in->read(value.u)) return false;
            else if (str_len != 0) {
                bool hasStr;
                if (!in->read(hasStr)) return false;
                if (hasStr) {
                    char* s = new char[str_len + 1];
                    if (!in->read((void*)s, str_len)) return false;
                    s[str_len] = 0;

                    value.s = s;
                }
            }

            if (!in->read(flags)) return false;

            u16 nodeFlags;
            u8 idx = 0;
            if (!in->read(nodeFlags)) return false;

            bool has_data_type           = (nodeFlags >> idx++) & 0b0000000000000001;
            bool has_lvalue              = (nodeFlags >> idx++) & 0b0000000000000001;
            bool has_rvalue              = (nodeFlags >> idx++) & 0b0000000000000001;
            bool has_cond                = (nodeFlags >> idx++) & 0b0000000000000001;
            bool has_body                = (nodeFlags >> idx++) & 0b0000000000000001;
            bool has_else_body           = (nodeFlags >> idx++) & 0b0000000000000001;
            bool has_initializer         = (nodeFlags >> idx++) & 0b0000000000000001;
            bool has_parameters          = (nodeFlags >> idx++) & 0b0000000000000001;
            bool has_template_parameters = (nodeFlags >> idx++) & 0b0000000000000001;
            bool has_modifier            = (nodeFlags >> idx++) & 0b0000000000000001;
            bool has_alias               = (nodeFlags >> idx++) & 0b0000000000000001;
            bool has_inheritance         = (nodeFlags >> idx++) & 0b0000000000000001;
            bool has_next                = (nodeFlags >> idx++) & 0b0000000000000001;

            if (has_data_type) {
                data_type = new ParseNode();
                if (!data_type->deserialize(in, ctx)) {
                    delete data_type;
                    data_type = nullptr;
                    return false;
                }
            }

            if (has_lvalue) {
                lvalue = new ParseNode();
                if (!lvalue->deserialize(in, ctx)) {
                    delete lvalue;
                    lvalue = nullptr;
                    return false;
                }
            }

            if (has_rvalue) {
                rvalue = new ParseNode();
                if (!rvalue->deserialize(in, ctx)) {
                    delete rvalue;
                    rvalue = nullptr;
                    return false;
                }
            }

            if (has_cond) {
                cond = new ParseNode();
                if (!cond->deserialize(in, ctx)) {
                    delete cond;
                    cond = nullptr;
                    return false;
                }
            }

            if (has_body) {
                body = new ParseNode();
                if (!body->deserialize(in, ctx)) {
                    delete body;
                    body = nullptr;
                    return false;
                }
            }

            if (has_else_body) {
                else_body = new ParseNode();
                if (!else_body->deserialize(in, ctx)) {
                    delete else_body;
                    else_body = nullptr;
                    return false;
                }
            }

            if (has_initializer) {
                initializer = new ParseNode();
                if (!initializer->deserialize(in, ctx)) {
                    delete initializer;
                    initializer = nullptr;
                    return false;
                }
            }

            if (has_parameters) {
                parameters = new ParseNode();
                if (!parameters->deserialize(in, ctx)) {
                    delete parameters;
                    parameters = nullptr;
                    return false;
                }
            }

            if (has_template_parameters) {
                template_parameters = new ParseNode();
                if (!template_parameters->deserialize(in, ctx)) {
                    delete template_parameters;
                    template_parameters = nullptr;
                    return false;
                }
            }

            if (has_modifier) {
                modifier = new ParseNode();
                if (!modifier->deserialize(in, ctx)) {
                    delete modifier;
                    modifier = nullptr;
                    return false;
                }
            }

            if (has_alias) {
                alias = new ParseNode();
                if (!alias->deserialize(in, ctx)) {
                    delete alias;
                    alias = nullptr;
                    return false;
                }
            }

            if (has_inheritance) {
                inheritance = new ParseNode();
                if (!inheritance->deserialize(in, ctx)) {
                    delete inheritance;
                    inheritance = nullptr;
                    return false;
                }
            }

            if (has_next) {
                next = new ParseNode();
                if (!next->deserialize(in, ctx)) {
                    delete next;
                    next = nullptr;
                    return false;
                }
            }

            return true;
        }
        
        void ParseNode::destroyDetachedAST(ParseNode* n) {
            if (!n || n->flags.is_detached == 0) return;
            
            if (n->str_len > 0) delete [] n->value.s;
            if (n->data_type) ParseNode::destroyDetachedAST(n->data_type);
            if (n->lvalue) ParseNode::destroyDetachedAST(n->lvalue);
            if (n->rvalue) ParseNode::destroyDetachedAST(n->rvalue);
            if (n->cond) ParseNode::destroyDetachedAST(n->cond);
            if (n->body) ParseNode::destroyDetachedAST(n->body);
            if (n->else_body) ParseNode::destroyDetachedAST(n->else_body);
            if (n->initializer) ParseNode::destroyDetachedAST(n->initializer);
            if (n->parameters) ParseNode::destroyDetachedAST(n->parameters);
            if (n->template_parameters) ParseNode::destroyDetachedAST(n->template_parameters);
            if (n->modifier) ParseNode::destroyDetachedAST(n->modifier);
            if (n->alias) ParseNode::destroyDetachedAST(n->alias);
            if (n->inheritance) ParseNode::destroyDetachedAST(n->inheritance);
            if (n->next) ParseNode::destroyDetachedAST(n->next);
        }
        
        
        
        //
        // Parser
        //
        
        utils::FixedAllocator<ParseNode>* Parser::allocatorPageGenerator() {
            return new utils::FixedAllocator<ParseNode>(1024, 1024, true);
        }

        Parser::Parser(Lexer* l, Logger* log) : IWithLogger(log), m_nodeAlloc(allocatorPageGenerator) {
            m_tokens = l->tokenize();
            m_currentIdx.reserve(32);
            m_currentIdx.push(0);
            m_parent = nullptr;
        }

        Parser::Parser(Lexer* l, Parser* parent) : IWithLogger(parent->getLogger()), m_nodeAlloc(allocatorPageGenerator)  {
            m_tokens = l->tokenize();
            m_currentIdx.reserve(32);
            m_currentIdx.push(0);
            m_parent = parent;
        }
        
        Parser::~Parser() {
        }
        
        void Parser::begin() {
            m_currentIdx.push(m_currentIdx[m_currentIdx.size() - 1]);
            beginLoggerTransaction();
        }
        
        void Parser::revert() {
            m_currentIdx.pop();
            revertLoggerTransaction();
        }
        
        void Parser::commit() {
            u32 idx = m_currentIdx.pop();
            m_currentIdx[m_currentIdx.size() - 1] = idx;
            commitLoggerTransaction();
        }
        
        void Parser::consume() {
            m_currentIdx[m_currentIdx.size() - 1]++;
        }
        
        const token& Parser::get() const {
            return m_tokens[m_currentIdx[m_currentIdx.size() - 1]];
        }
        
        const token& Parser::getPrev() const {
            u32 idx = m_currentIdx[m_currentIdx.size() - 1];
            return m_tokens[idx > 0 ? idx - 1 : 0];
        }
        
        ParseNode* Parser::parse() {
            m_nodeAlloc.reset();
            return program(this);
        }
        
        ParseNode* Parser::newNode(node_type tp, const token* src) {
            if (m_parent) return m_parent->newNode(tp, src ? src : &get());

            ParseNode* n = m_nodeAlloc.alloc();
            n->tok = src ? *src : get();
            n->tp = tp;
            return n;
        }
        
        void Parser::freeNode(ParseNode* n) {
            if (n->data_type) freeNode(n->data_type);
            if (n->lvalue) freeNode(n->lvalue);
            if (n->rvalue) freeNode(n->rvalue);
            if (n->cond) freeNode(n->cond);
            if (n->body) freeNode(n->body);
            if (n->else_body) freeNode(n->else_body);
            if (n->initializer) freeNode(n->initializer);
            if (n->template_parameters) freeNode(n->template_parameters);
            if (n->parameters) freeNode(n->parameters);
            if (n->modifier) freeNode(n->modifier);
            if (n->alias) freeNode(n->alias);
            if (n->inheritance) freeNode(n->inheritance);
            if (n->next) freeNode(n->next);
            
            m_nodeAlloc.free(n);
        }
        
        bool Parser::typeIs(token_type tp) const {
            return get().tp == tp;
        }
        
        bool Parser::textIs(const char* str) const {
            return get().text == str;
        }
        
        bool Parser::isKeyword(const char* str) const {
            const token& t = get();
            return t.tp == tt_keyword && t.text == str;
        }
        
        bool Parser::isSymbol(const char* str) const {
            const token& t = get();
            return t.tp == tt_symbol && t.text == str;
        }
        
        void Parser::error(log_message_code code, const utils::String& msg) {
            submitLog(lt_error, code, msg, get().src, nullptr);
        }
        
        void Parser::error(log_message_code code, const utils::String& msg, const token& tok) {
            submitLog(lt_error, code, msg, tok.src, nullptr);
        }
        
        
        
        //
        // Helpers
        //
        
        ParseNode* errorNode(Parser* ps) {
            return ps->newNode(nt_error, &ps->getPrev());
        }
        bool isError(ParseNode* n) {
            if (!n) return false;
            return n->tp == nt_error;
        }
        ParseNode* array_of(Parser* ps, parsefn fn) {
            ParseNode* f = fn(ps);
            if (f && isError(f)) return f;

            ParseNode* n = f;
            while (n) {
                n->next = fn(ps);
                if (n->next && isError(n->next)) {
                    ps->freeNode(f);
                    return errorNode(ps);
                }

                n = n->next;
            }

            return f;
        }
        ParseNode* list_of(
            Parser* ps, parsefn fn,
            log_message_code before_comma_err, const utils::String& before_comma_msg,
            log_message_code after_comma_err, const utils::String& after_comma_msg
        ) {
            ParseNode* f = fn(ps);
            if (!f) {
                if (before_comma_err != pm_none) {
                    ps->error(before_comma_err, before_comma_msg);
                    return errorNode(ps);
                }

                return nullptr;
            }

            ParseNode* n = f;
            while (ps->typeIs(tt_comma)) {
                ps->consume();
                n->next = fn(ps);

                if (!n->next) {
                    ps->error(after_comma_err, after_comma_msg);
                    ps->freeNode(f);
                    return errorNode(ps);
                }

                if (isError(n->next)) {
                    ps->freeNode(f);
                    return errorNode(ps);
                }

                n = n->next;
            }

            return f;
        }
        ParseNode* one_of(Parser* ps, std::initializer_list<parsefn> rules) {
            for (auto fn : rules) {
                ps->begin();

                ParseNode* n = fn(ps);
                if (n) {
                    ps->commit();
                    return n;
                }

                ps->revert();
            }

            return nullptr;
        }
        ParseNode* all_of(Parser* ps, std::initializer_list<parsefn> rules) {
            ParseNode* f = nullptr;
            ParseNode* n = nullptr;
            ps->begin();

            for (auto fn : rules) {

                if (!f) f = n = fn(ps);
                else {
                    n->next = fn(ps);
                    n = n->next;
                }

                if (!n || isError(n)) {
                    if (f) ps->freeNode(f);
                    ps->revert();
                    return nullptr;
                }
            }

            ps->commit();
            return f;
        }
        
        bool isAssignmentOperator(Parser* ps) {
            const token& op = ps->get();
            if (op.tp != tt_symbol) return false;
            // =, +=, -=, *=, /=, %=, <<=, >>=, &=, |=, &&=, ||=, ^=

            if (op.text[0] == '=' && op.text.size() == 1) return true;

            if (op.text.size() == 2 && op.text[1] == '=') {
                // +=, -=, *=, /=, %=, &=, |=, ^= (and >=, <=, ==, !=)
                if (op.text[0] == '>' || op.text[0] == '<' || op.text[0] == '=' || op.text[0] == '!') {
                    // >=, <=, ==, !=
                    return false;
                }

                // Can only be an assignment operator
                // +=, -=, *=, /=, %=, &=, |=, ^=
                return true;
            }

            if (op.text.size() == 3) {
                // The only length-3 symbols are assignment operators
                // &&=, ||=, <<=, >>=
                return true;
            }

            return false;
        }
        expr_operator getOperatorType(Parser* ps) {
            // Note:
            // This function does not cover the following operations
            // - op_placementNew (not overridable)
            // - op_new          (not overridable)
            // - op_member       (not overridable)
            // - op_negate
            //
            // The only one of these that needs special handling due to this is op_negate.
            // When declaring an operator override for a class, "operator -()" will be parsed
            // as if it's an override for the binary subtraction operator. The compiler will
            // interpret an override of the binary subtraction operator as a negation operator
            // override if it takes no arguments.

            const token& op = ps->get();
            if (op.tp != tt_symbol) {
                if (op.tp == tt_open_parenth) {
                    ps->begin();
                    ps->consume();
                    if (ps->typeIs(tt_close_parenth)) {
                        ps->revert();
                        return op_call;
                    }
                }

                if (op.tp == tt_open_bracket) {
                    ps->begin();
                    ps->consume();
                    if (ps->typeIs(tt_close_bracket)) {
                        ps->revert();
                        return op_index;
                    }
                }

                return op_undefined;
            }

            u32 len = op.text.size();

            switch (op.text[0]) {
                case '+': {
                    if (len == 1) return op_add;
                    if (op.text[1] == '+') return op_undefined; // pre/post unknown
                    return op_addEq;
                }
                case '-': {
                    if (len == 1) return op_sub;
                    if (op.text[1] == '-') return op_undefined; // pre/post unknown
                    return op_subEq;
                }
                case '*': {
                    if (len == 1) return op_mul;
                    return op_mulEq;
                }
                case '/': {
                    if (len == 1) return op_div;
                    return op_divEq;
                }
                case '%': {
                    if (len == 1) return op_mod;
                    return op_modEq;
                }
                case '~': {
                    return op_bitInv;
                }
                case '!': {
                    if (len == 1) return op_not;
                    return op_notEq;
                }
                case '^': {
                    if (len == 1) return op_xor;
                    return op_xorEq;
                }
                case '<': {
                    if (len == 1) return op_lessThan;
                    if (op.text[1] == '<') {
                        if (len == 2) return op_shLeft;
                        return op_shLeftEq;
                    }
                    return op_lessThanEq;
                }
                case '>': {
                    if (len == 1) return op_greaterThan;
                    if (op.text[1] == '>') {
                        if (len == 2) return op_shRight;
                        return op_shRightEq;
                    }
                    return op_greaterThanEq;
                }
                case '=': {
                    if (len == 1) return op_assign;
                    return op_compare;
                }
                case '&': {
                    if (len == 1) return op_bitAnd;
                    if (op.text[1] == '&') {
                        if (len == 2) return op_logAnd;
                        return op_logAndEq;
                    }
                    return op_bitAndEq;
                }
                case '|': {
                    if (len == 1) return op_bitOr;
                    if (op.text[1] == '|') {
                        if (len == 2) return op_logOr;
                        return op_logOrEq;
                    }
                    return op_bitOrEq;
                }
                case '?': return op_conditional;
            }

            return op_undefined;
        }
        
        const token& skipToNextType(Parser* ps, std::initializer_list<token_type> stopAt) {
            ps->begin();

            do {
                const token& t = ps->get();
                for (token_type tt : stopAt) {
                    if (t.tp == tt) {
                        ps->commit();
                        return t;
                    }
                }

                if (t.tp == tt_eof) {
                    ps->revert();
                    return t;
                }

                ps->consume();
            } while (true);

            return ps->get();
        }
        const token& skipToNextText(Parser* ps, const char* str, std::initializer_list<token_type> stopAt = {}) {
            ps->begin();

            do {
                const token& t = ps->get();
                for (token_type tt : stopAt) {
                    if (t.tp == tt) {
                        ps->revert();
                        return t;
                    }
                }
                
                if (t.tp == tt_eof) {
                    ps->revert();
                    return t;
                }

                if (ps->textIs(str)) {
                    ps->commit();
                    return t;
                }

                ps->consume();
            } while (true);

            return ps->get();
        }
        const token& skipToNextKeyword(Parser* ps, const char* str, std::initializer_list<token_type> stopAt = {}) {
            ps->begin();

            do {
                const token& t = ps->get();
                for (token_type tt : stopAt) {
                    if (t.tp == tt) {
                        ps->revert();
                        return t;
                    }
                }
                
                if (t.tp == tt_eof) {
                    ps->revert();
                    return t;
                }

                if (ps->isKeyword(str)) {
                    ps->commit();
                    return t;
                }

                ps->consume();
            } while (true);

            return ps->get();
        }
        const token& skipToNextSymbol(Parser* ps, const char* str, std::initializer_list<token_type> stopAt = {}) {
            ps->begin();

            do {
                const token& t = ps->get();
                for (token_type tt : stopAt) {
                    if (t.tp == tt) {
                        ps->revert();
                        return t;
                    }
                }
                
                if (t.tp == tt_eof) {
                    ps->revert();
                    return t;
                }
                
                if (ps->isSymbol(str)) {
                    ps->commit();
                    return t;
                }

                ps->consume();
            } while (true);

            return ps->get();
        }

        
        
        //
        // Misc
        //
        ParseNode* eos(Parser* ps) {
            if (!ps->typeIs(tt_semicolon)) return nullptr;
            ps->consume();
            return ps->newNode(nt_eos, &ps->getPrev());
        }
        ParseNode* eosRequired(Parser* ps) {
            if (!ps->typeIs(tt_semicolon)) {
                ps->error(pm_expected_eos, "Expected ';'");

                skipToNextType(ps, { tt_semicolon });

                return errorNode(ps);
            }
            ps->consume();
            return ps->newNode(nt_eos, &ps->getPrev());
        }
        ParseNode* identifier(Parser* ps) {
            if (!ps->typeIs(tt_identifier)) return nullptr;
            ParseNode* n = ps->newNode(nt_identifier);
            const token& t = ps->get();
            n->value.s = t.text.c_str();
            n->str_len = t.text.size();
            ps->consume();
            return n;
        }
        ParseNode* typedIdentifier(Parser* ps) {
            ps->begin();
            ParseNode* id = identifier(ps);
            if (!id) {
                ps->revert();
                return nullptr;
            }

            if (!ps->typeIs(tt_colon)) {
                ps->freeNode(id);
                ps->revert();
                return nullptr;
            }
            ps->consume();

            id->data_type = typeSpecifier(ps);
            if (!id->data_type || isError(id->data_type)) {
                if (!id->data_type) ps->error(pm_expected_type_specifier, "Expected type specifier after ':'");
                ps->freeNode(id);
                id = errorNode(ps);
            }

            ps->commit();
            return id;
        }
        ParseNode* objectDecompositorProperty(Parser* ps) {
            return one_of(ps, {
                typedAssignable,
                assignable
            });
        }
        ParseNode* objectDecompositor(Parser* ps) {
            const token* tok = &ps->get();
            if (!ps->typeIs(tt_open_brace)) return nullptr;
            ps->begin();
            ps->consume();

            ParseNode* f = objectDecompositorProperty(ps);
            if (!f) {
                ps->revert();
                return nullptr;
            }
            ps->commit();

            ParseNode* n = f;
            while (n) {
                if (!ps->typeIs(tt_comma)) break;
                ps->consume();

                n->next = objectDecompositorProperty(ps);
                n = n->next;
                if (!n || isError(n)) {
                    ps->error(pm_expected_object_property, "Expected property after ','");
                    const token& r = skipToNextType(ps, { tt_comma, tt_close_brace, tt_semicolon });
                    switch (r.tp) {
                        case tt_comma: continue;
                        case tt_close_brace: {
                            ps->consume();
                            n = ps->newNode(nt_object_decompositor, tok);
                            n->body = f;
                            return n;
                        }
                        default: {
                            ps->freeNode(f);
                            return errorNode(ps);
                        }
                    }
                }
            }

            if (!ps->typeIs(tt_close_brace)) {
                ps->error(pm_expected_closing_brace, "Expected '}' to close object decompositor");
                ps->freeNode(f);
                return errorNode(ps);
            }
            ps->consume();

            n = ps->newNode(nt_object_decompositor, tok);
            n->body = f;

            return n;
        }
        
        
        
        //
        // Parameters / Arguments
        //
        
        ParseNode* templateArgs(Parser* ps) {
            if (!ps->isSymbol("<")) return nullptr;
            ps->consume();

            ParseNode* args = typeSpecifier(ps);
            if (!args) {
                ps->error(pm_expected_type_specifier, "Expected template argument");
                const token& r = skipToNextType(ps, { tt_comma });

                // May not actually be template args, not enough info here
                if (r.tp != tt_comma) return nullptr;
            } else if (isError(args)) return args;

            ParseNode* n = args;
            while (n) {
                if (!ps->typeIs(tt_comma)) break;
                ps->consume();

                n->next = typeSpecifier(ps);
                n = n->next;
                if (!n) {
                    ps->error(pm_expected_type_specifier, "Expected template argument");

                    const token& r = skipToNextSymbol(ps, ">", { tt_comma });
                    switch (r.tp) {
                        case tt_comma: continue;
                        case tt_symbol: break;
                        default: {
                            ps->freeNode(args);
                            return errorNode(ps);
                        }
                    }
                }
            }

            if (!ps->isSymbol(">")) {
                ps->error(pm_expected_symbol, "Expected '>' after template argument list");

                const token& r = skipToNextSymbol(ps, ">");
                if (r.tp == tt_symbol) {
                    ps->consume();
                    return args;
                }
                
                if (args) ps->freeNode(args);
                return errorNode(ps);
            }

            ps->consume();

            return args;
        }
        ParseNode* templateParams(Parser* ps) {
            if (!ps->isSymbol("<")) return nullptr;
            ps->consume();

            ParseNode* args = identifier(ps);
            if (!args) {
                ps->error(pm_expected_type_specifier, "Expected template parameter");
                const token& r = skipToNextType(ps, { tt_comma });

                // May not actually be template params, not enough info here
                if (r.tp != tt_comma) return nullptr;
            }

            if (isError(args)) return args;

            ParseNode* n = args;
            while (n) {
                if (!ps->typeIs(tt_comma)) break;
                ps->consume();

                n->next = identifier(ps);
                n = n->next;
                if (!n) {
                    ps->error(pm_expected_type_specifier, "Expected template parameter");

                    const token& r = skipToNextType(ps, { tt_comma });
                    switch (r.tp) {
                        case tt_comma: continue;
                        default: {
                            ps->freeNode(args);
                            return errorNode(ps);
                        }
                    }
                }
            }

            if (!ps->isSymbol(">")) {
                ps->error(pm_expected_symbol, "Expected '>' after template parameter list");

                const token& r = skipToNextSymbol(ps, ">");
                if (r.tp == tt_symbol) {
                    ps->consume();
                    return args;
                }
                
                if (args) ps->freeNode(args);
                return errorNode(ps);
            }

            ps->consume();
            return args;
        }
        ParseNode* parameter(Parser* ps) {
            return one_of(ps, {
                typedAssignable,
                assignable
            });
        }
        ParseNode* typedParameter(Parser* ps) {
            return typedAssignable(ps);
        }
        ParseNode* maybeTypedParameterList(Parser* ps) {
            if (!ps->typeIs(tt_open_parenth)) return nullptr;
            ps->begin();

            const token& ft = ps->get();
            ps->consume();

            ParseNode* f = typedParameter(ps);
            if (!f && !ps->typeIs(tt_close_parenth)) {
                ps->revert();
                return nullptr;
            }

            ps->commit();

            ParseNode* n = f;
            while (n) {
                if (!ps->typeIs(tt_comma)) break;
                ps->consume();

                n->next = typedParameter(ps);
                n = n->next;
                if (!n) {
                    ps->error(pm_expected_parameter, "Expected typed parameter after ','");

                    const token& r = skipToNextType(ps, { tt_comma, tt_close_parenth });
                    switch (r.tp) {
                        case tt_comma: continue;
                        case tt_close_parenth: continue;
                        default: {
                            ps->freeNode(f);
                            return errorNode(ps);
                        }
                    }
                }
            }

            if (!ps->typeIs(tt_close_parenth)) {
                ps->error(pm_expected_closing_parenth, "Expected ')' after typed parameter list");

                if (f) {
                    const token& r = skipToNextType(ps, { tt_close_parenth });
                    if (r.tp != tt_close_parenth) {
                        // malformed typed parameter list
                        ps->freeNode(f);
                        return errorNode(ps);
                    }
                } else {
                    // possibly not a typed parameter list
                    return nullptr;
                }
            }
            ps->consume();

            if (!f) {
                f = ps->newNode(nt_empty, &ft);
                f->manuallySpecifyRange(ps->getPrev());
            }

            return f;
        }
        ParseNode* maybeParameterList(Parser* ps) {
            if (!ps->typeIs(tt_open_parenth)) return nullptr;
            const token& ft = ps->get();
            ps->consume();

            ParseNode* f = parameter(ps);
            ParseNode* n = f;
            while (n) {
                if (!ps->typeIs(tt_comma)) break;
                ps->consume();

                n->next = parameter(ps);
                n = n->next;
                if (!n) {
                    ps->error(pm_expected_parameter, "Expected parameter after ','");

                    const token& r = skipToNextType(ps, { tt_comma, tt_close_parenth });
                    switch (r.tp) {
                        case tt_comma: continue;
                        case tt_close_parenth: continue;
                        default: {
                            ps->freeNode(f);
                            return errorNode(ps);
                        }
                    }
                }
            }

            if (!ps->typeIs(tt_close_parenth)) {
                ps->error(pm_expected_closing_parenth, "Expected ')' after parameter list");

                if (f) {
                    const token& r = skipToNextType(ps, { tt_close_parenth });
                    if (r.tp != tt_close_parenth) {
                        // malformed parameter list
                        ps->freeNode(f);
                        return errorNode(ps);
                    }
                } else {
                    // possibly not a parameter list
                    return nullptr;
                }
            }
            ps->consume();

            if (!f) {
                f = ps->newNode(nt_empty, &ft);
                f->manuallySpecifyRange(ps->getPrev());
            }

            return f;
        }
        ParseNode* parameterList(Parser* ps) {
            if (!ps->typeIs(tt_open_parenth)) return nullptr;
            const token& ft = ps->get();
            ps->consume();

            ParseNode* f = parameter(ps);
            ParseNode* n = f;
            while (n) {
                if (!ps->typeIs(tt_comma)) break;
                ps->consume();

                n->next = parameter(ps);
                n = n->next;
                if (!n) {
                    ps->error(pm_expected_parameter, "Expected parameter after ','");

                    const token& r = skipToNextType(ps, { tt_comma, tt_close_parenth });
                    switch (r.tp) {
                        case tt_comma: continue;
                        case tt_close_parenth: continue;
                        default: {
                            ps->freeNode(f);
                            return errorNode(ps);
                        }
                    }
                }
            }

            if (!ps->typeIs(tt_close_parenth)) {
                ps->error(pm_expected_closing_parenth, "Expected ')' to close parameter list");

                const token& r = skipToNextType(ps, { tt_close_parenth });
                switch (r.tp) {
                    case tt_close_parenth: {
                        ps->consume();

                        if (!f) {
                            f = ps->newNode(nt_empty, &ft);
                            f->manuallySpecifyRange(ps->getPrev());
                        }

                        return f;
                    }
                    default: {
                        if (f) ps->freeNode(f);
                        return errorNode(ps);
                    }
                }
            }
            
            ps->consume();

            if (!f) {
                f = ps->newNode(nt_empty, &ft);
                f->manuallySpecifyRange(ps->getPrev());
            }

            return f;
        }
        ParseNode* typedParameterList(Parser* ps) {
            if (!ps->typeIs(tt_open_parenth)) return nullptr;
            const token& ft = ps->get();
            ps->consume();

            ParseNode* f = typedParameter(ps);
            ParseNode* n = f;
            while (n) {
                if (!ps->typeIs(tt_comma)) break;
                ps->consume();

                n->next = typedParameter(ps);
                n = n->next;
                if (!n) {
                    ps->error(pm_expected_parameter, "Expected typed parameter after ','");

                    const token& r = skipToNextType(ps, { tt_comma, tt_close_parenth });
                    switch (r.tp) {
                        case tt_comma: continue;
                        case tt_close_parenth: continue;
                        default: {
                            ps->freeNode(f);
                            return errorNode(ps);
                        }
                    }
                }
            }

            if (!ps->typeIs(tt_close_parenth)) {
                ps->error(pm_expected_closing_parenth, "Expected ')' to close typed parameter list");

                const token& r = skipToNextType(ps, { tt_close_parenth });
                switch (r.tp) {
                    case tt_close_parenth: {
                        ps->consume();
                        return f;
                    }
                    default: {
                        if (f) ps->freeNode(f);
                        return errorNode(ps);
                    }
                }
            }

            ps->consume();

            if (!f) {
                f = ps->newNode(nt_empty, &ft);
                f->manuallySpecifyRange(ps->getPrev());
            }
            return f;
        }
        ParseNode* arguments(Parser* ps) {
            if (!ps->typeIs(tt_open_parenth)) return nullptr;
            const token& ft = ps->get();
            ps->consume();

            ParseNode* n = expressionSequence(ps);
            if (!n) n = ps->newNode(nt_empty);

            n->tok = ft;

            if (!ps->typeIs(tt_close_parenth)) {
                ps->error(pm_expected_closing_parenth, "Expected ')' to close argument list");

                const token& r = skipToNextType(ps, { tt_close_parenth });
                switch (r.tp) {
                    case tt_close_parenth: {
                        n->manuallySpecifyRange(ps->get());
                        ps->consume();
                        return n;
                    }
                    default: {
                        if (n) {
                            ps->freeNode(n);
                            return errorNode(ps);
                        }

                        return nullptr;
                    }
                }
            }

            n->manuallySpecifyRange(ps->get());
            ps->consume();
            return n;
        }
        
        
        //
        // Types
        //
        
        ParseNode* typeModifier(Parser* ps) {
            ParseNode* mod = nullptr;
            if (ps->isSymbol("*")) {
                ps->consume();

                mod = ps->newNode(nt_type_modifier, &ps->getPrev());
                mod->flags.is_pointer = 1;
            } else if (ps->typeIs(tt_open_bracket)) {
                const token* t = &ps->get();
                ps->consume();
                if (!ps->typeIs(tt_close_bracket)) {
                    ps->error(pm_expected_closing_bracket, "Expected ']'");

                    const token& r = skipToNextType(ps, { tt_close_bracket });
                    switch (r.tp) {
                        case tt_close_bracket: {
                            ps->consume();
                            break;
                        }
                        default: {
                            return errorNode(ps);
                        }
                    }
                } else ps->consume();

                mod = ps->newNode(nt_type_modifier, t);
                mod->flags.is_array = 1;
            }

            if (mod) mod->modifier = typeModifier(ps);
            return mod;
        }
        ParseNode* typeProperty(Parser* ps) {
            ps->begin();
            ParseNode* n = identifier(ps);
            if (!n) {
                ps->revert();
                return nullptr;
            }

            if (!ps->typeIs(tt_colon)) {
                ps->revert();
                ps->freeNode(n);
                return nullptr;
            }
            ps->consume();

            ps->commit();

            n->tp = nt_type_property;
            n->data_type = typeSpecifier(ps);
            if (!n->data_type) {
                ps->error(pm_expected_type_specifier, "Expected type specifier after ':'");
                ps->freeNode(n);
                return errorNode(ps);
            }

            if (isError(n->data_type)) {
                ps->freeNode(n);
                return errorNode(ps);
            }

            return n;
        }
        ParseNode* parenthesizedTypeSpecifier(Parser* ps) {
            if (!ps->typeIs(tt_open_parenth)) return nullptr;
            const token* t = &ps->get();
            ps->begin();
            ps->consume();

            ParseNode* n = typeSpecifier(ps);
            if (!n) {
                ps->revert();
                return nullptr;
            }

            n->tok = *t;

            ps->commit();

            if (!ps->typeIs(tt_close_parenth)) {
                ps->error(pm_expected_closing_parenth, "Expected ')'");

                const token& r = skipToNextType(ps, { tt_close_parenth });
                switch (r.tp) {
                    case tt_close_parenth: break;
                    default: {
                        ps->freeNode(n);
                        return errorNode(ps);
                    }
                }
            }

            ps->consume();
            return n;
        }
        ParseNode* identifierTypeSpecifier(Parser* ps, ParseNode* id) {
            if (!id) return nullptr;
            
            ParseNode* n = ps->newNode(nt_type_specifier, &id->tok);
            n->body = id;
            n->template_parameters = templateArgs(ps);
            n->modifier = typeModifier(ps);

            return n;
        }
        ParseNode* typeSpecifier(Parser* ps) {
            const token* tok = &ps->get();
            ParseNode* id = identifier(ps);
            if (id) return identifierTypeSpecifier(ps, id);

            if (ps->typeIs(tt_open_brace)) {
                ps->begin();
                ps->consume();

                ParseNode* f = typeProperty(ps);
                if (!f) {
                    ps->revert();
                    return nullptr;
                }
                ps->commit();
                
                if (!ps->typeIs(tt_semicolon)) {
                    ps->error(pm_expected_eos, "Expected ';' after type property");
                    if (ps->typeIs(tt_comma)) ps->consume(); // a likely mistake
                    // attempt to continue
                } else ps->consume();

                ParseNode* n = f;
                while (true) {
                    n->next = typeProperty(ps);
                    n = n->next;
                    
                    if (!n) break;

                    if (!ps->typeIs(tt_semicolon)) {
                        ps->error(pm_expected_eos, "Expected ';' after type property");
                        if (ps->typeIs(tt_comma)) ps->consume(); // a likely mistake
                        // attempt to continue
                    } else ps->consume();
                }

                if (!ps->typeIs(tt_close_brace)) {
                    ps->error(pm_expected_closing_brace, "Expected '}' to close object type definition");
                    const token& r = skipToNextType(ps, { tt_close_brace });
                    if (r.tp != tt_close_brace) {
                        ps->freeNode(f);
                        return errorNode(ps);
                    }
                }

                ps->consume();

                ParseNode* t = ps->newNode(nt_type_specifier, tok);
                t->body = f;
                t->modifier = typeModifier(ps);

                return t;
            }

            if (ps->typeIs(tt_open_parenth)) {
                const token* t = &ps->get();
                ps->begin();
                ParseNode* p = maybeTypedParameterList(ps);
                if (p && !isError(p)) {
                    ParseNode* n = ps->newNode(nt_type_specifier, t);
                    n->parameters = p;

                    if (ps->typeIs(tt_forward)) {
                        ps->consume();

                        n->data_type = typeSpecifier(ps);
                        if (!n->data_type) {
                            ps->error(pm_expected_type_specifier, "Expected return type specifier");
                            ps->freeNode(n);
                            return errorNode(ps);
                        }
                    } else {
                        ps->error(pm_expected_symbol, "Expected '=>' to indicate return type specifier after parameter list");
                        ps->freeNode(n);
                        return errorNode(ps);
                    }

                    return n;
                } else if (isError(p)) {
                    ps->freeNode(p);
                }

                ps->revert();
            }

            ParseNode* n = parenthesizedTypeSpecifier(ps);
            if (n) n->modifier = typeModifier(ps);

            return n;
        }
        ParseNode* assignable(Parser* ps) {
            return identifier(ps);
        }
        ParseNode* typedAssignable(Parser* ps) {
            ps->begin();
            ParseNode* id = identifier(ps);
            if (!id) {
                ps->revert();
                return nullptr;
            }

            if (!ps->typeIs(tt_colon)) {
                ps->freeNode(id);
                ps->revert();
                return nullptr;
            }
            ps->consume();

            id->data_type = typeSpecifier(ps);
            if (!id->data_type) {
                ps->error(pm_expected_type_specifier, "Expected type specifier after ':'");
                ps->freeNode(id);
                id = errorNode(ps);
            }

            ps->commit();
            return id;
        }
        
        
        
        //
        // Literals
        //
        
        ParseNode* numberLiteral(Parser* ps) {
            if (!ps->typeIs(tt_number)) return nullptr;

            const token& t = ps->get();
            ps->consume();

            ParseNode* n = ps->newNode(nt_literal, &t);

            if (ps->typeIs(tt_number_suffix)) {
                const token& s = ps->get();
                ps->consume();
                char c = s.text[0];
                
                if (c == 'f') n->value_tp = lt_f32;
                else if (c == 'b') n->value_tp = lt_i8;
                else if (c == 's') n->value_tp = lt_i16;
                // if s.text[0] == 'l' then s.text must == 'll'. 'l' is not
                // a valid suffix, it is the default for number literals
                // without decimals
                else if (c == 'l') n->value_tp = lt_i64;
                else if (c == 'u') {
                    c = s.text[1];
                    if (c == 'b') n->value_tp = lt_u8;
                    else if (c == 's') n->value_tp = lt_u16;
                    else if (c == 'l') {
                        if (s.text.size() == 2) n->value_tp = lt_u32;
                        else n->value_tp = lt_u64; // only other option is 'ull'
                    }
                }
            } else {
                if (t.text.firstIndexOf('.') > 0) n->value_tp = lt_f64;
                else n->value_tp = lt_i32;
            }

            // todo: error on numbers outside of valid ranges
            switch (n->value_tp) {
                case lt_u8:
                case lt_u16:
                case lt_u32:
                case lt_u64: {
                    n->value.u = std::stoull(t.text.c_str());
                    break;
                }
                case lt_i8:
                case lt_i16:
                case lt_i32:
                case lt_i64: {
                    n->value.i = std::stoll(t.text.c_str());
                    break;
                }
                case lt_f32:
                case lt_f64: {
                    n->value.f = std::stod(t.text.c_str());
                    break;
                }
                default: break;
            }

            return n;
        }
        ParseNode* stringLiteral(Parser* ps) {
            if (!ps->typeIs(tt_string)) return nullptr;
            const token& t = ps->get();
            ps->consume();
            ParseNode* n = ps->newNode(nt_literal, &t);
            n->value_tp = lt_string;
            n->value.s = t.text.c_str();
            n->str_len = t.text.size();
            return n;
        }
        ParseNode* templateStringLiteral(Parser* ps) {
            if (!ps->typeIs(tt_template_string)) return nullptr;

            const token& t = ps->get();
            ps->consume();
            ParseNode* n = ps->newNode(nt_literal, &t);
            n->value_tp = lt_template_string;
            n->value.s = t.text.c_str();
            n->str_len = t.text.size();

            struct fragment {
                utils::String str;
                bool isExpr;
                SourceLocation src;
            };

            utils::Array<fragment> frags;
            u32 fragBegin = 0;
            u32 fragEnd = 0;
            SourceLocation src = t.src;
            src++; // '`'
            SourceLocation fragBeginLoc = src;

            for (u32 i = 0;i < t.text.size();i++, src++) {
                if (t.text[i] == '$' && i < t.text.size() - 1 && t.text[i + 1] == '{') {
                    src++;
                    src++;

                    if (fragEnd > fragBegin) {
                        frags.push({
                            utils::String::View(t.text.c_str() + fragBegin, (fragEnd - fragBegin) - 1),
                            false,
                            fragBeginLoc
                        });
                        fragBegin = fragEnd = 0;
                    }

                    u32 braceCount = 1;
                    fragBeginLoc = src;
                    for (u32 j = i + 2;j < t.text.size();j++, src++) {
                        if (t.text[j] == '{') braceCount++;
                        else if (t.text[j] == '}') {
                            braceCount--;

                            if (braceCount == 0) {
                                if (j != i + 2) {
                                    frags.push({
                                        utils::String::View(t.text.c_str() + i + 2, j - (i + 2)),
                                        true,
                                        fragBeginLoc
                                    });
                                }
                                i = j;
                                fragBegin = fragEnd = j + 1;
                                fragBeginLoc = src;
                                fragBeginLoc++;
                                break;
                            }
                        }
                    }
                }

                fragEnd++;
            }

            if (fragEnd > fragBegin) {
                frags.push({
                    utils::String::View(t.text.c_str() + fragBegin, (fragEnd - fragBegin) - 1),
                    false,
                    fragBeginLoc
                });
            }
            
            ParseNode* f = nullptr;
            for (u32 i = 0;i < frags.size();i++) {
                if (frags[i].isExpr) {
                    ModuleSource src(frags[i].str, nullptr);
                    Lexer l(&src);
                    Parser p(&l, ps);
                    ParseNode* expr = expression(&p);

                    if (!expr) {
                        ps->error(pm_expected_expr, "Expected expression in template literal");
                    } else {
                        expr->offsetSourceLocations(frags[i].src);
                        if (!f) {
                            f = expr;
                            n->body = f;
                        } else {
                            f->next = expr;
                            f = expr;
                        }
                    }
                } else {
                    token lt;
                    lt.tp = tt_string;
                    lt.text = frags[i].str;
                    lt.src = frags[i].src;
                    lt.len = frags[i].str.size();

                    if (!f) {
                        f = ps->newNode(nt_literal, &lt);
                        f->value_tp = lt_string;
                        f->value.s = frags[i].str.c_str();
                        f->str_len = frags[i].str.size();
                        n->body = f;
                    } else {
                        ParseNode* fn = ps->newNode(nt_literal, &lt);
                        fn->value_tp = lt_string;
                        fn->value.s = frags[i].str.c_str();
                        fn->str_len = frags[i].str.size();
                        f->next = fn;
                        f = fn;
                    }
                }
            }

            return n;
        }
        ParseNode* arrayLiteral(Parser* ps) {
            const token* tok = &ps->get();
            if (!ps->typeIs(tt_open_bracket)) return nullptr;
            ps->begin();
            ps->consume();

            ParseNode* n = expressionSequence(ps);
            if (!n) {
                if (!ps->typeIs(tt_close_bracket)) {
                    ps->revert();
                    return nullptr;
                }

                n = ps->newNode(nt_empty);
            }

            if (!ps->typeIs(tt_close_bracket)) {
                ps->error(pm_expected_closing_bracket, "Expected ']' to close array literal");

                const token& r = skipToNextType(ps, { tt_close_bracket });
                switch (r.tp) {
                    case tt_close_bracket: break;
                    default: {
                        if (n) ps->freeNode(n);
                        ps->commit();
                        return errorNode(ps);
                    }
                }
            }

            n->manuallySpecifyRange(ps->get());
            ps->consume();
            ps->commit();

            ParseNode* a = ps->newNode(nt_literal, tok);
            a->value_tp = lt_array;
            a->body = n;
            return a;
        }
        ParseNode* objectLiteralProperty(Parser* ps, bool expected) {
            ParseNode* n = identifier(ps);
            if (!n) return nullptr;

            if (!ps->typeIs(tt_colon)) {
                if (expected) {
                    ps->error(pm_expected_colon, utils::String::Format("Expected ':' after '%s'", n->str().c_str()));
                }
                ps->freeNode(n);
                return expected ? errorNode(ps) : nullptr;
            }

            ps->consume();

            n->tp = nt_object_literal_property;
            n->initializer = singleExpression(ps);
            if (!n->initializer) {
                ps->error(pm_expected_expr, utils::String::Format("Expected expression after '%s:'", n->str().c_str()));
                ps->freeNode(n);
                return errorNode(ps);
            }

            return n;
        }
        ParseNode* objectLiteral(Parser* ps) {
            const token* tok = &ps->get();
            if (!ps->typeIs(tt_open_brace)) return nullptr;
            ps->begin();
            ps->consume();

            ParseNode* f = objectLiteralProperty(ps, false);
            if (!f) {
                ps->revert();
                return nullptr;
            }
            
            ps->commit();
            
            if (isError(f)) {
                auto& t = skipToNextType(ps, { tt_comma, tt_close_brace });
                if (t.tp != tt_comma) {
                    if (t.tp == tt_close_brace) ps->consume();
                    return f;
                }

                // attempt to continue
                ps->begin();
                ps->consume();
                ParseNode* nf = objectLiteralProperty(ps, true);
                if (!nf || isError(nf)) {
                    // oh well
                    if (nf) ps->freeNode(nf);
                    ps->revert();
                    return f;
                }

                ps->commit();

                ps->freeNode(f);
                f = nf;
            }

            ParseNode* n = f;
            while (n) {
                if (!ps->typeIs(tt_comma)) break;
                ps->consume();
                n->next = objectLiteralProperty(ps, true);
                n = n->next;
                if (!n || isError(n)) {
                    if (!n) ps->error(pm_expected_object_property, "Expected object literal property after ','");

                    const token& r = skipToNextType(ps, { tt_comma, tt_close_brace });
                    switch (r.tp) {
                        case tt_comma: continue;
                        case tt_close_brace: {
                            ps->consume();
                            ps->freeNode(f);
                            return errorNode(ps);
                        }
                        default: {
                            ps->freeNode(f);
                            return errorNode(ps);
                        }
                    }
                }
            }

            if (!ps->typeIs(tt_close_brace)) {
                ps->error(pm_expected_closing_brace, "Expected '}' to close object literal");

                const token& r = skipToNextType(ps, { tt_close_brace });
                switch (r.tp) {
                    case tt_close_brace: break;
                    default: {
                        ps->freeNode(f);
                        return errorNode(ps);
                    }
                }
            }

            f->manuallySpecifyRange(ps->get());
            ps->consume();

            ParseNode* a = ps->newNode(nt_literal, tok);
            a->value_tp = lt_object;
            a->body = f;
            return a;
        }
        
        
        
        //
        // Expressions
        //
        
        ParseNode* primaryExpression(Parser* ps) {
            if (ps->isKeyword("this")) {
                ps->consume();
                return ps->newNode(nt_this, &ps->getPrev());
            }

            if (ps->isKeyword("null")) {
                ps->consume();
                ParseNode* n = ps->newNode(nt_literal, &ps->getPrev());
                n->value_tp = lt_null;
                return n;
            }

            if (ps->isKeyword("true")) {
                ps->consume();
                ParseNode* n = ps->newNode(nt_literal, &ps->getPrev());
                n->value_tp = lt_true;
                return n;
            }

            if (ps->isKeyword("false")) {
                ps->consume();
                ParseNode* n = ps->newNode(nt_literal, &ps->getPrev());
                n->value_tp = lt_false;
                return n;
            }

            if (ps->isKeyword("sizeof")) {
                ps->consume();
                ParseNode* n = ps->newNode(nt_sizeof, &ps->getPrev());

                if (!ps->isSymbol("<")) {
                    ps->error(pm_expected_single_template_arg, "Expected single template parameter after 'sizeof'");
                    ps->freeNode(n);
                    return errorNode(ps);
                }

                n->data_type = templateArgs(ps);
                if (!n->data_type) {
                    // error already emitted
                    ps->freeNode(n);
                    return errorNode(ps);
                }

                if (n->data_type->next) {
                    ps->error(pm_expected_single_template_arg, "Expected single template parameter after 'sizeof'", n->data_type->next->tok);
                    ps->freeNode(n->data_type->next);
                    n->data_type->next = nullptr;
                }

                return n;
            }

            if (ps->isKeyword("typeinfo")) {
                ps->consume();
                ParseNode* n = ps->newNode(nt_typeinfo, &ps->getPrev());

                if (!ps->isSymbol("<")) {
                    ps->error(pm_expected_single_template_arg, "Expected single template parameter after 'typeinfo'");
                    ps->freeNode(n);
                    return errorNode(ps);
                }

                n->data_type = templateArgs(ps);
                if (!n->data_type) {
                    // error already emitted
                    ps->freeNode(n);
                    return errorNode(ps);
                }

                if (n->data_type->next) {
                    ps->error(pm_expected_single_template_arg, "Expected single template parameter after 'typeinfo'", n->data_type->next->tok);
                    ps->freeNode(n->data_type->next);
                    n->data_type->next = nullptr;
                }

                return n;
            }

            // todo:
            // hex literal
            // regex literal?

            ParseNode* n = numberLiteral(ps);
            if (n) return n;

            n = stringLiteral(ps);
            if (n) return n;

            n = templateStringLiteral(ps);
            if (n) return n;

            n = arrayLiteral(ps);
            if (n) return n;

            n = objectLiteral(ps);
            if (n) return n;

            n = identifier(ps);
            if (n) {
                if (ps->isSymbol("<")) {
                    ps->begin();
                    ParseNode* tspec = identifierTypeSpecifier(ps, n);
                    if (tspec && !isError(tspec) && tspec->template_parameters && !isError(tspec->template_parameters)) {
                        ps->commit();
                        return tspec;
                    }

                    // It's not a type specifier
                    ps->revert();
                    ps->begin();

                    ParseNode* args = templateArgs(ps);
                    
                    if (args && !isError(args)) {
                        // it's probably a function specialization
                        ps->commit();
                        n->template_parameters = args;
                        return n;
                    } else ps->revert();

                    // It's neither, it's probably the start of a relational expression
                    // and n is just a variable reference
                }
                
                return n;
            }

            n = expressionSequenceGroup(ps);
            if (n) return n;

            return nullptr;
        }
        ParseNode* functionExpression(Parser* ps) {
            const token* tok = &ps->get();
            ps->begin();
            ParseNode* params = one_of(ps, { identifier, maybeParameterList });
            if (!params || isError(params)) {
                ps->revert();
                return nullptr;
            }

            if (!ps->typeIs(tt_forward)) {
                ps->revert();
                ps->freeNode(params);
                return nullptr;
            }
            ps->commit();
            ps->consume();

            ParseNode* n = ps->newNode(nt_function, tok);
            n->parameters = params;
            n->body = one_of(ps, { block, expression });

            if (!n->body || isError(n->body)) {
                if (!n->body) ps->error(pm_expected_function_body, "Expected arrow function body");
                ps->freeNode(n);
                return errorNode(ps);
            }

            return n;
        }
        ParseNode* memberExpression(Parser* ps) {
            ParseNode* n = functionExpression(ps);
            if (!n) n = primaryExpression(ps);
            if (!n) return nullptr;

            ParseNode* l = n;
            while (true) {
                if (ps->typeIs(tt_dot)) {
                    const token& tok = ps->get();
                    ps->consume();

                    ParseNode* r = identifier(ps);
                    if (!r) {
                        ps->error(pm_expected_identifier, "Expected identifier after '.'");
                        ps->freeNode(l);
                        return errorNode(ps);
                    }

                    ParseNode* e = ps->newNode(nt_expression, &tok);
                    e->op = op_member;
                    e->lvalue = l;
                    e->rvalue = r;
                    l = e;
                    continue;
                } else if (ps->typeIs(tt_open_bracket)) {
                    const token& tok = ps->get();
                    ps->consume();
                    
                    ParseNode* r = expression(ps);
                    if (!r) {
                        ps->error(pm_expected_expr, "Expected expression after '['");

                        const token& rt = skipToNextType(ps, { tt_close_bracket });
                        switch (rt.tp) {
                            case tt_close_bracket: {
                                r = errorNode(ps);
                                break;
                            }
                            default: {
                                ps->freeNode(l);
                                return errorNode(ps);
                            }
                        }
                    }

                    if (!ps->typeIs(tt_close_bracket)) {
                        ps->error(pm_expected_closing_bracket, "Expected ']'");

                        const token& rt = skipToNextType(ps, { tt_close_bracket });
                        switch (rt.tp) {
                            case tt_close_bracket: break;
                            default: {
                                ps->freeNode(l);
                                return errorNode(ps);
                            }
                        }
                    }
                    
                    ps->consume();

                    ParseNode* e = ps->newNode(nt_expression, &tok);
                    e->op = op_index;
                    e->lvalue = l;
                    e->rvalue = r;

                    l = e;
                    continue;
                }

                break;
            }

            return l;
        }
        ParseNode* newExpression(Parser* ps) {
            if (!ps->isKeyword("new")) return memberExpression(ps);
            ps->consume();

            ParseNode* n = ps->newNode(nt_expression, &ps->getPrev());
            n->op = op_new;
            n->body = memberExpression(ps);
            if (!n->body) {
                ps->error(pm_expected_type_specifier, "Expected type specifier after 'new'");
                ps->freeNode(n);
                return errorNode(ps);
            }

            if (n->body->tp == nt_expression) {
                if (n->body->op != op_member) {
                    ps->error(pm_expected_type_specifier, "Expected type specifier after 'new'");
                    ps->freeNode(n);
                    return errorNode(ps);
                }
            } else if (n->body->tp != nt_type_specifier && n->body->tp != nt_identifier) {
                ps->error(pm_expected_type_specifier, "Expected type specifier after 'new'");
                ps->freeNode(n);
                return errorNode(ps);
            }

            n->parameters = arguments(ps);

            return n;
        }
        ParseNode* callExpression(Parser* ps) {
            ps->begin();
            ParseNode* callee = newExpression(ps);
            if (!callee) {
                ps->revert();
                return nullptr;
            }

            const token& argTok = ps->get();

            ParseNode* args = arguments(ps);
            if (!args) {
                if (callee->tp == nt_expression && callee->op == op_new) {
                    // new expression is valid output here, the result of
                    // the new expression just isn't called itself
                    return callee;
                }

                ps->revert();
                ps->freeNode(callee);
                return nullptr;
            }

            ParseNode* n = ps->newNode(nt_expression, &argTok);
            n->op = op_call;
            n->lvalue = callee;
            n->parameters = args;

            while (true) {
                if (ps->typeIs(tt_open_parenth)) {
                    ParseNode* e = ps->newNode(nt_expression, &ps->get());
                    e->op = op_call;
                    e->lvalue = n;
                    e->parameters = arguments(ps);

                    n = e;
                    continue;
                } else if (ps->typeIs(tt_dot)) {
                    ps->consume();
                    ParseNode* e = ps->newNode(nt_expression, &ps->getPrev());
                    e->op = op_member;
                    e->lvalue = n;
                    e->rvalue = identifier(ps);
                    
                    if (!e->rvalue) {
                        auto& etok = ps->get();
                        ps->freeNode(e);
                        ps->error(pm_expected_identifier, "Expected identifier after '.'", etok);
                        return errorNode(ps);
                    }

                    n = e;
                    continue;
                } else if (ps->typeIs(tt_open_bracket)) {
                    ps->consume();
                    ParseNode* e = ps->newNode(nt_expression, &ps->getPrev());
                    e->op = op_index;
                    e->lvalue = n;
                    e->rvalue = expression(ps);
                    
                    if (!e->rvalue) {
                        ps->error(pm_expected_expr, "Expected expression after '['");

                        const token& rt = skipToNextType(ps, { tt_close_bracket });
                        switch (rt.tp) {
                            case tt_close_bracket: {
                                e->rvalue = errorNode(ps);
                                break;
                            }
                            default: {
                                ps->freeNode(e);
                                return errorNode(ps);
                            }
                        }
                    }

                    if (!ps->typeIs(tt_close_bracket)) {
                        ps->error(pm_expected_closing_bracket, "Expected ']'");

                        const token& rt = skipToNextType(ps, { tt_close_bracket });
                        switch (rt.tp) {
                            case tt_close_bracket: break;
                            default: {
                                ps->freeNode(e);
                                return errorNode(ps);
                            }
                        }
                    }

                    ps->consume();
                    n = e;
                    continue;
                }

                break;
            }

            ps->commit();

            return n;
        }
        ParseNode* leftHandSideExpression(Parser* ps) {
            ParseNode* n = nullptr;

            const token& f = ps->get();
            if (f.text == "*") {
                ps->consume();
                n = ps->newNode(nt_expression, &f);
                n->op = op_dereference;
                n->lvalue = callExpression(ps);
                if (!n->lvalue) n->lvalue = memberExpression(ps);
                if (!n->lvalue) {
                    ps->error(pm_expected_expr, "Expected expression after dereference operator");
                    ps->freeNode(n);
                    return errorNode(ps);
                }
            }

            if (!n) n = callExpression(ps);
            if (!n) n = memberExpression(ps);

            const token& t = ps->get();

            if (t.text == "as") {
                ParseNode* o = ps->newNode(nt_cast);
                ps->consume();

                o->body = n;
                o->data_type = typeSpecifier(ps);

                if (!o->data_type) {
                    ps->error(pm_expected_type_specifier, "Expected type specifier after 'as' keyword");
                    ps->freeNode(o);
                    return errorNode(ps);
                }

                n = o;
            }

            return n;
        }
        ParseNode* postfixExpression(Parser* ps) {
            ParseNode* n = leftHandSideExpression(ps);
            if (!n) return nullptr;

            const token& t = ps->get();
            if (t.text.size() == 2 && (t.text[0] == '-' || t.text[0] == '+') && t.text[0] == t.text[1]) {
                // --, ++
                ps->consume();

                ParseNode* e = ps->newNode(nt_expression, &t);
                e->op = (t.text[0] == '-') ? op_postDec : op_postInc;
                e->lvalue = n;

                return e;
            }

            return n;
        }
        ParseNode* unaryExpression(Parser* ps) {
            ParseNode* n = postfixExpression(ps);
            if (n) return n;

            const token& t = ps->get();
            if (t.text.size() == 1) {
                expr_operator op = op_undefined;
                switch (t.text[0]) {
                    case '-': { op = op_negate; break; }
                    case '~': { op = op_bitInv; break; }
                    case '!': { op = op_not; break; }
                    default: break;
                }

                if (op != op_undefined) {
                    ps->consume();

                    ParseNode* e = ps->newNode(nt_expression, &t);
                    e->op = op;
                    e->lvalue = unaryExpression(ps);

                    if (!e->lvalue) {
                        ps->error(pm_expected_expr, utils::String::Format("Expected expression after '%c'", t.text[0]));
                        ps->freeNode(e);
                        return errorNode(ps);
                    }

                    return e;
                }
            }

            if (t.text.size() == 2 && (t.text[0] == '-' || t.text[0] == '+') && t.text[0] == t.text[1]) {
                // --, ++
                ps->consume();

                ParseNode* e = ps->newNode(nt_expression, &t);
                e->op = (t.text[0] == '-') ? op_preDec : op_preInc;
                e->lvalue = unaryExpression(ps);
                
                if (!e->lvalue) {
                    ps->error(pm_expected_expr, utils::String::Format("Expected expression after '%c%c'", t.text[0], t.text[0]));
                    ps->freeNode(e);
                    return errorNode(ps);
                }

                return e;
            }

            return nullptr;
        }
        ParseNode* multiplicativeExpression(Parser* ps) {
            const token* first = &ps->get();
            ParseNode* n = unaryExpression(ps);

            const token* t = &ps->get();
            while (n && t->text.size() == 1 && (t->text[0] == '*' || t->text[0] == '/' || t->text[0] == '%')) {
                ps->consume();

                ParseNode* e = ps->newNode(nt_expression, first);
                e->op = (t->text[0] == '*') ? op_mul : ((t->text[0] == '/') ? op_div : op_mod);
                e->lvalue = n;
                e->rvalue = unaryExpression(ps);
                
                if (!e->rvalue) {
                    ps->error(pm_expected_expr, utils::String::Format("Expected expression after '%c'", t->text[0]));
                    ps->freeNode(e);
                    return errorNode(ps);
                }

                n = e;
                t = &ps->get();
            }

            return n;
        }
        ParseNode* additiveExpression(Parser* ps) {
            const token* first = &ps->get();
            ParseNode* n = multiplicativeExpression(ps);

            const token* t = &ps->get();
            while (n && t->text.size() == 1 && (t->text[0] == '+' || t->text[0] == '-')) {
                ps->consume();

                ParseNode* e = ps->newNode(nt_expression, first);
                e->op = (t->text[0] == '+') ? op_add : op_sub;
                e->lvalue = n;
                e->rvalue = multiplicativeExpression(ps);
                
                if (!e->rvalue) {
                    ps->error(pm_expected_expr, utils::String::Format("Expected expression after '%c'", t->text[0]));
                    ps->freeNode(e);
                    return errorNode(ps);
                }

                n = e;
                t = &ps->get();
            }

            return n;
        }
        ParseNode* shiftExpression(Parser* ps) {
            const token* first = &ps->get();
            ParseNode* n = additiveExpression(ps);

            const token* t = &ps->get();
            while (n && t->text.size() == 2 && (t->text[0] == '<' || t->text[0] == '>') && t->text[0] == t->text[1]) {
                ps->consume();

                ParseNode* e = ps->newNode(nt_expression, first);
                e->op = (t->text[0] == '<') ? op_shLeft : op_shRight;
                e->lvalue = n;
                e->rvalue = additiveExpression(ps);
                
                if (!e->rvalue) {
                    ps->error(pm_expected_expr, utils::String::Format("Expected expression after '%c%c'", t->text[0], t->text[0]));
                    ps->freeNode(e);
                    return errorNode(ps);
                }

                n = e;
                t = &ps->get();
            }

            return n;
        }
        ParseNode* relationalExpression(Parser* ps) {
            const token* first = &ps->get();
            ParseNode* n = shiftExpression(ps);

            const token* t = &ps->get();
            while (n && (t->text[0] == '<' || t->text[0] == '>') && (t->text.size() == 1 || t->text[1] == '=')) {
                ps->consume();

                ParseNode* e = ps->newNode(nt_expression, first);

                const char* sym = nullptr;
                if (t->text[0] == '<') {
                    if (t->text.size() == 1) {
                        e->op = op_lessThan;
                        sym = "<";
                    } else {
                        e->op = op_lessThanEq;
                        sym = "<=";
                    }
                } else {
                    if (t->text.size() == 1) {
                        e->op = op_greaterThan;
                        sym = ">";
                    } else {
                        e->op = op_greaterThanEq;
                        sym = ">=";
                    }
                }

                e->lvalue = n;
                e->rvalue = shiftExpression(ps);
                
                if (!e->rvalue) {
                    ps->error(pm_expected_expr, utils::String::Format("Expected expression after '%s'", sym));
                    ps->freeNode(e);
                    return errorNode(ps);
                }

                n = e;
                t = &ps->get();
            }

            return n;
        }
        ParseNode* equalityExpression(Parser* ps) {
            const token* first = &ps->get();
            ParseNode* n = relationalExpression(ps);

            const token* t = &ps->get();
            while (n && t->text.size() == 2 && (t->text[0] == '!' || t->text[0] == '=')) {
                ps->consume();

                ParseNode* e = ps->newNode(nt_expression, first);
                e->op = (t->text[0] == '!') ? op_notEq : op_compare;
                e->lvalue = n;
                e->rvalue = relationalExpression(ps);
                
                if (!e->rvalue) {
                    ps->error(pm_expected_expr, utils::String::Format("Expected expression after '%c='", t->text[0]));
                    ps->freeNode(e);
                    return errorNode(ps);
                }

                n = e;
                t = &ps->get();
            }

            return n;
        }
        ParseNode* bitwiseAndExpression(Parser* ps) {
            const token* first = &ps->get();
            ParseNode* n = equalityExpression(ps);

            while (n && ps->textIs("&")) {
                ps->consume();

                ParseNode* e = ps->newNode(nt_expression, first);
                e->op = op_bitAnd;
                e->lvalue = n;
                e->rvalue = equalityExpression(ps);
                
                if (!e->rvalue) {
                    ps->error(pm_expected_expr, "Expected expression after '&'");
                    ps->freeNode(e);
                    return errorNode(ps);
                }

                n = e;
            }

            return n;
        }
        ParseNode* XOrExpression(Parser* ps) {
            const token* first = &ps->get();
            ParseNode* n = bitwiseAndExpression(ps);

            while (n && ps->textIs("^")) {
                ps->consume();

                ParseNode* e = ps->newNode(nt_expression, first);
                e->op = op_xor;
                e->lvalue = n;
                e->rvalue = bitwiseAndExpression(ps);
                
                if (!e->rvalue) {
                    ps->error(pm_expected_expr, "Expected expression after '^'");
                    ps->freeNode(e);
                    return errorNode(ps);
                }

                n = e;
            }

            return n;
        }
        ParseNode* bitwiseOrExpression(Parser* ps) {
            const token* first = &ps->get();
            ParseNode* n = XOrExpression(ps);

            while (n && ps->textIs("|")) {
                ps->consume();

                ParseNode* e = ps->newNode(nt_expression, first);
                e->op = op_bitOr;
                e->lvalue = n;
                e->rvalue = XOrExpression(ps);
                
                if (!e->rvalue) {
                    ps->error(pm_expected_expr, "Expected expression after '|'");
                    ps->freeNode(e);
                    return errorNode(ps);
                }

                n = e;
            }

            return n;
        }
        ParseNode* logicalAndExpression(Parser* ps) {
            const token* first = &ps->get();
            ParseNode* n = bitwiseOrExpression(ps);

            while (n && ps->textIs("&&")) {
                ps->consume();

                ParseNode* e = ps->newNode(nt_expression, first);
                e->op = op_logAnd;
                e->lvalue = n;
                e->rvalue = bitwiseOrExpression(ps);
                
                if (!e->rvalue) {
                    ps->error(pm_expected_expr, "Expected expression after '&&'");
                    ps->freeNode(e);
                    return errorNode(ps);
                }

                n = e;
            }

            return n;
        }
        ParseNode* logicalOrExpression(Parser* ps) {
            const token* first = &ps->get();
            ParseNode* n = logicalAndExpression(ps);

            while (n && ps->textIs("||")) {
                ps->consume();

                ParseNode* e = ps->newNode(nt_expression, first);
                e->op = op_logOr;
                e->lvalue = n;
                e->rvalue = logicalAndExpression(ps);
                
                if (!e->rvalue) {
                    ps->error(pm_expected_expr, "Expected expression after '||'");
                    ps->freeNode(e);
                    return errorNode(ps);
                }

                n = e;
            }

            return n;
        }
        ParseNode* conditionalExpression(Parser* ps) {
            const token* first = &ps->get();
            ParseNode* n = logicalOrExpression(ps);

            if (n && ps->get().text[0] == '?') {
                ps->consume();
                ParseNode* c = ps->newNode(nt_expression, first);
                c->op = op_conditional;
                c->cond = n;
                c->lvalue = assignmentExpression(ps);

                if (!c->lvalue) {
                    ps->error(pm_expected_expr, "Expected expression for conditional expression's truth result");
                    ps->freeNode(c);
                    return errorNode(ps);
                }

                if (!ps->typeIs(tt_colon)) {
                    ps->error(pm_expected_symbol, "Expected ':' after conditional expression's truth result");
                    ps->freeNode(c);
                    return errorNode(ps);
                }
                ps->consume();
                c->rvalue = assignmentExpression(ps);

                if (!c->rvalue) {
                    ps->error(pm_expected_expr, "Expected expression for conditional expression's false result");
                    ps->freeNode(c);
                    return errorNode(ps);
                }

                return c;
            }

            return n;
        }
        ParseNode* assignmentExpression(Parser* ps) {
            const token* first = &ps->get();
            ps->begin();
            ParseNode* n = leftHandSideExpression(ps);

            if (n) {
                bool isAssignment = isAssignmentOperator(ps);

                if (isAssignment) {
                    ps->commit();
                    do {
                        expr_operator op = getOperatorType(ps);
                        ps->consume();
                        ParseNode* rvalue = assignmentExpression(ps);

                        if (!rvalue) {
                            ps->freeNode(n);
                            ps->error(pm_expected_expr, "Expected expression for rvalue");
                            return errorNode(ps);
                        }

                        ParseNode* e = ps->newNode(nt_expression, first);
                        e->op = op;
                        e->lvalue = n;
                        e->rvalue = rvalue;
                        n = e;
                    } while (isAssignmentOperator(ps));

                    return n;
                }
            }

            ps->revert();
            return conditionalExpression(ps);
        }
        ParseNode* singleExpression(Parser* ps) {
            return assignmentExpression(ps);
        }
        ParseNode* expression(Parser* ps) {
            ParseNode* n = assignmentExpression(ps);
            ParseNode* b = n;
            while (b && ps->typeIs(tt_comma)) {
                ps->consume();

                b->next = assignmentExpression(ps);
                if (!b->next || isError(b->next)) {
                    if (!isError(b->next)) ps->error(pm_expected_expr, "Expected expression after ','");

                    const token& r = skipToNextType(ps, { tt_comma, tt_semicolon, tt_close_parenth, tt_close_bracket });
                    switch (r.tp) {
                        case tt_comma: continue;
                        default: {
                            ps->freeNode(n);
                            return errorNode(ps);
                        }
                    }
                }

                b = b->next;
            }

            return n;
        }
        ParseNode* expressionSequence(Parser* ps) {
            ParseNode* f = singleExpression(ps);
            if (!f) return nullptr;
            
            ParseNode* n = f;
            while (true) {
                if (!ps->typeIs(tt_comma)) break;
                ps->consume();

                n->next = singleExpression(ps);
                n = n->next;

                if (!n) {
                    ps->error(pm_expected_expr, "Expected expression after ','");
                    const token& r = skipToNextType(ps, { tt_comma, tt_semicolon, tt_close_parenth });
                    switch (r.tp) {
                        case tt_comma: continue;
                        default: {
                            ps->freeNode(f);
                            return errorNode(ps);
                        }
                    }
                }
            }

            if (f->next) {
                ParseNode* s = ps->newNode(nt_expression_sequence, &f->tok);
                s->body = f;
                return s;
            }

            return f;
        }
        ParseNode* expressionSequenceGroup(Parser* ps) {
            if (!ps->typeIs(tt_open_parenth)) return nullptr;
            ps->begin();
            ps->consume();
            
            ParseNode* n = expressionSequence(ps);
            
            if (!n) {
                ps->revert();
                return nullptr;
            }

            ps->commit();
            if (!ps->typeIs(tt_close_parenth)) {
                ps->error(pm_expected_closing_parenth, "Expected ')'");

                const token& r = skipToNextType(ps, { tt_close_parenth, tt_semicolon });
                switch (r.tp) {
                    case tt_close_parenth: break;
                    default: {
                        ps->freeNode(n);
                        return errorNode(ps);
                    }
                }

                return n;
            }

            ps->consume();

            return n;
        }
        
        
        //
        // Declarations
        //

        ParseNode* variableDecl(Parser* ps) {
            ParseNode* _assignable = one_of(ps, { typedAssignable, assignable, objectDecompositor });
            if (!_assignable) return nullptr;
            if (isError(_assignable)) return _assignable;

            ParseNode* decl = ps->newNode(nt_variable, &_assignable->tok);
            decl->body = _assignable;
            if (ps->isSymbol("=")) {
                ps->consume();
                decl->initializer = singleExpression(ps);
                if (!decl->initializer || isError(decl->initializer)) {
                    if (!decl->initializer) ps->error(pm_expected_expr, "Expected expression for variable initializer");
                    
                    const token& r = skipToNextType(ps, { tt_semicolon });
                    switch (r.tp) {
                        case tt_semicolon: break;
                        default: {
                            ps->freeNode(decl);
                            return errorNode(ps);
                        }
                    }
                }
            }

            return decl;
        }
        ParseNode* variableDeclList(Parser* ps) {
            bool isConst = ps->isKeyword("const");
            if (!isConst && !ps->isKeyword("let")) return nullptr;
            const token* tok = &ps->get();

            ps->consume();

            ParseNode* f = variableDecl(ps);
            if (!f || isError(f)) {
                if (!isError(f)) ps->error(pm_expected_variable_decl, "Expected variable declaration");
                else return f;
                return errorNode(ps);
            }

            f->flags.is_const = isConst ? 1 : 0;
            f->tok = *tok;

            ParseNode* n = f;
            while (true) {
                if (!ps->typeIs(tt_comma)) break;
                ps->consume();

                n->next = variableDecl(ps);
                n = n->next;

                if (!n) {
                    ps->error(pm_expected_variable_decl, "Expected variable declaration");
                    
                    const token& r = skipToNextType(ps, { tt_comma, tt_semicolon });
                    switch (r.tp) {
                        case tt_comma: continue;
                        default: {
                            ps->freeNode(f);
                            return errorNode(ps);
                        }
                    }
                }

                n->flags.is_const = isConst ? 1 : 0;
            }

            return f;
        }
        ParseNode* classPropertyOrMethod(Parser* ps) {
            access_modifier am = public_access;
            bool isStatic = false;
            bool isPointer = false;
            const token& ft = ps->get();

            bool didCommit = false;
            ps->begin();

            if (ps->isKeyword("public")) {
                ps->commit();
                didCommit = true;
                ps->consume();
            } else if (ps->isKeyword("private")) {
                ps->commit();
                didCommit = true;
                am = private_access;
                ps->consume();
            }

            if (ps->isKeyword("static")) {
                if (!didCommit) ps->commit();
                didCommit = true;
                isStatic = true;
                ps->consume();
            }

            bool isGetter = false;
            bool isSetter = false;

            if (ps->isKeyword("get")) {
                if (!didCommit) ps->commit();
                didCommit = true;
                isGetter = true;
                ps->consume();
            } else if (ps->isKeyword("set")) {
                if (!didCommit) ps->commit();
                didCommit = true;
                isSetter = true;
                ps->consume();
            }

            ParseNode* n = identifier(ps);
            bool isOperator = false;
            bool isCastOperator = false;
            if (!n && ps->typeIs(tt_keyword) && ps->textIs("operator")) {
                if (isGetter || isSetter) {
                    ps->error(pm_reserved_word, "Cannot name a getter or setter method 'operator', 'operator' is a reserved word");
                    isGetter = isSetter = false;
                }

                // treat "operator" as an identifier for the method name
                n = ps->newNode(nt_identifier);
                const token& t = ps->get();
                n->value.s = t.text.c_str();
                n->str_len = t.text.size();
                ps->consume();
                n->op = getOperatorType(ps);
                if (n->op == op_undefined) {
                    n->data_type = typeSpecifier(ps);
                    if (!n->data_type) {
                        ps->error(pm_expected_operator_override_target, "Expected operator or type specifier after 'operator' keyword");
                
                        const token& r = skipToNextType(ps, { tt_open_brace });
                        switch (r.tp) {
                            case tt_open_brace: break;
                            default: {
                                ps->freeNode(n);
                                return errorNode(ps);
                            }
                        }
                    }

                    isCastOperator = true;
                } else {
                    ps->consume();
                    if (n->op == op_call || n->op == op_index) {
                        // actually two tokens
                        ps->consume();
                    }
                }

                isOperator = true;
            }

            if (!n) {
                if (isGetter) {
                    ps->error(pm_expected_identifier, "Expected property name for getter method");
                    
                    const token& r = skipToNextType(ps, { tt_open_brace });
                    switch (r.tp) {
                        case tt_open_brace: {
                            n = errorNode(ps);
                            break;
                        }
                        default: return errorNode(ps);
                    }
                } else if (isSetter) {
                    ps->error(pm_expected_identifier, "Expected property name for setter method");
                    
                    const token& r = skipToNextType(ps, { tt_open_brace });
                    switch (r.tp) {
                        case tt_open_brace: {
                            n = errorNode(ps);
                            break;
                        }
                        default: return errorNode(ps);
                    }
                } else if (didCommit) {
                    ps->error(pm_expected_identifier, "Expected property or method name");
                    
                    const token& r = skipToNextType(ps, { tt_semicolon, tt_open_parenth });
                    switch (r.tp) {
                        case tt_semicolon: {
                            n = errorNode(ps);
                            break;
                        }
                        case tt_open_parenth: {
                            n = errorNode(ps);
                            break;
                        }
                        default: return errorNode(ps);
                    }
                } else {
                    ps->revert();
                    return nullptr;
                }
            }

            if (!didCommit) {
                ps->commit();
                didCommit = true;
            }
            
            n->flags.is_private = (am == private_access) ? 1 : 0;
            n->flags.is_static = isStatic ? 1 : 0;
            n->flags.is_getter = isGetter ? 1 : 0;
            n->flags.is_setter = isSetter ? 1 : 0;
            n->tok = ft;

            if (!isGetter && !isSetter && ps->typeIs(tt_colon)) {
                if (n->str() == "constructor") {
                    ps->error(pm_reserved_word, "Cannot name a class property 'constructor', which is a reserved word", n->tok);
                } else if (n->str() == "destructor") {
                    ps->error(pm_reserved_word, "Cannot name a class property 'destructor', which is a reserved word", n->tok);
                }

                ps->consume();
                n->tp = nt_property;
                n->data_type = typeSpecifier(ps);
                if (!n->data_type || isError(n->data_type)) {
                    if (!isError(n->data_type)) ps->error(pm_expected_type_specifier, "Expected type specifier for class property");
                }

                if (!ps->typeIs(tt_semicolon)) {
                    ps->error(pm_expected_eos, "Expected ';' after property declaration");
                    
                    const token& r = skipToNextType(ps, { tt_semicolon });
                    switch (r.tp) {
                        case tt_semicolon: break;
                        default: {
                            ps->freeNode(n);
                            return errorNode(ps);
                        }
                    }
                }
                
                ps->consume();
                return n;
            }

            ParseNode* templArgs = nullptr;
            if (ps->isSymbol("<")) {
                templArgs = templateParams(ps);
                if (!templArgs || isError(templArgs)) {
                    
                    const token& r = skipToNextType(ps, { tt_open_parenth, tt_open_brace, tt_close_brace });
                    switch (r.tp) {
                        case tt_open_parenth: break;
                        default: {
                            ps->freeNode(n);
                            return errorNode(ps);
                        }
                    }
                }
            }

            if (ps->typeIs(tt_open_parenth)) {
                n->tp = nt_function;
                n->parameters = typedParameterList(ps);
                n->template_parameters = templArgs;

                if (ps->typeIs(tt_colon)) {
                    if (isCastOperator) {
                        ps->error(pm_unexpected_type_specifier, "Cast operator overrides should not have return types specified");
                        // attempt to continue
                        ps->freeNode(n->data_type); // (was already allocated)
                        n->data_type = nullptr;
                    } else if (n->str() == "constructor") {
                        ps->error(pm_unexpected_type_specifier, "Class constructors should not have return types specified");
                        // attempt to continue
                    } else if (n->str() == "destructor") {
                        ps->error(pm_unexpected_type_specifier, "Class destructors should not have return types specified");
                        // attempt to continue
                    }

                    ps->consume();
                    n->data_type = typeSpecifier(ps);
                    if (!n->data_type) {
                        if (!isCastOperator && n->str() != "constructor" && n->str() != "destructor") {
                            ps->error(pm_expected_type_specifier, "Expected type specifier for return value of class method");
                        }

                        const token& r = skipToNextType(ps, { tt_open_brace });
                        switch (r.tp) {
                            case tt_open_brace: {
                                n->data_type = errorNode(ps);
                                break;
                            }
                            default: {
                                ps->freeNode(n);
                                return errorNode(ps);
                            }
                        }
                    }
                } else if (isGetter || isSetter) {
                    ps->error(pm_expected_return_type, "Getter/setter methods must specify return types explicitly");
                }

                n->body = block(ps);
                if (!n->body) {
                    ps->error(pm_expected_function_body, "Expected function body after class method declaration");

                    const token& r = skipToNextType(ps, { tt_close_brace });
                    switch (r.tp) {
                        case tt_close_brace: {
                            n->body = errorNode(ps);
                            break;
                        }
                        default: {
                            ps->freeNode(n);
                            return errorNode(ps);
                        }
                    }
                }

                return n;
            } else if (isOperator || templArgs) {
                // Only possibility is that it was supposed to be a method
                ps->error(pm_expected_parameter_list, "Expected parameter list for operator override or method");
                ps->freeNode(n);
                return errorNode(ps);
            } else if (isGetter) {
                ps->error(pm_expected_identifier, "Expected class property getter method");
                ps->freeNode(n);
                return errorNode(ps);
            } else if (isSetter) {
                ps->error(pm_expected_identifier, "Expected class property setter method");
                ps->freeNode(n);
                return errorNode(ps);
            }
        
            ps->freeNode(n);
            ps->error(pm_malformed_class_element, "Expected property type specifier or method parameter list after identifier");
            return errorNode(ps);
        }
        ParseNode* classDef(Parser* ps) {
            if (!ps->isKeyword("class")) return nullptr;
            const token& ft = ps->get();
            ps->consume();

            ParseNode* n = identifier(ps);
            if (!n || isError(n)) {
                if (!n) ps->error(pm_expected_identifier, "Expected identifier for class name");

                const token& r = skipToNextKeyword(ps, "extends", { tt_open_brace });
                switch (r.tp) {
                    case tt_open_brace: {
                        if (!n) n = errorNode(ps);
                        break;
                    }
                    case tt_keyword: {
                        if (!n) n = errorNode(ps);
                        break;
                    }
                    default: {
                        ps->freeNode(n);
                        return errorNode(ps);
                    }
                }
            }

            n->tok = ft;
            n->tp = nt_class;
            n->template_parameters = templateParams(ps);

            if (n->template_parameters && isError(n->template_parameters)) {
                const token& r = skipToNextKeyword(ps, "extends", { tt_open_brace });
                switch (r.tp) {
                    case tt_open_brace: break;
                    case tt_keyword: break;
                    default: {
                        ps->freeNode(n);
                        return errorNode(ps);
                    }
                }
            }

            if (ps->isKeyword("extends")) {
                ps->consume();
                n->inheritance = list_of(
                    ps,
                    typeSpecifier,
                    pm_expected_type_specifier,
                    "Expected one or more type specifiers after 'extends'",
                    pm_expected_type_specifier,
                    "Expected type specifier after ','"
                );
                
                if (!n->inheritance || isError(n->inheritance)) {
                    const token& r = skipToNextType(ps, { tt_open_brace });
                    switch (r.tp) {
                        case tt_open_brace: break;
                        default: {
                            // error already emitted
                            ps->freeNode(n);
                            return errorNode(ps);
                        }
                    }
                }
            }

            if (!ps->typeIs(tt_open_brace)) {
                ps->error(pm_expected_open_brace, "Expected '{' after class name");
                ps->freeNode(n);
                return nullptr;
            }
            ps->consume();

            n->body = array_of(ps, classPropertyOrMethod);

            if (!n->body || isError(n->body)) {
                if (!n->body) {
                    // no error emitted yet
                    ps->error(pm_empty_class, "Class body cannot be empty");
                }

                const token& r = skipToNextType(ps, { tt_close_brace });
                switch (r.tp) {
                    case tt_close_brace: break;
                    default: {
                        ps->freeNode(n);
                        return errorNode(ps);
                    }
                }
            }

            if (!ps->typeIs(tt_close_brace)) {
                ps->error(pm_expected_open_brace, "Expected '}' after class body");
                ps->freeNode(n);
                return nullptr;
            }
            n->manuallySpecifyRange(ps->get());
            ps->consume();

            if (!ps->typeIs(tt_semicolon)) {
                ps->error(pm_expected_eos, "Expected ';' after class definition");
                // attempt to continue
            } else ps->consume();

            return n;
        }
        ParseNode* typeDef(Parser* ps) {
            if (!ps->isKeyword("type")) return nullptr;
            const token* t = &ps->get();
            ps->consume();

            ParseNode* n = identifier(ps);

            if (!n || isError(n)) {
                if (!n) ps->error(pm_expected_identifier, "Expected identifier for type name");

                const token& r = skipToNextType(ps, { tt_open_brace, tt_identifier });
                switch (r.tp) {
                    case tt_open_brace: {
                        if (!n) n = errorNode(ps);
                        break;
                    }
                    case tt_identifier: {
                        if (!n) n = errorNode(ps);
                        break;
                    }
                    default: {
                        ps->freeNode(n);
                        return errorNode(ps);
                    }
                }
            }

            n->template_parameters = templateParams(ps);

            if (n->template_parameters && isError(n->template_parameters)) {
                const token& r = skipToNextSymbol(ps, "=", { tt_open_brace, tt_identifier });
                switch (r.tp) {
                    case tt_symbol: break;
                    case tt_open_brace: break;
                    case tt_identifier: break;
                    default: {
                        ps->freeNode(n);
                        return errorNode(ps);
                    }
                }
            }

            if (!ps->isSymbol("=")) {
                ps->error(pm_expected_symbol, utils::String::Format("Expected '=' after 'type %s'", n->str().c_str()));
                
                const token& r = skipToNextType(ps, { tt_open_brace, tt_identifier });
                switch (r.tp) {
                    case tt_open_brace: break;
                    case tt_identifier: break;
                    default: {
                        ps->freeNode(n);
                        return errorNode(ps);
                    }
                }
            }
            ps->consume();

            n->tp = nt_type;
            n->tok = *t;
            n->data_type = typeSpecifier(ps);
            if (!n->data_type) {
                ps->error(pm_expected_type_specifier, utils::String::Format("Expected type specifier after 'type %s ='", n->str().c_str()));
                ps->freeNode(n);
                return errorNode(ps);
            }

            if (!ps->typeIs(tt_semicolon)) {
                ps->error(pm_expected_eos, "Expected ';' after type definition");
                // attempt to continue
            } else ps->consume();

            return n;
        }
        ParseNode* functionDecl(Parser* ps) {
            if (!ps->isKeyword("function")) return nullptr;
            const token* t = &ps->get();
            ps->consume();

            ParseNode* n = identifier(ps);

            if (!n || isError(n)) {
                if (!n) ps->error(pm_expected_identifier, "Expected identifier after 'function'");

                const token& r = skipToNextType(ps, { tt_open_parenth });
                switch (r.tp) {
                    case tt_open_parenth: {
                        if (!n) n = errorNode(ps);
                        break;
                    }
                    default: {
                        ps->freeNode(n);
                        return errorNode(ps);
                    }
                }
            }

            n->tp = nt_function;
            n->tok = *t;
            n->template_parameters = templateParams(ps);
            n->parameters = parameterList(ps);
            if (!n->parameters || isError(n->parameters)) {
                if (!n->parameters) ps->error(pm_expected_parameter_list, "Expected function parameter list");

                const token& r = skipToNextType(ps, { tt_open_brace, tt_semicolon });
                switch (r.tp) {
                    case tt_open_parenth: break;
                    case tt_semicolon: break;
                    default: {
                        ps->freeNode(n);
                        return errorNode(ps);
                    }
                }
            }

            return n;
        }
        ParseNode* functionDef(Parser* ps) {
            ParseNode* n = functionDecl(ps);
            if (!n) return nullptr;

            n->body = block(ps);

            if (!n->body) {
                ps->error(pm_expected_eos, "Expected function body");
            }

            return n;
        }
        ParseNode* declaration(Parser* ps) {
            return one_of(ps, {
                variableStatement,
                classDef,
                typeDef,
                functionDef
            });
        }
        
        
        
        //
        // Statements
        //

        ParseNode* variableStatement(Parser* ps) {
            return all_of(ps, { variableDeclList, eosRequired });
        }
        ParseNode* emptyStatement(Parser* ps) {
            return eos(ps);
        }
        ParseNode* expressionStatement(Parser* ps) {
            ParseNode* expr = expressionSequence(ps);
            if (!expr) return nullptr;

            if (isError(expr)) {
                const token& r = skipToNextType(ps, { tt_semicolon });
                if (r.tp != tt_semicolon) return expr;
            }

            expr->next = eosRequired(ps);
            
            return expr;
        }
        ParseNode* ifStatement(Parser* ps) {
            if (!ps->isKeyword("if")) return nullptr;
            ps->consume();
            ParseNode* n = ps->newNode(nt_if, &ps->getPrev());
            n->cond = expressionSequenceGroup(ps);
            if (!n->cond) {
                ps->error(pm_expected_expgroup, "Expected '(<expression>)' after 'if'");
                ps->freeNode(n);
                return nullptr;
            }

            n->body = statement(ps);
            if (!n->body) {
                ps->error(pm_expected_statement, "Expected block or statement after 'if (<expression>)'");
                ps->freeNode(n);
                return nullptr;
            }

            if (n->body->tp != nt_scoped_block) {
                n->manuallySpecifyRange(ps->get());
            }
            
            if (ps->isKeyword("else")) {
                ps->consume();
                n->else_body = statement(ps);
                if (!n->else_body) {
                    ps->error(pm_expected_statement, "Expected block or statement after 'else'");
                    ps->freeNode(n);
                    return nullptr;
                }

                if (n->else_body->tp != nt_scoped_block) {
                    n->manuallySpecifyRange(ps->get());
                }
            }

            return n;
        }
        ParseNode* continueStatement(Parser* ps) {
            if (!ps->isKeyword("continue")) return nullptr;
            ps->consume();

            if (!ps->typeIs(tt_semicolon)) {
                ps->error(pm_expected_eos, "Expected ';' after 'continue'");
                // attempt to continue
            } else ps->consume();

            return ps->newNode(nt_continue, &ps->getPrev());
        }
        ParseNode* breakStatement(Parser* ps) {
            if (!ps->isKeyword("break")) return nullptr;
            ps->consume();

            if (!ps->typeIs(tt_semicolon)) {
                ps->error(pm_expected_eos, "Expected ';' after 'break'");
                // attempt to continue
            } else ps->consume();

            return ps->newNode(nt_continue, &ps->getPrev());
        }
        ParseNode* iterationStatement(Parser* ps) {
            if (ps->isKeyword("do")) {
                ps->consume();
                ParseNode* n = ps->newNode(nt_loop, &ps->getPrev());
                n->body = statement(ps);
                if (!n->body) {
                    ps->error(pm_expected_statement, "Expected block or statement after 'do'");
                    ps->freeNode(n);
                    return nullptr;
                }

                if (!ps->isKeyword("while")) {
                    ps->error(pm_expected_while, "Expected 'while' after 'do ...'");
                    ps->freeNode(n);
                    return nullptr;
                }
                ps->consume();

                n->cond = expressionSequenceGroup(ps);
                if (!n->cond) {
                    ps->error(pm_expected_expgroup, "Expected '(<expression>)' after 'do ... while'");
                    ps->freeNode(n);
                    return nullptr;
                }

                if (!ps->typeIs(tt_semicolon)) {
                    ps->error(pm_expected_eos, "Expected ';' after 'do ... while (<expression>)'");
                    // attempt to continue
                } else {
                    if (n->body->tp != nt_scoped_block) n->manuallySpecifyRange(ps->get());
                    ps->consume();
                }
                
                n->flags.defer_cond = 1;
                return n;
            } else if (ps->isKeyword("while")) {
                ps->consume();
                ParseNode* n = ps->newNode(nt_loop, &ps->getPrev());
                n->cond = expressionSequenceGroup(ps);
                if (!n->cond) {
                    ps->error(pm_expected_expgroup, "Expected '(<expression>)' after 'while'");
                    ps->freeNode(n);
                    return nullptr;
                }

                n->body = statement(ps);
                if (!n->body) {
                    ps->error(pm_expected_statement, "Expected block or statement after 'while (<expression>)'");
                    ps->freeNode(n);
                    return nullptr;
                }

                if (n->body->tp != nt_scoped_block) {
                    if (!ps->typeIs(tt_semicolon)) {
                        ps->error(pm_expected_eos, "Expected ';' after 'while (<expression>) statement'");
                        // attempt to continue
                    } else {
                        n->manuallySpecifyRange(ps->get());
                        ps->consume();
                    }
                }

                return n;
            } else if (ps->isKeyword("for")) {
                ps->consume();
                ParseNode* n = ps->newNode(nt_loop, &ps->getPrev());

                if (!ps->typeIs(tt_open_parenth)) {
                    ps->error(pm_expected_open_parenth, "Expected '(' after 'for'");
                    ps->freeNode(n);
                    return nullptr;
                }
                ps->consume();

                n->initializer = one_of(ps, { expressionSequence, variableDeclList });

                if (!ps->typeIs(tt_semicolon)) {
                    ps->error(pm_expected_eos, "Expected ';' after 'for (...'");
                    ps->freeNode(n);
                    return nullptr;
                }
                ps->consume();

                n->cond = expressionSequence(ps);

                if (!ps->typeIs(tt_semicolon)) {
                    ps->error(pm_expected_eos, "Expected ';' after 'for (...;...'");
                    ps->freeNode(n);
                    return nullptr;
                }
                ps->consume();

                n->modifier = expressionSequence(ps);
                
                if (!ps->typeIs(tt_close_parenth)) {
                    ps->error(pm_expected_closing_parenth, "Expected ')' after 'for (...;...;...'");
                    ps->freeNode(n);
                    return nullptr;
                }
                ps->consume();

                n->body = statement(ps);
                if (!n->body) {
                    ps->error(pm_expected_statement, "Expected block or statement after 'for (...;...;...)'");
                    ps->freeNode(n);
                    return nullptr;
                }

                if (n->body->tp != nt_scoped_block) {
                    if (!ps->typeIs(tt_semicolon)) {
                        ps->error(pm_expected_eos, "Expected ';' after 'for (...;...;...) statement'");
                        // attempt to continue
                    } else {
                        n->manuallySpecifyRange(ps->get());
                        ps->consume();
                    }
                }

                return n;
            }

            return nullptr;
        }
        ParseNode* deleteStatement(Parser* ps) {
            if (!ps->isKeyword("delete")) return nullptr;
            ps->consume();

            ParseNode* n = ps->newNode(nt_delete, &ps->getPrev());
            n->body = singleExpression(ps);

            if (!ps->typeIs(tt_semicolon)) {
                ps->error(pm_expected_eos, "Expected ';' after delete statement");
                // attempt to continue
            } else ps->consume();

            return n;
        }
        ParseNode* returnStatement(Parser* ps) {
            if (!ps->isKeyword("return")) return nullptr;
            ps->consume();

            ParseNode* n = ps->newNode(nt_return, &ps->getPrev());
            n->body = singleExpression(ps);

            if (!ps->typeIs(tt_semicolon)) {
                ps->error(pm_expected_eos, "Expected ';' after return statement");
                // attempt to continue
            } else ps->consume();

            return n;
        }
        ParseNode* switchCase(Parser* ps) {
            if (!ps->isKeyword("case")) return nullptr;
            ps->consume();

            ParseNode* n = ps->newNode(nt_switch_case, &ps->getPrev());
            n->cond = expressionSequence(ps);
            if (!n->cond) {
                ps->error(pm_expected_expgroup, "Expected '(<expression>)' after 'case'");
                ps->freeNode(n);
                return nullptr;
            }
            
            if (!ps->typeIs(tt_colon)) {
                ps->error(pm_expected_colon, "Expected ':' after 'case <expression>'");
                // attempt to continue
            } else ps->consume();

            n->body = statement(ps);

            return n;
        }
        ParseNode* switchStatement(Parser* ps) {
            if (!ps->isKeyword("switch")) return nullptr;
            ps->consume();

            ParseNode* n = ps->newNode(nt_return, &ps->getPrev());
            n->cond = expressionSequenceGroup(ps);
            if (!n->cond) {
                ps->error(pm_expected_expgroup, "Expected '(<expression>)' after 'switch'");
                ps->freeNode(n);
                return nullptr;
            }

            if (!ps->typeIs(tt_open_brace)) {
                ps->error(pm_expected_open_brace, "Expected '{' after 'switch (<expression>)'");
                // attempt to continue
            } else ps->consume();

            n->body = array_of(ps, { switchCase });

            if (ps->isKeyword("default")) {
                ps->consume();

                if (!ps->typeIs(tt_colon)) {
                    ps->error(pm_expected_colon, "Expected ':' after 'switch (<expression>) { ... default'");
                    // attempt to continue
                } else ps->consume();

                n->else_body = statement(ps);
            }

            if (!ps->typeIs(tt_close_brace)) {
                ps->error(pm_expected_closing_brace, "Expected '}' after 'switch (<expression>) { ...'");
                // attempt to continue
            } else ps->consume();

            return n;
        }
        ParseNode* throwStatement(Parser* ps) {
            if (!ps->isKeyword("throw")) return nullptr;
            const token* t = &ps->get();
            ps->consume();
            ParseNode* expr = singleExpression(ps);
            if (!expr) {
                ps->error(pm_expected_expr, "Expected expression after 'throw'");
                return nullptr;
            }

            if (!ps->typeIs(tt_semicolon)) {
                ps->error(pm_expected_eos, "Expected ';' after 'throw <expression>'");
                // attempt to continue
            } else ps->consume();

            ParseNode* n = ps->newNode(nt_throw, t);
            n->body = expr;
            return n;
        }
        ParseNode* catchBlock(Parser* ps) {
            if (!ps->isKeyword("catch")) return nullptr;
            const token* t = &ps->get();
            ps->consume();

            if (!ps->typeIs(tt_open_parenth)) {
                ps->error(pm_expected_open_parenth, "Expected '(' after 'catch'");
                return nullptr;
            }
            ps->consume();

            ParseNode* n = ps->newNode(nt_catch, t);
            n->parameters = parameter(ps);
            if (!n->parameters) {
                ps->error(pm_expected_catch_param, "Expected 'catch' block to take exactly one parameter");
                ps->freeNode(n);
                return nullptr;
            }

            if (!n->parameters->data_type) {
                ps->error(pm_expected_catch_param_type, "Catch block parameter must be explicitly typed");
                ps->freeNode(n);
                return nullptr;
            }

            n->body = block(ps);
            if (!n->body) {
                // expect semicolon
                if (!ps->typeIs(tt_semicolon)) {
                    ps->error(pm_expected_eos, "Expected ';' after empty 'catch' statement");
                    // attempt to continue
                } else ps->consume();
            }

            return n;
        }
        ParseNode* tryStatement(Parser* ps) {
            if (!ps->isKeyword("try")) return nullptr;
            const token* t = &ps->get();
            ps->consume();

            ParseNode* body = block(ps);
            if (!body) {
                ps->error(pm_expected_statement, "Expected block or statement after 'try'");
                return nullptr;
            }

            ParseNode* n = ps->newNode(nt_try, t);
            n->body = body;
            n->else_body = catchBlock(ps);
            if (!n->else_body) {
                ps->error(pm_expected_catch, "Expected at least one 'catch' block after 'try { ... }'");
                ps->freeNode(n);
                return nullptr;
            }

            ParseNode* c = n->else_body;
            while (c) {
                c->next = catchBlock(ps);
                c = c->next;
            }

            return n;
        }
        ParseNode* placementNewStatement(Parser* ps) {
            if (!ps->isKeyword("new")) return nullptr;
            const token* first = &ps->get();
            ps->begin();
            ps->consume();

            ParseNode* type = typeSpecifier(ps);
            if (!type) {
                ps->revert();
                return nullptr;
            }

            if (!ps->typeIs(tt_open_parenth)) {
                ps->revert();
                ps->freeNode(type);
                return nullptr;
            }

            ParseNode* args = arguments(ps);

            if (!ps->typeIs(tt_forward)) {
                // It's probably a normal 'new' expression
                ps->revert();
                ps->freeNode(type);
                if (args) ps->freeNode(args);
                return nullptr;
            }

            ps->commit();
            ps->consume();

            ParseNode* n = ps->newNode(nt_expression, first);
            n->op = op_placementNew;
            n->data_type = type;
            n->parameters = args;
            n->lvalue = singleExpression(ps);

            if (!n->lvalue) {
                ps->error(pm_expected_expr, "Expected expression for placement new target");
                ps->revert();
                ps->freeNode(n);
                return nullptr;
            }

            if (!ps->typeIs(tt_semicolon)) {
                ps->error(pm_expected_eos, "Expected ';' after placement new statement");
                // attempt to continue
            } else ps->consume();

            return n;
        }
        ParseNode* symbolImport(Parser* ps) {
            ParseNode* n = one_of(ps, {
                typedIdentifier,
                identifier
            });
            if (!n) return nullptr;
            n->tp = nt_import_symbol;

            if (ps->isKeyword("as")) {
                ps->consume();

                n->alias = identifier(ps);
                if (!n->alias) {
                    ps->error(pm_expected_identifier, "Expected identifier for symbol alias");
                    return errorNode(ps);
                }
            }

            return n;
        }
        ParseNode* importList(Parser* ps) {
            if (!ps->typeIs(tt_open_brace)) return nullptr;
            ps->consume();

            ParseNode* f = symbolImport(ps);
            if (f && isError(f)) {
                const token& r = skipToNextType(ps, { tt_comma, tt_close_brace, tt_semicolon });
                if (r.tp != tt_comma && r.tp != tt_close_brace) {
                    return f;
                }
            }

            ParseNode* n = f;
            while (n) {
                if (!ps->typeIs(tt_comma)) break;
                ps->consume();

                n->next = symbolImport(ps);
                n = n->next;
                if (!n) {
                    ps->error(pm_expected_parameter, "Expected symbol to import after ','");
                    // attempt to continue
                    break;
                }
            }

            if (!ps->typeIs(tt_close_brace)) {
                ps->error(pm_expected_closing_parenth, "Expected '}' to close import list");

                const token& r = skipToNextType(ps, { tt_close_brace });
                if (r.tp != tt_close_brace) {
                    ps->freeNode(f);
                    return errorNode(ps);
                }
            }
            ps->consume();

            return f;
        }
        ParseNode* importModule(Parser* ps) {
            if (!ps->typeIs(tt_open_brace)) return nullptr;
            ps->begin();
            ps->consume();

            if (!ps->isSymbol("*")) {
                ps->revert();
                return nullptr;
            }
            ps->consume();
            ps->commit();

            if (!ps->isKeyword("as")) {
                ps->error(pm_expected_import_alias, "Expected 'as' after 'import { *'");
                
                const token& r = skipToNextType(ps, { tt_close_brace });
                if (r.tp == tt_close_brace) ps->consume();
                return errorNode(ps);
            }
            ps->consume();

            ParseNode* n = identifier(ps);
            if (!n) {
                ps->error(pm_expected_identifier, "Expected identifier for module alias");

                const token& r = skipToNextType(ps, { tt_close_brace });
                if (r.tp == tt_close_brace) ps->consume();
                return errorNode(ps);
            }

            if (!ps->typeIs(tt_close_brace)) {
                ps->error(pm_expected_closing_parenth, "Expected '}' to close import list");
                const token& r = skipToNextType(ps, { tt_close_brace });
                if (r.tp == tt_close_brace) ps->consume();
                ps->freeNode(n);
                return errorNode(ps);
            }
            ps->consume();

            n->tp = nt_import_module;
            return n;
        }
        ParseNode* importStatement(Parser* ps) {
            if (!ps->isKeyword("import")) return nullptr;
            ps->consume();

            ParseNode* n = ps->newNode(nt_import, &ps->getPrev());
            n->body = importModule(ps);
            if (!n->body) n->body = importList(ps);
            if (!n->body) {
                ps->error(pm_expected_import_list, "Expected import list after 'import'");

                const token& r = skipToNextKeyword(ps, "from", { tt_semicolon });
                if (r.tp != tt_keyword) {
                    ps->freeNode(n);
                    return errorNode(ps);
                }
                n->body = errorNode(ps);
            }

            if (!ps->isKeyword("from")) {
                ps->error(pm_expected_import_name, "Expected 'from'");
                ps->freeNode(n);
                const token& r = skipToNextType(ps, { tt_semicolon });
                return errorNode(ps);
            }
            ps->consume();

            if (!ps->typeIs(tt_string)) {
                ps->error(pm_expected_import_name, "Expected string literal module name or file path after 'from'");
                ps->freeNode(n);
                const token& r = skipToNextType(ps, { tt_semicolon });
                return errorNode(ps);
            }
            const token& t = ps->get();
            ps->consume();

            n->value.s = t.text.c_str();
            n->str_len = t.text.size();
            n->manuallySpecifyRange(t);
            
            return n;
        }
        ParseNode* exportStatement(Parser* ps) {
            if (!ps->isKeyword("export")) return nullptr;
            const token* t = &ps->get();
            ps->consume();

            ParseNode* decl = declaration(ps);
            if (!decl) {
                ps->error(pm_expected_export_decl, "Expected function, class, type, or variable declaration after 'export'");
                return nullptr;
            }

            ParseNode* e = ps->newNode(nt_export, t);
            e->body = decl;
            return e;
        }
        ParseNode* statement(Parser* ps) {
            ParseNode* n = one_of(ps, {
                block,
                variableStatement,
                importStatement,
                exportStatement,
                emptyStatement,
                classDef,
                typeDef,
                placementNewStatement,
                expressionStatement,
                ifStatement,
                iterationStatement,
                continueStatement,
                breakStatement,
                deleteStatement,
                returnStatement,
                switchStatement,
                throwStatement,
                tryStatement,
                functionDef,
                eos
            });

            return n;
        }
        ParseNode* statementList(Parser* ps) {
            ParseNode* f = statement(ps);

            ParseNode* n = f;
            while (n) {
                n->next = statement(ps);
                if (n->next && isError(n->next)) {
                    const token& r = skipToNextType(ps, { tt_semicolon });
                    switch (r.tp) {
                        case tt_semicolon: break;
                        default: {
                            ps->freeNode(n->next);
                            return errorNode(ps);
                        }
                    }
                }

                n = n->next;
            }

            return f;
        }
        ParseNode* block(Parser* ps) {
            if (!ps->typeIs(tt_open_brace)) return nullptr;
            ps->consume();

            ParseNode* b = ps->newNode(nt_scoped_block, &ps->getPrev());
            b->body = statementList(ps);
            if (!ps->typeIs(tt_close_brace)) {
                ps->error(pm_expected_closing_brace, "Expected '}'");
                ps->freeNode(b);
                return nullptr;
            }
            b->manuallySpecifyRange(ps->get());
            ps->consume();

            return b;
        }
        
        
        
        //
        // Entry
        //

        ParseNode* program(Parser* ps) {
            ParseNode* statements = statementList(ps);

            if (!ps->typeIs(tt_eof)) {
                ps->error(pm_expected_eof, "Expected end of file");
                if (statements) ps->freeNode(statements);
                return nullptr;
            }
            ps->consume();

            if (!statements) return nullptr;
            
            ParseNode* root = ps->newNode(nt_root, &statements->tok);
            root->body = statements;
            return root;
        }
    };
};