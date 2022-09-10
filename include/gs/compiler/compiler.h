#pragma once
#include <gs/common/types.h>
#include <gs/interfaces/IContextual.h>
#include <gs/utils/SourceLocation.h>
#include <gs/compiler/Scope.h>

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

        class Compiler : public IContextual {
            public:
                Compiler(Context* ctx, ast_node* programTree);
                ~Compiler();

                void enterNode(ast_node* n);
                void exitNode();

                void enterFunction(FunctionDef* fn);
                void exitFunction();
                FunctionDef* currentFunction() const;
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

            private:
                ast_node* m_program;
                utils::Array<ast_node*> m_nodeStack;
                utils::Array<FunctionDef*> m_funcStack;
                utils::Array<ffi::DataType*> m_addedTypes;
                utils::Array<ffi::Function*> m_addedFuncs;
                CompilerOutput* m_output;
                ScopeManager m_scopeMgr;
        };
    
        ffi::DataType* resolveTypeSpecifier(ast_node* n, Compiler* c);
        ffi::DataType* getArrayType(ffi::DataType* elemTp, Compiler* c);
        ffi::DataType* getPointerType(ffi::DataType* destTp, Compiler* c);
        ffi::DataType* resolveTemplateTypeSubstitution(ast_node* templateArgs, ffi::DataType* _type, Compiler* c);
        ffi::DataType* resolveObjectTypeSpecifier(ast_node* n, Compiler* c);
        ffi::DataType* resolveFunctionTypeSpecifier(ast_node* n, Compiler* c);
        ffi::DataType* resolveTypeNameSpecifier(ast_node* n, Compiler* c);
        ffi::DataType* applyTypeModifiers(ffi::DataType* tp, ast_node* mod, Compiler* c);
        ffi::DataType* resolveTypeSpecifier(ast_node* n, Compiler* c);

        Value generateCall(Compiler* c, ffi::Function* fn, const utils::Array<Value>& args, const Value* self = nullptr);
        Value generateCall(Compiler* c, FunctionDef* fn, const utils::Array<Value>& args, const Value* self = nullptr);

        ffi::Method* compileMethodDecl(ast_node* n, Compiler* c, u64 thisOffset, bool* wasDtor, bool templatesDefined = false, bool dtorExists = false);
        void compileMethodDef(ast_node* n, Compiler* c, ffi::DataType* methodOf, ffi::Method* m);
        ffi::DataType* compileType(ast_node* n, Compiler* c);
        ffi::DataType* compileClass(ast_node* n, Compiler* c, bool templatesDefined = false);
        void compileFunction(ast_node* n, Compiler* c, bool templatesDefined = false);
        void compileExport(ast_node* n, Compiler* c);
        void compileImport(ast_node* n, Compiler* c);
        void compileExpression(ast_node* n, Compiler* c);
        void compileIfStatement(ast_node* n, Compiler* c);
        void compileReturnStatement(ast_node* n, Compiler* c);
        void compileSwitchStatement(ast_node* n, Compiler* c);
        void compileTryBlock(ast_node* n, Compiler* c);
        void compileVarDecl(ast_node* n, Compiler* c);
        void compileLoop(ast_node* n, Compiler* c);
        void compileBlock(ast_node* n, Compiler* c);
        void compileAny(ast_node* n, Compiler* c);
    };
};