#include <tsn/compiler/Compiler.h>
#include <tsn/compiler/FunctionDef.hpp>
#include <tsn/compiler/Parser.h>
#include <tsn/common/Context.h>
#include <tsn/common/Module.h>
#include <tsn/ffi/Function.h>
#include <tsn/ffi/DataType.h>
#include <tsn/ffi/DataTypeRegistry.h>
#include <tsn/ffi/FunctionRegistry.h>
#include <tsn/ffi/Closure.h>
#include <tsn/compiler/Value.hpp>
#include <tsn/compiler/OutputBuilder.h>
#include <tsn/compiler/TemplateContext.h>
#include <tsn/interfaces/IDataTypeHolder.hpp>
#include <tsn/io/Workspace.h>
#include <tsn/utils/function_match.h>

#include <utils/Array.hpp>

#include <stdarg.h>
#include <filesystem>

using namespace utils;
using namespace tsn::ffi;

namespace tsn {
    namespace compiler {
        utils::String argListStr(const utils::Array<Value>& args) {
            if (args.size() == 0) return "()";
            utils::String out = "(";
            for (u32 i = 0;i < args.size();i++) {
                if (i > 0) out += ", ";
                out += args[i].getType()->getName();
            }
            out += ")";
            return out;
        }

        utils::String argListStr(const utils::Array<const DataType*>& argTps) {
            if (argTps.size() == 0) return "()";
            utils::String out = "(";
            for (u32 i = 0;i < argTps.size();i++) {
                if (i > 0) out += ", ";
                out += argTps[i]->getName();
            }
            out += ")";
            return out;
        }



        //
        // Compiler
        //

        Compiler::Compiler(Context* ctx, Logger* log, ParseNode* programTree, const script_metadata* meta)
            : IContextual(ctx), IWithLogger(log), m_scopeMgr(this)
        {
            m_meta = meta;
            m_program = programTree;
            m_output = nullptr;
            m_curFunc = nullptr;
            m_trustEnable = false;
            enterNode(programTree);
            m_scopeMgr.enter();
        }

        Compiler::~Compiler() {
            exitNode();
        }

        void Compiler::enterNode(ParseNode* n) {
            n->computeSourceLocationRange();
            m_nodeStack.push(n);
        }

        void Compiler::exitNode() {
            m_nodeStack.pop();
        }

        void Compiler::enterClass(ffi::DataType* c) {
            m_classStack.push(c);
        }

        ffi::DataType* Compiler::currentClass() {
            if (m_classStack.size() == 0) return nullptr;
            return m_classStack.last();
        }

        void Compiler::exitClass() {
            m_classStack.pop();
        }

        expression_context* Compiler::enterExpr() {
            expression_context ctx;
            ctx.loc = rsl_auto;
            ctx.md_access = private_access;
            ctx.destination = nullptr;
            ctx.expectedType = nullptr;
            ctx.targetNextCall = false;
            ctx.targetNextConstructor = false;

            m_exprCtx.push(ctx);
            return &m_exprCtx.last();
        }

        expression_context* Compiler::currentExpr() {
            if (m_exprCtx.size() == 0) return nullptr;
            return &m_exprCtx.last();
        }

        void Compiler::exitExpr() {
            m_exprCtx.pop();
        }

        void Compiler::enterFunction(FunctionDef* fn) {
            m_funcStack.push(fn);
            m_curFunc = fn;
            m_scopeMgr.enter();
            fn->onEnter();
        }

        Function* Compiler::exitFunction() {
            if (m_funcStack.size() == 0) return nullptr;
            FunctionDef* fd = m_funcStack.last();
            Function* fn = fd->onExit();
            if (fn) fn->m_src = fd->getSource();
            m_funcStack.pop();
            m_scopeMgr.exit();
            if (m_funcStack.size() == 0) m_curFunc = nullptr;
            else m_curFunc = m_funcStack[m_funcStack.size() - 1];
            return fn;
        }

        FunctionDef* Compiler::currentFunction() const {
            return m_curFunc;
        }

        ParseNode* Compiler::currentNode() {
            return m_nodeStack.last();
        }

        bool Compiler::inInitFunction() const {
            return m_funcStack.size() == 1;
        }
        
        bool Compiler::isTrusted() const {
            if (m_meta->is_trusted || m_trustEnable) return true;

            TemplateContext* curTempl = getTemplateContext();
            if (curTempl && curTempl->getOrigin()->getInfo()) {
                if (curTempl->getOrigin()->getInfo()->is_trusted) {
                    return true;
                }
            }

            return false;
        }
        
        utils::String Compiler::generateFullQualifierPrefix() const {
            TemplateContext* ctx = getTemplateContext();
            Module* m = ctx ? ctx->getOrigin() : m_output->getModule();
            
            utils::String out = m->getName() + "::";
            return out;
        }

        TemplateContext* Compiler::createTemplateContext(ParseNode* n) {
            TemplateContext* current = getTemplateContext();
            TemplateContext* tctx = new TemplateContext(n, current ? current->getOrigin() : m_output->getModule());

            robin_hood::unordered_set<utils::String> added;
            buildTemplateContext(n, tctx, added);

            return tctx;
        }
        
        void Compiler::pushTemplateContext(TemplateContext* tctx) {
            importTemplateContext(tctx);
            m_templateStack.push(tctx);
        }

        TemplateContext* Compiler::getTemplateContext() const {
            if (m_templateStack.size() == 0) return nullptr;
            return m_templateStack.last();
        }

        void Compiler::popTemplateContext() {
            m_templateStack.pop();
        }

        const SourceLocation& Compiler::getCurrentSrc() const {
            return m_nodeStack[m_nodeStack.size() - 1]->tok.src;
        }

        ScopeManager& Compiler::scope() {
            return m_scopeMgr;
        }
        
        const script_metadata* Compiler::getScriptInfo() const {
            return m_meta;
        }

        OutputBuilder* Compiler::getOutput() {
            return m_output;
        }

        OutputBuilder* Compiler::compile() {
            Module* m = m_ctx->createModule(
                std::filesystem::path(m_meta->path).filename().replace_extension("").string(),
                m_meta->path,
                m_meta
            );
            m_output = new OutputBuilder(this, m);

            // init function
            FunctionDef* fd = m_output->newFunc("__init__", nullptr);
            enterFunction(fd);

            const utils::Array<tsn::ffi::DataType*>& importTypes = m_ctx->getGlobal()->allTypes();
            for (auto* t : importTypes) {
                m_output->import(t, t->getName());
            }

            const utils::Array<tsn::ffi::Function*>& importFuncs = m_ctx->getGlobal()->allFunctions();
            for (auto* f : importFuncs) {
                if (!f) continue;
                m_output->import(f, f->getName());
            }

            ParseNode* n = m_program->body;
            while (n) {
                if (n->tp == nt_import) {
                    compileImport(n);
                } else if (n->tp == nt_function) {
                    m_output->newFunc(n->str(), n);
                }
                n = n->next;
            }

            n = m_program->body;
            while (n) {
                compileAny(n);
                n = n->next;
            }

            exitFunction();
            return m_output;
        }
        
        static log_message null_msg = {
            lt_error,
            (log_message_code)0,
            utils::String(),
            SourceLocation(),
            nullptr
        };

        log_message& Compiler::typeError(ffi::DataType* tp, log_message_code code, const char* msg, ...) {
            if (tp->isEqualTo(currentFunction()->getPoison().getType())) {
                // If the type is poisoned, an error was already emitted. All (or most) subsequent
                // errors would not actually be errors if the original error had not occurred. This
                // should prevent a cascade of irrelevant errors.
                return null_msg;
            }

            char out[1024] = { 0 };
            va_list l;
            va_start(l, msg);
            i32 len = vsnprintf(out, 1024, msg, l);
            va_end(l);

            ParseNode* n = m_nodeStack.last();
            return submitLog(
                lt_error,
                code,
                utils::String(out, len),
                n->tok.src,
                n->clone()
            );
        }

        log_message& Compiler::typeError(ParseNode* node, ffi::DataType* tp, log_message_code code, const char* msg, ...) {
            if (tp->isEqualTo(currentFunction()->getPoison().getType())) {
                // If the type is poisoned, an error was already emitted. All (or most) subsequent
                // errors would not actually be errors if the original error had not occurred. This
                // should prevent a cascade of irrelevant errors.
                return null_msg;
            }

            char out[1024] = { 0 };
            va_list l;
            va_start(l, msg);
            i32 len = vsnprintf(out, 1024, msg, l);
            va_end(l);
            
            return submitLog(
                lt_error,
                code,
                utils::String(out, len),
                node->tok.src,
                node->clone()
            );
        }

        log_message& Compiler::valueError(const Value& val, log_message_code code, const char* msg, ...) {
            if (val.getType()->isEqualTo(currentFunction()->getPoison().getType())) {
                // If the type is poisoned, an error was already emitted. All (or most) subsequent
                // errors would not actually be errors if the original error had not occurred. This
                // should prevent a cascade of irrelevant errors.
                return null_msg;
            }

            char out[1024] = { 0 };
            va_list l;
            va_start(l, msg);
            i32 len = vsnprintf(out, 1024, msg, l);
            va_end(l);
            
            ParseNode* n = m_nodeStack.last();
            return submitLog(
                lt_error,
                code,
                utils::String(out, len),
                n->tok.src,
                n->clone()
            );
        }

        log_message& Compiler::valueError(ParseNode* node, const Value& val, log_message_code code, const char* msg, ...) {
            if (val.getType()->isEqualTo(currentFunction()->getPoison().getType())) {
                // If the type is poisoned, an error was already emitted. All (or most) subsequent
                // errors would not actually be errors if the original error had not occurred. This
                // should prevent a cascade of irrelevant errors.
                return null_msg;
            }

            char out[1024] = { 0 };
            va_list l;
            va_start(l, msg);
            i32 len = vsnprintf(out, 1024, msg, l);
            va_end(l);
            
            return submitLog(
                lt_error,
                code,
                utils::String(out, len),
                node->tok.src,
                node->clone()
            );
        }

        log_message& Compiler::functionError(ffi::DataType* selfTp, ffi::DataType* retTp, const utils::Array<Value>& args, log_message_code code, const char* msg, ...) {
            // If any of the involved types are poisoned, an error was already emitted. All
            // (or most) subsequent errors would not actually be errors if the original error
            // had not occurred. This should prevent a cascade of irrelevant errors.
            ffi::DataType* poisonTp = currentFunction()->getPoison().getType();

            if (retTp && retTp->isEqualTo(poisonTp)) return null_msg;
            if (selfTp && selfTp->isEqualTo(poisonTp)) return null_msg;
            for (u32 i = 0;i < args.size();i++) {
                if (args[i].getType()->isEqualTo(poisonTp)) return null_msg;
            }
            
            char out[1024] = { 0 };
            va_list l;
            va_start(l, msg);
            i32 len = vsnprintf(out, 1024, msg, l);
            va_end(l);

            ParseNode* n = m_nodeStack.last();
            return submitLog(
                lt_error,
                code,
                utils::String(out, len),
                n->tok.src,
                n->clone()
            );
        }

        log_message& Compiler::functionError(ParseNode* node, ffi::DataType* selfTp, ffi::DataType* retTp, const utils::Array<Value>& args, log_message_code code, const char* msg, ...) {
            // If any of the involved types are poisoned, an error was already emitted. All
            // (or most) subsequent errors would not actually be errors if the original error
            // had not occurred. This should prevent a cascade of irrelevant errors.
            ffi::DataType* poisonTp = currentFunction()->getPoison().getType();

            if (retTp && retTp->isEqualTo(poisonTp)) return null_msg;
            if (selfTp && selfTp->isEqualTo(poisonTp)) return null_msg;
            for (u32 i = 0;i < args.size();i++) {
                if (args[i].getType()->isEqualTo(poisonTp)) return null_msg;
            }

            char out[1024] = { 0 };
            va_list l;
            va_start(l, msg);
            i32 len = vsnprintf(out, 1024, msg, l);
            va_end(l);
            
            return submitLog(
                lt_error,
                code,
                utils::String(out, len),
                node->tok.src,
                node->clone()
            );
        }

        log_message& Compiler::functionError(ffi::DataType* selfTp, ffi::DataType* retTp, const utils::Array<const ffi::DataType*>& argTps, log_message_code code, const char* msg, ...) {
            // If any of the involved types are poisoned, an error was already emitted. All
            // (or most) subsequent errors would not actually be errors if the original error
            // had not occurred. This should prevent a cascade of irrelevant errors.
            ffi::DataType* poisonTp = currentFunction()->getPoison().getType();

            if (retTp && retTp->isEqualTo(poisonTp)) return null_msg;
            if (selfTp && selfTp->isEqualTo(poisonTp)) return null_msg;
            for (u32 i = 0;i < argTps.size();i++) {
                if (argTps[i]->isEqualTo(poisonTp)) return null_msg;
            }

            char out[1024] = { 0 };
            va_list l;
            va_start(l, msg);
            i32 len = vsnprintf(out, 1024, msg, l);
            va_end(l);

            ParseNode* n = m_nodeStack.last();
            return submitLog(
                lt_error,
                code,
                utils::String(out, len),
                n->tok.src,
                n->clone()
            );
        }

        log_message& Compiler::functionError(ParseNode* node, ffi::DataType* selfTp, ffi::DataType* retTp, const utils::Array<const ffi::DataType*>& argTps, log_message_code code, const char* msg, ...) {
            // If any of the involved types are poisoned, an error was already emitted. All
            // (or most) subsequent errors would not actually be errors if the original error
            // had not occurred. This should prevent a cascade of irrelevant errors.
            ffi::DataType* poisonTp = currentFunction()->getPoison().getType();

            if (retTp && retTp->isEqualTo(poisonTp)) return null_msg;
            if (selfTp && selfTp->isEqualTo(poisonTp)) return null_msg;
            for (u32 i = 0;i < argTps.size();i++) {
                if (argTps[i]->isEqualTo(poisonTp)) return null_msg;
            }

            char out[1024] = { 0 };
            va_list l;
            va_start(l, msg);
            i32 len = vsnprintf(out, 1024, msg, l);
            va_end(l);
            
            return submitLog(
                lt_error,
                code,
                utils::String(out, len),
                node->tok.src,
                node->clone()
            );
        }

        log_message& Compiler::error(log_message_code code, const char* msg, ...) {
            char out[1024] = { 0 };
            va_list l;
            va_start(l, msg);
            i32 len = vsnprintf(out, 1024, msg, l);
            va_end(l);

            ParseNode* n = m_nodeStack.last();
            return submitLog(
                lt_error,
                code,
                utils::String(out, len),
                n->tok.src,
                n->clone()
            );
        }
        
        log_message& Compiler::error(ParseNode* node, log_message_code code, const char* msg, ...) {
            char out[1024] = { 0 };
            va_list l;
            va_start(l, msg);
            i32 len = vsnprintf(out, 1024, msg, l);
            va_end(l);
            
            return submitLog(
                lt_error,
                code,
                utils::String(out, len),
                node->tok.src,
                node->clone()
            );
        }

        log_message& Compiler::warn(log_message_code code, const char* msg, ...) {
            char out[1024] = { 0 };
            va_list l;
            va_start(l, msg);
            i32 len = vsnprintf(out, 1024, msg, l);
            va_end(l);

            ParseNode* n = m_nodeStack.last();
            return submitLog(
                lt_warn,
                code,
                utils::String(out, len),
                n->tok.src,
                n->clone()
            );
        }

