#include <gs/compiler/Compiler.h>
#include <gs/compiler/FunctionDef.h>
#include <gs/compiler/Parser.h>
#include <gs/common/Context.h>
#include <gs/common/Module.h>
#include <gs/common/Function.h>
#include <gs/common/DataType.h>
#include <gs/common/TypeRegistry.h>
#include <gs/common/FunctionRegistry.h>
#include <gs/interfaces/IDataTypeHolder.hpp>
#include <gs/interfaces/ICodeHolder.h>

#include <utils/Array.hpp>

using namespace utils;
using namespace gs::ffi;

namespace gs {
    namespace compiler {
        void compileAny(ast_node* n, Compiler* c);

        //
        // CompilerOutput
        //

        CompilerOutput::CompilerOutput(Compiler* c, Module* m) {
            m_comp = c;
            m_mod = m;
        }

        CompilerOutput::~CompilerOutput() {
            m_funcs.each([](FunctionDef* f) {
                delete f;
            });
        }

        FunctionDef* CompilerOutput::newFunc(const utils::String& name, ffi::DataType* methodOf) {
            if (getFunc(name, methodOf)) {
                return nullptr;
            }

            FunctionDef* fn = new FunctionDef(m_comp, name, methodOf);
            m_comp->scope().add(name, fn);
            m_funcs.push(fn);
            return fn;
        }

        FunctionDef* CompilerOutput::getFunc(const utils::String& name, ffi::DataType* methodOf) {
            return m_funcs.find([&name, methodOf](FunctionDef* fn) {
                return fn->getThisType() == methodOf && fn->getName() == name;
            });
        }
        
        const utils::Array<FunctionDef*>& CompilerOutput::getFuncs() const {
            return m_funcs;
        }

        Module* CompilerOutput::getModule() {
            return m_mod;
        }



        //
        // Compiler
        //

        Compiler::Compiler(Context* ctx, ast_node* programTree) : IContextual(ctx) {
            m_program = programTree;
            m_output = nullptr;
            enterNode(programTree);
            m_scopeMgr.enter();
        }

        Compiler::~Compiler() {
            exitNode();
        }

        void Compiler::enterNode(ast_node* n) {
            m_nodeStack.push(n);
        }

        void Compiler::exitNode() {
            m_nodeStack.pop();
        }

        const SourceLocation& Compiler::getCurrentSrc() const {
            return m_nodeStack[m_nodeStack.size() - 1]->tok->src;
        }

        ScopeManager& Compiler::scope() {
            return m_scopeMgr;
        }

        CompilerOutput* Compiler::getOutput() {
            return m_output;
        }

        CompilerOutput* Compiler::compile() {
            Module* m = new Module(m_ctx, "test");
            m_output = new CompilerOutput(this, m);

            // todo: Import all symbols from global module
            const utils::Array<gs::ffi::DataType*>& importTypes = m_ctx->getGlobal()->allTypes();
            for (auto* t : importTypes) {
                m_scopeMgr.add(t->getName(), t);
            }

            const utils::Array<gs::ffi::Function*>& importFuncs = m_ctx->getGlobal()->allFunctions();
            for (auto* f : importFuncs) {
                if (!f) continue;
                m_scopeMgr.add(f->getName(), f);
            }

            ast_node* n = m_program->body;
            while (n) {
                if (n->tp == nt_import) {
                    // todo: Deal with the import...
                } else if (n->tp == nt_function) {
                    m_output->newFunc(n->str());
                }
                n = n->next;
            }

            n = m_program->body;
            while (n) {
                compileAny(n, this);
                n = n->next;
            }

            bool failureCondition = false;

            return m_output;
        }

        


        //
        // Data type resolution
        //

        DataType* resolveTypeSpecifier(ast_node* n, Compiler* c);

