#pragma once
#include <tsn/common/types.h>
#include <tsn/interfaces/IContextual.h>
#include <tsn/interfaces/IWithLogger.h>
#include <tsn/utils/SourceLocation.h>
#include <tsn/compiler/Scope.h>
#include <tsn/compiler/IR.h>

#include <utils/String.h>

namespace tsn {
    class Module;
    struct script_metadata;

    namespace ffi {
        class DataType;
        class Function;
        class Method;
        struct function_argument;
    };

    namespace compiler {
        class TemplateContext;
        class ParseNode;
        class FunctionDef;
        class OutputBuilder;
        class Value;

        struct loop_or_switch_context {
            bool is_loop;
            label_id loop_begin;
            utils::Array<InstructionRef> pending_end_label;
            Scope* outer_scope;
        };

        struct member_expr_hints {
            bool for_func_call;
            utils::Array<Value> args;
            Value* self;
        };

        class Compiler : public IContextual, IWithLogger {
            public:
                Compiler(Context* ctx, Logger* log, ParseNode* programTree, const script_metadata* meta);
                ~Compiler();

                void enterNode(ParseNode* n);
                void exitNode();

                void enterFunction(FunctionDef* fn);
                ffi::Function* exitFunction();
                FunctionDef* currentFunction() const;
                ffi::DataType* currentClass() const;
                ParseNode* currentNode();
                bool inInitFunction() const;

                const SourceLocation& getCurrentSrc() const;
                ScopeManager& scope();

                const script_metadata* getScriptInfo() const;
                OutputBuilder* getOutput();
                OutputBuilder* compile();

                void updateMethod(ffi::DataType* classTp, ffi::Method* m);

                log_message& typeError(ffi::DataType* tp, log_message_code code, const char* msg, ...);
                log_message& typeError(ParseNode* node, ffi::DataType* tp, log_message_code code, const char* msg, ...);
                log_message& valueError(const Value& val, log_message_code code, const char* msg, ...);
                log_message& valueError(ParseNode* node, const Value& val, log_message_code code, const char* msg, ...);
                log_message& functionError(ffi::DataType* selfTp, ffi::DataType* retTp, const utils::Array<Value>& args, log_message_code code, const char* msg, ...);
                log_message& functionError(ParseNode* node, ffi::DataType* selfTp, ffi::DataType* retTp, const utils::Array<Value>& args, log_message_code code, const char* msg, ...);
                log_message& functionError(ffi::DataType* selfTp, ffi::DataType* retTp, const utils::Array<const ffi::DataType*>& argTps, log_message_code code, const char* msg, ...);
                log_message& functionError(ParseNode* node, ffi::DataType* selfTp, ffi::DataType* retTp, const utils::Array<const ffi::DataType*>& argTps, log_message_code code, const char* msg, ...);
                log_message& error(log_message_code code, const char* msg, ...);
                log_message& error(ParseNode* node, log_message_code code, const char* msg, ...);
                log_message& warn(log_message_code code, const char* msg, ...);
                log_message& warn(ParseNode* node, log_message_code code, const char* msg, ...);
                log_message& info(log_message_code code, const char* msg, ...);
                log_message& info(ParseNode* node, log_message_code code, const char* msg, ...);

            protected:
                friend class Value;
                friend class ScopeManager;

                InstructionRef add(ir_instruction inst);
                void constructObject(const Value& dest, ffi::DataType* tp, const utils::Array<Value>& args);
                void constructObject(const Value& dest, const utils::Array<Value>& args);
                Value constructObject(ffi::DataType* tp, const utils::Array<Value>& args);
                Value functionValue(FunctionDef* fn);
                Value functionValue(ffi::Function* fn);
                Value typeValue(ffi::DataType* tp);
                Value moduleValue(Module* mod);
                Value moduleData(Module* m, u32 slot);
                Value generateCall(const Value& fn, const utils::String& name, ffi::DataType* retTp, const utils::Array<ffi::function_argument>& fargs, const utils::Array<Value>& params, const Value* self = nullptr);
                Value generateCall(ffi::Function* fn, const utils::Array<Value>& args, const Value* self = nullptr);
                Value generateCall(FunctionDef* fn, const utils::Array<Value>& args, const Value* self = nullptr);
                Value generateCall(const Value& fn, const utils::Array<Value>& args, const Value* self = nullptr);


                ffi::DataType* getArrayType(ffi::DataType* elemTp);
                ffi::DataType* getPointerType(ffi::DataType* destTp);
                ffi::DataType* resolveTemplateTypeSubstitution(ParseNode* templateArgs, ffi::DataType* _type);
                ffi::DataType* resolveObjectTypeSpecifier(ParseNode* n);
                ffi::DataType* resolveFunctionTypeSpecifier(ParseNode* n);
                ffi::DataType* resolveTypeNameSpecifier(ParseNode* n);
                ffi::DataType* applyTypeModifiers(ffi::DataType* tp, ParseNode* mod);
                ffi::DataType* resolveTypeSpecifier(ParseNode* n);
                void importTemplateContext(TemplateContext* tctx);
                void buildTemplateContext(ParseNode* n, TemplateContext* tctx, robin_hood::unordered_set<utils::String>& added);
                ffi::Method* compileMethodDecl(ParseNode* n, u64 thisOffset, bool* wasDtor, bool templatesDefined = false, bool dtorExists = false);
                void compileMethodDef(ParseNode* n, ffi::DataType* methodOf, ffi::Method* m);
                ffi::DataType* compileType(ParseNode* n);
                ffi::DataType* compileClass(ParseNode* n, bool templatesDefined = false);
                ffi::Function* compileFunction(ParseNode* n, bool templatesDefined = false);
                void compileExport(ParseNode* n);
                void compileImport(ParseNode* n);
                Value compileConditionalExpr(ParseNode* n);
                Value compileMemberExpr(ParseNode* n, bool topLevel = true, member_expr_hints* hints = nullptr);
                Value compileArrowFunction(ParseNode* n);
                Value compileExpressionInner(ParseNode* n);
                Value compileExpression(ParseNode* n);
                void compileIfStatement(ParseNode* n);
                void compileReturnStatement(ParseNode* n);
                void compileSwitchStatement(ParseNode* n);
                void compileTryBlock(ParseNode* n);
                void compileThrow(ParseNode* n);
                Value& compileVarDecl(ParseNode* n, u32* moduleSlot = nullptr);
                void compileObjectDecompositor(ParseNode* n);
                void compileLoop(ParseNode* n);
                void compileContinue(ParseNode* n);
                void compileBreak(ParseNode* n);
                void compileBlock(ParseNode* n);
                void compileAny(ParseNode* n);

            private:
                const script_metadata* m_meta;
                ParseNode* m_program;
                utils::Array<ParseNode*> m_nodeStack;
                utils::Array<FunctionDef*> m_funcStack;
                ffi::DataType* m_curClass;
                FunctionDef* m_curFunc;
                OutputBuilder* m_output;
                ScopeManager m_scopeMgr;

                utils::Array<loop_or_switch_context> m_lsStack;
        };

        utils::String argListStr(const utils::Array<Value>& args);
        utils::String argListStr(const utils::Array<const ffi::DataType*>& args);
    };
};