        log_message& Compiler::warn(ParseNode* node, log_message_code code, const char* msg, ...) {
            char out[1024] = { 0 };
            va_list l;
            va_start(l, msg);
            i32 len = vsnprintf(out, 1024, msg, l);
            va_end(l);
            
            return submitLog(
                lt_warn,
                code,
                utils::String(out, len),
                node->tok.src,
                node->clone()
            );
        }

        log_message& Compiler::info(log_message_code code, const char* msg, ...) {
            char out[1024] = { 0 };
            va_list l;
            va_start(l, msg);
            i32 len = vsnprintf(out, 1024, msg, l);
            va_end(l);

            ParseNode* n = m_nodeStack.last();
            return submitLog(
                lt_info,
                code,
                utils::String(out, len),
                n->tok.src,
                n->clone()
            );
        }

        log_message& Compiler::info(ParseNode* node, log_message_code code, const char* msg, ...) {
            char out[1024] = { 0 };
            va_list l;
            va_start(l, msg);
            i32 len = vsnprintf(out, 1024, msg, l);
            va_end(l);
            
            return submitLog(
                lt_info,
                code,
                utils::String(out, len),
                node->tok.src,
                node->clone()
            );
        }



        InstructionRef Compiler::add(ir_instruction inst) {
            return m_curFunc->add(inst);
        }
        
        Value Compiler::getStorageForExpr(ffi::DataType* tp) {
            auto* cf = currentFunction();
            auto* exp = currentExpr();
            if (!exp || exp->loc == rsl_auto) {
                if (tp->getInfo().is_primitive || tp->getInfo().size == 0) return currentFunction()->val(tp);
                else return cf->stack(tp);
            } else if (exp->loc == rsl_stack) {
                return cf->stack(tp);
            } else if (exp->loc == rsl_module_data) {
                u32 slot = m_output->getModule()->addData(exp->md_name, tp, exp->md_access);
                return cf->val(m_output->getModule(), slot);
            }

            return *exp->destination;
        }
        
        Value Compiler::copyValueToExprStorage(const Value& result) {
            FunctionDef* cf = currentFunction();

            auto* exp = currentExpr();
            if (!exp || exp->loc == rsl_auto) return result;

            DataType* storageTp = exp->expectedType ? exp->expectedType : result.getType();
            Value storage = getStorageForExpr(storageTp);
            if (storage.getType()->getInfo().is_primitive && result.getType()->getInfo().is_primitive) {
                storage = result;
                return result;
            }

            constructObject(storage, { result });
            
            return result;
        }
        
        void Compiler::constructObject(const Value& dest, ffi::DataType* tp, const utils::Array<Value>& args) {
            Array<DataType*> argTps = args.map([](const Value& v) { return v.getType(); });

            if (tp->getInfo().is_primitive) {
                if (args.size() > 1) {
                    functionError(
                        tp,
                        nullptr,
                        args,
                        cm_err_no_matching_constructor,
                        "Type '%s' has no constructor with arguments matching '%s'",
                        tp->getName().c_str(),
                        argListStr(args).c_str()
                    );
                } else if (args.size() == 1) {
                    Value v = args[0].convertedTo(tp);
                    currentFunction()->add(ir_store).op(v).op(dest);
                }
                return;
            }

            ffi::DataType* curClass = currentClass();
            Array<Function *> ctors = tp->findMethods(
                "constructor",
                nullptr,
                (const DataType**)argTps.data(),
                argTps.size(),
                fm_skip_implicit_args
            );

            if (ctors.size() == 1) {
                if (ctors[0]->getAccessModifier() == private_access && (!curClass || !curClass->isEqualTo(tp))) {
                    error(cm_err_private_constructor, "Constructor '%s' is private", ctors[0]->getDisplayName().c_str());
                    return;
                } else generateCall(ctors[0], args, &dest);
            } else if (ctors.size() > 1) {
                ffi::Function* func = nullptr;
                u8 looseC = 0;
                for (u32 i = 0;i < ctors.size();i++) {
                    if (ctors[i]->getAccessModifier() == private_access && (!curClass || !curClass->isEqualTo(tp))) continue;
                    looseC++;
                    func = ctors[i];
                }

                if (looseC == 1) {
                    generateCall(func, args, &dest);
                    return;
                } else if (looseC == 0) {
                    functionError(
                        tp,
                        nullptr,
                        args,
                        cm_err_no_matching_constructor,
                        "Type '%s' has no accessible constructor with arguments matching '%s'",
                        tp->getName().c_str(),
                        argListStr(args).c_str()
                    );
                    return;
                }
                
                u8 strictC = 0;
                for (u32 i = 0;i < ctors.size();i++) {
                    if (ctors[i]->getAccessModifier() == private_access && (!curClass || !curClass->isEqualTo(tp))) continue;
                    const auto& args = ctors[i]->getSignature()->getArguments();
                    bool isCompatible = true;
                    u32 ea = 0;
                    for (u32 a = 0;a < args.size() && ea < argTps.size();a++) {
                        if (args[a].isImplicit()) continue;
                        if (!args[a].dataType->isEqualTo(argTps[ea++])) {
                            isCompatible = false;
                            break;
                        }

                        if (ea == argTps.size() && a < (args.size() - 1)) {
                            // constructor has more explicit arguments than provided
                            isCompatible = false;
                            break;
                        }
                    }

                    if (isCompatible) {
                        strictC++;
                        func = ctors[i];
                    }
                }
                
                if (strictC == 1) {
                    generateCall(func, args, &dest);
                    return;
                }

                functionError(
                    tp,
                    nullptr,
                    args,
                    cm_err_ambiguous_constructor,
                    "Construction of type '%s' with arguments '%s' is ambiguous",
                    tp->getName().c_str(),
                    argListStr(args).c_str()
                );

                for (u32 i = 0;i < ctors.size();i++) {
                    if (ctors[i]->getAccessModifier() == private_access && (!curClass || !curClass->isEqualTo(tp))) continue;
                    info(
                        cm_info_could_be,
                        "^ Could be: '%s'",
                        ctors[i]->getDisplayName().c_str()
                    ).src = ctors[i]->getSource();
                }
            } else {
                functionError(
                    tp,
                    nullptr,
                    args,
                    cm_err_no_matching_constructor,
                    "Type '%s' has no constructor with arguments matching '%s'",
                    tp->getName().c_str(),
                    argListStr(args).c_str()
                );
            }
        }

        void Compiler::constructObject(const Value& dest, const utils::Array<Value>& args) {
            constructObject(dest, dest.getType(), args);
        }

        Value Compiler::constructObject(ffi::DataType* tp, const utils::Array<Value>& args) {
            auto* exp = currentExpr();

            bool resultShouldBeHandledInternally = !exp || exp->loc == rsl_auto || !exp->targetNextConstructor;
            bool resultHandledInternally = resultShouldBeHandledInternally;
            if (!resultShouldBeHandledInternally && exp && exp->expectedType) {
                if (!exp->expectedType->isEqualTo(tp)) {
                    // Value needs conversion before going to final location
                    resultHandledInternally = true;
                }
            }

            if (!resultHandledInternally) {
                Value out = getStorageForExpr(tp);
                constructObject(out, tp, args);
                exp->targetNextConstructor = false;
                exp->targetNextCall = false;
                return out;
            }

            FunctionDef* cf = currentFunction();

            Value out = cf->stack(tp);
            constructObject(out, out.getType(), args);

            if (resultHandledInternally && !resultShouldBeHandledInternally) {
                exp->targetNextConstructor = false;
                exp->targetNextCall = false;
                return copyValueToExprStorage(out);
            }

            return out;
        }

        Value Compiler::functionValue(FunctionDef* fn) {
            return currentFunction()->imm(fn);
        }

        Value Compiler::functionValue(ffi::Function* fn) {
            return currentFunction()->imm(m_output->getFunctionDef(fn));
        }
        
        Value Compiler::typeValue(ffi::DataType* tp) {
            return currentFunction()->imm(tp);
        }
        
        Value Compiler::moduleValue(Module* mod) {
            return currentFunction()->imm(mod);
        }

        Value Compiler::moduleData(Module* m, u32 slot) {
            return currentFunction()->val(m, slot);
        }

        Value Compiler::newClosure(function_id target, Value* captureData) {
            FunctionDef* cf = currentFunction();

            ffi::Function* newClosure = m_ctx->getModule("<host>/memory.tsn")->allFunctions().find([](const ffi::Function* fn) {
                return fn && fn->getName() == "$newClosure";
            });

            m_trustEnable = true;
            Value closure = generateCall(newClosure, {
                cf->imm(target),
                captureData ? *captureData : cf->getNull()
            });
            m_trustEnable = false;

            // result is a Pointer<Closure*>
            // Pointer will be destructed automatically, just return the raw Closure*
            add(ir_load).op(closure).op(closure);
            closure.setType(m_ctx->getTypes()->getClosure());
            closure.getFlags().is_pointer = 1;

            return closure;
        }

        Value Compiler::allocateCaptureData(const utils::Array<Value>& captures, utils::Array<u32>& outOffsets) {
            FunctionDef* cf = currentFunction();
            if (captures.size() == 0) return cf->getNull();

            ffi::Function* newCaptureData = m_ctx->getModule("<host>/memory.tsn")->allFunctions().find([](const ffi::Function* fn) {
                return fn && fn->getName() == "$newCaptureData";
            });

            u32 captureSize = 0;
            captures.each([&captureSize](const Value& v) {
                captureSize += v.getType()->getInfo().size;
            });

            Value count = cf->imm(captures.size());

            m_trustEnable = true;
            Value out = generateCall(newCaptureData, { cf->imm(captureSize), count });
            m_trustEnable = false;

            Value data = cf->val(out.getType());
            add(ir_assign).op(data).op(out);

            u32 offset = 0;

            // write capture count
            add(ir_store).op(count).op(data).comment("capture count");
            add(ir_uadd).op(data).op(data).op(cf->imm<u32>(sizeof(u32)));
            offset += sizeof(u32);

            for (u32 i = 0;i < captures.size();i++) {
                auto& v = captures[i];
                DataType* tp = v.getType();
                type_id tid = tp->getId();
                u32 size = tp->getInfo().size;

                add(ir_noop).comment(utils::String::Format("---- capture[%d] '%s'", i, v.getName().c_str()));

                // write capture type id
                add(ir_noop).comment(utils::String::Format("---- capture[%d] type id", i));
                add(ir_store).op(cf->imm(tid)).op(data);
                add(ir_uadd).op(data).op(data).op(cf->imm<u32>(sizeof(type_id)));
                offset += sizeof(type_id);

                // copy captured value
                add(ir_noop).comment(utils::String::Format("---- capture[%d] data", i));
                constructObject(data, tp, { v });
                outOffsets.push(offset);

                // offset by capture size
                add(ir_noop).comment(utils::String::Format("---- capture[%d] size", i));
                add(ir_uadd).op(data).op(data).op(cf->imm(size));
                offset += size;
            }

            return out;
        }

        void Compiler::findCaptures(ParseNode* node, utils::Array<Value>& outCaptures, robin_hood::unordered_map<utils::String, u32>& declaredNames, u32 scopeIdx) {
            ParseNode* n = node;

            while (n) {
                u32 nestedScopeIdx = scopeIdx;
                bool ignoreBody = false;

                if (n->tp == nt_variable) {
                    if (n->body->tp == nt_object_decompositor) {
                        ParseNode* p = n->body;
                        while (p) {
                            if (declaredNames.count(p->str()) == 0) {
                                declaredNames[p->str()] = scopeIdx;
                            }
                            p = p->next;
                        }
                    } else {
                        if (declaredNames.count(n->body->str()) == 0) {
                            declaredNames[n->body->str()] = scopeIdx;
                        }
                    }

                    n = n->next;
                    continue;
                } else if (n->tp == nt_function) {
                    nestedScopeIdx = scopeIdx + 1;

                    ParseNode* p = n->parameters;
                    while (p) {
                        if (declaredNames.count(p->str()) == 0) {
                            declaredNames[p->str()] = nestedScopeIdx;
                        }
                        p = p->next;
                    }
                } else if (n->tp == nt_identifier) {
                    if (declaredNames.count(n->str()) == 0) {
                        symbol* s = scope().get(n->str());
                        if (s && s->value && !s->value->isImm()) outCaptures.push(*s->value);
                    }
                    
                    n = n->next;
                    continue;
                } else if (n->tp == nt_type_specifier) {
                    ignoreBody = n->body && n->body->tp == nt_identifier;
                } else if (n->tp == nt_scoped_block) nestedScopeIdx = scopeIdx + 1;
                else if (n->tp == nt_loop) nestedScopeIdx = scopeIdx + 1;
                else if (n->tp == nt_if) nestedScopeIdx = scopeIdx + 1;

                if (n->data_type) findCaptures(n->data_type, outCaptures, declaredNames, nestedScopeIdx);
                if (n->lvalue) findCaptures(n->lvalue, outCaptures, declaredNames, nestedScopeIdx);
                if (n->rvalue) findCaptures(n->rvalue, outCaptures, declaredNames, nestedScopeIdx);
                if (n->cond) findCaptures(n->cond, outCaptures, declaredNames, nestedScopeIdx);
                if (n->body && !ignoreBody) findCaptures(n->body, outCaptures, declaredNames, nestedScopeIdx);
                if (n->else_body) findCaptures(n->else_body, outCaptures, declaredNames, nestedScopeIdx);
                if (n->initializer) findCaptures(n->initializer, outCaptures, declaredNames, nestedScopeIdx);
                if (n->parameters && n->tp != nt_function) findCaptures(n->parameters, outCaptures, declaredNames, nestedScopeIdx);
                if (n->modifier) findCaptures(n->modifier, outCaptures, declaredNames, nestedScopeIdx);

                if (nestedScopeIdx != scopeIdx) {
                    auto it = declaredNames.begin();
                    while (it != declaredNames.end()) {
                        if (it->second == scopeIdx + 1) {
                            it = declaredNames.erase(it);
                            continue;
                        }

                        ++it;
                    }
                }

                n = n->next;
            }
        }

        Value Compiler::newClosureRef(const Value& closure, ffi::DataType* signature) {
            Value cr = constructObject(m_ctx->getTypes()->getClosureRef(), { closure });
            cr.setType(signature);
            cr.getFlags().is_function = 1;
            return cr;
        }

        Value Compiler::newClosureRef(const Value& fnImm) {
            FunctionDef* cf = currentFunction();

            if (!fnImm.isImm() || !fnImm.isFunction()) {
                valueError(fnImm, cm_err_internal, "Provided value is not a function immediate");
                return cf->getPoison();
            }

            FunctionDef* fd = fnImm.getImm<FunctionDef*>();
            ffi::Function* fn = fd->getOutput();
            ffi::DataType* sig = fn ? fn->getSignature() : nullptr;

            if (!fn || !sig) {
                error(
                    cm_err_function_signature_not_determined,
                    "Could not determine signature of function '%s'",
                    fn ? fn->getFullyQualifiedName().c_str() : fd->getName().c_str()
                );

                return cf->getPoison();
            }

            enterExpr();
            // Closure is ref counted and freed when the last closureRef is destroyed
            Value closure = newClosure(fn->getId());
            Value* self = fnImm.getSrcSelf();
            if (self) {
                // set self ptr on closure
                Value selfPtr = cf->val(m_ctx->getTypes()->getVoidPtr());
                add(ir_uadd).op(selfPtr).op(closure).op(cf->imm<u64>(offsetof(Closure, m_self)));
                add(ir_store).op(*self).op(selfPtr);
            }
            
            exitExpr();

            return newClosureRef(closure, sig);
        }

