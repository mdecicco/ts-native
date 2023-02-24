#include <tsn/pipeline/Pipeline.h>
#include <tsn/common/Context.h>
#include <tsn/common/Module.h>
#include <tsn/common/Config.h>
#include <tsn/ffi/Function.h>
#include <tsn/ffi/FunctionRegistry.h>
#include <tsn/ffi/DataTypeRegistry.h>
#include <tsn/compiler/Lexer.h>
#include <tsn/compiler/Parser.h>
#include <tsn/compiler/Compiler.h>
#include <tsn/compiler/FunctionDef.h>
#include <tsn/compiler/OutputBuilder.h>
#include <tsn/compiler/Output.h>
#include <tsn/optimize/Optimize.h>
#include <tsn/compiler/CodeHolder.h>
#include <tsn/optimize/OptimizationGroup.h>
#include <tsn/interfaces/IPersistable.h>
#include <tsn/interfaces/IOptimizationStep.h>
#include <tsn/interfaces/IBackend.h>
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
        m_source = nullptr;
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

        if (m_source) {
            // Source code not claimed, Pipeline still owns it
            delete m_source;
            m_source = nullptr;
        }

        if (m_logger && !m_parent) {
            delete m_logger;
            m_logger = nullptr;
        }
        
        m_optimizations.each([](optimize::IOptimizationStep* step) {
            delete step;
        });
        m_optimizations.clear();

        optimize::OptimizationGroup* rootStep = new optimize::OptimizationGroup(m_ctx);
        rootStep->addStep(optimize::defaultOptimizations(m_ctx));
        m_optimizations.push(rootStep);
    }
    
    void Pipeline::addOptimizationStep(optimize::IOptimizationStep* step) {
        optimize::OptimizationGroup* rootStep = ((optimize::OptimizationGroup*)m_optimizations[0]);
        rootStep->addStep(step);
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

        backend::IBackend* be = m_ctx->getBackend();
        if (be) be->beforeCompile(this);


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

        ModuleSource* src = new ModuleSource(code, script);
        delete code;

        m_source = src;

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

        bool didError = m_logger->hasErrors();

        m_compOutput = new compiler::Output(out);
        if (!didError) {
            auto& funcs = m_compOutput->getCode();
            funcs.each([this](compiler::CodeHolder* ch) {
                ffi::Function* fn = ch->owner;
                if (!fn) return;
                ch->rebuildAll();

                this->m_ctx->getFunctions()->registerFunction(fn);
                this->m_compiler->getOutput()->getModule()->addFunction(fn);

                if (!this->m_ctx->getConfig()->disableOptimizations) {
                    this->m_optimizations.each([this, ch](optimize::IOptimizationStep* step) {
                        for (auto& b : ch->cfg.blocks) {
                            while (step->execute(ch, &b, this));
                        }
                        
                        while (step->execute(ch, this));
                    });
                }

                // Update call instruction operands to make imm FunctionDef operands imm function_ids
                // ...Also update types, in case the signature was not defined at the time that the
                // call instruction was emitted...
                auto& code = ch->code;
                for (u32 i = 0;i < code.size();i++) {
                    if (code[i].op == compiler::ir_call && code[i].operands[0].isImm()) {
                        compiler::FunctionDef* callee = code[i].operands[0].getImm<compiler::FunctionDef*>();
                        code[i].operands[0].setImm<function_id>(callee->getOutput()->getId());
                        code[i].operands[0].setType(callee->getOutput()->getSignature());
                        code[i].operands[0].getFlags().is_function_id = 1;
                    }
                }
            });

            const auto& types = m_compiler->getOutput()->getTypes();
            types.each([this](ffi::DataType* tp) {
                this->m_ctx->getTypes()->addForeignType(tp);
                this->m_compiler->getOutput()->getModule()->addForeignType(tp);
            });

            m_compOutput->processInput();
            if (!script->is_external && !m_ctx->getWorkspace()->getPersistor()->onScriptCompiled(script, m_compOutput)) {
                m_logger->submit(
                    compiler::log_type::lt_warn,
                    compiler::iom_failed_to_cache_script,
                    utils::String::Format("Failed to cache compiled script '%s'", script->path.c_str())
                );
            }
        }

        m_isCompiling = false;

        Module* mod = out->getModule();
        if (mod) {
            mod->setSrc(m_source);
            // source code ownership was taken by the Module
            m_source = nullptr;
        }

        if (!didError && be) {
            be->generate(this);
        }

        if (mod && be) mod->init();

        return mod;
    }

    Module* Pipeline::buildFromCached(script_metadata* script) {
        if (m_isCompiling) {
            Pipeline child = Pipeline(m_ctx, this, m_root);
            Module* m = child.buildFromCached(script);
            if (m) return m;
            return nullptr;
        }

        reset();
        if (!m_logger) m_logger = new compiler::Logger();
        
        backend::IBackend* be = m_ctx->getBackend();
        if (be) be->beforeCompile(this);

        std::filesystem::path absSourcePath = m_ctx->getConfig()->workspaceRoot;
        absSourcePath /= script->path;
        absSourcePath = std::filesystem::absolute(absSourcePath);

        utils::Buffer* code = utils::Buffer::FromFile(absSourcePath.string());
        if (!code) {
            m_logger->submit(
                compiler::log_type::lt_error,
                compiler::iom_failed_to_open_file,
                utils::String::Format("Failed to open source file '%s'", script->path.c_str())
            );
            m_isCompiling = false;
            return nullptr;
        }

        ModuleSource* src = new ModuleSource(code, script);
        delete code;

        m_source = src;

        utils::String cachePath = utils::String::Format(
            "%s/%u.tsnc",
            m_ctx->getConfig()->supportDir.c_str(),
            script->module_id
        );

        utils::Buffer* cache = utils::Buffer::FromFile(cachePath);
        if (!cache) {
            m_isCompiling = false;
            return nullptr;
        }

        m_compOutput = new compiler::Output(script, m_source);
        if (!m_compOutput->deserialize(cache, m_ctx)) {
            delete cache;
            delete m_compOutput;
            m_compOutput = nullptr;
            m_isCompiling = false;
            return nullptr;
        }

        delete cache;
        
        Module* mod = m_compOutput->getModule();

        if (mod) {
            // source code ownership was taken by the Module in m_compOutput->deserialize()
            m_source = nullptr;
        }

        m_isCompiling = false;

        if (!m_logger->hasErrors() && be) be->generate(this);

        if (mod && be) mod->init();

        return mod;
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