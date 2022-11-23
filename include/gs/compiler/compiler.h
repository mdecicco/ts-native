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
                FunctionDef* getFunc(const utils::String& name, ffi::DataType* methodOf = nullptr);
                const utils::Array<FunctionDef*>& getFuncs() const;

                Module* getModule();

            protected:
                Compiler* m_comp;
                Module* m_mod;
                utils::Array<FunctionDef*> m_funcs;
        };

        struct loop_or_switch_context {
            bool is_loop;
            label_id loop_begin;
            utils::Array<InstructionRef> pending_end_label;
            Scope* outer_scope;
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
                void addDataType(ffi::DataType* tp);
                void addFunction(ffi::Function* fn);

                CompilerOutput* getOutput();
                CompilerOutput* compile();
                
                const utils::Array<ffi::DataType*>& getAddedDataTypes() const;
                const utils::Array<ffi::Function*>& getAddedFunctions() const;

                void updateMethod(ffi::DataType* classTp, ffi::Method* m);

            protected:
                friend class Value;
                friend class ScopeManager;

                InstructionRef add(ir_instruction inst);
                void constructObject(const Value& dest, const utils::Array<Value>& args);
                void constructObject(const Value& dest, ffi::DataType* tp, const utils::Array<Value>& args);
                Value constructObject(ffi::DataType* tp, const utils::Array<Value>& args);

                ffi::DataType* getArrayType(ffi::DataType* elemTp);
                ffi::DataType* getPointerType(ffi::DataType* destTp);
                ffi::DataType* resolveTemplateTypeSubstitution(ast_node* templateArgs, ffi::DataType* _type);
                ffi::DataType* resolveObjectTypeSpecifier(ast_node* n);
                ffi::DataType* resolveFunctionTypeSpecifier(ast_node* n);
                ffi::DataType* resolveTypeNameSpecifier(ast_node* n);
                ffi::DataType* applyTypeModifiers(ffi::DataType* tp, ast_node* mod);
                ffi::DataType* resolveTypeSpecifier(ast_node* n);

                Value generateCall(ffi::Function* fn, const utils::Array<Value>& args, const Value* self = nullptr);
                Value generateCall(FunctionDef* fn, const utils::Array<Value>& args, const Value* self = nullptr);

                ffi::Method* compileMethodDecl(ast_node* n, u64 thisOffset, bool* wasDtor, bool templatesDefined = false, bool dtorExists = false);
                void compileMethodDef(ast_node* n, ffi::DataType* methodOf, ffi::Method* m);
                ffi::DataType* compileType(ast_node* n);
                ffi::DataType* compileClass(ast_node* n, bool templatesDefined = false);
                ffi::Function* compileFunction(ast_node* n, bool templatesDefined = false);
                void compileExport(ast_node* n);
                void compileImport(ast_node* n);
                Value compileConditionalExpr(ast_node* n);
                Value compileMemberExpr(ast_node* n);
                Value compileArrowFunction(ast_node* n);
                Value compileExpressionInner(ast_node* n);
                Value compileExpression(ast_node* n);
                void compileIfStatement(ast_node* n);
                void compileReturnStatement(ast_node* n);
                void compileSwitchStatement(ast_node* n);
                void compileTryBlock(ast_node* n);
                void compileThrow(ast_node* n);
                Value& compileVarDecl(ast_node* n);
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
                utils::Array<ffi::DataType*> m_addedTypes;
                utils::Array<ffi::Function*> m_addedFuncs;
                ffi::DataType* m_curClass;
                FunctionDef* m_curFunc;
                CompilerOutput* m_output;
                ScopeManager m_scopeMgr;

                utils::Array<loop_or_switch_context> m_lsStack;
        };
    };
};