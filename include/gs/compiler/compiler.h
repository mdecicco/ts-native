#pragma once
#include <gs/common/types.h>
#include <gs/interfaces/IContextual.h>
#include <gs/utils/SourceLocation.h>
#include <gs/compiler/Scope.h>
#include <gs/compiler/IR.h>

#include <utils/String.h>

namespace gs {
    class Module;

    namespace ffi {
        class DataType;
        class Function;
        class Method;
    };

    namespace compiler {
        struct ast_node;
        class FunctionDef;
        class Compiler;
        class Value;

        class CompilerOutput {
            public:
                CompilerOutput(Compiler* c, Module* m);
                ~CompilerOutput();

                FunctionDef* newFunc(const utils::String& name, ffi::DataType* methodOf = nullptr);
                FunctionDef* newFunc(ffi::Function* preCreated);
                void resolveFunctionDef(FunctionDef* def, ffi::Function* fn);
                FunctionDef* getFunctionDef(ffi::Function* fn);
                void add(ffi::DataType* tp);

                FunctionDef* import(ffi::Function* fn, const utils::String& as);
                void import(ffi::DataType* fn, const utils::String& as);
                const utils::Array<FunctionDef*>& getFuncs() const;
                const utils::Array<ffi::DataType*>& getTypes() const;

                Module* getModule();

            protected:
                Compiler* m_comp;
                Module* m_mod;
                utils::Array<FunctionDef*> m_funcs;
                utils::Array<ffi::DataType*> m_types;
                utils::Array<FunctionDef*> m_importedFuncs;
                robin_hood::unordered_map<ffi::Function*, u32> m_funcDefs;
                utils::Array<FunctionDef*> m_allFuncDefs;
        };

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
            ast_node* node;
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
                Compiler(Context* ctx, ast_node* programTree);
                ~Compiler();

                void enterNode(ast_node* n);
                void exitNode();

                void enterFunction(FunctionDef* fn);
                ffi::Function* exitFunction();
                FunctionDef* currentFunction() const;
                ffi::DataType* currentClass() const;
                bool inInitFunction() const;

                const SourceLocation& getCurrentSrc() const;
                ScopeManager& scope();

                CompilerOutput* getOutput();
                CompilerOutput* compile();

                void updateMethod(ffi::DataType* classTp, ffi::Method* m);

                void typeError(ffi::DataType* tp, compilation_message_code code, const char* msg, ...);
                void typeError(ast_node* node, ffi::DataType* tp, compilation_message_code code, const char* msg, ...);
                void valueError(const Value& val, compilation_message_code code, const char* msg, ...);
                void valueError(ast_node* node, const Value& val, compilation_message_code code, const char* msg, ...);
                void functionError(ffi::DataType* selfTp, ffi::DataType* retTp, const utils::Array<Value>& args, compilation_message_code code, const char* msg, ...);
                void functionError(ast_node* node, ffi::DataType* selfTp, ffi::DataType* retTp, const utils::Array<Value>& args, compilation_message_code code, const char* msg, ...);
                void functionError(ffi::DataType* selfTp, ffi::DataType* retTp, const utils::Array<const ffi::DataType*>& argTps, compilation_message_code code, const char* msg, ...);
                void functionError(ast_node* node, ffi::DataType* selfTp, ffi::DataType* retTp, const utils::Array<const ffi::DataType*>& argTps, compilation_message_code code, const char* msg, ...);
                void error(compilation_message_code code, const char* msg, ...);
                void error(ast_node* node, compilation_message_code code, const char* msg, ...);
                void warn(compilation_message_code code, const char* msg, ...);
                void warn(ast_node* node, compilation_message_code code, const char* msg, ...);
                void info(compilation_message_code code, const char* msg, ...);
                void info(ast_node* node, compilation_message_code code, const char* msg, ...);
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
                Value generateCall(ffi::Function* fn, const utils::Array<Value>& args, const Value* self = nullptr);
                Value generateCall(FunctionDef* fn, const utils::Array<Value>& args, const Value* self = nullptr);


                ffi::DataType* getArrayType(ffi::DataType* elemTp);
                ffi::DataType* getPointerType(ffi::DataType* destTp);
                ffi::DataType* resolveTemplateTypeSubstitution(ast_node* templateArgs, ffi::DataType* _type);
                ffi::DataType* resolveObjectTypeSpecifier(ast_node* n);
                ffi::DataType* resolveFunctionTypeSpecifier(ast_node* n);
                ffi::DataType* resolveTypeNameSpecifier(ast_node* n);
                ffi::DataType* applyTypeModifiers(ffi::DataType* tp, ast_node* mod);
                ffi::DataType* resolveTypeSpecifier(ast_node* n);
                ffi::Method* compileMethodDecl(ast_node* n, u64 thisOffset, bool* wasDtor, bool templatesDefined = false, bool dtorExists = false);
                void compileMethodDef(ast_node* n, ffi::DataType* methodOf, ffi::Method* m);
                ffi::DataType* compileType(ast_node* n);
                ffi::DataType* compileClass(ast_node* n, bool templatesDefined = false);
                ffi::Function* compileFunction(ast_node* n, bool templatesDefined = false);
                void compileExport(ast_node* n);
                void compileImport(ast_node* n);
                Value compileConditionalExpr(ast_node* n);
                Value compileMemberExpr(ast_node* n, bool topLevel = true, member_expr_hints* hints = nullptr);
                Value compileArrowFunction(ast_node* n);
                Value compileExpressionInner(ast_node* n);
                Value compileExpression(ast_node* n);
                void compileIfStatement(ast_node* n);
                void compileReturnStatement(ast_node* n);
                void compileSwitchStatement(ast_node* n);
                void compileTryBlock(ast_node* n);
                void compileThrow(ast_node* n);
                Value& compileVarDecl(ast_node* n, u32* moduleSlot = nullptr);
                void compileObjectDecompositor(ast_node* n);
                void compileLoop(ast_node* n);
                void compileContinue(ast_node* n);
                void compileBreak(ast_node* n);
                void compileBlock(ast_node* n);
                void compileAny(ast_node* n);

            private:
                ast_node* m_program;
                utils::Array<ast_node*> m_nodeStack;
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