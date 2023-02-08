#pragma once
#include <tsn/common/types.h>
#include <tsn/interfaces/IContextual.h>
#include <tsn/utils/SourceLocation.h>
#include <tsn/compiler/Scope.h>
#include <tsn/compiler/IR.h>

#include <utils/String.h>

namespace tsn {
    class Module;

    namespace ffi {
        class DataType;
        class Function;
        class Method;
        struct function_argument;
    };

    namespace compiler {
        class ParseNode;
        class FunctionDef;
        class CompilerOutput;
        class Value;
        
        enum compilation_message_code {
            cm_info_could_be,
            cm_info_arg_conversion,

            cm_err_ambiguous_constructor,
            cm_err_no_matching_constructor,
            cm_err_private_constructor,
            cm_err_identifier_does_not_refer_to_type,
            cm_err_identifier_not_found,
            cm_err_identifier_ambiguous,
            cm_err_template_type_expected,
            cm_err_too_few_template_args,
            cm_err_too_many_template_args,
            cm_err_symbol_already_exists,
            cm_err_dtor_already_exists,
            cm_err_expected_method_argument_type,
            cm_err_expected_function_argument_type,
            cm_err_export_not_in_root_scope,
            cm_err_export_invalid,
            cm_err_import_not_in_root_scope,
            cm_err_import_module_not_found,
            cm_err_property_not_found,
            cm_err_property_is_private,
            cm_err_property_not_static,
            cm_err_property_is_static,
            cm_err_property_no_read_access,
            cm_err_property_or_method_ambiguous,
            cm_err_value_not_callable_with_args,
            cm_err_method_not_found,
            cm_err_method_ambiguous,
            cm_err_method_is_static,
            cm_err_method_not_static,
            cm_err_export_ambiguous,
            cm_err_export_not_found,
            cm_err_malformed_member_expression,
            cm_err_this_outside_class,
            cm_err_function_must_return_a_value,
            cm_err_function_ambiguous,
            cm_err_function_property_access,
            cm_err_function_argument_count_mismatch,
            cm_err_could_not_deduce_type,
            cm_err_continue_scope,
            cm_err_break_scope,
            cm_err_class_scope,
            cm_err_function_scope,
            cm_err_type_scope,
            cm_err_type_not_convertible,
            cm_err_value_not_writable,
            cm_err_type_used_as_value,
            cm_err_module_used_as_value,
            cm_err_module_data_used_as_value,
            cm_err_internal
        };

        enum compilation_message_type {
            cmt_info,
            cmt_warn,
            cmt_error
        };

        struct compilation_message {
            compilation_message_type type;
            compilation_message_code code;
            utils::String msg;
            SourceLocation src;
            ParseNode* node;
        };

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

        class Compiler : public IContextual {
            public:
                Compiler(Context* ctx, ParseNode* programTree);
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

                CompilerOutput* getOutput();
                CompilerOutput* compile();

                void updateMethod(ffi::DataType* classTp, ffi::Method* m);

                compilation_message& typeError(ffi::DataType* tp, compilation_message_code code, const char* msg, ...);
                compilation_message& typeError(ParseNode* node, ffi::DataType* tp, compilation_message_code code, const char* msg, ...);
                compilation_message& valueError(const Value& val, compilation_message_code code, const char* msg, ...);
                compilation_message& valueError(ParseNode* node, const Value& val, compilation_message_code code, const char* msg, ...);
                compilation_message& functionError(ffi::DataType* selfTp, ffi::DataType* retTp, const utils::Array<Value>& args, compilation_message_code code, const char* msg, ...);
                compilation_message& functionError(ParseNode* node, ffi::DataType* selfTp, ffi::DataType* retTp, const utils::Array<Value>& args, compilation_message_code code, const char* msg, ...);
                compilation_message& functionError(ffi::DataType* selfTp, ffi::DataType* retTp, const utils::Array<const ffi::DataType*>& argTps, compilation_message_code code, const char* msg, ...);
                compilation_message& functionError(ParseNode* node, ffi::DataType* selfTp, ffi::DataType* retTp, const utils::Array<const ffi::DataType*>& argTps, compilation_message_code code, const char* msg, ...);
                compilation_message& error(compilation_message_code code, const char* msg, ...);
                compilation_message& error(ParseNode* node, compilation_message_code code, const char* msg, ...);
                compilation_message& warn(compilation_message_code code, const char* msg, ...);
                compilation_message& warn(ParseNode* node, compilation_message_code code, const char* msg, ...);
                compilation_message& info(compilation_message_code code, const char* msg, ...);
                compilation_message& info(ParseNode* node, compilation_message_code code, const char* msg, ...);
                const utils::Array<compilation_message>& getLogs() const;

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
                ParseNode* m_program;
                utils::Array<ParseNode*> m_nodeStack;
                utils::Array<FunctionDef*> m_funcStack;
                utils::Array<compilation_message> m_messages;
                ffi::DataType* m_curClass;
                FunctionDef* m_curFunc;
                CompilerOutput* m_output;
                ScopeManager m_scopeMgr;

                utils::Array<loop_or_switch_context> m_lsStack;
        };

        utils::String argListStr(const utils::Array<Value>& args);
        utils::String argListStr(const utils::Array<const ffi::DataType*>& args);
    };
};