        Value Compiler::newClosureRef(ffi::Function* fn) {
            ffi::DataType* sig = fn ? fn->getSignature() : nullptr;

            if (!fn || !sig) {
                error(
                    cm_err_function_signature_not_determined,
                    "Could not determine signature of function '%s'",
                    fn->getFullyQualifiedName().c_str()
                );

                return currentFunction()->getPoison();
            }

            enterExpr();
            // Closure is ref counted and freed when the last closureRef is destroyed
            Value closure = newClosure(fn->getId());
            exitExpr();

            return newClosureRef(closure, sig);
        }

        Value Compiler::newClosureRef(FunctionDef* fd) {
            ffi::Function* fn = fd->getOutput();
            ffi::DataType* sig = fn ? fn->getSignature() : nullptr;

            if (!fn || !sig) {
                error(
                    cm_err_function_signature_not_determined,
                    "Could not determine signature of function '%s'",
                    fn ? fn->getFullyQualifiedName().c_str() : fd->getName().c_str()
                );

                return currentFunction()->getPoison();
            }

            enterExpr();
            // Closure is ref counted and freed when the last closureRef is destroyed
            Value closure = newClosure(fn->getId());
            exitExpr();

            return newClosureRef(closure, sig);
        }

        Value Compiler::generateCall(const Value& fn, const utils::String& name, bool returnsPointer, ffi::DataType* retTp, const utils::Array<function_argument>& fargs, const utils::Array<Value>& params, const Value* self) {
            FunctionDef* cf = currentFunction();

            ffi::DataType* origRetTp = retTp;
            if (returnsPointer) {
                retTp = getPointerType(retTp);
            }

            Value callee = fn;
            Value capturePtr;

            add(ir_noop).comment(utils::String::Format("-------- call %s --------", fn.toString(m_ctx).c_str()));

            if (callee.isImm()) {
                FunctionDef* vfd = callee.getImm<FunctionDef*>();
                if (vfd->getOutput()) {
                    Function* f = vfd->getOutput();
                    if (f && f->getAccessModifier() == trusted_access && !isTrusted()) {
                        error(cm_err_not_trusted, "Function '%s' is only accessible to trusted scripts", f->getFullyQualifiedName().c_str());
                        add(ir_noop).comment(utils::String::Format("-------- end call %s --------", fn.toString(m_ctx).c_str()));
                        return currentFunction()->getPoison();
                    }
                }
            } else {
                capturePtr.reset(cf->val(m_ctx->getTypes()->getVoidPtr()));
                add(ir_load).op(capturePtr).op(fn).comment("Load pointer to capture data");
            }

            u8 argc = 0;
            for (u8 a = 0;a < fargs.size();a++) {
                if (!fargs[a].isImplicit()) argc++;
            }

            if (params.size() != argc) {
                if (name.size() > 0) {
                    functionError(
                        self ? self->getType() : nullptr,
                        retTp,
                        params,
                        cm_err_function_argument_count_mismatch,
                        "Function '%s' expects %d argument%s, but %d %s provided",
                        name.c_str(),
                        argc,
                        argc == 1 ? "" : "s",
                        params.size(),
                        params.size() == 1 ? "was" : "were"
                    );
                } else {
                    functionError(
                        self ? self->getType() : nullptr,
                        retTp,
                        params,
                        cm_err_function_argument_count_mismatch,
                        "Function expects %d argument%s, but %d %s provided",
                        argc,
                        argc == 1 ? "" : "s",
                        params.size(),
                        params.size() == 1 ? "was" : "were"
                    );
                }

                add(ir_noop).comment(utils::String::Format("-------- end call %s --------", fn.toString(m_ctx).c_str()));
                return cf->getPoison();
            }

            u8 aidx = 0;
            for (u8 a = 0;a < fargs.size();a++) {
                const auto& ainfo = fargs[a];
                if (ainfo.isImplicit()) continue;

                if (!params[aidx].getType()->isConvertibleTo(ainfo.dataType)) {
                    if (name.size() > 0) {
                        functionError(
                            self ? self->getType() : nullptr,
                            retTp,
                            params,
                            cm_err_type_not_convertible,
                            "Function '%s' is not callable with args '%s'",
                            name.c_str(),
                            argListStr(params).c_str()
                        );
                    } else {
                        functionError(
                            self ? self->getType() : nullptr,
                            retTp,
                            params,
                            cm_err_type_not_convertible,
                            "Function is not callable with args '%s'",
                            argListStr(params).c_str()
                        );
                    }
                    info(
                        cm_info_arg_conversion,
                        "^ No conversion found from type '%s' to type '%s' for argument %d",
                        params[aidx].getType()->getName().c_str(),
                        ainfo.dataType->getName().c_str(),
                        aidx + 1
                    );

                    add(ir_noop).comment(utils::String::Format("-------- end call %s --------", fn.toString(m_ctx).c_str()));
                    return cf->getPoison();
                }
                aidx++;
            }
            
            auto* exp = currentExpr();
            bool resultShouldBeHandledInternally = !exp || exp->loc == rsl_auto || !exp->targetNextCall;
            bool resultHandledInternally = resultShouldBeHandledInternally;
            if (!resultShouldBeHandledInternally && exp && exp->expectedType) {
                if (retTp->getInfo().size != 0 && !exp->expectedType->isEqualTo(retTp)) {
                    // return value needs conversion before going to final location
                    resultHandledInternally = true;
                }
            }

            Value resultStack;
            Value resultPtr;
            
            if (resultHandledInternally && retTp->getInfo().size != 0) {
                // unscoped because it will either be freed or added to the scope later
                resultStack.reset(cf->stack(retTp, true, "Allocate stack space for return value"));
                resultPtr.reset(cf->val(retTp));
                add(ir_stack_ptr).op(resultPtr).op(cf->imm(resultStack.getStackAllocId())).comment("Get return pointer for function call");

                if (returnsPointer) {
                    // retTp is a Pointer<T>. Construct it manually, setting '_external' property to true
                    Value tmp = cf->val(m_ctx->getTypes()->getVoidPtr());
                    add(ir_assign).op(tmp).op(resultPtr).comment(utils::String::Format("result : %s", retTp->getName().c_str()));
                    add(ir_store).op(cf->getNull()).op(tmp).comment("result._data = null");
                    add(ir_uadd).op(tmp).op(tmp).op(cf->imm<u64>(sizeof(void*)));
                    add(ir_store).op(cf->getNull()).op(tmp).comment("result._refs = null");
                    add(ir_uadd).op(tmp).op(tmp).op(cf->imm<u64>(sizeof(void*)));
                    add(ir_store).op(cf->imm<bool>(true)).op(tmp).comment("result._external = true");

                    // first property of Pointer<T> is _data, meaning resultPtr is _already_ a pointer to that pointer.
                    // Function will store a pointer in resultType, ie: Pointer<T>._data = ret
                }
            } else if (retTp->getInfo().size != 0) {
                resultPtr.reset(getStorageForExpr(retTp));

                if (returnsPointer) {
                    // retTp is a Pointer<T>. Construct it manually, setting '_external' property to true
                    Value tmp = cf->val(m_ctx->getTypes()->getVoidPtr());
                    add(ir_assign).op(tmp).op(resultPtr).comment(utils::String::Format("result : %s", retTp->getName().c_str()));
                    add(ir_store).op(cf->getNull()).op(tmp).comment("result._data = null");
                    add(ir_uadd).op(tmp).op(tmp).op(cf->imm<u64>(sizeof(void*)));
                    add(ir_store).op(cf->getNull()).op(tmp).comment("result._refs = null");
                    add(ir_uadd).op(tmp).op(tmp).op(cf->imm<u64>(sizeof(void*)));
                    add(ir_store).op(cf->imm<bool>(true)).op(tmp).comment("result._external = true");

                    // first property of Pointer<T> is _data, meaning resultPtr is _already_ a pointer to that pointer.
                    // Function will store a pointer in resultType, ie: Pointer<T>._data = ret
                }
            } else {
                // void return
                resultPtr.reset(cf->getNull());
            }

            utils::Array<Value> tempStackArgs(fargs.size() + 1);
            utils::Array<Value> finalParams(fargs.size() + 1);
            aidx = 0;
            for (u8 a = 0;a < fargs.size();a++) {
                const auto& arg = fargs[a];
                if (arg.isImplicit()) {
                    if (arg.argType == arg_type::context_ptr) finalParams.push(cf->getECtx());
                    else if (arg.argType == arg_type::func_ptr) finalParams.push(cf->imm<u64>(0));
                    else if (arg.argType == arg_type::ret_ptr) finalParams.push(resultPtr);
                    else if (arg.argType == arg_type::captures_ptr) finalParams.push(capturePtr);
                    else if (arg.argType == arg_type::this_ptr) {
                        if (!self) {
                            // TODO
                            // error
                            finalParams.push(cf->imm<u64>(0));
                        } else finalParams.push(*self);
                    }
                    continue;
                }

                const Value& p = params[aidx++];
                if (arg.argType == arg_type::value && !arg.dataType->isEqualTo(m_ctx->getTypes()->getVoidPtr())) {
                    if (p.getFlags().is_pointer) finalParams.push(*p);
                    else finalParams.push(p);
                } else {
                    if (p.getFlags().is_pointer || !p.getType()->getInfo().is_primitive || p.getType()->isEqualTo(m_ctx->getTypes()->getVoidPtr())) {
                        finalParams.push(p);
                    } else {
                        Value s = cf->stack(p.getType(), true, utils::String::Format("Copy value of parameter %d to stack to get pointer", a));
                        add(ir_assign).op(s).op(p);
                        tempStackArgs.push(s);

                        Value a = cf->val(p.getType());
                        add(ir_stack_ptr).op(a).op(cf->imm<alloc_id>(s.getStackAllocId()));
                        finalParams.push(a);
                    }
                }
            }

            for (u8 a = 0;a < fargs.size();a++) {
                const auto& arg = fargs[a];
                add(ir_param).op(finalParams[a]).op(cf->imm<u8>(u8(arg.argType)));
            }

            auto call = add(ir_call).op(callee);

            if (tempStackArgs.size() > 0) {
                add(ir_noop).comment("Free temporary stack slots");

                for (u32 i = 0;i < tempStackArgs.size();i++) {
                    add(ir_stack_free).op(cf->imm<alloc_id>(tempStackArgs[i].getStackAllocId()));
                }
            }

            Value result;
            if (retTp->getInfo().size == 0) {
                result.reset(cf->val(m_ctx->getTypes()->getVoid()));
            } else {
                if (resultHandledInternally && !resultShouldBeHandledInternally) {
                    // Result needs to be moved to expression result storage
                    add(ir_noop).comment("Result needs to be moved to expression result storage");

                    if (returnsPointer) {
                        // resultPtr = Pointer<origRetTp>

                        result.reset(copyValueToExprStorage(resultPtr));

                        // resultStack should be freed, the value was copy constructed elsewhere
                        add(ir_stack_free).op(cf->imm(resultStack.getStackAllocId()));
                    } else if (retTp->getInfo().is_primitive) {
                        // resultPtr = retTp*

                        // value must be loaded from resultPtr
                        Value tmp = Value(resultPtr);
                        tmp.setType(m_ctx->getTypes()->getVoidPtr());
                        result.reset(cf->val(retTp));
                        add(ir_load).op(result).op(tmp);

                        result.reset(copyValueToExprStorage(result));

                        // resultStack should be freed, the value now exists elsewhere
                        add(ir_stack_free).op(cf->imm(resultStack.getStackAllocId()));
                    } else {
                        // resultPtr = retTp* (non-primitive)

                        result.reset(copyValueToExprStorage(resultPtr));
                        
                        // resultStack should be freed, the value was copy constructed elsewhere
                        add(ir_stack_free).op(cf->imm(resultStack.getStackAllocId()));
                    }
                } else if (resultShouldBeHandledInternally) {
                    // Default behavior

                    if (returnsPointer) {
                        // resultPtr = Pointer<origRetTp>
                        result.reset(resultPtr);
                        result.setStackSrc(resultStack);

                        // resultStack will be destroyed at the end of the scope
                        scope().get().addToStack(resultStack);
                    } else if (retTp->getInfo().is_primitive) {
                        // resultPtr = retTp*
                        result.reset(cf->val(retTp));

                        // value must be loaded from resultPtr
                        Value tmp = Value(resultPtr);
                        tmp.setType(m_ctx->getTypes()->getVoidPtr());
                        add(ir_load).op(result).op(tmp).comment("Load result from stack");

                        // resultStack should be freed, the value now exists elsewhere
                        add(ir_stack_free).op(cf->imm(resultStack.getStackAllocId()));
                    } else {
                        // resultPtr = retTp* (non-primitive)

                        result.reset(resultPtr);
                        result.setStackSrc(resultStack);

                        // resultStack will be destroyed at the end of the scope
                        scope().get().addToStack(resultStack);
                    }
                } else {
                    // Result pointer came nicely from getStorageForExpr

                    if (returnsPointer) {
                        // resultPtr = Pointer<origRetTp>
                        result.reset(resultPtr);
                    } else if (retTp->getInfo().is_primitive) {
                        // resultPtr = retTp*
                        result.reset(cf->val(retTp));

                        // value must be loaded from resultPtr
                        Value tmp = Value(resultPtr);
                        tmp.setType(m_ctx->getTypes()->getVoidPtr());
                        add(ir_load).op(result).op(tmp).comment("Load result from return pointer");;
                    } else {
                        // resultPtr = retTp* (non-primitive)

                        result.reset(resultPtr);
                    }
                }
            }

            if (exp) {
                exp->targetNextCall = false;
                exp->targetNextConstructor = false;
            }

            add(ir_noop).comment(utils::String::Format("-------- end call %s --------", fn.toString(m_ctx).c_str()));
            return result;
        }
        
        Value Compiler::generateCall(Function* fn, const utils::Array<Value>& args, const Value* self) {
            FunctionType* sig = fn->getSignature();
            return generateCall(functionValue(fn), fn->getDisplayName(), sig->returnsPointer(), sig->getReturnType(), sig->getArguments(), args, self);
        }

        Value Compiler::generateCall(FunctionDef* fn, const utils::Array<Value>& args, const Value* self) {
            utils::String name;
            Function* ofn = fn->getOutput();
            
            if (ofn) {
                return generateCall(ofn, args, self);
            } else {
                if (fn->getReturnType() && fn->isReturnTypeExplicit()) {
                    name = fn->getReturnType()->getName() + " ";
                }
                if (fn->getThisType()) name += fn->getThisType()->getName() + "::";
                name = fn->getName();
            }

            return generateCall(functionValue(fn), name, false, fn->getReturnType(), fn->getArgs(), args, self);
        }

        Value Compiler::generateCall(const Value& fn, const utils::Array<Value>& args, const Value* self) {
            if (fn.isFunction() && fn.isImm()) {
                FunctionDef* fd = fn.getImm<FunctionDef*>();
                Function* ofn = fd->getOutput();
                if (ofn) return generateCall(ofn, args, self);

                return generateCall(functionValue(fd), fd->getName(), false, fd->getReturnType(), fd->getArgs(), args, self);
            }

            FunctionType* sig = (FunctionType*)fn.getType();
            return generateCall(fn, fn.getName(), sig->returnsPointer(), sig->getReturnType(), sig->getArguments(), args, self);
        }

        //
        // Data type resolution
        //

        DataType* getTypeFromSymbol(symbol* s) {
            if (!s) return nullptr;

            if (s->value->getFlags().is_type) return s->value->getType();

            return nullptr;
        }
        