        DataType* getArrayType(DataType* elemTp, Compiler* c) {
            DataType* outTp = nullptr;
            String name = "Array<" + elemTp->getName() + ">";
            symbol* s = c->scope().getBase().get(name);

            if (s) {
                if (s->tp == st_type) outTp = s->type;
                else {
                    // todo: error
                }
            } else {
                String fullName = "Array<" + elemTp->getFullyQualifiedName() + ">";
                TemplateType* at = (TemplateType*)c->scope().getBase().get("Array");
                ast_node* arrayAst = at->getAST();
                Scope& tscope = c->scope().enter();

                // Add the array element type to the symbol table as the template argument name
                tscope.add(arrayAst->template_parameters->str(), elemTp);

                if (arrayAst->tp == nt_type) {
                    outTp = resolveTypeSpecifier(arrayAst->data_type, c);
                } else if (arrayAst->tp == nt_class) {
                    // todo
                    // outTp = compileClass(arrayAst, c);
                }
                
                c->getOutput()->getModule()->addForeignType(outTp);

                c->scope().exit();
            }

            return outTp;
        }
        DataType* getPointerType(DataType* destTp, Compiler* c) {
            DataType* outTp = nullptr;
            String name = "Pointer<" + destTp->getName() + ">";
            symbol* s = c->scope().getBase().get(name);
            
            if (s) {
                if (s->tp == st_type) outTp = s->type;
                else {
                    // todo: error
                }
            } else {
                String fullName = "Pointer<" + destTp->getFullyQualifiedName() + ">";
                TemplateType* pt = (TemplateType*)c->scope().getBase().get("Array");
                ast_node* pointerAst = pt->getAST();
                Scope& tscope = c->scope().enter();

                // Add the pointer destination type to the symbol table as the template argument name
                tscope.add(pointerAst->template_parameters->str(), destTp);

                if (pointerAst->tp == nt_type) {
                    outTp = resolveTypeSpecifier(pointerAst->data_type, c);
                } else if (pointerAst->tp == nt_class) {
                    // todo
                    // outTp = compileClass(pointerAst, c);
                }

                c->getOutput()->getModule()->addForeignType(outTp);

                c->scope().exit();
            }

            return outTp;
        }

        DataType* resolveTemplateTypeSubstitution(ast_node* templateArgs, DataType* _type, Compiler* c) {
            if (!_type->getInfo().is_template) {
                // todo: errors
                return nullptr;
            }
            TemplateType* type = (TemplateType*)_type;

            Scope& s = c->scope().enter();

            ast_node* tpAst = type->getAST();

            // Add template parameters to symbol table, representing the provided arguments
            // ...Also construct type name
            String name = _type->getName();
            String fullName = _type->getFullyQualifiedName();
            name += '<';
            fullName += '<';

            ast_node* n = tpAst->template_parameters;
            while (n) {
                DataType* pt = resolveTypeSpecifier(templateArgs, c);
                s.add(n->str(), pt);
                name += pt->getName();
                fullName += pt->getFullyQualifiedName();
                if (n != tpAst->template_parameters) {
                    name += ", ";
                    fullName += ", ";
                }

                n = n->next;
                templateArgs = templateArgs->next;

                if (n && !templateArgs) {
                    // todo: errors (too few template args)
                    c->scope().exit();
                    return nullptr;
                }
            }

            name += '>';
            fullName += '>';

            if (!n && templateArgs) {
                // todo: errors (too many template args)
                c->scope().exit();
                return nullptr;
            }

            symbol* existing = c->scope().getBase().get(name);
            if (existing) {
                if (existing->tp != st_type) {
                    // todo: errors
                    c->scope().exit();
                    return nullptr;
                }

                c->scope().exit();
                return existing->type;
            }

            DataType* tp = nullptr;
            if (tpAst->tp == nt_type) {
                tp = resolveTypeSpecifier(tpAst->data_type, c);
            } else if (tpAst->tp == nt_class) {
                // todo
                // tp = compileClass(tpAst, c);
            }
            
            c->scope().exit();

            tp = new AliasType(name, fullName, tp);
            c->scope().getBase().add(name, tp);
            c->getOutput()->getModule()->addForeignType(tp);
            return tp;
        }
        DataType* resolveObjectTypeSpecifier(ast_node* n, Compiler* c) {
            Array<type_property> props;
            Function* dtor = nullptr;
            type_meta info;
            info.is_pod = 1;
            info.is_trivially_constructible = 1;
            info.is_trivially_copyable = 1;
            info.is_trivially_destructible = 1;
            info.is_primitive = 0;
            info.is_floating_point = 0;
            info.is_integral = 0;
            info.is_unsigned = 0;
            info.is_function = 0;
            info.is_host = 0;
            info.is_anonymous = 1;
            info.size = 0;
            info.host_hash = 0;

            // todo
            ast_node* b = n->body;
            while (b) {
                switch (b->tp) {
                    case nt_type_property: {
                        DataType* pt = resolveTypeSpecifier(b->data_type, c);
                        props.push({
                            b->str(),
                            public_access,
                            info.size,
                            pt,
                            {
                                1, // can read
                                1, // can write
                                0, // static
                                0  // pointer
                            },
                            nullptr,
                            nullptr
                        });

                        const type_meta& i = pt->getInfo();
                        if (!i.is_pod) info.is_pod = 0;
                        if (!i.is_trivially_constructible) info.is_trivially_constructible = 0;
                        if (!i.is_trivially_copyable) info.is_trivially_copyable = 0;
                        if (!i.is_trivially_destructible) info.is_trivially_destructible = 0;
                        info.size += pt->getInfo().size;

                        break;
                    }
                    default: {
                        break;
                    }
                }

                b = b->next;
            }

            String name = String::Format("$anon_%d", c->getContext()->getTypes()->getNextAnonTypeIndex());
            DataType* tp = new DataType(
                name,
                name,
                info,
                props,
                {},
                dtor,
                {}
            );

            const Array<DataType*>& types = c->getContext()->getTypes()->allTypes();
            bool alreadyExisted = false;
            for (u32 i = 0;i < types.size();i++) {
                if (types[i]->isEquivalentTo(tp)) {
                    delete tp;
                    tp = types[i];
                    alreadyExisted = true;
                }
            }

            if (!alreadyExisted) {
                c->getContext()->getTypes()->addForeignType(tp);
            }

            return tp;
        }
        DataType* resolveFunctionTypeSpecifier(ast_node* n, Compiler* c) {
            DataType* retTp = resolveTypeSpecifier(n->data_type, c);
            DataType* ptrTp = c->getContext()->getTypes()->getType<void*>();
            Array<function_argument> args;
            args.push({ arg_type::func_ptr, ptrTp });
            args.push({ arg_type::ret_ptr, retTp });
            args.push({ arg_type::context_ptr, ptrTp });


            ast_node* a = n->parameters;
            while (a) {
                DataType* atp = resolveTypeSpecifier(a->data_type, c);
                args.push({
                    atp->getInfo().is_primitive ? arg_type::value : arg_type::pointer,
                    atp
                });
                a = a->next;
            }

            return new FunctionSignatureType(retTp, args);
        }
        DataType* resolveTypeNameSpecifier(ast_node* n, Compiler* c) {
            String name = n->body->str();
            symbol* s = c->scope().get(name);

            if (n->body->template_parameters) {
                DataType* tp = resolveTemplateTypeSubstitution(n->body->template_parameters, s->type, c);
                return tp;
            }

            if (!s || s->tp != st_type) {
                // todo: errors
                return nullptr;
            }

            return s->type;
        }
        DataType* applyTypeModifiers(DataType* tp, ast_node* mod, Compiler* c) {
            if (!mod) return tp;

            DataType* outTp = nullptr;

            if (mod->flags.is_array) outTp = getArrayType(tp, c);
            else if (mod->flags.is_pointer) outTp = getPointerType(tp, c);

            return applyTypeModifiers(outTp, mod->modifier, c);
        }
        DataType* resolveTypeSpecifier(ast_node* n, Compiler* c) {
            DataType* tp = nullptr;
            if (n->body && n->body->tp == nt_type_property) tp = resolveObjectTypeSpecifier(n, c);
            else if (n->parameters) tp = resolveFunctionTypeSpecifier(n, c);
            else tp = resolveTypeNameSpecifier(n, c);

            return applyTypeModifiers(tp, n->modifier, c);
        }

