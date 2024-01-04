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
    class Pipeline;
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

        enum result_storage_location {
            /** Allow code generation to decide on its own how and where to store the result */
            rsl_auto,

            /** Store the result on the stack */
            rsl_stack,

            /** Store the result in the module data */
            rsl_module_data,

            /** Store the result in the current function's capture data */
            rsl_capture_data_if_non_primitive,

            /** Store the result in the provided destination */
            rsl_specified_dest
        };

        struct expression_context {
            /** Where the expression result should be stored */
            result_storage_location loc;

            /** If expression result is the initializer for a variable, this is the name of the variable */
            utils::String var_name;

            /** If expression result is the initializer for a variable, this is the scope the variable should be declared in */
            Scope* decl_scope;

            /** If loc == rsl_module_data, this is this is the access modifier of the module data entry */
            access_modifier md_access;

            /** The type of value the destination expects to receive */
            ffi::DataType* expectedType;

            /**
             * @brief If this is set and loc == rsl_specified_dest, the
             * expression result will be stored here. If this is set, the
             * value being represented should always be a pointer to allocated
             * but uninitialized memory. The responsibility for construction
             * is held by the expression evaluator, not the code that calls for
             * the expression to be evaluated (nor any of the code that preceeds
             * it).
             * 
             * If this field receives a value which has already been constructed
             * then the constructed object will be overwritten without first being
             * deconstructed.
             */
            Value* destination;

            /** Whether or not the next call instruction should respect this. If a call occurred, this will be reset to false. */
            bool targetNextCall;

            /** Whether or not the next constructor should respect this. If a constructor occurred, this will be reset to false. */
            bool targetNextConstructor;
        };

        class Compiler : public IContextual, IWithLogger {
            public:
                Compiler(Context* ctx, Logger* log, ParseNode* programTree, const script_metadata* meta);
                ~Compiler();

                void enterNode(ParseNode* n);
                void exitNode();

                void enterClass(ffi::DataType* n);
                ffi::DataType* currentClass();
                void exitClass();

                expression_context* enterExpr();
                expression_context* currentExpr();
                void exitExpr();

                void enterFunction(FunctionDef* fn);
                ffi::Function* exitFunction();
                FunctionDef* currentFunction() const;
                ffi::DataType* currentClass() const;
                ParseNode* currentNode();
                bool inInitFunction() const;
                bool isTrusted() const;
                utils::String generateFullQualifierPrefix() const;

                TemplateContext* createTemplateContext(ParseNode* n);
                void pushTemplateContext(TemplateContext* tctx);
                TemplateContext* getTemplateContext() const;
                void popTemplateContext();

                const SourceLocation& getCurrentSrc() const;
                ScopeManager& scope();

                const script_metadata* getScriptInfo() const;
                OutputBuilder* getOutput();
                void begin();
                void end();
                OutputBuilder* compileAll();

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
                friend class FunctionDef;
                friend class Pipeline;

                InstructionRef add(ir_instruction inst);
                Value getStorageForExpr(ffi::DataType* tp);
                Value copyValueToExprStorage(const Value& result);
                void constructObject(const Value& dest, ffi::DataType* tp, const utils::Array<Value>& args);
                void constructObject(const Value& dest, const utils::Array<Value>& args);
                Value constructObject(ffi::DataType* tp, const utils::Array<Value>& args);
                Value functionValue(FunctionDef* fn);
                Value functionValue(ffi::Function* fn);
                Value typeValue(ffi::DataType* tp);
                Value moduleValue(Module* mod);
                Value moduleData(Module* m, u32 slot);
                Value newCaptureData(function_id target, Value* captureData = nullptr);
                void findCaptures(ParseNode* node, utils::Array<Value>& outCaptures, utils::Array<u32>& outCaptureOffsets, u32 scopeIdx);
                bool isVarCaptured(ParseNode* n, const utils::String& name, u32 closureDepth = 0);
                Value newClosure(const Value& closure, ffi::DataType* signature);
                Value newClosure(const Value& fnImm);
                Value newClosure(ffi::Function* fn);
                Value newClosure(FunctionDef* fn);
                Value generateCall(const Value& fn, const utils::String& name, bool returnsPointer, ffi::DataType* retTp, const utils::Array<ffi::function_argument>& fargs, const utils::Array<Value>& params, const Value* self = nullptr);
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
                void compileDefaultConstructor(ffi::DataType* forTp, u64 thisOffset);
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
                Value compileObjectLiteral(ParseNode* n);
                Value compileArrayLiteral(ParseNode* n);
                Value compileExpressionInner(ParseNode* n);
                Value compileExpression(ParseNode* n);
                void compileIfStatement(ParseNode* n);
                void compileDeleteStatement(ParseNode* n);
                void compileReturnStatement(ParseNode* n);
                void compileSwitchStatement(ParseNode* n);
                void compileTryBlock(ParseNode* n);
                void compileThrow(ParseNode* n);
                Value& compileVarDecl(ParseNode* n, bool isExported = false);
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
                utils::Array<TemplateContext*> m_templateStack;
                utils::Array<expression_context> m_exprCtx;
                utils::Array<ffi::DataType*> m_classStack;
                FunctionDef* m_curFunc;
                OutputBuilder* m_output;
                ScopeManager m_scopeMgr;
                bool m_trustEnable;

                utils::Array<loop_or_switch_context> m_lsStack;
        };

        utils::String argListStr(const utils::Array<Value>& args);
        utils::String argListStr(const utils::Array<const ffi::DataType*>& args);
    };
};