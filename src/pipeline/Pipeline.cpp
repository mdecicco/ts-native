#include <tsn/pipeline/Pipeline.h>
#include <tsn/common/Context.h>
#include <tsn/common/Module.h>
#include <tsn/common/Config.h>
#include <tsn/ffi/Function.h>
#include <tsn/ffi/DataType.h>
#include <tsn/ffi/FunctionRegistry.h>
#include <tsn/ffi/DataTypeRegistry.h>
#include <tsn/compiler/TemplateContext.h>
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
        if (m_logger && m_logger->hasErrors()) {
            // all functions, types, the module should be freed
            if (m_compOutput) {
                auto& funcs = m_compOutput->getCode();
                funcs.each([this](compiler::CodeHolder* ch) {
                    ffi::Function* fn = ch->owner;
                    if (!fn) return;
                    delete fn;
                });

                const auto& types = m_compiler->getOutput()->getTypes();
                types.each([this](ffi::DataType* tp) {
                    delete tp;
                });

                Module* mod = m_compOutput->getModule();
                if (mod) m_ctx->destroyModule(mod, true);
            }
        }

        if (m_compOutput) delete m_compOutput;
        m_compOutput = nullptr;

        if (m_compiler) delete m_compiler;
        m_compiler = nullptr;

        if (m_parser) delete m_parser;
        m_parser = nullptr;

        if (m_lexer) delete m_lexer;
        m_lexer = nullptr;

        m_source = nullptr;

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
    
    void Pipeline::addOptimizationStep(optimize::IOptimizationStep* step, bool isRequired) {
        optimize::OptimizationGroup* rootStep = ((optimize::OptimizationGroup*)m_optimizations[0]);
        rootStep->addStep(step, isRequired);
    }

    Module* Pipeline::buildFromSource(script_metadata* script) {
        if (m_isCompiling) {
            Pipeline child = Pipeline(m_ctx, this, m_root);
            return child.buildFromSource(script);
        }

        m_isCompiling = true;
        reset();

        if (!m_logger) m_logger = new compiler::Logger();

        backend::IBackend* be = m_ctx->getBackend();
        if (be) be->beforeCompile(this);

        m_source = m_ctx->getWorkspace()->getSource(
            m_ctx->getWorkspace()->getSourcePath(script->module_id)
        );
        
        if (!m_source) {
            m_logger->submit(
                compiler::log_type::lt_error,
                compiler::iom_failed_to_open_file,
                utils::String::Format("Failed to open source file '%s'", script->path.c_str())
            );
            m_isCompiling = false;
            return nullptr;
        }

        m_lexer = new compiler::Lexer(m_source);
        m_parser = new compiler::Parser(m_lexer, getLogger());
        m_ast = m_parser->parse();

        if (!m_ast || m_logger->hasErrors()) {
            m_isCompiling = false;
            return nullptr;
        }

        m_compiler = new compiler::Compiler(m_ctx, getLogger(), m_ast, script);

        compiler::OutputBuilder* out = m_compiler->compileAll();
        if (!out) {
            m_isCompiling = false;
            return nullptr;
        }
        
        postCompile(script);

        m_isCompiling = false;

        return out->getModule();
    }

    Module* Pipeline::buildFromCached(script_metadata* script) {
        if (m_isCompiling) {
            Pipeline child = Pipeline(m_ctx, this, m_root);
            return child.buildFromCached(script);
        }

        m_isCompiling = true;
        reset();

        if (!m_logger) m_logger = new compiler::Logger();
        
        backend::IBackend* be = m_ctx->getBackend();
        if (be) be->beforeCompile(this);

        m_source = m_ctx->getWorkspace()->getSource(
            m_ctx->getWorkspace()->getSourcePath(script->module_id)
        );

        Module* mod = tryCache(script);

        m_isCompiling = false;

        return mod;
    }
    
    ffi::DataType* Pipeline::specializeTemplate(ffi::TemplateType* type, const utils::Array<ffi::DataType*> templateArgs) {
        if (!type || templateArgs.size() == 0) return nullptr;
        
        if (m_isCompiling) {
            Pipeline child = Pipeline(m_ctx, this, m_root);
            return child.specializeTemplate(type, templateArgs);
        }
        
        m_isCompiling = true;
        reset();

        if (!m_logger) m_logger = new compiler::Logger();
        
        backend::IBackend* be = m_ctx->getBackend();
        if (be) be->beforeCompile(this);

        compiler::TemplateContext* tctx = type->getTemplateData();
        Module* originalModule = tctx->getOrigin();
        const script_metadata* originalMeta = originalModule->getInfo();
        m_source = originalModule->getSource();

        utils::String modName = generateTemplateModuleName(type, templateArgs);

        script_metadata* meta = m_ctx->getWorkspace()->createMeta(
            "<templates>/" + modName + ".tsn",
            originalMeta->size,
            0,
            originalMeta->is_trusted,
            true
        );

        // See if it's cached
        Module* cached = tryCache(meta);
        if (cached) {
            ffi::DataType* outTp = cached->allTypes().find([type, &templateArgs](ffi::DataType* tp) {
                return tp->isSpecializationOf(type, templateArgs);
            });

            // source is already owned by the module that the template
            // type is from
            m_source = nullptr;
            m_isCompiling = false;
            reset();

            if (outTp) {
                // template specializations should always be global.
                // scripts won't be able to instantiate them without
                // importing the original module that the template
                // is defined in
                m_ctx->getGlobal()->addForeignType(outTp);
                m_ctx->getTypes()->addForeignType(outTp);
            }

            return outTp;
        }

        // If there was a cache file that failed to load, it will have
        // deleted the script metadata. If there was no cache file and
        // the script metadata that was created earlier wasn't deleted
        // then this will just return that same metadata and not a new
        // one.
        meta = m_ctx->getWorkspace()->createMeta(
            "<templates>/" + modName + ".tsn",
            originalMeta->size,
            0,
            originalMeta->is_trusted,
            true
        );

        meta->modified_on = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

        m_ast = tctx->getAST();
        m_compiler = new compiler::Compiler(m_ctx, getLogger(), m_ast, meta);

        m_compiler->begin();

        compiler::OutputBuilder* out = m_compiler->getOutput();
        Module* mod = out->getModule();

        if (!out) {
            // source is already owned by the module that the template
            // type is from
            m_source = nullptr;
            m_isCompiling = false;
            return nullptr;
        }
        
        out->addDependency(originalModule);
        for (u32 i = 0;i < templateArgs.size();i++) {
            Module* owner = templateArgs[i]->getSource();
            if (!owner) continue;
            out->addDependency(owner);
        }

        compiler::Scope& s = m_compiler->scope().enter();

        compiler::ParseNode* tp = m_ast->template_parameters;
        bool didError = false;
        for (u32 i = 0;i < templateArgs.size();i++) {
            if (!tp) {
                m_logger->submit(
                    compiler::lt_error,
                    compiler::log_message_code::cm_err_too_many_template_args,
                    utils::String::Format("Attempted to instantiate object of type '%s' with too many template arguments", type->getName().c_str())
                );
                didError = true;
                break;
            }

            s.add(tp->str(), templateArgs[i]);
            tp = tp->next;
        }

        if (didError) {
            // source is already owned by the module that the template
            // type is from
            m_source = nullptr;
            m_isCompiling = false;
            return nullptr;
        }

        if (tp) {
            m_logger->submit(
                compiler::lt_error,
                compiler::log_message_code::cm_err_too_few_template_args,
                utils::String::Format("Attempted to instantiate object of type '%s' with too few template arguments", type->getName().c_str())
            );
            // source is already owned by the module that the template
            // type is from
            m_source = nullptr;
            m_isCompiling = false;
            return nullptr;
        }

        m_compiler->pushTemplateContext(tctx);
        
        ffi::DataType* outTp = nullptr;
        if (m_ast->tp == compiler::nt_type) {
            outTp = m_compiler->resolveTypeSpecifier(m_ast->data_type);
        } else if (m_ast->tp == compiler::nt_class) {
            outTp = m_compiler->compileClass(m_ast, true);
        }

        m_compiler->popTemplateContext();
        m_compiler->scope().exit();
        m_compiler->end();

        if (outTp) {
            outTp->m_templateBase = type;
            outTp->m_templateArgs = templateArgs;
            outTp->m_sourceModule = mod;
            mod->addForeignType(outTp);

            // template specializations should always be global.
            // scripts won't be able to instantiate them without
            // importing the original module that the template
            // is defined in
            m_ctx->getGlobal()->addForeignType(outTp);
            m_ctx->getTypes()->addForeignType(outTp);
        }

        postCompile(meta);

        m_ctx->getWorkspace()->mapSourcePath(
            mod->getId(),
            m_ctx->getWorkspace()->getSourcePath(originalModule->getId())
        );

        // source is already owned by the module that the template
        // type is from
        m_source = nullptr;
        m_isCompiling = false;

        return outTp;
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

    utils::String Pipeline::generateTemplateModuleName(ffi::TemplateType* type, const utils::Array<ffi::DataType*> templateArgs) const {
        utils::String modName = type->getFullyQualifiedName() + "(";
        for (u32 i = 0;i < templateArgs.size();i++) {
            if (i != 0) modName += ",";
            modName += templateArgs[i]->getFullyQualifiedName();
        }
        modName += ")";
        modName.replaceAll(':', '-');

        return modName;
    }

    Module* Pipeline::tryCache(const script_metadata* script) {
        utils::String cachePath = m_ctx->getWorkspace()->getCachePath(script);
        utils::Buffer* cache = utils::Buffer::FromFile(cachePath);
        if (!cache) {
            return nullptr;
        }

        m_compOutput = new compiler::Output(script, m_source);
        if (!m_compOutput->deserializeDependencies(cache, m_ctx)) {
            delete cache;
            delete m_compOutput;
            m_compOutput = nullptr;
            return nullptr;
        }

        // determine if any dependencies changed
        const auto& deps = m_compOutput->getDependencies();
        const auto& depVersions = m_compOutput->getDependencyVersions();

        for (u32 i = 0;i < depVersions.size();i++) {
            if (depVersions[i] != deps[i]->getInfo()->modified_on) {
                // dependency was changed, code needs to be recompiled
                delete cache;
                delete m_compOutput;
                m_compOutput = nullptr;
                return nullptr;
            }
        }

        if (!m_compOutput->deserialize(cache, m_ctx)) {
            delete cache;
            delete m_compOutput;
            m_compOutput = nullptr;
            return nullptr;
        }


        delete cache;
        
        Module* mod = m_compOutput->getModule();

        backend::IBackend* be = m_ctx->getBackend();
        if (!m_logger->hasErrors() && be) be->generate(this);

        return mod;
    }

    void Pipeline::postCompile(script_metadata* script) {
        compiler::OutputBuilder* out = m_compiler->getOutput();
        Module* mod = out->getModule();

        if (mod) mod->setSrc(m_source);

        if (m_logger->hasErrors()) return;

        m_compOutput = new compiler::Output(out);

        auto& funcs = m_compOutput->getCode();
        funcs.each([this, mod](compiler::CodeHolder* ch) {
            ffi::Function* fn = ch->owner;
            if (!fn) return;
            ch->rebuildAll();

            this->m_ctx->getFunctions()->registerFunction(fn);
            this->m_compiler->getOutput()->getModule()->addFunction(fn);

            bool optimizationsDisabled = m_ctx->getConfig()->disableOptimizations;
            m_optimizations.each([this, ch, optimizationsDisabled](optimize::IOptimizationStep* step) {
                if (optimizationsDisabled && !step->isRequired()) return;

                for (auto& b : ch->cfg.blocks) {
                    while (step->execute(ch, &b, this));
                }
                
                while (step->execute(ch, this));
            });

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
        types.each([this, mod](ffi::DataType* tp) {
            if (tp->getSource()) return;

            m_ctx->getTypes()->addForeignType(tp);
            m_compiler->getOutput()->getModule()->addForeignType(tp);
            tp->m_sourceModule = mod;
        });

        m_compOutput->processInput();
        if (!script->is_external && !m_ctx->getWorkspace()->getPersistor()->onScriptCompiled(script, m_compOutput)) {
            m_logger->submit(
                compiler::log_type::lt_warn,
                compiler::iom_failed_to_cache_script,
                utils::String::Format("Failed to cache compiled script '%s'", script->path.c_str())
            );
        }

        backend::IBackend* be = m_ctx->getBackend();
        if (be) be->generate(this);
    }
};
