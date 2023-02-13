#include <tsn/pipeline/Pipeline.h>
#include <tsn/common/Context.h>
#include <tsn/common/Module.h>
#include <tsn/common/Config.h>
#include <tsn/common/FunctionRegistry.h>
#include <tsn/common/TypeRegistry.h>
#include <tsn/compiler/Lexer.h>
#include <tsn/compiler/Parser.h>
#include <tsn/compiler/Compiler.h>
#include <tsn/compiler/FunctionDef.h>
#include <tsn/compiler/OutputBuilder.h>
#include <tsn/compiler/Output.h>
#include <tsn/interfaces/IPersistable.h>
#include <tsn/interfaces/IOptimizationStep.h>
#include <tsn/io/Workspace.h>
#include <tsn/utils/ModuleSource.h>

#include <utils/Buffer.hpp>
#include <utils/Array.hpp>

#include <filesystem>

namespace tsn {
    Pipeline::Pipeline(Context* ctx, Pipeline* parent, Pipeline* root) : IContextual(ctx) {
        m_isCompiling = false;
        m_parent = parent;
        m_root = root ? root : this;
        m_logger = parent ? parent->m_logger : nullptr;
        m_lexer = nullptr;
        m_parser = nullptr;
        m_compiler = nullptr;
        m_ast = nullptr;
        m_compOutput = nullptr;
    }

    Pipeline::~Pipeline() {
        reset();
    }

    void Pipeline::reset() {
        if (m_compOutput) delete m_compOutput;
        m_compOutput = nullptr;

        if (m_compiler) delete m_compiler;
        m_compiler = nullptr;

        if (m_parser) delete m_parser;
        m_parser = nullptr;

        if (m_lexer) delete m_lexer;
        m_lexer = nullptr;

        if (m_sources.size() > 0) {
            m_sources.each([](ModuleSource* src) {
                delete src;
            });
        }

        if (m_logger && !m_parent) {
            delete m_logger;
            m_logger = nullptr;
        }
        
        m_optimizations.each([](IOptimizationStep* step) {
            delete step;
        });
    }
    
    void Pipeline::addOptimizationStep(IOptimizationStep* step) {
        m_optimizations.push(step);
    }

    Module* Pipeline::buildFromSource(script_metadata* script) {
        if (m_isCompiling) {
            Pipeline child = Pipeline(m_ctx, this, m_root);
            Module* m = child.buildFromSource(script);
            if (m) return m;
            return nullptr;
        }
        m_isCompiling = true;

        reset();

        if (!m_logger) m_logger = new compiler::Logger();


        std::filesystem::path absPath = m_ctx->getConfig()->workspaceRoot;
        absPath /= script->path;
        absPath = std::filesystem::absolute(absPath);

        utils::Buffer* code = utils::Buffer::FromFile(absPath.string());
        if (!code) {
            m_logger->submit(
                compiler::log_type::lt_error,
                compiler::iom_failed_to_open_file,
                utils::String::Format("Failed to open source file '%s'", script->path.c_str())
            );
            m_isCompiling = false;
            return nullptr;
        }

        u64 lastModifiedOn = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::filesystem::last_write_time(absPath).time_since_epoch()
        ).count();

        ModuleSource* src = new ModuleSource(code, lastModifiedOn);
        delete code;

        if (m_root) m_root->m_sources.push(src);
        else m_sources.push(src);

        m_lexer = new compiler::Lexer(src);
        m_parser = new compiler::Parser(m_lexer, getLogger());
        m_ast = m_parser->parse();

        if (!m_ast || m_logger->hasErrors()) {
            m_isCompiling = false;
            return nullptr;
        }

        m_compiler = new compiler::Compiler(m_ctx, getLogger(), m_ast, script);

        compiler::OutputBuilder* out = m_compiler->compile();
        if (!out) {
            m_isCompiling = false;
            return nullptr;
        }

        m_compOutput = new compiler::Output(out);
        if (!m_logger->hasErrors()) {
            auto& funcs = out->getFuncs();
            funcs.each([this](compiler::FunctionDef* fd) {
                ffi::Function* fn = fd->getOutput();
                if (!fn) return;
                this->m_ctx->getFunctions()->registerFunction(fn);
                this->m_compiler->getOutput()->getModule()->addFunction(fn);

                CodeHolder ch = CodeHolder(fd->getCode());
                this->m_optimizations.each([this, &ch](IOptimizationStep* step) {
                    while (step->execute(&ch, this));
                });
            });

            const auto& types = m_compiler->getOutput()->getTypes();
            types.each([this](ffi::DataType* tp) {
                this->m_ctx->getTypes()->addForeignType(tp);
                this->m_compiler->getOutput()->getModule()->addForeignType(tp);
            });

            if (!m_ctx->getWorkspace()->getPersistor()->onScriptCompiled(script, m_compOutput)) {
                m_logger->submit(
                    compiler::log_type::lt_warn,
                    compiler::iom_failed_to_cache_script,
                    utils::String::Format("Failed to cache compiled script '%s'", script->path.c_str())
                );
            }
        }

        m_isCompiling = false;
        return out->getModule();
    }

    Module* Pipeline::buildFromCached(script_metadata* script) {
        if (m_isCompiling) {
            Pipeline child = Pipeline(m_ctx, this, m_root);
            Module* m = child.buildFromCached(script);
            if (m) return m;
            return nullptr;
        }

        utils::String cachePath = utils::String::Format(
            "%s/%u.tsnc",
            m_ctx->getConfig()->supportDir.c_str(),
            script->module_id
        );

        utils::Buffer* cache = utils::Buffer::FromFile(cachePath);
        if (!cache) return nullptr;

        m_compOutput = new compiler::Output();
        if (!m_compOutput->deserialize(cache, m_ctx, nullptr)) {
            delete cache;
            delete m_compOutput;
            m_compOutput = nullptr;
            return nullptr;
        }

        delete cache;
        return m_compOutput->getModule();
    }

    Pipeline* Pipeline::getParent() const {
        return m_parent;
    }

    compiler::Logger* Pipeline::getLogger() const {
        return m_logger;
    }

    compiler::Lexer* Pipeline::getLexer() const {
        return m_lexer;
    }

    compiler::Parser* Pipeline::getParser() const {
        return m_parser;
    }

    compiler::Compiler* Pipeline::getCompiler() const {
        return m_compiler;
    }

    compiler::ParseNode* Pipeline::getAST() const {
        return m_ast;
    }

    compiler::Output* Pipeline::getCompilerOutput() const {
        return m_compOutput;
    }
};