        DataType* Compiler::getArrayType(DataType* elemTp) {
            String name = "Array<" + elemTp->getName() + ">";
            symbol* s = scope().get(name);

            if (s) {
                DataType* tp = getTypeFromSymbol(s);
                if (tp) return tp;
                else {
                    error(
                        cm_err_internal,
                        "'Array<%s>' does not name a data type",
                        elemTp->getName().c_str()
                    );

                    return currentFunction()->getPoison().getType();
                }
            } else {
                String fullName = "Array<" + elemTp->getFullyQualifiedName() + ">";
                TemplateType* at = m_ctx->getTypes()->getArray();
                TemplateContext* tctx = at->getTemplateData();
                ParseNode* arrayAst = tctx->getAST();
                Scope& tscope = scope().enter();
                DataType* outTp = nullptr;

                // Add the array element type to the symbol table as the template argument name
                tscope.add(arrayAst->template_parameters->str(), elemTp);

                pushTemplateContext(tctx);

                if (arrayAst->tp == nt_type) {
                    outTp = resolveTypeSpecifier(arrayAst->data_type);
                } else if (arrayAst->tp == nt_class) {
                    outTp = compileClass(arrayAst, true);
                }
                outTp->m_templateBase = at;
                outTp->m_templateArgs.push(elemTp);

                popTemplateContext();
                
                getOutput()->getModule()->addForeignType(outTp);

                scope().exit();
                return outTp;
            }

            return nullptr;
        }
        DataType* Compiler::getPointerType(DataType* destTp) {
            String name = "Pointer<" + destTp->getName() + ">";
            symbol* s = scope().get(name);
            
            if (s) {
                DataType* tp = getTypeFromSymbol(s);
                if (tp) return tp;
                
                error(
                    cm_err_internal,
                    "'Pointer<%s>' does not name a data type",
                    destTp->getName().c_str()
                );

                return currentFunction()->getPoison().getType();
            } else {
                String fullName = "Pointer<" + destTp->getFullyQualifiedName() + ">";
                TemplateType* pt = m_ctx->getTypes()->getPointer();
                TemplateContext* tctx = pt->getTemplateData();
                ParseNode* pointerAst = tctx->getAST();
                Scope& tscope = scope().enter();

                // Add the pointer destination type to the symbol table as the template argument name
                tscope.add(pointerAst->template_parameters->str(), destTp);

                pushTemplateContext(tctx);

                DataType* outTp = nullptr;
                if (pointerAst->tp == nt_type) {
                    outTp = resolveTypeSpecifier(pointerAst->data_type);
                } else if (pointerAst->tp == nt_class) {
                    outTp = compileClass(pointerAst, true);
                }
                outTp->m_templateBase = pt;
                outTp->m_templateArgs.push(destTp);

                popTemplateContext();

                getOutput()->getModule()->addForeignType(outTp);

                scope().exit();

                return outTp;
            }

            return nullptr;
        }
        DataType* Compiler::resolveTemplateTypeSubstitution(ParseNode* templateArgs, DataType* _type) {
            if (!_type->getInfo().is_template) {
                typeError(
                    _type,
                    cm_err_template_type_expected,
                    "Type '%s' is not a template type but template arguments were provided",
                    _type->getName().c_str()
                );

                return currentFunction()->getPoison().getType();
            }

            TemplateType* type = (TemplateType*)_type;

            TemplateContext* tctx = type->getTemplateData();
            ParseNode* tpAst = tctx->getAST();
            enterNode(tpAst);
            Scope& s = scope().enter();

            // Add template parameters to symbol table, representing the provided arguments
            // ...Also construct type name
            String name = _type->getName();
            String fullName = _type->getFullyQualifiedName();
            utils::Array<DataType*> targs;
            name += '<';
            fullName += '<';

            ParseNode* n = tpAst->template_parameters;
            u32 pc = 0;
            while (n) {
                enterNode(n);
                DataType* pt = resolveTypeSpecifier(templateArgs);
                targs.push(pt);
                s.add(n->str(), pt);
                name += pt->getName();
                fullName += pt->getFullyQualifiedName();
                if (n != tpAst->template_parameters) {
                    name += ", ";
                    fullName += ", ";
                }

                pc++;
                n = n->next;
                templateArgs = templateArgs->next;

                if (n && !templateArgs) {
                    u32 ec = pc;
                    ParseNode* en = n;
                    while (en) {
                        ec++;
                        en = en->next;
                    }

                    typeError(
                        type,
                        cm_err_too_few_template_args,
                        "Type '%s' expects %d template argument%s, but %d %s provided",
                        type->getName().c_str(),
                        ec,
                        ec == 1 ? "" : "s",
                        pc,
                        pc == 1 ? "was" : "were"
                    );

                    scope().exit();
                    exitNode();
                    return currentFunction()->getPoison().getType();
                }
                exitNode();
            }

            name += '>';
            fullName += '>';

            if (!n && templateArgs) {
                u32 ec = pc;
                ParseNode* pn = templateArgs;
                while (pn) {
                    pc++;
                    pn = pn->next;
                }

                typeError(
                    type,
                    cm_err_too_few_template_args,
                    "Type '%s' expects %d template argument%s, but %d %s provided",
                    type->getName().c_str(),
                    ec,
                    ec == 1 ? "" : "s",
                    pc,
                    pc == 1 ? "was" : "were"
                );
                scope().exit();
                exitNode();
                return currentFunction()->getPoison().getType();
            }

            symbol* existing = scope().get(name);
            if (existing) {
                DataType* etp = getTypeFromSymbol(existing);

                if (!etp) {
                    typeError(
                        type,
                        cm_err_symbol_already_exists,
                        "Symbol '%s' already exists and it is not a data type",
                        name.c_str()
                    );
                    scope().exit();
                    exitNode();
                    return currentFunction()->getPoison().getType();
                }

                scope().exit();
                exitNode();
                return etp;
            }

            pushTemplateContext(tctx);

            DataType* tp = nullptr;
            if (tpAst->tp == nt_type) {
                tp = resolveTypeSpecifier(tpAst->data_type);
                tp->m_templateBase = type;
                tp->m_templateArgs = targs;

                tp = new AliasType(name, fullName, tp);
                tp->setAccessModifier(private_access);
                m_output->add(tp);
            } else if (tpAst->tp == nt_class) {
                tp = compileClass(tpAst, true);
                tp->m_templateBase = type;
                tp->m_templateArgs = targs;
            }
            
            popTemplateContext();

            scope().exit();
            exitNode();
            return tp;
        }
        DataType* Compiler::resolveObjectTypeSpecifier(ParseNode* n) {
            enterNode(n);
            Array<type_property> props;
            Function* dtor = nullptr;
            type_meta info = { 0 };
            info.is_pod = 1;
            info.is_trivially_constructible = 1;
            info.is_trivially_copyable = 1;
            info.is_trivially_destructible = 1;
            info.is_anonymous = 1;

            ParseNode* b = n->body;
            while (b) {
                switch (b->tp) {
                    case nt_type_property: {
                        DataType* pt = resolveTypeSpecifier(b->data_type);
                        props.push({
                            b->str(),
                            public_access,
                            info.size,
                            pt,
                            {
                                1, // can read
                                1, // can write
                                0, // static
                                0  // pointer
                            },
                            nullptr,
                            nullptr
                        });

                        const type_meta& i = pt->getInfo();
                        if (!i.is_pod) info.is_pod = 0;
                        if (!i.is_trivially_constructible) info.is_trivially_constructible = 0;
                        if (!i.is_trivially_copyable) info.is_trivially_copyable = 0;
                        if (!i.is_trivially_destructible) info.is_trivially_destructible = 0;
                        info.size += pt->getInfo().size;

                        break;
                    }
                    default: {
                        break;
                    }
                }

                b = b->next;
            }

            String name = String::Format("$anon_%d", getContext()->getTypes()->getNextAnonTypeIndex());
            DataType* tp = new DataType(
                name,
                name,
                info,
                props,
                {},
                dtor,
                {}
            );

            // anon types are always public
            tp->setAccessModifier(public_access);

            const Array<DataType*>& types = getContext()->getTypes()->allTypes();
            bool alreadyExisted = false;
            for (u32 i = 0;i < types.size();i++) {
                if (types[i]->getInfo().is_anonymous && types[i]->isEquivalentTo(tp)) {
                    delete tp;
                    tp = types[i];
                    alreadyExisted = true;
                }
            }

            if (!alreadyExisted) {
                m_output->add(tp);
            }

            exitNode();
            return tp;
        }
        DataType* Compiler::resolveFunctionTypeSpecifier(ParseNode* n) {
            enterNode(n);
            DataType* retTp = resolveTypeSpecifier(n->data_type);
            DataType* ptrTp = m_ctx->getTypes()->getVoidPtr();
            Array<function_argument> args;
            args.push({ arg_type::func_ptr, ptrTp });
            args.push({ arg_type::ret_ptr, retTp });
            args.push({ arg_type::context_ptr, ptrTp });

            ParseNode* a = n->parameters;
            while (a) {
                DataType* atp = resolveTypeSpecifier(a->data_type);
                args.push({
                    atp->getInfo().is_primitive ? arg_type::value : arg_type::pointer,
                    atp
                });
                a = a->next;
            }

            DataType* tp = new FunctionType(retTp, args, false);
            
            // function types are always public
            tp->setAccessModifier(public_access);

            m_output->add(tp);
            exitNode();
            return tp;
        }
        DataType* Compiler::resolveTypeNameSpecifier(ParseNode* n) {
            String name = n->body->str();
            symbol* s = scope().get(name);
            if (!s) {
                error(
                    n,
                    cm_err_identifier_not_found,
                    "Undefined identifier '%s'",
                    name.c_str()
                );

                return currentFunction()->getPoison().getType();
            }

            DataType* srcTp = getTypeFromSymbol(s);

            if (!srcTp) {
                error(
                    n,
                    cm_err_identifier_does_not_refer_to_type,
                    "Identifier '%s' does not name a type",
                    name.c_str()
                );

                return currentFunction()->getPoison().getType();
            }

            if (n->template_parameters) {
                enterNode(n);
                DataType* tp = resolveTemplateTypeSubstitution(n->template_parameters, srcTp);
                exitNode();
                return tp;
            }

            return srcTp;
        }
        DataType* Compiler::applyTypeModifiers(DataType* tp, ParseNode* mod) {
            if (!mod) return tp;
            enterNode(mod);
            DataType* outTp = nullptr;

            if (mod->flags.is_array) outTp = getArrayType(tp);
            else if (mod->flags.is_pointer) outTp = getPointerType(tp);

            outTp = applyTypeModifiers(outTp, mod->modifier);
            exitNode();
            return outTp;
        }
        DataType* Compiler::resolveTypeSpecifier(ParseNode* n) {
            DataType* tp = nullptr;
            if (n->body && n->body->tp == nt_type_property) tp = resolveObjectTypeSpecifier(n);
            else if (n->parameters) tp = resolveFunctionTypeSpecifier(n);
            else tp = resolveTypeNameSpecifier(n);

            return applyTypeModifiers(tp, n->modifier);
        }
        void Compiler::importTemplateContext(TemplateContext* tctx) {
            const auto& moduleData = tctx->getModuleDataImports();
            const auto& modules = tctx->getModuleImports();
            const auto& funcs = tctx->getFunctionImports();
            const auto& types = tctx->getTypeImports();

            for (u32 i = 0;i < moduleData.size();i++) {
                scope().add(
                    moduleData[i].alias,
                    m_ctx->getModule(moduleData[i].module_id),
                    moduleData[i].slot_id
                );
            }

            for (u32 i = 0;i < modules.size();i++) {
                scope().add(
                    modules[i].alias,
                    m_ctx->getModule(modules[i].id)
                );
            }

            for (u32 i = 0;i < funcs.size();i++) {
                scope().add(
                    funcs[i].alias,
                    getOutput()->getFunctionDef(funcs[i].fn)
                );
            }

            for (u32 i = 0;i < types.size();i++) {
                scope().add(
                    types[i].alias,
                    types[i].tp
                );
            }
        }
        void Compiler::buildTemplateContext(ParseNode* n, TemplateContext* tctx, robin_hood::unordered_set<utils::String>& added) {
            if (n->tp == nt_identifier) {
                utils::String alias = n->str();
                if (added.count(alias) > 0) return;
                
                symbol* s = scope().get(alias);
                if (!s) return;

                Value& v = *s->value;
                auto& f = v.getFlags();

                if (f.is_module) {
                    tctx->addModuleImport(alias, v.getImm<Module*>()->getId());
                    added.insert(alias);
                } else if (f.is_module_data) {
                    tctx->addModuleDataImport(alias, v.getImm<Module*>()->getId(), v.getModuleDataSlotId());
                    added.insert(alias);
                } else if (f.is_type) {
                    tctx->addTypeImport(alias, v.getType());
                    added.insert(alias);
                } else if (f.is_function && v.isImm()) {
                    tctx->addFunctionImport(alias, v.getImm<FunctionDef*>()->getOutput());
                    added.insert(alias);
                }

                return;
            }

            if (n->data_type) buildTemplateContext(n->data_type, tctx, added);
            if (n->lvalue) buildTemplateContext(n->lvalue, tctx, added);
            if (n->rvalue) {
                if (n->tp == nt_expression && n->op == op_member) {
                    // Do not count member expression rvalues. If the
                    // property/method/export name exists in the symbol
                    // table then it is a coincidence and should not be
                    // imported when instantiating the template.
                } else buildTemplateContext(n->rvalue, tctx, added);
            }
            if (n->cond) buildTemplateContext(n->cond, tctx, added);
            if (n->body) buildTemplateContext(n->body, tctx, added);
            if (n->else_body) buildTemplateContext(n->else_body, tctx, added);
            if (n->initializer) buildTemplateContext(n->initializer, tctx, added);
            if (n->parameters) buildTemplateContext(n->parameters, tctx, added);
            if (n->template_parameters) buildTemplateContext(n->template_parameters, tctx, added);
            if (n->modifier) buildTemplateContext(n->modifier, tctx, added);
            if (n->alias) buildTemplateContext(n->alias, tctx, added);
            if (n->inheritance) buildTemplateContext(n->inheritance, tctx, added);
            if (n->next) buildTemplateContext(n->next, tctx, added);
        }

