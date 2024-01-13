#pragma once
#include <tsn/common/types.h>
#include <tsn/interfaces/IContextual.h>

#include <utils/String.h>
#include <utils/Array.h>

namespace tsn {
    namespace compiler {
        class Logger;
        class Lexer;
        class Parser;
        class Compiler;
        class ParseNode;
        class Output;
    };

    namespace optimize {
        class IOptimizationStep;
    };

    namespace ffi {
        class DataType;
        class TemplateType;
    }
    
    struct script_metadata;
    class ModuleSource;
    class Module;

    class Pipeline : public IContextual {
        public:
            Pipeline(Context* ctx, Pipeline* parent, Pipeline* root);
            ~Pipeline();

            void reset();
            void addOptimizationStep(optimize::IOptimizationStep* step, bool isRequired = false);

            Module* buildFromSource(script_metadata* script);
            Module* buildFromCached(script_metadata* script);
            ffi::DataType* specializeTemplate(ffi::TemplateType* type, const utils::Array<ffi::DataType*> templateArgs);
            ffi::DataType* tryInstantiateTemplateFromCache(script_metadata* script);

            Pipeline* getParent() const;
            compiler::Logger* getLogger() const;
            compiler::Lexer* getLexer() const;
            compiler::Parser* getParser() const;
            compiler::Compiler* getCompiler() const;

            compiler::ParseNode* getAST() const;
            compiler::Output* getCompilerOutput() const;
        
        protected:
            utils::String generateTemplateModuleName(ffi::TemplateType* type, const utils::Array<ffi::DataType*> templateArgs) const;
            Module* tryCache(const script_metadata* script);
            void postCompile(script_metadata* script);

            bool m_isCompiling;
            Pipeline* m_parent;
            Pipeline* m_root;
            compiler::Logger* m_logger;
            ModuleSource* m_source;
            compiler::Lexer* m_lexer;
            compiler::Parser* m_parser;
            compiler::Compiler* m_compiler;

            compiler::ParseNode* m_ast;
            compiler::Output* m_compOutput;
            utils::Array<optimize::IOptimizationStep*> m_optimizations;
    };
};