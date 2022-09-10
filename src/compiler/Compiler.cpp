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
            FunctionDef* fn = new FunctionDef(m_comp, name, methodOf);
            m_comp->scope().add(name, fn);
            m_funcs.push(fn);
            return fn;
        }
        
        FunctionDef* CompilerOutput::newFunc(ffi::Function* preCreated) {
            FunctionDef* fn = new FunctionDef(m_comp, preCreated);
            m_comp->scope().add(preCreated->getName(), fn);
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

        void Compiler::enterFunction(FunctionDef* fn) {
            m_scopeMgr.enter();
            m_funcStack.push(fn);
            fn->onEnter();
        }

        void Compiler::exitFunction() {
            m_funcStack.pop();
            m_scopeMgr.exit();
        }

        FunctionDef* Compiler::currentFunction() const {
            if (m_funcStack.size() == 0) return nullptr;
            return m_funcStack[m_funcStack.size() - 1];
        }

        bool Compiler::inInitFunction() const {
            return m_funcStack.size() == 1;
        }

        const SourceLocation& Compiler::getCurrentSrc() const {
            return m_nodeStack[m_nodeStack.size() - 1]->tok->src;
        }

        ScopeManager& Compiler::scope() {
            return m_scopeMgr;
        }

        void Compiler::addDataType(DataType* tp) {
            m_addedTypes.push(tp);
        }
        
        void Compiler::addFunction(Function* fn) {
            m_addedFuncs.push(fn);
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
                    compileImport(n, this);
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

        const utils::Array<DataType*>& Compiler::getAddedDataTypes() const {
            return m_addedTypes;
        }

        const utils::Array<Function*>& Compiler::getAddedFunctions() const {
            return m_addedFuncs;
        }
        
        void Compiler::updateMethod(DataType* classTp, Method* m) {
            m->setThisType(classTp);
        }



        //
        // Data type resolution
        //

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
                    outTp = compileClass(arrayAst, c, true);
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
                    outTp = compileClass(pointerAst, c, true);
                }

                c->getOutput()->getModule()->addForeignType(outTp);

                c->scope().exit();
            }

            return outTp;
        }
        DataType* resolveTemplateTypeSubstitution(ast_node* templateArgs, DataType* _type, Compiler* c) {
            c->enterNode(templateArgs);
            if (!_type->getInfo().is_template) {
                // todo: errors

                c->exitNode();
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
                c->enterNode(n);
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
                    c->exitNode();
                    return nullptr;
                }
                c->exitNode();
            }

            name += '>';
            fullName += '>';

            if (!n && templateArgs) {
                // todo: errors (too many template args)
                c->scope().exit();
                c->exitNode();
                return nullptr;
            }

            symbol* existing = c->scope().getBase().get(name);
            if (existing) {
                if (existing->tp != st_type) {
                    // todo: errors
                    c->scope().exit();
                    c->exitNode();
                    return nullptr;
                }

                c->scope().exit();
                c->exitNode();
                return existing->type;
            }

            DataType* tp = nullptr;
            if (tpAst->tp == nt_type) {
                tp = resolveTypeSpecifier(tpAst->data_type, c);

                tp = new AliasType(name, fullName, tp);
                c->scope().getBase().add(name, tp);
                c->addDataType(tp);
            } else if (tpAst->tp == nt_class) {
                tp = compileClass(tpAst, c, true);
            }
            
            c->scope().exit();
            c->exitNode();
            return tp;
        }
        DataType* resolveObjectTypeSpecifier(ast_node* n, Compiler* c) {
            c->enterNode(n);
            Array<type_property> props;
            Function* dtor = nullptr;
            type_meta info = { 0 };
            info.is_pod = 1;
            info.is_trivially_constructible = 1;
            info.is_trivially_copyable = 1;
            info.is_trivially_destructible = 1;
            info.is_anonymous = 1;

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
                c->addDataType(tp);
            }

            c->exitNode();
            return tp;
        }
        DataType* resolveFunctionTypeSpecifier(ast_node* n, Compiler* c) {
            c->enterNode(n);
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

            DataType* tp = new FunctionType(retTp, args);
            c->addDataType(tp);
            c->exitNode();
            return tp;
        }
        DataType* resolveTypeNameSpecifier(ast_node* n, Compiler* c) {
            String name = n->body->str();
            symbol* s = c->scope().get(name);

            if (n->body->template_parameters) {
                c->enterNode(n);
                DataType* tp = resolveTemplateTypeSubstitution(n->body->template_parameters, s->type, c);
                c->exitNode();
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
            c->enterNode(mod);
            DataType* outTp = nullptr;

            if (mod->flags.is_array) outTp = getArrayType(tp, c);
            else if (mod->flags.is_pointer) outTp = getPointerType(tp, c);

            outTp = applyTypeModifiers(outTp, mod->modifier, c);
            c->exitNode();
            return outTp;
        }
        DataType* resolveTypeSpecifier(ast_node* n, Compiler* c) {
            DataType* tp = nullptr;
            if (n->body && n->body->tp == nt_type_property) tp = resolveObjectTypeSpecifier(n, c);
            else if (n->parameters) tp = resolveFunctionTypeSpecifier(n, c);
            else tp = resolveTypeNameSpecifier(n, c);

            return applyTypeModifiers(tp, n->modifier, c);
        }


        Value generateCall(Compiler* c, ffi::Function* fn, const utils::Array<Value>& args, const Value* self) {
            return c->currentFunction()->getPoison();
        }

        Value generateCall(Compiler* c, FunctionDef* fn, const utils::Array<Value>& args, const Value* self) {
            return c->currentFunction()->getPoison();
        }

        DataType* compileType(ast_node* n, Compiler* c) {
            DataType* tp = nullptr;
            String name = n->str();

            if (n->template_parameters) {
                tp = new TemplateType(name, c->getOutput()->getModule()->getName() + "::" + name, n->clone());
                c->addDataType(tp);
                c->scope().getBase().add(name, tp);
            } else {
                tp = resolveTypeSpecifier(n->data_type, c);

                if (tp) {
                    tp = new AliasType(name, c->getOutput()->getModule()->getName() + "::" + name, tp);
                    c->addDataType(tp);
                    c->scope().getBase().add(name, tp);
                }
            }

            return tp;
        }
        void compileMethodDef(ast_node* n, Compiler* c, DataType* methodOf, Method* m) {
            c->updateMethod(methodOf, m);
            
            FunctionType* sig = m->getSignature();
            const auto& args = sig->getArguments();
            ast_node* p = n->parameters;

            c->enterNode(n->body);

            FunctionDef* fd = c->getOutput()->newFunc(m);
            c->enterFunction(fd);

            while (p) {
                fd->addArg(p->str(), resolveTypeSpecifier(p->data_type, c));
                p = p->next;
            }

            compileBlock(n->body, c);

            c->exitFunction();
            c->exitNode();
        }
        Method* compileMethodDecl(ast_node* n, Compiler* c, u64 thisOffset, bool* wasDtor, bool templatesDefined, bool dtorExists) {
            c->enterNode(n);
            utils::String name = n->str();
            DataType* voidTp = c->getContext()->getTypes()->getType<void>();
            DataType* ptrTp = c->getContext()->getTypes()->getType<void*>();

            bool isOperator = false;
            if (name == "operator") {
                isOperator = true;
                if (n->op != op_undefined) {
                    static const char* opStrs[] = {
                        "",
                        "+", "+=",
                        "-", "-=",
                        "*", "*=",
                        "/", "/=",
                        "%", "%=",
                        "^", "^=",
                        "&", "&=",
                        "|", "|=",
                        "~",
                        "<<", "<<=",
                        ">>", ">>=",
                        "!", "!=",
                        "&&", "&&=",
                        "||", "||=",
                        "=", "==",
                        "<", "<=",
                        ">", ">=",
                        "?",
                        "++", "++", "++",
                        "--", "--", "--",
                        "-",
                        ".",
                        "[]",
                        "new", "new",
                        "()"
                    };
                    name += opStrs[n->op];
                }
            }
            
            if (n->template_parameters) {
                if (!templatesDefined) {
                    Method* m = new TemplateMethod(
                        name,
                        n->flags.is_private ? private_access : public_access,
                        thisOffset,
                        n
                    );
                    c->addFunction(m);
                    c->exitNode();
                    return m;
                }

                name += '<';

                ast_node* arg = n->template_parameters;
                while (arg) {
                    symbol* s = c->scope().get(arg->str());
                    // symbol for template parameter should always be defined here

                    DataType* pt = s->type;
                    name += pt->getFullyQualifiedName();
                    if (arg != n->template_parameters) name += ", ";

                    arg = arg->next;
                }

                name += '>';
            }

            if (isOperator) {
                if (n->op == op_undefined) {
                    DataType* castTarget = resolveTypeSpecifier(n->data_type, c);
                    name += " " + castTarget->getFullyQualifiedName();
                }
            }

            bool isCtor = false;
            if (name == "destructor") {
                *wasDtor = true;
                if (dtorExists) {
                    // todo: errors
                }
            } else if (name == "constructor") {
                isCtor = true;
            }

            DataType* ret = *wasDtor ? voidTp : (isCtor ? ptrTp : voidTp);
            if (n->data_type) {
                ret = resolveTypeSpecifier(n->data_type, c);
            }

            bool sigExisted = false;
            utils::Array<function_argument> args;
            args.push({ arg_type::func_ptr, ptrTp });
            args.push({ arg_type::ret_ptr, ret });
            args.push({ arg_type::context_ptr, ptrTp });

            // This type will be assigned later
            args.push({ arg_type::this_ptr, ptrTp });

            ast_node* p = n->parameters;
            u8 a = u8(args.size());
            while (p) {
                if (!p->data_type) {
                    // todo: errors
                }

                arg_type tp = arg_type::value;
                args.push({ tp, resolveTypeSpecifier(p->data_type, c) });

                p = p->next;
                a++;
            }

            FunctionType* sig = new FunctionType(ret, args);
            const auto& types = c->getContext()->getTypes()->allTypes();
            for (auto* t : types) {
                const auto& info = t->getInfo();
                if (info.is_function || info.is_template) continue;
                if (sig->isEquivalentTo(t)) {
                    delete sig;
                    sig = (FunctionType*)t;
                    sigExisted = true;
                    break;
                }
            }

            if (!sigExisted) {
                c->addDataType(sig);
            }

            Method* m = new Method(
                name,
                sig,
                n->flags.is_private ? private_access : public_access,
                nullptr,
                nullptr,
                thisOffset
            );

            c->addFunction(m);
            c->exitNode();
            return m;
        }
        DataType* compileClass(ast_node* n, Compiler* c, bool templatesDefined) {
            c->enterNode(n);
            utils::String name = n->str();
            String fullName = c->getOutput()->getModule()->getName() + "::" + name;
            
            if (n->template_parameters) {
                if (!templatesDefined) {
                    DataType* tp = new TemplateType(name, fullName, n->clone());
                    c->addDataType(tp);
                    c->scope().add(name, tp);
                    c->exitNode();
                    return tp;
                }

                name += '<';
                fullName += '<';

                ast_node* arg = n->template_parameters;
                while (arg) {
                    symbol* s = c->scope().get(arg->str());
                    // symbol for template parameter should always be defined here

                    DataType* pt = s->type;
                    name += pt->getName();
                    fullName += pt->getFullyQualifiedName();
                    if (arg != n->template_parameters) {
                        name += ", ";
                        fullName += ", ";
                    }

                    arg = arg->next;
                }

                name += '>';
                fullName += '>';
            }

            type_meta oinfo = { 0 };
            oinfo.is_pod = 1;
            oinfo.is_trivially_constructible = 1;
            oinfo.is_trivially_copyable = 1;
            oinfo.is_trivially_destructible = 1;
            
            u64 offset = 0;
            ast_node* inherits = n->inheritance;
            utils::Array<type_base> bases;
            while (inherits) {
                DataType* t = resolveTypeSpecifier(inherits, c);
                const type_meta& info = t->getInfo();

                // todo: Check added properties don't have name collisions with
                //       existing properties

                type_base b;
                b.access = public_access;
                b.offset = offset;
                b.type = t;
                bases.push(b);

                inherits = inherits->next;
                offset += info.size;
            }

            utils::Array<type_property> props;
            ast_node* prop = n->body;
            u64 thisOffset = offset;
            while (prop) {
                if (prop->tp != nt_property) {
                    prop = prop->next;
                    continue;
                }

                DataType* t = resolveTypeSpecifier(prop->data_type, c);
                const type_meta& info = t->getInfo();

                type_property p;
                p.name = prop->str();
                p.access = prop->flags.is_private ? private_access : public_access;
                p.flags.can_read = 1;
                p.flags.can_write = (!prop->flags.is_const) ? 1 : 0;
                p.flags.is_pointer = prop->flags.is_pointer;
                p.flags.is_static = prop->flags.is_static;
                p.getter = nullptr;
                p.setter = nullptr;
                p.offset = offset;
                p.type = t;
                props.push(p);
                
                if (!info.is_pod) oinfo.is_pod = 0;
                if (!info.is_trivially_constructible) oinfo.is_trivially_constructible = 0;
                if (!info.is_trivially_copyable) oinfo.is_trivially_copyable = 0;
                if (!info.is_trivially_destructible) oinfo.is_trivially_destructible = 0;

                offset += info.size;
                prop = prop->next;
            }

            ffi::Function* dtor = nullptr;
            utils::Array<ffi::Function*> methods;
            ast_node* meth = n->body;
            while (meth) {
                if (meth->tp != nt_function) {
                    meth = meth->next;
                    continue;
                }

                bool isDtor = false;
                Method* m = compileMethodDecl(meth, c, thisOffset, &isDtor, false, dtor != nullptr);
                if (isDtor) dtor = m;
                else methods.push(m);
                meth = meth->next;
            }

            DataType* tp = new DataType(
                name,
                fullName,
                oinfo,
                props,
                bases,
                dtor,
                methods
            );

            u32 midx = 0;
            meth = n->body;
            while (meth) {
                if (meth->tp != nt_function) {
                    meth = meth->next;
                    continue;
                }

                compileMethodDef(meth, c, tp, (Method*)methods[midx++]);
                meth = meth->next;
            }

            c->addDataType(tp);
            c->scope().add(name, tp);
            c->exitNode();

            return tp;
        }
        void compileFunction(ast_node* n, Compiler* c, bool templatesDefined) {
            c->enterNode(n);
            utils::String name = n->str();
            String fullName = c->getOutput()->getModule()->getName() + "::" + name;
            
            if (n->template_parameters) {
                if (!templatesDefined) {
                    TemplateFunction* f = new TemplateFunction(
                        name,
                        n->flags.is_private ? private_access : public_access,
                        n->clone()
                    );
                    c->addFunction(f);
                    c->scope().add(name, f);
                    c->exitNode();
                    return;
                }

                name += '<';

                ast_node* arg = n->template_parameters;
                while (arg) {
                    symbol* s = c->scope().get(arg->str());
                    // symbol for template parameter should always be defined here

                    DataType* pt = s->type;
                    name += pt->getFullyQualifiedName();
                    if (arg != n->template_parameters) name += ", ";

                    arg = arg->next;
                }

                name += '>';
            }

            FunctionDef* fd = c->getOutput()->newFunc(name);

            c->enterFunction(fd);

            if (n->data_type) {
                fd->setReturnType(resolveTypeSpecifier(n->data_type, c));
            }

            ast_node* p = n->parameters;
            while (p) {
                fd->addArg(p->str(), resolveTypeSpecifier(p->data_type, c));
                p = p->next;
            }

            compileBlock(n->body, c);
            
            c->exitFunction();
            c->exitNode();
        }
        void compileExport(ast_node* n, Compiler* c) {

        }
        void compileImport(ast_node* n, Compiler* c) {

        }
        void compileExpression(ast_node* n, Compiler* c) {

        }
        void compileIfStatement(ast_node* n, Compiler* c) {

        }
        void compileReturnStatement(ast_node* n, Compiler* c) {
            
        }
        void compileSwitchStatement(ast_node* n, Compiler* c) {

        }
        void compileTryBlock(ast_node* n, Compiler* c) {

        }
        void compileVarDecl(ast_node* n, Compiler* c) {

        }
        void compileLoop(ast_node* n, Compiler* c) {

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
                case nt_class: {
                    if (c->inInitFunction()) {
                        compileClass(n, c);
                        return;
                    }

                    // todo: errors
                    return;
                }
                case nt_export: {
                    if (!c->inInitFunction()) {
                        // todo: errors
                        return;
                    }

                    return compileExport(n, c);
                }
                case nt_expression: return compileExpression(n, c);
                case nt_function: {
                    if (c->inInitFunction()) {
                        compileFunction(n, c);
                        return;
                    }

                    // todo: errors
                    return;
                }
                case nt_if: return compileIfStatement(n, c);
                case nt_loop: return compileLoop(n, c);
                case nt_return: return compileReturnStatement(n, c);
                case nt_scoped_block: return compileBlock(n, c);
                case nt_switch: return compileSwitchStatement(n, c);
                case nt_try: return compileTryBlock(n, c);
                case nt_type: {
                    if (c->inInitFunction()) {
                        compileType(n, c);
                        return;
                    }

                    // todo: errors
                    return;
                }
                case nt_variable: return compileVarDecl(n, c);
                default: break;
            }
        }
    };
};