        void compileType(ast_node* n, Compiler* c) {
            DataType* tp = nullptr;
            String name = n->str();

            if (n->template_parameters) {
                tp = new TemplateType(name, c->getOutput()->getModule()->getName() + "::" + name, n);
            } else {
                tp = resolveTypeSpecifier(n->data_type, c);

                if (tp) {
                    tp = new AliasType(name, c->getOutput()->getModule()->getName() + "::" + name, tp);
                }
            }

            if (tp) {
                c->scope().getBase().add(name, tp);
                c->getOutput()->getModule()->addForeignType(tp);
            }
        }



        //
        // Class compilation
        //

        void compileClass(ast_node* n, Compiler* c) {

        }


        void compileBlock(ast_node* n, Compiler* c) {
            n = n->body;
            while (n) {
                compileAny(n, c);
                n = n->next;
            }
        }
        void compileAny(ast_node* n, Compiler* c) {
            switch (n->tp) {
                case nt_empty:
                case nt_root:
                case nt_eos:
                case nt_break:
                case nt_catch: break;
                case nt_class: return compileClass(n, c);
                case nt_continue:
                case nt_export:
                case nt_expression:
                case nt_function:
                case nt_function_type:
                case nt_identifier:
                case nt_if:
                case nt_import:
                case nt_import_symbol:
                case nt_import_module:
                case nt_literal:
                case nt_loop:
                case nt_object_decompositor:
                case nt_object_literal_property:
                case nt_parameter:
                case nt_property:
                case nt_return: break;
                case nt_scoped_block: return compileBlock(n, c);
                case nt_switch:
                case nt_switch_case:
                case nt_this:
                case nt_throw:
                case nt_try: break;
                case nt_type: return compileType(n, c);
                case nt_type_modifier:
                case nt_type_property:
                case nt_type_specifier:
                case nt_variable: break;
            }
        }
    };
};