        void Compiler::compileDefaultConstructor(ffi::DataType* forTp, u64 thisOffset) {
            if (!forTp->getInfo().is_trivially_constructible) return;

            DataType* voidTp = m_ctx->getTypes()->getVoid();
            DataType* ptrTp = m_ctx->getTypes()->getVoidPtr();
            utils::Array<function_argument> args;
            args.push({ arg_type::func_ptr, ptrTp });
            args.push({ arg_type::ret_ptr, voidTp });
            args.push({ arg_type::context_ptr, ptrTp });
            args.push({ arg_type::this_ptr, forTp });
            
            FunctionType* sig = new FunctionType(voidTp, args, false);
            // function types are always public
            sig->setAccessModifier(public_access);

            bool sigExisted = false;
            const auto& types = m_output->getTypes();
            for (auto* t : types) {
                const auto& info = t->getInfo();
                if (!info.is_function) continue;
                if (sig->isEquivalentTo(t)) {
                    delete sig;
                    sig = (FunctionType*)t;
                    sigExisted = true;
                    break;
                }
            }

            if (!sigExisted) {
                const auto& types = getContext()->getTypes()->allTypes();
                for (auto* t : types) {
                    const auto& info = t->getInfo();
                    if (!info.is_function) continue;
                    if (sig->isEquivalentTo(t)) {
                        delete sig;
                        sig = (FunctionType*)t;
                        sigExisted = true;
                        break;
                    }
                }
            }

            if (!sigExisted) {
                m_output->add(sig);
            }

            Method* m = new Method(
                "constructor",
                generateFullQualifierPrefix(),
                sig,
                public_access,
                nullptr,
                nullptr,
                thisOffset
            );

            m_output->newFunc(m, nullptr, true);
            enterFunction(m_output->getFunctionDef(m));
            currentFunction()->setThisType(forTp);

            // todo:
            // - Call base constructors with their respective offsets to self
            // - Initialize type's own properties with their default constructors, or to 0 if primitive

            exitFunction();

            forTp->m_methods.push(m);
        }
        DataType* Compiler::compileType(ParseNode* n) {
            enterNode(n);
            DataType* tp = nullptr;
            String name = n->str();

            if (n->template_parameters) {
                TemplateContext* tctx = createTemplateContext(n);
                tp = new TemplateType(name, generateFullQualifierPrefix() + name, tctx);
                tp->setAccessModifier(private_access);
                m_output->add(tp);
            } else {
                tp = resolveTypeSpecifier(n->data_type);

                if (tp->m_info.size == 0) {
                    // No 0-byte structures allowed, add padding
                    tp->m_info.size = 1;
                }

                if (tp) {
                    compileDefaultConstructor(tp, 0);
                    tp = new AliasType(name, generateFullQualifierPrefix() + name, tp);
                    tp->setAccessModifier(private_access);
                    m_output->add(tp);
                }
            }

            exitNode();
            return tp;
        }
        void Compiler::compileMethodDef(ParseNode* n, DataType* methodOf, Method* m) {
            type_id prevSigId = m->getSignature()->getId();
            m->setThisType(methodOf, m_ctx->getTypes());
            if (m->getSignature()->getId() != prevSigId) {
                prevSigId = m->getSignature()->getId();
                m_output->add(m->getSignature());
            }
            
            FunctionType* sig = m->getSignature();
            const auto& args = sig->getArguments();
            ParseNode* p = n->parameters;

            enterNode(n->body);

            FunctionDef* def = m_output->getFunctionDef(m);
            def->setThisType(methodOf);
            enterFunction(def);

            while (p && p->tp != nt_empty) {
                def->addDeferredArg(p->str());
                p = p->next;
            }

            compileBlock(n->body);

            m->setRetType(def->getReturnType(), false, m_ctx->getTypes());
            if (m->getSignature()->getId() != prevSigId) {
                m_output->add(m->getSignature());
            }

            exitFunction();
            exitNode();
        }
        Method* Compiler::compileMethodDecl(ParseNode* n, u64 thisOffset, bool* wasDtor, bool templatesDefined, bool dtorExists) {
            enterNode(n);
            utils::String name = n->str();
            DataType* voidTp = m_ctx->getTypes()->getVoid();
            DataType* ptrTp = m_ctx->getTypes()->getVoidPtr();

            bool isOperator = false;
            if (name == "operator") {
                isOperator = true;
                if (n->op != op_undefined) {
                    static const char* opStrs[] = {
                        ""   ,
                        "+"  , "+=",
                        "-"  , "-=",
                        "*"  , "*=",
                        "/"  , "/=",
                        "%"  , "%=",
                        "^"  , "^=",
                        "&"  , "&=",
                        "|"  , "|=",
                        "~"  ,
                        "<<" , "<<=",
                        ">>" , ">>=",
                        "!"  , "!=",
                        "&&" , "&&=",
                        "||" , "||=",
                        "="  , "==",
                        "<"  , "<=",
                        ">"  , ">=",
                        "++" , "++",
                        "--" , "--",
                        "-"  , "*",
                        "[]" ,
                        "?"  ,
                        "."  ,
                        "new", "new",
                        "()"
                    };
                    name += opStrs[n->op];
                }
            }
            
            if (n->template_parameters) {
                if (!templatesDefined) {
                    TemplateContext* tctx = createTemplateContext(n);
                    Method* m = new TemplateMethod(
                        name,
                        generateFullQualifierPrefix(),
                        n->flags.is_private ? private_access : public_access,
                        thisOffset,
                        tctx
                    );
                    m_output->newFunc(m, n);
                    exitNode();
                    return m;
                }

                name += '<';

                ParseNode* arg = n->template_parameters;
                while (arg) {
                    symbol* s = scope().get(arg->str());
                    // symbol for template parameter should always be defined here

                    DataType* pt = getTypeFromSymbol(s);
                    name += pt->getFullyQualifiedName();
                    if (arg != n->template_parameters) name += ", ";

                    arg = arg->next;
                }

                name += '>';
            }

            if (isOperator) {
                if (n->op == op_undefined) {
                    DataType* castTarget = resolveTypeSpecifier(n->data_type);
                    name += " " + castTarget->getFullyQualifiedName();
                }
            }

            bool isCtor = false;
            if (name == "destructor") {
                *wasDtor = true;
                if (dtorExists) {
                    error(
                        cm_err_dtor_already_exists,
                        "Destructor already exists for type '%s'",
                        currentClass()->getName().c_str()
                    );
                }
            } else if (name == "constructor") {
                isCtor = true;
            }

            DataType* ret = voidTp;
            if (n->data_type) {
                ret = resolveTypeSpecifier(n->data_type);
            }

            utils::Array<function_argument> args;
            args.push({ arg_type::func_ptr, ptrTp });
            args.push({ arg_type::ret_ptr, ret });
            args.push({ arg_type::context_ptr, ptrTp });

            // This type will be assigned later
            args.push({ arg_type::this_ptr, ptrTp });

            ParseNode* p = n->parameters;
            u8 a = u8(args.size());
            DataType* poisonTp = currentFunction()->getPoison().getType();
            Array<DataType*> argTps;
            Array<String> argNames;
            while (p && p->tp != nt_empty) {
                DataType* argTp = poisonTp;
                if (!p->data_type) {
                    error(
                        cm_err_expected_method_argument_type,
                        "No type specified for argument '%s' of method '%s' of class '%s'",
                        p->str().c_str(),
                        name.c_str(),
                        currentClass()->getName().c_str()
                    );
                } else argTp = resolveTypeSpecifier(p->data_type);

                args.push({ argTp->getInfo().is_primitive ? arg_type::value : arg_type::pointer, argTp });
                argTps.push(argTp);
                argNames.push(p->str());

                p = p->next;
                a++;
            }

            FunctionType* sig = new FunctionType(ret, args, false);
            
            // function types are always public
            sig->setAccessModifier(public_access);

            bool sigExisted = false;
            const auto& types = m_output->getTypes();
            for (auto* t : types) {
                const auto& info = t->getInfo();
                if (!info.is_function) continue;
                if (sig->isEquivalentTo(t)) {
                    delete sig;
                    sig = (FunctionType*)t;
                    sigExisted = true;
                    break;
                }
            }

            if (!sigExisted) {
                const auto& types = getContext()->getTypes()->allTypes();
                for (auto* t : types) {
                    const auto& info = t->getInfo();
                    if (!info.is_function) continue;
                    if (sig->isEquivalentTo(t)) {
                        delete sig;
                        sig = (FunctionType*)t;
                        sigExisted = true;
                        break;
                    }
                }
            }

            if (!sigExisted) {
                m_output->add(sig);
            }

            Method* m = new Method(
                name,
                generateFullQualifierPrefix(),
                sig,
                n->flags.is_private ? private_access : public_access,
                nullptr,
                nullptr,
                thisOffset
            );

            m_output->newFunc(m, n, n->data_type != nullptr);
            exitNode();
            return m;
        }
        DataType* Compiler::compileClass(ParseNode* n, bool templatesDefined) {
            enterNode(n);
            utils::String name = n->str();
            String fullName = generateFullQualifierPrefix() + name;
            
            if (n->template_parameters) {
                if (!templatesDefined) {
                    TemplateContext* tctx = createTemplateContext(n);
                    DataType* tp = new TemplateType(name, fullName, tctx);
                    tp->setAccessModifier(private_access);
                    m_output->add(tp);
                    exitNode();
                    return tp;
                }

                name += '<';
                fullName += '<';

                ParseNode* arg = n->template_parameters;
                while (arg) {
                    symbol* s = scope().get(arg->str());
                    DataType* pt = getTypeFromSymbol(s);
                    // symbol for template parameter should always be defined here

                    name += pt->getName();
                    fullName += pt->getFullyQualifiedName();
                    if (arg != n->template_parameters) {
                        name += ", ";
                        fullName += ", ";
                    }

                    arg = arg->next;
                }

                name += '>';
                fullName += '>';
            }

            ClassType* tp = new ClassType(name, fullName);
            tp->setAccessModifier(private_access);
            enterClass(tp);
            m_output->add(tp);
            scope().enter();
            
            ParseNode* inherits = n->inheritance;
            while (inherits) {
                // todo: Check added properties don't have name collisions with
                //       existing properties

                DataType* t = resolveTypeSpecifier(inherits);
                tp->addBase(t, public_access);
                inherits = inherits->next;
            }

            ParseNode* prop = n->body;
            u64 thisOffset = tp->getInfo().size;
            while (prop) {
                if (prop->tp != nt_property) {
                    prop = prop->next;
                    continue;
                }

                value_flags f = { 0 };
                f.can_read = 1;
                f.can_write = (!prop->flags.is_const) ? 1 : 0;
                f.is_pointer = prop->flags.is_pointer;
                f.is_static = prop->flags.is_static;

                DataType* t = resolveTypeSpecifier(prop->data_type);
                tp->addProperty(
                    prop->str(),
                    t,
                    f,
                    prop->flags.is_private ? private_access : public_access,
                    nullptr,
                    nullptr
                );

                prop = prop->next;
            }

            if (tp->m_info.size == 0) {
                // No 0-byte structures allowed, add padding
                tp->m_info.size = 1;
            }

            ParseNode* meth = n->body;
            ParseNode* dtor = nullptr;
            bool hasDefaultCtor = false;
            while (meth) {
                if (meth->tp != nt_function) {
                    meth = meth->next;
                    continue;
                }

                bool isDtor = false;
                Method* m = compileMethodDecl(meth, thisOffset, &isDtor, false, tp->getDestructor() != nullptr);
                m->m_src = meth->tok.src;

                if (!hasDefaultCtor && m->getName() == "constructor" && meth->parameters->tp == nt_empty) {
                    hasDefaultCtor = true;
                }

                if (isDtor) {
                    tp->setDestructor(m);
                    dtor = meth;
                }
                else tp->addMethod(m);

                meth = meth->next;
            }

            u32 midx = 0;
            const auto& methods = tp->getMethods();
            meth = n->body;
            while (meth) {
                if (meth->tp != nt_function) {
                    meth = meth->next;
                    continue;
                }

                Method* m = (Method*)(meth == dtor ? tp->getDestructor() : methods[midx++]);

                if (!m->isTemplate()) compileMethodDef(meth, tp, m);
                
                meth = meth->next;
            }

            if (!hasDefaultCtor) compileDefaultConstructor(tp, thisOffset);

            exitClass();
            scope().exit();
            exitNode();

            return tp;
        }
        Function* Compiler::compileFunction(ParseNode* n, bool templatesDefined) {
            enterNode(n);
            utils::String name = n->str();
            
            if (n->template_parameters) {
                if (!templatesDefined) {
                    TemplateContext* tctx = createTemplateContext(n);
                    TemplateFunction* f = new TemplateFunction(
                        name,
                        generateFullQualifierPrefix(),
                        n->flags.is_private ? private_access : public_access,
                        tctx
                    );
                    
                    f->m_src = n->tok.src;
                    m_output->newFunc(f, n);
                    exitNode();
                    return f;
                }

                name += '<';

                ParseNode* arg = n->template_parameters;
                while (arg) {
                    symbol* s = scope().get(arg->str());
                    // symbol for template parameter should always be defined here

                    DataType* pt = getTypeFromSymbol(s);
                    name += pt->getFullyQualifiedName();
                    if (arg != n->template_parameters) name += ", ";

                    arg = arg->next;
                }

                name += '>';
            }

            
            FunctionDef* fd = getOutput()->getFuncs().find([&name](FunctionDef* fd) { return fd->getName() == name; });
            if (!fd) {
                fd = getOutput()->newFunc(name, n);
            }

            enterFunction(fd);

            if (n->data_type) {
                fd->setReturnType(resolveTypeSpecifier(n->data_type));
            }

            ParseNode* p = n->parameters;
            DataType* poisonTp = currentFunction()->getPoison().getType();
            while (p && p->tp != nt_empty) {
                DataType* tp = poisonTp;
                if (!p->data_type) {
                    error(
                        cm_err_expected_function_argument_type,
                        "No type specified for argument '%s' of function '%s'",
                        p->str().c_str(),
                        name.c_str()
                    );
                } else tp = resolveTypeSpecifier(p->data_type);

                fd->addArg(p->str(), tp);
                p = p->next;
            }

            compileBlock(n->body);
            
            Function* out = exitFunction();
            exitNode();

            return out;
        }
        
        void Compiler::compileExport(ParseNode* n) {
            enterNode(n);
            if (!inInitFunction()) {
                error(cm_err_export_not_in_root_scope, "'export' keyword encountered outside of root scope");
                exitNode();
                return;
            }

            if (n->body->tp == nt_type) {
                compileType(n->body)->setAccessModifier(public_access);
                exitNode();
                return;
            } else if (n->body->tp == nt_function) {
                compileFunction(n->body)->setAccessModifier(public_access);
                exitNode();
                return;
            } else if (n->body->tp == nt_variable) {
                compileVarDecl(n->body, true);
                exitNode();
                return;
            } else if (n->body->tp == nt_class) {
                compileClass(n->body)->setAccessModifier(public_access);
                exitNode();
                return;
            }

            error(cm_err_export_invalid, "Expected type, function, class, or variable");
            exitNode();
        }
        void Compiler::compileImport(ParseNode* n) {
            enterNode(n);
            if (!inInitFunction()) {
                error(cm_err_import_not_in_root_scope, "'import' keyword encountered outside of root scope");
                exitNode();
                return;
            }

            Module* m = m_ctx->getModule(n->str(), std::filesystem::path(m_meta->path).remove_filename().string());
            if (!m) {
                error(cm_err_import_module_not_found, "Module '%s' not found", n->str().c_str());
                exitNode();
                return;
            }

            m_output->addDependency(m);

            if (n->body->tp == nt_import_module) {
                scope().add(n->body->str(), m);
            } else if (n->body->tp == nt_import_symbol) {
                ParseNode* sym = n->body;
                bool trusted = isTrusted();
                while (sym) {
                    DataType* tp = sym->data_type ? resolveTypeSpecifier(sym->data_type) : nullptr;
                    String name = sym->str();
                    utils::Array<Function*> funcs;
                    utils::Array<const module_data*> vars;
                    utils::Array<u32> var_slots;

                    m->getData().each([&name, &vars, &var_slots, trusted, tp](const module_data& m, u32 idx) {
                        if (m.access == private_access || (m.access == trusted_access && !trusted)) return;
                        if (m.name != name) return;
                        if (tp && !m.type->isEquivalentTo(tp)) return;
                        vars.push(&m);
                        var_slots.push(idx);
                    });

                    if (tp) {
                        if (tp->getInfo().is_function) {
                            FunctionType* ftp = (FunctionType*)tp;
                            utils::Array<DataType*> args = ftp->getArguments().map([](const function_argument& a) { return a.dataType; });
                            funcs = m->findFunctions(
                                name,
                                ftp->getReturnType(),
                                const_cast<const DataType**>(args.data()),
                                args.size(),
                                fm_exclude_private | fm_strict
                            );
                        }
                        
                        if (funcs.size() + vars.size() > 1) {
                            error(
                                cm_err_export_ambiguous,
                                "Module '%s' has multiple exports named '%s' that match the type '%s'",
                                n->str().c_str(),
                                name.c_str(),
                                tp->getName().c_str()
                            );
                            for (u32 i = 0;i < funcs.size();i++) {
                                info(cm_info_could_be, "^ Could be '%s'", funcs[i]->getFullyQualifiedName().c_str());
                            }
                            for (u32 i = 0;i < vars.size();i++) {
                                info(cm_info_could_be, "^ Could be '%s %s'", vars[i]->type->getName().c_str(), vars[i]->name.c_str());
                            }
                        } else if (funcs.size() == 1) {
                            m_output->import(funcs[0], sym->alias ? sym->alias->str() : name);
                        } else if (vars.size() == 1) {
                            scope().add(sym->alias ? sym->alias->str() : name, m, var_slots[0]);
                        } else {
                            DataType* mtp = m->allTypes().find([&name](const DataType* t) {
                                return t->getName() == name;
                            });

                            if (mtp) {
                                error(
                                    cm_err_import_type_spec_invalid,
                                    "Import type specifiers are not valid for imported types"
                                );
                            } else {
                                error(
                                    cm_err_export_not_found,
                                    "Module '%s' has no export named '%s' that matches the type '%s'",
                                    n->str().c_str(),
                                    name.c_str(),
                                    tp->getName().c_str()
                                );
                            }
                        }
                    } else {
                        DataType* mtp = m->allTypes().find([&name](const DataType* t) {
                            return t->getName() == name;
                        });

                        m->allFunctions().each([&funcs, &name, trusted](Function* fn) {
                            if (!fn) return;
                            access_modifier access = fn->getAccessModifier();
                            if (access == private_access || (access == trusted_access && !trusted)) return;
                            if (fn->getName() != name) return;
                            funcs.push(fn);
                        });

                        if ((funcs.size() + vars.size() + (mtp ? 1 : 0)) > 1) {
                            error(
                                cm_err_export_ambiguous,
                                "Module '%s' has multiple exports named '%s'",
                                n->str().c_str(),
                                name.c_str()
                            );
                            for (u32 i = 0;i < funcs.size();i++) {
                                info(cm_info_could_be, "^ Could be '%s'", funcs[i]->getFullyQualifiedName().c_str());
                            }
                            for (u32 i = 0;i < vars.size();i++) {
                                info(cm_info_could_be, "^ Could be '%s : %s'", vars[i]->name.c_str(), vars[i]->type->getName().c_str());
                            }
                            if (mtp) {
                                info(cm_info_could_be, "^ Could be '%s'", mtp->getFullyQualifiedName().c_str());
                            }
                        } else if (funcs.size() == 1) {
                            m_output->import(funcs[0], sym->alias ? sym->alias->str() : name);
                        } else if (vars.size() == 1) {
                            scope().add(sym->alias ? sym->alias->str() : name, m, var_slots[0]);
                        } else if (mtp) {
                            m_output->import(mtp, sym->alias ? sym->alias->str() : name);
                        } else {
                            error(
                                cm_err_export_not_found,
                                "Module '%s' has no export named '%s'",
                                n->str().c_str(),
                                name.c_str()
                            );
                        }
                    }

                    sym = sym->next;
                }
            }

            exitNode();
        }
        Value Compiler::compileConditionalExpr(ParseNode* n) {
            FunctionDef* cf = currentFunction();

            enterExpr();

            scope().enter();
            Value cond = compileExpression(n->cond);
            Value condBool = cond.convertedTo(m_ctx->getTypes()->getBoolean());
            scope().exit(condBool);

            auto reserve = add(ir_reserve);
            auto branch = add(ir_branch).op(condBool);

            // Truth body
            scope().enter();
            branch.label(cf->label());
            Value lvalue = compileExpressionInner(n->lvalue);

            // jumps to just after the false body
            auto jump = add(ir_jump);

            // Output must be created now since only now do we have the
            // output type. No instructions are emitted as a result of
            // this so this is not contingent upon the condition being
            // evaluated to true
            DataType* tp = lvalue.getType();
            Value out;

            if (!tp->getInfo().is_primitive) {
                // If the result is not a primitive type then the output
                // of the conditional must be allocated on the stack and
                // constructed from the resulting value
                // 
                // That being the case, the resolve instruction should
                // be replaced with a stack_allocate instruction and no
                // resolve instruction should be added
                // 
                // The stack id must be set now (prior to any uses of
                // result), but it must be added to the parent block's
                // stack
                out.reset(cf->stack(tp));

                constructObject(out, { lvalue });
                scope().exit(out);
            } else {
                out.reset(cf->val(tp));
                reserve.op(out);
                add(ir_resolve).op(out).op(lvalue);
                scope().exit();
            }
            // End truth body

            // False body
            branch.label(cf->label());
            scope().enter();
            
            Value rvalue = compileExpressionInner(n->rvalue);

            if (!tp->getInfo().is_primitive) constructObject(out, { rvalue });
            else add(ir_resolve).op(out).op(rvalue);

            scope().exit();
            // End false body

            // join address
            jump.label(cf->label());

            exitExpr();

            return out;
        }
        Value Compiler::compileMemberExpr(ParseNode* n, bool topLevel, member_expr_hints* hints) {
            FunctionDef* cf = currentFunction();

            enterExpr();
            enterNode(n->lvalue);
            Value lv;
            if (n->lvalue->tp == nt_expression) {
                if (n->lvalue->op == op_member) lv.reset(compileMemberExpr(n->lvalue, false, hints));
                else lv.reset(compileExpression(n->lvalue));
            } else lv.reset(compileExpressionInner(n->lvalue));
            exitNode();
            
            enterNode(n->rvalue);
            ffi::DataType* curClass = currentClass();
            Value out = lv.getProp(
                n->rvalue->str(),
                false,                                                                          // exclude inherited
                !curClass || lv.getFlags().is_module || !lv.getType()->isEqualTo(curClass),     // exclude private
                false,                                                                          // exclude methods
                true,                                                                           // emit errors
                hints                                                                           // hints
            );
            exitNode();
            exitExpr();

            if (hints) {
                if (hints->self) delete hints->self;
                hints->self = new Value(lv);
            }

            return out;
        }
        Value Compiler::compileArrowFunction(ParseNode* n) {
            enterExpr();

            utils::Array<Value> captures;
            utils::Array<u32> captureOffsets;
            robin_hood::unordered_map<utils::String, u32> declaredNames;
            findCaptures(n->body, captures, declaredNames, 0);

            Value captureData;
            Value* pCaptureData = nullptr;
            if (captures.size() > 0) {
                captureOffsets.reserve(captures.size());
                captureData.reset(allocateCaptureData(captures, captureOffsets));
                pCaptureData = &captureData;
            }

            Function* closureFunc = nullptr;
            {
                enterNode(n);
                utils::String name = utils::String::Format("$anon_%x_%x", uintptr_t(n), rand() % UINT16_MAX);

                FunctionDef* fd = getOutput()->getFuncs().find([&name](FunctionDef* fd) { return fd->getName() == name; });
                if (!fd) fd = getOutput()->newClosure(name, n);

                enterFunction(fd);

                DataType* voidp = m_ctx->getTypes()->getVoidPtr();
                Value& capPtr = fd->getCaptures();
                Scope& s = scope().get();
                for (u32 i = 0;i < captures.size();i++) {
                    DataType* tp = captures[i].getType();
                    Value& c = fd->val(captures[i].getName(), tp);

                    if (tp->getInfo().is_primitive) {
                        Value tmp = fd->val(voidp);
                        fd->add(ir_uadd).op(tmp).op(capPtr).op(fd->imm(captureOffsets[i]));
                        fd->add(ir_load).op(c).op(tmp);
                    } else {
                        fd->add(ir_uadd).op(c).op(capPtr).op(fd->imm(captureOffsets[i]));
                    }
                }

                if (n->data_type) {
                    fd->setReturnType(resolveTypeSpecifier(n->data_type));
                }

                ParseNode* p = n->parameters;
                DataType* poisonTp = currentFunction()->getPoison().getType();
                while (p && p->tp != nt_empty) {
                    DataType* tp = poisonTp;
                    if (!p->data_type) {
                        error(
                            cm_err_expected_function_argument_type,
                            "No type specified for argument '%s' of function '%s'",
                            p->str().c_str(),
                            name.c_str()
                        );
                    } else tp = resolveTypeSpecifier(p->data_type);

                    fd->addArg(p->str(), tp);
                    p = p->next;
                }

                compileBlock(n->body);
                
                closureFunc = exitFunction();
                exitNode();
            }

            Value closure = newClosure(closureFunc->getId(), pCaptureData);
            Value closureRef = newClosureRef(closure, closureFunc->getSignature());

            exitExpr();
            return closureRef;
        }
        Value Compiler::compileObjectLiteral(ParseNode* n) {
            Array<Value> values;
            Array<type_property> props;

            type_meta info = { 0 };
            info.is_pod = 1;
            info.is_trivially_constructible = 1;
            info.is_trivially_copyable = 1;
            info.is_trivially_destructible = 1;
            info.is_anonymous = 1;

            ParseNode* p = n->body;

            enterExpr();
            while (p) {
                values.push(compileExpression(p->initializer));
                DataType* pt = values.last().getType();
                props.push({
                    p->str(),
                    public_access,
                    info.size,
                    pt,
                    {
                        1, // can read
                        1, // can write
                        0, // static
                        0  // pointer
                    },
                    nullptr,
                    nullptr
                });

                const type_meta& i = pt->getInfo();
                if (!i.is_pod) info.is_pod = 0;
                if (!i.is_trivially_constructible) info.is_trivially_constructible = 0;
                if (!i.is_trivially_copyable) info.is_trivially_copyable = 0;
                if (!i.is_trivially_destructible) info.is_trivially_destructible = 0;
                info.size += pt->getInfo().size;

                p = p->next;
            }
            exitExpr();

            if (info.size == 0) {
                // No 0-byte structures allowed, add padding
                info.size = 1;
            }

            String name = String::Format("$anon_%d", getContext()->getTypes()->getNextAnonTypeIndex());
            DataType* tp = new DataType(
                name,
                name,
                info,
                props,
                {},
                nullptr,
                {}
            );

            // anon types are always public
            tp->setAccessModifier(public_access);

            const Array<DataType*>& types = getContext()->getTypes()->allTypes();
            bool alreadyExisted = false;
            for (u32 i = 0;i < types.size();i++) {
                if (types[i]->getInfo().is_anonymous && types[i]->isEquivalentTo(tp)) {
                    delete tp;
                    tp = types[i];
                    alreadyExisted = true;
                }
            }

            if (!alreadyExisted) {
                m_output->add(tp);
            }

            FunctionDef* cf = currentFunction();

            Value out;
            
            auto* exp = currentExpr();
            bool resultShouldBeHandledInternally = !exp || exp->loc == rsl_auto;
            bool resultHandledInternally = resultShouldBeHandledInternally;
            if (!resultShouldBeHandledInternally && exp && exp->expectedType) {
                if (!exp->expectedType->isEqualTo(tp)) {
                    // Value needs conversion before going to final location
                    resultHandledInternally = true;
                }
            }

            if (resultHandledInternally) {
                out.reset(cf->stack(tp));
            } else {
                out.reset(getStorageForExpr(tp));
            }

            enterExpr();
            for (u32 p = 0;p < props.size();p++) {
                constructObject(out.getPropPtr(props[p].name), { values[p] });
            }
            exitExpr();

            if (resultHandledInternally && !resultShouldBeHandledInternally) {
                return copyValueToExprStorage(out);
            }

            return out;
        }
        Value Compiler::compileArrayLiteral(ParseNode* n) {
            FunctionDef* cf = currentFunction();
            ParseNode* elem;
            if (n->body->tp == nt_expression_sequence) elem = n->body->body;
            else elem = n->body;

            ParseNode* countEle = elem;
            u32 count = 0;
            while (countEle) {
                count++;
                countEle = countEle->next;
            }

            Value out;
            DataType* tp = nullptr;
            DataType* arrTp = nullptr;
            Value ptr;
            u32 idx = 0;

            auto* exp = currentExpr();
            if (exp->expectedType && exp->expectedType->isInstantiationOf(m_ctx->getTypes()->getArray())) {
                // type known ahead of time, construct the array and initialize the array element pointer
                arrTp = exp->expectedType->getEffectiveType();
                tp = exp->expectedType->getTemplateArguments()[0];

                if (exp) exp->targetNextConstructor = true;
                out.reset(constructObject(arrTp, { cf->imm(count) }));
                if (exp) exp->targetNextConstructor = false;

                enterExpr();
                ptr.reset(out.getProp("_data", false, false, true, false).convertedTo(tp));
                out.getProp("_length", false, false, true, false) = cf->imm(count);
                exitExpr();
            }

            while (elem) {
                if (idx == 0 && !tp) {
                    // type not known ahead of time. First element must be evaluated to get
                    // array type.

                    enterExpr();
                    Value first = compileExpression(elem);

                    tp = first.getType();
                    arrTp = getArrayType(tp);
                    exitExpr();

                    if (exp) exp->targetNextConstructor = true;
                    out.reset(constructObject(arrTp, { cf->imm(count) }));
                    if (exp) exp->targetNextConstructor = false;

                    enterExpr();
                    ptr.reset(out.getProp("_data", false, false, true, false).convertedTo(tp));
                    ptr.getFlags().is_pointer = 1;
                    out.getProp("_length", false, false, true, false) = cf->imm(count);

                    // move first element into array
                    constructObject(ptr, tp, { first });

                    // increment pointer
                    cf->add(ir_uadd).op(ptr).op(ptr).op(cf->imm<u32>(tp->getInfo().size));
                    exitExpr();
                } else {
                    // Evaluate the expression, with the expression hints indicating that
                    // the result should be stored in the array
                    auto* nestExp = enterExpr();
                    nestExp->destination = &ptr;
                    nestExp->expectedType = tp;
                    nestExp->loc = rsl_specified_dest;
                    compileExpression(elem);
                    exitExpr();

                    // increment pointer
                    cf->add(ir_uadd).op(ptr).op(ptr).op(cf->imm<u32>(tp->getInfo().size));
                }

                elem = elem->next;
                idx++;
            }

            return out;
        }
        Value Compiler::compileExpressionInner(ParseNode* n) {
            // name shortened to reduce clutter
            auto vs = [this](const Value& v) {
                return copyValueToExprStorage(v);
            };

            if (n->tp == nt_identifier) {
                symbol* s = scope().get(n->str());

                if (s) {
                    if (s->value->getFlags().is_function && s->value->isImm() && !s->value->getType()) {
                        FunctionDef* fd = s->value->getImm<FunctionDef*>();
                        ffi::Function* fn = fd->getOutput();
                        if (!fn && fd->getNode()) {
                            fn = compileFunction(fd->getNode());
                        }

                        ffi::DataType* sig = fn ? fn->getSignature() : nullptr;

                        if (!sig) {
                            error(
                                cm_err_function_signature_not_determined,
                                "Signature for function '%s' could not be determined",
                                fd->getName().c_str()
                            );

                            return vs(currentFunction()->getPoison());
                        }
                    }

                    return vs(*s->value);
                }

                error(n, cm_err_identifier_not_found, "Undefined identifier '%s'", n->str().c_str());
                return currentFunction()->getPoison();
            } else if (n->tp == nt_literal) {
                switch (n->value_tp) {
                    case lt_u8 : return vs(currentFunction()->imm<u8>((u8)n->value.u));
                    case lt_u16: return vs(currentFunction()->imm<u16>((u16)n->value.u));
                    case lt_u32: return vs(currentFunction()->imm<u32>((u32)n->value.u));
                    case lt_u64: return vs(currentFunction()->imm<u64>((u64)n->value.u));
                    case lt_i8 : return vs(currentFunction()->imm<i8>((i8)n->value.i));
                    case lt_i16: return vs(currentFunction()->imm<i16>((i16)n->value.i));
                    case lt_i32: return vs(currentFunction()->imm<i32>((i32)n->value.i));
                    case lt_i64: return vs(currentFunction()->imm<i64>(n->value.i));
                    case lt_f32: return vs(currentFunction()->imm<f32>((f32)n->value.f));
                    case lt_f64: return vs(currentFunction()->imm<f64>(n->value.f));
                    case lt_string: {
                        u32 slot = m_output->getModule()->getData().size();
                        m_output->getModule()->addData(utils::String::Format("$str_%u", slot), n->str_len);
                        char* s = (char*)m_output->getModule()->getDataInfo(slot).ptr;
                        memcpy(s, n->value.s, n->str_len);

                        Value d = currentFunction()->val(m_output->getModule(), slot);

                        auto* exp = currentExpr();
                        if (exp) exp->targetNextConstructor = true;
                        m_trustEnable = true;
                        Value ret = constructObject(
                            m_ctx->getTypes()->getString(),
                            { d, currentFunction()->imm(n->str_len) }
                        );
                        m_trustEnable = false;
                        if (exp) exp->targetNextConstructor = false;

                        return ret;
                    }
                    case lt_template_string: {
                        return currentFunction()->getPoison(); // todo
                    }
                    case lt_object: return compileObjectLiteral(n);
                    case lt_array: return compileArrayLiteral(n);
                    case lt_null: return vs(currentFunction()->getNull());
                    case lt_true: return vs(currentFunction()->imm(true));
                    case lt_false: return vs(currentFunction()->imm(false));
                }

                return currentFunction()->getPoison();
            } else if (n->tp == nt_sizeof) {
                DataType* tp = resolveTypeSpecifier(n->data_type);
                if (tp) return vs(currentFunction()->imm(tp->getInfo().size));

                return currentFunction()->getPoison();
            } else if (n->tp == nt_this) {
                if (!currentClass()) {
                    error(n, cm_err_this_outside_class, "Use of 'this' keyword outside of class scope");
                    return currentFunction()->getPoison();
                }

                return vs(currentFunction()->getThis());
            } else if (n->tp == nt_cast) {
                enterExpr();
                Value v = compileExpressionInner(n->body);
                exitExpr();
                
                auto* exp = currentExpr();
                if (exp) {
                    exp->targetNextCall = true;
                    exp->targetNextConstructor = true;
                }

                Value ret = v.convertedTo(resolveTypeSpecifier(n->data_type));

                if (exp) {
                    if (exp->targetNextCall || exp->targetNextConstructor) {
                        copyValueToExprStorage(ret);
                    } else {
                        exp->targetNextCall = false;
                        exp->targetNextConstructor = false;
                    }
                }

                return ret;
            } else if (n->tp == nt_function) return vs(compileArrowFunction(n));

            switch (n->op) {
                case op_undefined: break;
                case op_add: {
                    enterExpr();
                    Value lv = compileExpressionInner(n->lvalue);
                    Value rv = compileExpressionInner(n->rvalue);
                    exitExpr();

                    auto* exp = currentExpr();
                    if (exp) exp->targetNextCall = true;
                    Value ret = lv + rv;
                    if (exp) exp->targetNextCall = false;
                    return ret;
                }
                case op_addEq: {
                    enterExpr();
                    Value lv = compileExpressionInner(n->lvalue);
                    Value rv = compileExpressionInner(n->rvalue);
                    exitExpr();

                    auto* exp = currentExpr();
                    if (exp) exp->targetNextCall = true;
                    Value ret = lv += rv;
                    if (exp) exp->targetNextCall = false;
                    return ret;
                }
                case op_sub: {
                    enterExpr();
                    Value lv = compileExpressionInner(n->lvalue);
                    Value rv = compileExpressionInner(n->rvalue);
                    exitExpr();

                    auto* exp = currentExpr();
                    if (exp) exp->targetNextCall = true;
                    Value ret = lv - rv;
                    if (exp) exp->targetNextCall = false;
                    return ret;
                }
                case op_subEq: {
                    enterExpr();
                    Value lv = compileExpressionInner(n->lvalue);
                    Value rv = compileExpressionInner(n->rvalue);
                    exitExpr();

                    auto* exp = currentExpr();
                    if (exp) exp->targetNextCall = true;
                    Value ret = lv -= rv;
                    if (exp) exp->targetNextCall = false;
                    return ret;
                }
                case op_mul: {
                    enterExpr();
                    Value lv = compileExpressionInner(n->lvalue);
                    Value rv = compileExpressionInner(n->rvalue);
                    exitExpr();

                    auto* exp = currentExpr();
                    if (exp) exp->targetNextCall = true;
                    Value ret = lv * rv;
                    if (exp) exp->targetNextCall = false;
                    return ret;
                }
                case op_mulEq: {
                    enterExpr();
                    Value lv = compileExpressionInner(n->lvalue);
                    Value rv = compileExpressionInner(n->rvalue);
                    exitExpr();

                    auto* exp = currentExpr();
                    if (exp) exp->targetNextCall = true;
                    Value ret = lv *= rv;
                    if (exp) exp->targetNextCall = false;
                    return ret;
                }
                case op_div: {
                    enterExpr();
                    Value lv = compileExpressionInner(n->lvalue);
                    Value rv = compileExpressionInner(n->rvalue);
                    exitExpr();

                    auto* exp = currentExpr();
                    if (exp) exp->targetNextCall = true;
                    Value ret = lv / rv;
                    if (exp) exp->targetNextCall = false;
                    return ret;
                }
                case op_divEq: {
                    enterExpr();
                    Value lv = compileExpressionInner(n->lvalue);
                    Value rv = compileExpressionInner(n->rvalue);
                    exitExpr();

                    auto* exp = currentExpr();
                    if (exp) exp->targetNextCall = true;
                    Value ret = lv /= rv;
                    if (exp) exp->targetNextCall = false;
                    return ret;
                }
                case op_mod: {
                    enterExpr();
                    Value lv = compileExpressionInner(n->lvalue);
                    Value rv = compileExpressionInner(n->rvalue);
                    exitExpr();

                    auto* exp = currentExpr();
                    if (exp) exp->targetNextCall = true;
                    Value ret = lv % rv;
                    if (exp) exp->targetNextCall = false;
                    return ret;
                }
                case op_modEq: {
                    enterExpr();
                    Value lv = compileExpressionInner(n->lvalue);
                    Value rv = compileExpressionInner(n->rvalue);
                    exitExpr();

                    auto* exp = currentExpr();
                    if (exp) exp->targetNextCall = true;
                    Value ret = lv %= rv;
                    if (exp) exp->targetNextCall = false;
                    return ret;
                }
                case op_xor: {
                    enterExpr();
                    Value lv = compileExpressionInner(n->lvalue);
                    Value rv = compileExpressionInner(n->rvalue);
                    exitExpr();

                    auto* exp = currentExpr();
                    if (exp) exp->targetNextCall = true;
                    Value ret = lv ^ rv;
                    if (exp) exp->targetNextCall = false;
                    return ret;
                }
                case op_xorEq: {
                    enterExpr();
                    Value lv = compileExpressionInner(n->lvalue);
                    Value rv = compileExpressionInner(n->rvalue);
                    exitExpr();

                    auto* exp = currentExpr();
                    if (exp) exp->targetNextCall = true;
                    Value ret = lv ^= rv;
                    if (exp) exp->targetNextCall = false;
                    return ret;
                }
                case op_bitAnd: {
                    enterExpr();
                    Value lv = compileExpressionInner(n->lvalue);
                    Value rv = compileExpressionInner(n->rvalue);
                    exitExpr();

                    auto* exp = currentExpr();
                    if (exp) exp->targetNextCall = true;
                    Value ret = lv & rv;
                    if (exp) exp->targetNextCall = false;
                    return ret;
                }
                case op_bitAndEq: {
                    enterExpr();
                    Value lv = compileExpressionInner(n->lvalue);
                    Value rv = compileExpressionInner(n->rvalue);
                    exitExpr();

                    auto* exp = currentExpr();
                    if (exp) exp->targetNextCall = true;
                    Value ret = lv &= rv;
                    if (exp) exp->targetNextCall = false;
                    return ret;
                }
                case op_bitOr: {
                    enterExpr();
                    Value lv = compileExpressionInner(n->lvalue);
                    Value rv = compileExpressionInner(n->rvalue);
                    exitExpr();

                    auto* exp = currentExpr();
                    if (exp) exp->targetNextCall = true;
                    Value ret = lv | rv;
                    if (exp) exp->targetNextCall = false;
                    return ret;
                }
                case op_bitOrEq: {
                    enterExpr();
                    Value lv = compileExpressionInner(n->lvalue);
                    Value rv = compileExpressionInner(n->rvalue);
                    exitExpr();

                    auto* exp = currentExpr();
                    if (exp) exp->targetNextCall = true;
                    return lv |= rv;
                }
                case op_bitInv: {
                    enterExpr();
                    Value lv = compileExpressionInner(n->lvalue);
                    exitExpr();

                    auto* exp = currentExpr();
                    if (exp) exp->targetNextCall = true;
                    Value ret = ~lv;
                    if (exp) exp->targetNextCall = false;
                    return ret;
                }
                case op_shLeft: {
                    enterExpr();
                    Value lv = compileExpressionInner(n->lvalue);
                    Value rv = compileExpressionInner(n->rvalue);
                    exitExpr();

                    auto* exp = currentExpr();
                    if (exp) exp->targetNextCall = true;
                    Value ret = lv << rv;
                    if (exp) exp->targetNextCall = false;
                    return ret;
                }
                case op_shLeftEq: {
                    enterExpr();
                    Value lv = compileExpressionInner(n->lvalue);
                    Value rv = compileExpressionInner(n->rvalue);
                    exitExpr();

                    auto* exp = currentExpr();
                    if (exp) exp->targetNextCall = true;
                    Value ret = lv <<= rv;
                    if (exp) exp->targetNextCall = false;
                    return ret;
                }
                case op_shRight: {
                    enterExpr();
                    Value lv = compileExpressionInner(n->lvalue);
                    Value rv = compileExpressionInner(n->rvalue);
                    exitExpr();

                    auto* exp = currentExpr();
                    if (exp) exp->targetNextCall = true;
                    Value ret = lv >> rv;
                    if (exp) exp->targetNextCall = false;
                    return ret;
                }
                case op_shRightEq: {
                    enterExpr();
                    Value lv = compileExpressionInner(n->lvalue);
                    Value rv = compileExpressionInner(n->rvalue);
                    exitExpr();

                    auto* exp = currentExpr();
                    if (exp) exp->targetNextCall = true;
                    Value ret = lv >>= rv;
                    if (exp) exp->targetNextCall = false;
                    return ret;
                }
                case op_not: {
                    enterExpr();
                    Value lv = compileExpressionInner(n->lvalue);
                    exitExpr();

                    auto* exp = currentExpr();
                    if (exp) exp->targetNextCall = true;
                    Value ret = !lv;
                    if (exp) exp->targetNextCall = false;
                    return ret;
                }
                case op_notEq: {
                    enterExpr();
                    Value lv = compileExpressionInner(n->lvalue);
                    Value rv = compileExpressionInner(n->rvalue);
                    exitExpr();

                    auto* exp = currentExpr();
                    if (exp) exp->targetNextCall = true;
                    Value ret = lv != rv;
                    if (exp) exp->targetNextCall = false;
                    return ret;
                }
                case op_logAnd: {
                    enterExpr();
                    Value lv = compileExpressionInner(n->lvalue);
                    Value rv = compileExpressionInner(n->rvalue);
                    exitExpr();

                    auto* exp = currentExpr();
                    if (exp) exp->targetNextCall = true;
                    Value ret = lv && rv;
                    if (exp) exp->targetNextCall = false;
                    return ret;
                }
                case op_logAndEq: {
                    enterExpr();
                    Value lv = compileExpressionInner(n->lvalue);
                    Value rv = compileExpressionInner(n->rvalue);
                    exitExpr();

                    auto* exp = currentExpr();
                    if (exp) exp->targetNextCall = true;
                    Value ret = lv.operator_logicalAndAssign(rv);
                    if (exp) exp->targetNextCall = false;
                    return ret;
                }
                case op_logOr: {
                    enterExpr();
                    Value lv = compileExpressionInner(n->lvalue);
                    Value rv = compileExpressionInner(n->rvalue);
                    exitExpr();

                    auto* exp = currentExpr();
                    if (exp) exp->targetNextCall = true;
                    Value ret = lv || rv;
                    if (exp) exp->targetNextCall = false;
                    return ret;
                }
                case op_logOrEq: {
                    enterExpr();
                    Value lv = compileExpressionInner(n->lvalue);
                    Value rv = compileExpressionInner(n->rvalue);
                    exitExpr();

                    auto* exp = currentExpr();
                    if (exp) exp->targetNextCall = true;
                    Value ret = lv.operator_logicalOrAssign(rv);
                    if (exp) exp->targetNextCall = false;
                    return ret;
                }
                case op_assign: {
                    enterExpr();
                    Value lv = compileExpressionInner(n->lvalue);
                    Value rv = compileExpressionInner(n->rvalue);
                    exitExpr();

                    auto* exp = currentExpr();
                    if (exp) exp->targetNextCall = true;
                    Value ret = lv = rv;
                    if (exp) exp->targetNextCall = false;
                    return ret;
                }
                case op_compare: {
                    enterExpr();
                    Value lv = compileExpressionInner(n->lvalue);
                    Value rv = compileExpressionInner(n->rvalue);
                    exitExpr();

                    auto* exp = currentExpr();
                    if (exp) exp->targetNextCall = true;
                    Value ret = lv == rv;
                    if (exp) exp->targetNextCall = false;
                    return ret;
                }
                case op_lessThan: {
                    enterExpr();
                    Value lv = compileExpressionInner(n->lvalue);
                    Value rv = compileExpressionInner(n->rvalue);
                    exitExpr();

                    auto* exp = currentExpr();
                    if (exp) exp->targetNextCall = true;
                    Value ret = lv < rv;
                    if (exp) exp->targetNextCall = false;
                    return ret;
                }
                case op_lessThanEq: {
                    enterExpr();
                    Value lv = compileExpressionInner(n->lvalue);
                    Value rv = compileExpressionInner(n->rvalue);
                    exitExpr();

                    auto* exp = currentExpr();
                    if (exp) exp->targetNextCall = true;
                    Value ret = lv <= rv;
                    if (exp) exp->targetNextCall = false;
                    return ret;
                }
                case op_greaterThan: {
                    enterExpr();
                    Value lv = compileExpressionInner(n->lvalue);
                    Value rv = compileExpressionInner(n->rvalue);
                    exitExpr();

                    auto* exp = currentExpr();
                    if (exp) exp->targetNextCall = true;
                    Value ret = lv > rv;
                    if (exp) exp->targetNextCall = false;
                    return ret;
                }
                case op_greaterThanEq: {
                    enterExpr();
                    Value lv = compileExpressionInner(n->lvalue);
                    Value rv = compileExpressionInner(n->rvalue);
                    exitExpr();

                    auto* exp = currentExpr();
                    if (exp) exp->targetNextCall = true;
                    Value ret = lv >= rv;
                    if (exp) exp->targetNextCall = false;
                    return ret;
                }
                case op_preInc: {
                    enterExpr();
                    Value lv = compileExpressionInner(n->lvalue);
                    exitExpr();

                    auto* exp = currentExpr();
                    if (exp) exp->targetNextCall = true;
                    Value ret = ++lv;
                    if (exp) exp->targetNextCall = false;
                    return ret;
                }
                case op_postInc: {
                    enterExpr();
                    Value lv = compileExpressionInner(n->lvalue);
                    exitExpr();

                    auto* exp = currentExpr();
                    if (exp) exp->targetNextCall = true;
                    Value ret = lv++;
                    if (exp) exp->targetNextCall = false;
                    return ret;
                }
                case op_preDec: {
                    enterExpr();
                    Value lv = compileExpressionInner(n->lvalue);
                    exitExpr();

                    auto* exp = currentExpr();
                    if (exp) exp->targetNextCall = true;
                    Value ret = --lv;
                    if (exp) exp->targetNextCall = false;
                    return ret;
                }
                case op_postDec: {
                    enterExpr();
                    Value lv = compileExpressionInner(n->lvalue);
                    exitExpr();

                    auto* exp = currentExpr();
                    if (exp) exp->targetNextCall = true;
                    Value ret = lv--;
                    if (exp) exp->targetNextCall = false;
                    return ret;
                }
                case op_negate: {
                    enterExpr();
                    Value lv = compileExpressionInner(n->lvalue);
                    exitExpr();

                    auto* exp = currentExpr();
                    if (exp) exp->targetNextCall = true;
                    Value ret = -lv;
                    if (exp) exp->targetNextCall = false;
                    return ret;
                }
                case op_dereference: {
                    if (!isTrusted()) {
                        error(cm_err_not_trusted, "Pointer dereferencing is only accessible to trusted scripts");
                        return currentFunction()->getPoison();
                    }

                    enterExpr();
                    Value lv = compileExpressionInner(n->lvalue);
                    exitExpr();

                    auto* exp = currentExpr();
                    if (exp) exp->targetNextCall = true;
                    Value ret = *lv;
                    if (exp) exp->targetNextCall = false;
                    return ret;
                }
                case op_index: {
                    enterExpr();
                    Value lv = compileExpressionInner(n->lvalue);
                    Value rv = compileExpressionInner(n->rvalue);
                    exitExpr();

                    auto* exp = currentExpr();
                    if (exp) exp->targetNextCall = true;
                    Value ret = lv[rv];
                    if (exp) exp->targetNextCall = false;
                    return ret;
                }
                case op_conditional: return vs(compileConditionalExpr(n));
                case op_member: return vs(compileMemberExpr(n));
                case op_new: {
                    DataType* type = nullptr;

                    if (n->body->tp == nt_type_specifier) type = resolveTypeSpecifier(n->body);
                    else if (n->body->tp == nt_expression || n->body->tp == nt_identifier) {
                        enterExpr();
                        Value tp = compileExpression(n->body);
                        exitExpr();

                        if (!tp.isType()) {
                            valueError(
                                tp,
                                cm_err_expected_type_specifier,
                                "Expected type specifier after 'new', got '%s'",
                                tp.toString(m_ctx).c_str()
                            );
                            return currentFunction()->getPoison();
                        }

                        type = tp.getType();
                    }

                    Array<Value> args;
                    ParseNode* p = n->parameters;
                    enterExpr();
                    while (p && p->tp != nt_empty) {
                        args.push(compileExpression(p));
                        p = p->next;
                    }
                    exitExpr();

                    auto* exp = currentExpr();
                    if (exp) exp->targetNextConstructor = true;
                    Value out = constructObject(type, args);
                    if (exp) exp->targetNextConstructor = false;

                    return out;
                }
                case op_placementNew: {
                    DataType* type = resolveTypeSpecifier(n->data_type);

                    Array<Value> args;
                    ParseNode* p = n->parameters;

                    enterExpr();
                    while (p) {
                        args.push(compileExpression(p));
                        p = p->next;
                    }

                    Value dest = compileExpressionInner(n->lvalue);
                    constructObject(dest, type, args);
                    exitExpr();

                    return vs(dest);
                }
                case op_call: {
                    member_expr_hints h;
                    h.for_func_call = true;
                    h.self = nullptr;
                    ParseNode* p = n->parameters;
                    if (p->tp == nt_expression_sequence) p = p->body;
                    if (p->tp == nt_empty) p = nullptr;

                    enterExpr();

                    while (p) {
                        h.args.push(compileExpression(p));
                        p = p->next;
                    }

                    Value callee;
                    
                    if (n->lvalue->tp == nt_expression && n->lvalue->op == op_member) {
                        callee.reset(compileMemberExpr(n->lvalue, true, &h));
                    } else callee.reset(compileExpressionInner(n->lvalue));

                    exitExpr();

                    auto* exp = currentExpr();
                    if (exp) exp->targetNextCall = true;

                    if (callee.isFunction() && !callee.isImm() && !h.self) {
                        DataType* selfTp = ((FunctionType*)callee.getType())->getThisType();
                        if (selfTp) {
                            // Get 'self' argument from ClosureRef
                            FunctionDef* cf = currentFunction();
                            Value self = cf->val(selfTp);
                            add(ir_uadd).op(self).op(callee).op(cf->imm<u64>(offsetof(ClosureRef, m_ref)));
                            add(ir_load).op(self).op(self);
                            add(ir_uadd).op(self).op(self).op(cf->imm<u64>(offsetof(Closure, m_self)));
                            add(ir_load).op(self).op(self);
                            h.self = new Value(self);
                        }
                    }

                    Value ret = callee(h.args, h.self);

                    if (exp) exp->targetNextCall = false;
                    if (h.self) delete h.self;
                    return ret;
                }
            }

            return currentFunction()->getPoison();
        }
        Value Compiler::compileExpression(ParseNode* n) {
            enterNode(n);
            scope().enter();

            Value out;
            
            if (n->tp == nt_expression_sequence) {
                ParseNode* s = n->body;
                while (s) {
                    enterNode(s);
                    out.reset(compileExpressionInner(s));
                    exitNode();
                    s = s->next;
                }
            } else out.reset(compileExpressionInner(n));
            
            scope().exit(out);
            exitNode();

            return out;
        }
        void Compiler::compileIfStatement(ParseNode* n) {
            FunctionDef* cf = currentFunction();
            enterNode(n);

            scope().enter();
            Value cond = compileExpression(n->cond);
            Value condBool = cond.convertedTo(m_ctx->getTypes()->getBoolean());
            scope().exit(condBool);

            auto branch = add(ir_branch).op(condBool).label(cf->label());
            
            scope().enter();
            compileAny(n->body);
            scope().exit();

            if (n->else_body) {
                auto jump = add(ir_jump);

                branch.label(cf->label());

                
                scope().enter();
                compileAny(n->else_body);
                scope().exit();

                jump.label(cf->label());
            } else branch.label(cf->label());

            exitNode();
        }
        void Compiler::compileDeleteStatement(ParseNode* n) {
            enterNode(n);
            
            if (!isTrusted()) {
                error(cm_err_not_trusted, "The 'delete' keyword is only accessible to trusted scripts");
                return;
            }

            Value v = compileExpression(n->body);

            Function* dtor = v.getType()->getDestructor();
            if (dtor) generateCall(dtor, {}, &v);

            exitNode();
        }
        void Compiler::compileReturnStatement(ParseNode* n) {
            FunctionDef* cf = currentFunction();
            
            enterNode(n);
            DataType* retTp = cf->getReturnType();

            if (n->body) {
                if (retTp && cf->isReturnTypeExplicit()) {
                    auto* exp = enterExpr();
                    exp->expectedType = retTp;
                    exp->destination = &cf->getRetPtr();
                    exp->loc = rsl_specified_dest;
                    compileExpression(n->body);
                    scope().emitReturnInstructions();
                    add(ir_ret);
                    exitExpr();
                    exitNode();
                    return;
                }
                
                Value ret = compileExpression(n->body);
                cf->setReturnType(ret.getType());

                Value rptr = cf->getRetPtr();
                constructObject(rptr, { ret });

                scope().emitReturnInstructions();
                add(ir_ret);
                exitNode();
                return;
            }

            if (retTp && cf->isReturnTypeExplicit() && retTp->getInfo().size > 0) {
                error(
                    cm_err_function_must_return_a_value,
                    "Function '%s' must return a value of type '%s'",
                    cf->getName().c_str(),
                    retTp->getName().c_str()
                );
                scope().emitReturnInstructions();
                add(ir_ret);
                exitNode();
                return;
            }

            cf->setReturnType(m_ctx->getTypes()->getVoid());
            scope().emitReturnInstructions();
            add(ir_ret);
            exitNode();
        }
        void Compiler::compileSwitchStatement(ParseNode* n) {
            // todo
        }
        void Compiler::compileThrow(ParseNode* n) {
            // todo
        }
        void Compiler::compileTryBlock(ParseNode* n) {
            // todo
        }
        Value& Compiler::compileVarDecl(ParseNode* n, bool isExported) {
            FunctionDef* cf = currentFunction();

            enterNode(n);

            DataType* dt = nullptr;
            if (n->body->data_type) dt = resolveTypeSpecifier(n->body->data_type);
            else {
                if (!n->initializer) {
                    error(
                        cm_err_could_not_deduce_type,
                        "Uninitilized variable declarations must have an explicit type specifier"
                    );
                    exitNode();
                    return cf->getPoison();
                }
            }

            if (n->initializer) {
                auto* ctx = enterExpr();
                ctx->expectedType = dt;
                ctx->loc = inInitFunction() ? rsl_module_data : rsl_auto;
                ctx->md_name = n->body->str();
                ctx->md_access = isExported ? public_access : private_access;

                Value init = compileExpression(n->initializer);

                exitExpr();
                exitNode();

                if (init.isImm() && init.isFunction()) {
                    init.reset(newClosureRef(init));
                }

                return cf->promote(init, n->body->str());
            }
            
            Value* v = nullptr;
            if (inInitFunction()) {
                u32 slot = m_output->getModule()->addData(n->body->str(), dt, private_access);
                if (isExported) {
                    m_output->getModule()->setDataAccess(slot, public_access);
                }
                v = &cf->val(n->body->str(), slot);
            } else {
                v = &cf->val(n->body->str(), dt);
            }

            if (!dt->getInfo().is_primitive) constructObject(*v, {});

            exitNode();
            return *v;
        }
        void Compiler::compileObjectDecompositor(ParseNode* n) {
            enterNode(n);
            FunctionDef* cf = currentFunction();

            Value source = compileExpression(n->initializer);
            
            ParseNode* p = n->body;
            while (p) {
                enterNode(p);
                Value init = source.getProp(p->str());

                DataType* dt = p->data_type ? resolveTypeSpecifier(p->data_type) : init.getType();
                Value* v = nullptr;

                if (inInitFunction()) {
                    u32 slot = m_output->getModule()->addData(p->str(), dt, private_access);
                    v = &currentFunction()->val(p->str(), slot);
                } else {
                    v = &currentFunction()->val(p->str(), dt);
                }

                if (dt->getInfo().is_primitive) *v = init.convertedTo(dt);
                else constructObject(*v, { init });

                exitNode();
                p = p->next;
            }

            exitNode();
        }
        void Compiler::compileLoop(ParseNode* n) {
            FunctionDef* cf = currentFunction();
            enterNode(n);
            Scope& s = scope().enter();

            // initializer will either be expression or variable decl
            if (n->initializer) compileAny(n->initializer);

            label_id loop_begin = cf->label();
            m_lsStack.push({
                true,       // is_loop
                loop_begin, // loop_begin
                {},         // pending_end_label
                &s          // outer_scope
            });

            if (n->flags.defer_cond == 0) {
                scope().enter();
                Value cond = compileExpression(n->cond);
                Value condBool = cond.convertedTo(m_ctx->getTypes()->getBoolean());
                scope().exit(condBool);

                m_lsStack.last().pending_end_label.push(add(ir_branch).op(condBool).label(cf->label()));

                scope().enter();
                compileAny(n->body);
                scope().exit();

                if (n->modifier) {
                    scope().enter();
                    compileExpression(n->modifier);
                    scope().exit();
                }

                add(ir_jump).label(loop_begin);
            } else {
                if (n->modifier) {
                    scope().enter();
                    compileExpression(n->modifier);
                    scope().exit();
                }

                scope().enter();
                compileAny(n->body);
                scope().exit();

                scope().enter();
                Value cond = compileExpression(n->cond);
                Value condBool = cond.convertedTo(m_ctx->getTypes()->getBoolean());
                scope().exit(condBool);

                m_lsStack.last().pending_end_label.push(add(ir_branch).op(condBool).label(loop_begin));
            }

            label_id loop_end = cf->label();
            m_lsStack.last().pending_end_label.each([loop_end](InstructionRef& i) {
                i.label(loop_end);
            });
            m_lsStack.pop();

            scope().exit();
            exitNode();
        }
        void Compiler::compileContinue(ParseNode* n) {
            if (m_lsStack.size() == 0) {
                error(n, cm_err_continue_scope, "'continue' used outside of loop scope");
                return;
            }

            auto& ls = m_lsStack.last();
            if (!ls.is_loop) {
                error(n, cm_err_continue_scope, "'continue' used outside of loop scope");
                return;
            }

            scope().emitScopeExitInstructions(*ls.outer_scope);
            add(ir_jump).label(ls.loop_begin);
        }
        void Compiler::compileBreak(ParseNode* n) {
            if (m_lsStack.size() == 0) {
                error(n, cm_err_continue_scope, "'break' used outside of loop or switch scope");
                return;
            }
            auto& ls = m_lsStack.last();

            scope().emitScopeExitInstructions(*ls.outer_scope);
            ls.pending_end_label.push(add(ir_jump));
        }
        void Compiler::compileBlock(ParseNode* n) {
            scope().enter();

            n = n->body;
            while (n) {
                compileAny(n);
                n = n->next;
            }

            scope().exit();
        }
        void Compiler::compileAny(ParseNode* n) {
            switch (n->tp) {
                case nt_break: return compileBreak(n);
                case nt_class: {
                    if (!inInitFunction()) {
                        error(n, cm_err_class_scope, "'class' used outside of root scope");
                        return;
                    }

                    compileClass(n);
                    return;
                }
                case nt_continue: return compileContinue(n);
                case nt_export: return compileExport(n);
                case nt_expression: return (void)compileExpression(n);
                case nt_expression_sequence: return (void)compileExpression(n);
                case nt_function: {
                    if (!inInitFunction()) {
                        error(n, cm_err_function_scope, "'function' used outside of root scope");
                        return;
                    }

                    compileFunction(n);
                    return;
                }
                case nt_if: return compileIfStatement(n);
                case nt_loop: return compileLoop(n);
                case nt_return: return compileReturnStatement(n);
                case nt_scoped_block: return compileBlock(n);
                case nt_switch: return compileSwitchStatement(n);
                case nt_throw: return compileThrow(n);
                case nt_try: return compileTryBlock(n);
                case nt_type: {
                    if (!inInitFunction()) {
                        error(n, cm_err_type_scope, "'type' used outside of root scope");
                        return;
                    }

                    compileType(n);
                    return;
                }
                case nt_variable: {
                    if (n->body->tp == nt_object_decompositor) {
                        compileObjectDecompositor(n);
                        return;
                    }

                    compileVarDecl(n);
                    return;
                }
                default: break;
            }
        }
    };
};