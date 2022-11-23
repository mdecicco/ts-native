#include <gs/compiler/Compiler.h>
#include <gs/compiler/FunctionDef.hpp>
#include <gs/compiler/Parser.h>
#include <gs/common/Context.h>
#include <gs/common/Module.h>
#include <gs/common/Function.h>
#include <gs/common/DataType.h>
#include <gs/common/TypeRegistry.h>
#include <gs/common/FunctionRegistry.h>
#include <gs/compiler/Value.hpp>
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

        Compiler::Compiler(Context* ctx, ast_node* programTree) : IContextual(ctx), m_scopeMgr(this) {
            m_program = programTree;
            m_output = nullptr;
            m_curFunc = nullptr;
            m_curClass = nullptr;
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
            m_curFunc = fn;
            fn->onEnter();
        }

        Function* Compiler::exitFunction() {
            if (m_funcStack.size() == 0) return nullptr;
            Function* fn = m_funcStack[m_funcStack.size() - 1]->onExit();
            m_funcStack.pop();
            m_scopeMgr.exit();
            if (m_funcStack.size() == 0) m_curFunc = nullptr;
            else m_curFunc = m_funcStack[m_funcStack.size() - 1];
            return fn;
        }

        FunctionDef* Compiler::currentFunction() const {
            return m_curFunc;
        }

        DataType* Compiler::currentClass() const {
            return m_curClass;
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
                    compileImport(n);
                } else if (n->tp == nt_function) {
                    m_output->newFunc(n->str());
                }
                n = n->next;
            }

            // init function
            FunctionDef* fd = m_output->newFunc("__init__");
            enterFunction(fd);

            n = m_program->body;
            while (n) {
                compileAny(n);
                n = n->next;
            }

            exitFunction();
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

        InstructionRef Compiler::add(ir_instruction inst) {
            return m_curFunc->add(inst);
        }

        void Compiler::constructObject(const Value& dest, const utils::Array<Value>& args) {
            Array<DataType*> argTps = args.map([](const Value& v) { return v.getType(); });

            Array<Function *> ctors = dest.getType()->findMethods("constructor", nullptr, (const DataType**)argTps.data(), argTps.size(), fm_skip_implicit_args);
            if (ctors.size() == 1) {
                generateCall(ctors[0], args, &dest);
            } else if (ctors.size() > 1) {
                u8 strictC = 0;
                ffi::Function* func = nullptr;
                for (u32 i = 0;i < ctors.size();i++) {
                    const auto& args = ctors[i]->getSignature()->getArguments();
                    for (u32 a = 0;a < args.size();a++) {
                        if (args[a].isImplicit()) continue;
                        if (args[a].dataType->isEqualTo(argTps[a])) {
                            if (args.size() > a + 1) {
                                // Function has more arguments which are not explicitly required
                                break;
                            }

                            strictC++;
                            func = ctors[i];
                        }
                    }
                }
                
                if (strictC == 1) {
                    generateCall(func, args, &dest);
                }

                // todo: Error, ambiguous call
            } else {
                // todo: Error, no matches
            }
        }

        void Compiler::constructObject(const Value& dest, ffi::DataType* tp, const utils::Array<Value>& args) {
            Array<DataType*> argTps = args.map([](const Value& v) { return v.getType(); });

            Array<Function *> ctors = tp->findMethods("constructor", nullptr, (const DataType**)argTps.data(), argTps.size(), fm_skip_implicit_args);
            if (ctors.size() == 1) {
                generateCall(ctors[0], args, &dest);
            } else if (ctors.size() > 1) {
                u8 strictC = 0;
                ffi::Function* func = nullptr;
                for (u32 i = 0;i < ctors.size();i++) {
                    const auto& args = ctors[i]->getSignature()->getArguments();
                    for (u32 a = 0;a < args.size();a++) {
                        if (args[a].isImplicit()) continue;
                        if (args[a].dataType->isEqualTo(argTps[a])) {
                            if (args.size() > a + 1) {
                                // Function has more arguments which are not explicitly required
                                break;
                            }

                            strictC++;
                            func = ctors[i];
                        }
                    }
                }
                
                if (strictC == 1) {
                    generateCall(func, args, &dest);
                }

                // todo: Error, ambiguous call
            } else {
                // todo: Error, no matches
            }
        }

        Value Compiler::constructObject(ffi::DataType* tp, const utils::Array<Value>& args) {
            FunctionDef* cf = currentFunction();
            alloc_id stackId = cf->reserveStackId();

            Value out = currentFunction()->val(tp);
            cf->setStackId(out, stackId);
            cf->add(ir_stack_allocate).op(out).op(cf->imm(tp->getInfo().size)).op(cf->imm(stackId));

            constructObject(out, args);
            return out;
        }

        //
        // Data type resolution
        //
        
        DataType* Compiler::getArrayType(DataType* elemTp) {
            DataType* outTp = nullptr;
            String name = "Array<" + elemTp->getName() + ">";
            symbol* s = scope().getBase().get(name);

            if (s) {
                if (s->tp == st_type) outTp = s->type;
                else {
                    // todo: error
                }
            } else {
                String fullName = "Array<" + elemTp->getFullyQualifiedName() + ">";
                TemplateType* at = (TemplateType*)scope().getBase().get("Array");
                ast_node* arrayAst = at->getAST();
                Scope& tscope = scope().enter();

                // Add the array element type to the symbol table as the template argument name
                tscope.add(arrayAst->template_parameters->str(), elemTp);

                if (arrayAst->tp == nt_type) {
                    outTp = resolveTypeSpecifier(arrayAst->data_type);
                } else if (arrayAst->tp == nt_class) {
                    // todo
                    outTp = compileClass(arrayAst, true);
                }
                
                getOutput()->getModule()->addForeignType(outTp);

                scope().exit();
            }

            return outTp;
        }
        DataType* Compiler::getPointerType(DataType* destTp) {
            DataType* outTp = nullptr;
            String name = "Pointer<" + destTp->getName() + ">";
            symbol* s = scope().getBase().get(name);
            
            if (s) {
                if (s->tp == st_type) outTp = s->type;
                else {
                    // todo: error
                }
            } else {
                String fullName = "Pointer<" + destTp->getFullyQualifiedName() + ">";
                TemplateType* pt = (TemplateType*)scope().getBase().get("Pointer");
                ast_node* pointerAst = pt->getAST();
                Scope& tscope = scope().enter();

                // Add the pointer destination type to the symbol table as the template argument name
                tscope.add(pointerAst->template_parameters->str(), destTp);

                if (pointerAst->tp == nt_type) {
                    outTp = resolveTypeSpecifier(pointerAst->data_type);
                } else if (pointerAst->tp == nt_class) {
                    // todo
                    outTp = compileClass(pointerAst, true);
                }

                getOutput()->getModule()->addForeignType(outTp);

                scope().exit();
            }

            return outTp;
        }
        DataType* Compiler::resolveTemplateTypeSubstitution(ast_node* templateArgs, DataType* _type) {
            enterNode(templateArgs);
            if (!_type->getInfo().is_template) {
                // todo: errors

                exitNode();
                return nullptr;
            }

            TemplateType* type = (TemplateType*)_type;

            Scope& s = scope().enter();

            ast_node* tpAst = type->getAST();

            // Add template parameters to symbol table, representing the provided arguments
            // ...Also construct type name
            String name = _type->getName();
            String fullName = _type->getFullyQualifiedName();
            name += '<';
            fullName += '<';

            ast_node* n = tpAst->template_parameters;
            while (n) {
                enterNode(n);
                DataType* pt = resolveTypeSpecifier(templateArgs);
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
                    scope().exit();
                    exitNode();
                    return nullptr;
                }
                exitNode();
            }

            name += '>';
            fullName += '>';

            if (!n && templateArgs) {
                // todo: errors (too many template args)
                scope().exit();
                exitNode();
                return nullptr;
            }

            symbol* existing = scope().getBase().get(name);
            if (existing) {
                if (existing->tp != st_type) {
                    // todo: errors
                    scope().exit();
                    exitNode();
                    return nullptr;
                }

                scope().exit();
                exitNode();
                return existing->type;
            }

            DataType* tp = nullptr;
            if (tpAst->tp == nt_type) {
                tp = resolveTypeSpecifier(tpAst->data_type);

                tp = new AliasType(name, fullName, tp);
                scope().getBase().add(name, tp);
                addDataType(tp);
            } else if (tpAst->tp == nt_class) {
                tp = compileClass(tpAst, true);
            }
            
            scope().exit();
            exitNode();
            return tp;
        }
        DataType* Compiler::resolveObjectTypeSpecifier(ast_node* n) {
            enterNode(n);
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
                        DataType* pt = resolveTypeSpecifier(b->data_type);
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

            String name = String::Format("$anon_%d", getContext()->getTypes()->getNextAnonTypeIndex());
            DataType* tp = new DataType(
                name,
                name,
                info,
                props,
                {},
                dtor,
                {}
            );

            const Array<DataType*>& types = getContext()->getTypes()->allTypes();
            bool alreadyExisted = false;
            for (u32 i = 0;i < types.size();i++) {
                if (types[i]->isEquivalentTo(tp)) {
                    delete tp;
                    tp = types[i];
                    alreadyExisted = true;
                }
            }

            if (!alreadyExisted) {
                addDataType(tp);
            }

            exitNode();
            return tp;
        }
        DataType* Compiler::resolveFunctionTypeSpecifier(ast_node* n) {
            enterNode(n);
            DataType* retTp = resolveTypeSpecifier(n->data_type);
            DataType* ptrTp = getContext()->getTypes()->getType<void*>();
            Array<function_argument> args;
            args.push({ arg_type::func_ptr, ptrTp });
            args.push({ arg_type::ret_ptr, retTp });
            args.push({ arg_type::context_ptr, ptrTp });

            ast_node* a = n->parameters;
            while (a) {
                DataType* atp = resolveTypeSpecifier(a->data_type);
                args.push({
                    atp->getInfo().is_primitive ? arg_type::value : arg_type::pointer,
                    atp
                });
                a = a->next;
            }

            DataType* tp = new FunctionType(retTp, args);
            addDataType(tp);
            exitNode();
            return tp;
        }
        DataType* Compiler::resolveTypeNameSpecifier(ast_node* n) {
            String name = n->body->str();
            symbol* s = scope().get(name);

            if (n->body->template_parameters) {
                enterNode(n);
                DataType* tp = resolveTemplateTypeSubstitution(n->body->template_parameters, s->type);
                exitNode();
                return tp;
            }

            if (!s || s->tp != st_type) {
                // todo: errors
                return nullptr;
            }

            return s->type;
        }
        DataType* Compiler::applyTypeModifiers(DataType* tp, ast_node* mod) {
            if (!mod) return tp;
            enterNode(mod);
            DataType* outTp = nullptr;

            if (mod->flags.is_array) outTp = getArrayType(tp);
            else if (mod->flags.is_pointer) outTp = getPointerType(tp);

            outTp = applyTypeModifiers(outTp, mod->modifier);
            exitNode();
            return outTp;
        }
        DataType* Compiler::resolveTypeSpecifier(ast_node* n) {
            DataType* tp = nullptr;
            if (n->body && n->body->tp == nt_type_property) tp = resolveObjectTypeSpecifier(n);
            else if (n->parameters) tp = resolveFunctionTypeSpecifier(n);
            else tp = resolveTypeNameSpecifier(n);

            return applyTypeModifiers(tp, n->modifier);
        }


        Value Compiler::generateCall(ffi::Function* fn, const utils::Array<Value>& args, const Value* self) {
            return currentFunction()->getPoison();
        }

        Value Compiler::generateCall(FunctionDef* fn, const utils::Array<Value>& args, const Value* self) {
            return currentFunction()->getPoison();
        }

        DataType* Compiler::compileType(ast_node* n) {
            DataType* tp = nullptr;
            String name = n->str();

            if (n->template_parameters) {
                tp = new TemplateType(name, getOutput()->getModule()->getName() + "::" + name, n->clone());
                addDataType(tp);
                scope().getBase().add(name, tp);
            } else {
                tp = resolveTypeSpecifier(n->data_type);

                if (tp) {
                    tp = new AliasType(name, getOutput()->getModule()->getName() + "::" + name, tp);
                    addDataType(tp);
                    scope().getBase().add(name, tp);
                }
            }

            return tp;
        }
        void Compiler::compileMethodDef(ast_node* n, DataType* methodOf, Method* m) {
            updateMethod(methodOf, m);
            
            FunctionType* sig = m->getSignature();
            const auto& args = sig->getArguments();
            ast_node* p = n->parameters;

            enterNode(n->body);

            FunctionDef* fd = getOutput()->newFunc(m);
            enterFunction(fd);

            while (p) {
                fd->addArg(p->str(), resolveTypeSpecifier(p->data_type));
                p = p->next;
            }

            compileBlock(n->body);

            exitFunction();
            exitNode();
        }
        Method* Compiler::compileMethodDecl(ast_node* n, u64 thisOffset, bool* wasDtor, bool templatesDefined, bool dtorExists) {
            enterNode(n);
            utils::String name = n->str();
            DataType* voidTp = getContext()->getTypes()->getType<void>();
            DataType* ptrTp = getContext()->getTypes()->getType<void*>();

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
                        n->clone()
                    );
                    addFunction(m);
                    exitNode();
                    return m;
                }

                name += '<';

                ast_node* arg = n->template_parameters;
                while (arg) {
                    symbol* s = scope().get(arg->str());
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
                    DataType* castTarget = resolveTypeSpecifier(n->data_type);
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
                ret = resolveTypeSpecifier(n->data_type);
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
                args.push({ tp, resolveTypeSpecifier(p->data_type) });

                p = p->next;
                a++;
            }

            FunctionType* sig = new FunctionType(ret, args);
            const auto& types = getContext()->getTypes()->allTypes();
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
                addDataType(sig);
            }

            Method* m = new Method(
                name,
                sig,
                n->flags.is_private ? private_access : public_access,
                nullptr,
                nullptr,
                thisOffset
            );

            addFunction(m);
            exitNode();
            return m;
        }
        DataType* Compiler::compileClass(ast_node* n, bool templatesDefined) {
            enterNode(n);
            utils::String name = n->str();
            String fullName = getOutput()->getModule()->getName() + "::" + name;
            
            if (n->template_parameters) {
                if (!templatesDefined) {
                    DataType* tp = new TemplateType(name, fullName, n->clone());
                    addDataType(tp);
                    scope().add(name, tp);
                    exitNode();
                    return tp;
                }

                name += '<';
                fullName += '<';

                ast_node* arg = n->template_parameters;
                while (arg) {
                    symbol* s = scope().get(arg->str());
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

            ClassType* tp = new ClassType(name, fullName);
            m_curClass = tp;
            addDataType(tp);
            scope().getBase().add(name, tp);
            
            ast_node* inherits = n->inheritance;
            while (inherits) {
                // todo: Check added properties don't have name collisions with
                //       existing properties

                DataType* t = resolveTypeSpecifier(inherits);
                tp->addBase(t, public_access);
                inherits = inherits->next;
            }

            ast_node* prop = n->body;
            u64 thisOffset = tp->getInfo().size;
            while (prop) {
                if (prop->tp != nt_property) {
                    prop = prop->next;
                    continue;
                }

                value_flags f = { 0 };
                f.can_read = 1;
                f.can_write = (!prop->flags.is_const) ? 1 : 0;
                f.is_pointer = prop->flags.is_pointer;
                f.is_static = prop->flags.is_static;

                DataType* t = resolveTypeSpecifier(prop->data_type);
                tp->addProperty(
                    prop->str(),
                    t,
                    f,
                    prop->flags.is_private ? private_access : public_access,
                    nullptr,
                    nullptr
                );

                prop = prop->next;
            }

            ast_node* meth = n->body;
            while (meth) {
                if (meth->tp != nt_function) {
                    meth = meth->next;
                    continue;
                }

                bool isDtor = false;
                Method* m = compileMethodDecl(meth, thisOffset, &isDtor, false, tp->getDestructor() != nullptr);

                if (isDtor) tp->setDestructor(m);
                else tp->addMethod(m);

                meth = meth->next;
            }

            u32 midx = 0;
            const auto& methods = tp->getMethods();
            meth = n->body;
            while (meth) {
                if (meth->tp != nt_function) {
                    meth = meth->next;
                    continue;
                }

                if (!methods[midx]->isTemplate()) {
                    compileMethodDef(meth, tp, (Method*)methods[midx++]);
                }
                
                meth = meth->next;
            }
            m_curClass = nullptr;
            exitNode();

            return tp;
        }
        Function* Compiler::compileFunction(ast_node* n, bool templatesDefined) {
            enterNode(n);
            utils::String name = n->str();
            String fullName = getOutput()->getModule()->getName() + "::" + name;
            
            if (n->template_parameters) {
                if (!templatesDefined) {
                    TemplateFunction* f = new TemplateFunction(
                        name,
                        n->flags.is_private ? private_access : public_access,
                        n->clone()
                    );
                    addFunction(f);
                    scope().add(name, f);
                    exitNode();
                    return f;
                }

                name += '<';

                ast_node* arg = n->template_parameters;
                while (arg) {
                    symbol* s = scope().get(arg->str());
                    // symbol for template parameter should always be defined here

                    DataType* pt = s->type;
                    name += pt->getFullyQualifiedName();
                    if (arg != n->template_parameters) name += ", ";

                    arg = arg->next;
                }

                name += '>';
            }

            FunctionDef* fd = getOutput()->newFunc(name);

            enterFunction(fd);

            if (n->data_type) {
                fd->setReturnType(resolveTypeSpecifier(n->data_type));
            }

            ast_node* p = n->parameters;
            while (p) {
                fd->addArg(p->str(), resolveTypeSpecifier(p->data_type));
                p = p->next;
            }

            compileBlock(n->body);
            
            Function* out = exitFunction();
            exitNode();

            return out;
        }
        void Compiler::compileExport(ast_node* n) {
            if (!inInitFunction()) {
                // todo: error
            }

            if (n->body->tp == nt_type) {
                compileType(n->body)->setAccessModifier(public_access);
                return;
            } else if (n->body->tp == nt_function) {
                compileFunction(n->body)->setAccessModifier(public_access);
                return;
            } else if (n->body->tp == nt_variable) {
                Value& var = compileVarDecl(n->body);
                m_output->getModule()->setDataAccess(var.getImm<u32>(), public_access);
                return;
            } else if (n->body->tp == nt_class) {
                compileClass(n->body)->setAccessModifier(public_access);
                return;
            }

            // todo: error
        }
        void Compiler::compileImport(ast_node* n) {

        }
        Value Compiler::compileConditionalExpr(ast_node* n) {
            FunctionDef* cf = currentFunction();

            scope().enter();
            Value cond = compileExpression(n->cond);
            Value condBool = cond.convertedTo(m_ctx->getTypes()->getType<bool>());
            scope().exit(condBool);

            auto reserve = add(ir_reserve);
            auto branch = add(ir_branch).op(condBool);

            // Truth body
            scope().enter();
            branch.label(cf->label());
            Value lvalue = compileExpressionInner(n->lvalue);

            // jumps to just after the false body
            auto jump = add(ir_jump);

            // Output must be created now since only now do we have the
            // output type. No instructions are emitted as a result of
            // this so this is not contingent upon the condition being
            // evaluated to true
            DataType* tp = lvalue.getType();
            Value out = cf->val(tp);

            if (!tp->getInfo().is_primitive) {
                // If the result is not a primitive type then the output
                // of the conditional must be allocated on the stack and
                // constructed from the resulting value
                // 
                // That being the case, the resolve instruction should
                // be replaced with a stack_allocate instruction and no
                // resolve instruction should be added
                // 
                // The stack id must be set now (prior to any uses of
                // result), but it must be added to the parent block's
                // stack
                alloc_id stackId = cf->reserveStackId();
                cf->setStackId(out, stackId);
                reserve.instr(ir_stack_allocate).op(out).op(cf->imm(tp->getInfo().size)).op(cf->imm(stackId));

                constructObject(out, { lvalue });
                scope().exit(out);
            } else {
                reserve.op(out);
                add(ir_resolve).op(out).op(lvalue);
                scope().exit();
            }
            // End truth body

            // False body
            branch.label(cf->label());
            scope().enter();
            
            Value rvalue = compileExpressionInner(n->rvalue);

            if (!tp->getInfo().is_primitive) constructObject(out, { rvalue });
            else add(ir_resolve).op(out).op(rvalue);

            scope().exit();
            // End false body

            // join address
            jump.label(cf->label());

            return out;
        }
        Value Compiler::compileMemberExpr(ast_node* n) {
            FunctionDef* cf = currentFunction();
            // Possibilities:
            // [type].[static var]
            // [type].[method] (not a call)
            // [module].[type].[static var]
            // [module].[global var]
            // [expression].[property]
            // [var].[property]

            if (n->lvalue->tp == nt_identifier) {
                // Possibilities:
                // [type].[static var]
                // [type].[method] (not a call)
                // [module].[global var]
                // [var].[property]

                symbol *s = scope().get(n->lvalue->str());
                if (!s) {
                    // todo: errors
                    return cf->getPoison();
                }

                if (s->tp == st_type) {
                    // Possibilities:
                    // [type].[static var]
                    // [type].[method] (not a call)
                    DataType* tp = s->type;
                    String propName = n->rvalue->str();
                    const type_property* prop = tp->getProp(propName);

                    // [type].[static var]
                    if (prop) {
                        if (prop->access == private_access && (!m_curClass || !m_curClass->isEqualTo(tp))) {
                            // todo: errors
                            return cf->getPoison();
                        }

                        if (!prop->flags.is_static) {
                            // todo: errors
                            return cf->getPoison();
                        }

                        if (!prop->flags.can_read) {
                            // todo: errors
                            return cf->getPoison();
                        }

                        if (prop->getter) {
                            return generateCall(prop->getter, {});
                        }

                        Value out = cf->val(prop->type);

                        if (prop->type->getInfo().is_primitive) {
                            add(ir_load).op(out).op(cf->imm<u64>(prop->offset));
                            if (prop->flags.is_pointer) add(ir_load).op(out).op(out);
                        } else {
                            if (prop->flags.is_pointer) {
                                add(ir_load).op(out).op(cf->imm<u64>(prop->offset));
                            } else {
                                add(ir_assign).op(out).op(cf->imm<u64>(prop->offset));
                            }
                        }

                        return out;
                    }

                    // [type].[method] (not a call)
                    Array<Function*> methods = tp->findMethods(propName, nullptr, nullptr, 0, fm_ignore_args);
                    if (methods.size() == 0) {
                        // todo: errors
                        return cf->getPoison();
                    } else if (methods.size() > 1) {
                        // todo: errors
                        return cf->getPoison();
                    } else {
                        return cf->imm(methods[0]);
                    }
                }

                if (s->tp == st_module) {
                    // Possibilities:
                    // [module].[global var]

                    Module* mod = s->mod;
                    String propName = n->rvalue->str();
                    i64 slotId = mod->getData().findIndex([&propName](const module_data& d) { return d.name == propName; });
                    if (slotId >= 0) {
                        const module_data& data = mod->getDataInfo((u32)slotId);
                    
                        Value out = cf->val(data.type);
                        add(ir_module_data).op(out).op(cf->imm(mod->getId())).op(cf->imm((u32)slotId));
                        if (data.type->getInfo().is_primitive) {
                            add(ir_load).op(out).op(out);
                        }
                        return out;
                    }

                    // todo: errors
                    return cf->getPoison();
                }

                if (s->tp == st_variable) {
                    // Possibilities:
                    // [var].[property]
                    
                    Value lv = *s->var;
                    String propName = n->rvalue->str();
                    const type_property* prop = lv.getType()->getProp(propName);

                    if (!prop) {
                        // todo: errors
                        return cf->getPoison();
                    }

                    if (prop->access == private_access && (!m_curClass || !m_curClass->isEqualTo(lv.getType()))) {
                        // todo: errors
                        return cf->getPoison();
                    }

                    if (prop->flags.is_static) {
                        // todo: errors
                        return cf->getPoison();
                    }

                    if (!prop->flags.can_read) {
                        // todo: errors
                        return cf->getPoison();
                    }

                    if (prop->getter) {
                        return generateCall(prop->getter, {}, &lv);
                    }

                    Value out = cf->val(prop->type);
                    add(ir_uadd).op(out).op(lv).op(cf->imm(prop->offset));

                    if (prop->type->getInfo().is_primitive || prop->flags.is_pointer) {
                        add(ir_load).op(out).op(out);
                    }
                    
                    if (prop->type->getInfo().is_primitive && prop->flags.is_pointer) {
                        // load primitive from pointer to primitive
                        add(ir_load).op(out).op(out);
                    }

                    return out;
                }
                
                // todo: errors
                return cf->getPoison();
            } else if (n->lvalue->tp == nt_expression && n->lvalue->op == op_member && n->lvalue->lvalue->tp == nt_identifier) {
                // Possibilities:
                // [module].[type].[static var]
                // [expression].[property]

                symbol *s = scope().get(n->lvalue->str());
                if (s) {
                    // Possibilities:
                    // [module].[type].[static var]

                    if (s->tp == st_module) {
                        Module* mod = s->mod;
                        String propName = n->rvalue->str();
                        DataType* tp = mod->allTypes().find([&propName](DataType* tp) { return tp->getName() == propName; });
                        if (tp) {
                            String propName = n->rvalue->str();
                            const type_property* prop = tp->getProp(propName);

                            // [module].[type].[static var]
                            if (prop) {
                                if (prop->access == private_access && (!m_curClass || !m_curClass->isEqualTo(tp))) {
                                    // todo: errors
                                    return cf->getPoison();
                                }

                                if (!prop->flags.is_static) {
                                    // todo: errors
                                    return cf->getPoison();
                                }

                                if (!prop->flags.can_read) {
                                    // todo: errors
                                    return cf->getPoison();
                                }

                                if (prop->getter) {
                                    return generateCall(prop->getter, {});
                                }

                                Value out = cf->val(prop->type);

                                if (prop->type->getInfo().is_primitive) {
                                    add(ir_load).op(out).op(cf->imm<u64>(prop->offset));
                                    if (prop->flags.is_pointer) add(ir_load).op(out).op(out);
                                } else {
                                    if (prop->flags.is_pointer) {
                                        add(ir_load).op(out).op(cf->imm<u64>(prop->offset));
                                    } else {
                                        add(ir_assign).op(out).op(cf->imm<u64>(prop->offset));
                                    }
                                }

                                return out;
                            }

                            // todo: errors
                            return cf->getPoison();
                        } else {
                            // todo: errors
                            return cf->getPoison();
                        }
                    }

                    // todo: errors
                    return cf->getPoison();
                }
            }

            // Possibilities:
            // [expression].[property]

            Value lv = compileExpressionInner(n->lvalue);
            return lv.getProp(n->rvalue->str());
        }
        Value Compiler::compileArrowFunction(ast_node* n) {
            // todo
            return currentFunction()->getPoison();
        }
        Value Compiler::compileExpressionInner(ast_node* n) {
            if (n->tp == nt_identifier) {
                symbol* s = scope().get(n->str());
                if (s && s->tp == st_variable) {
                    return *s->var;
                }

                // todo: error
                return currentFunction()->getPoison();
            } else if (n->tp == nt_literal) {
                switch (n->value_tp) {
                    case lt_u8: return currentFunction()->imm<u8>(n->value.u);
                    case lt_u16: return currentFunction()->imm<u16>(n->value.u);
                    case lt_u32: return currentFunction()->imm<u32>(n->value.u);
                    case lt_u64: return currentFunction()->imm<u64>(n->value.u);
                    case lt_i8: return currentFunction()->imm<i8>(n->value.i);
                    case lt_i16: return currentFunction()->imm<i16>(n->value.i);
                    case lt_i32: return currentFunction()->imm<i32>(n->value.i);
                    case lt_i64: return currentFunction()->imm<i64>(n->value.i);
                    case lt_f32: return currentFunction()->imm<f32>(n->value.f);
                    case lt_f64: return currentFunction()->imm<f64>(n->value.f);
                    case lt_string: {
                        return currentFunction()->getPoison(); // todo
                    }
                    case lt_template_string: {
                        return currentFunction()->getPoison(); // todo
                    }
                    case lt_object: {
                        return currentFunction()->getPoison(); // todo
                    }
                    case lt_array: {
                        return currentFunction()->getPoison(); // todo
                    }
                    case lt_null: {
                        return currentFunction()->getPoison(); // todo
                    }
                    case lt_true: return currentFunction()->imm(true);
                    case lt_false: return currentFunction()->imm(false);
                }

                return currentFunction()->getPoison();
            } else if (n->tp == nt_sizeof) {
                DataType* tp = resolveTypeSpecifier(n->data_type);
                if (tp) return currentFunction()->imm(tp->getInfo().size);

                return currentFunction()->getPoison();
            } else if (n->tp == nt_this) {
                if (!m_curClass) {
                    // todo: error
                    return currentFunction()->getPoison();
                }

                return currentFunction()->getThis();
            } else if (n->tp == nt_function) return compileArrowFunction(n);

            // todo: expression sequence

            switch (n->op) {
                case op_undefined: break;
                case op_add: return compileExpressionInner(n->lvalue) + compileExpressionInner(n->rvalue);
                case op_addEq: return compileExpressionInner(n->lvalue) += compileExpressionInner(n->rvalue);
                case op_sub: return compileExpressionInner(n->lvalue) - compileExpressionInner(n->rvalue);
                case op_subEq: return compileExpressionInner(n->lvalue) -= compileExpressionInner(n->rvalue);
                case op_mul: return compileExpressionInner(n->lvalue) * compileExpressionInner(n->rvalue);
                case op_mulEq: return compileExpressionInner(n->lvalue) *= compileExpressionInner(n->rvalue);
                case op_div: return compileExpressionInner(n->lvalue) / compileExpressionInner(n->rvalue);
                case op_divEq: return compileExpressionInner(n->lvalue) /= compileExpressionInner(n->rvalue);
                case op_mod: return compileExpressionInner(n->lvalue) % compileExpressionInner(n->rvalue);
                case op_modEq: return compileExpressionInner(n->lvalue) %= compileExpressionInner(n->rvalue);
                case op_xor: return compileExpressionInner(n->lvalue) ^ compileExpressionInner(n->rvalue);
                case op_xorEq: return compileExpressionInner(n->lvalue) ^= compileExpressionInner(n->rvalue);
                case op_bitAnd: return compileExpressionInner(n->lvalue) & compileExpressionInner(n->rvalue);
                case op_bitAndEq: return compileExpressionInner(n->lvalue) &= compileExpressionInner(n->rvalue);
                case op_bitOr: return compileExpressionInner(n->lvalue) | compileExpressionInner(n->rvalue);
                case op_bitOrEq: return compileExpressionInner(n->lvalue) |= compileExpressionInner(n->rvalue);
                case op_bitInv: return ~compileExpressionInner(n->lvalue);
                case op_shLeft: return compileExpressionInner(n->lvalue) << compileExpressionInner(n->rvalue);
                case op_shLeftEq: return compileExpressionInner(n->lvalue) <<= compileExpressionInner(n->rvalue);
                case op_shRight: return compileExpressionInner(n->lvalue) >> compileExpressionInner(n->rvalue);
                case op_shRightEq: return compileExpressionInner(n->lvalue) >>= compileExpressionInner(n->rvalue);
                case op_not: return !compileExpressionInner(n->lvalue);
                case op_notEq: return compileExpressionInner(n->lvalue) != compileExpressionInner(n->rvalue);
                case op_logAnd: return compileExpressionInner(n->lvalue) && compileExpressionInner(n->rvalue);
                case op_logAndEq: return compileExpressionInner(n->lvalue).operator_logicalAndAssign(compileExpressionInner(n->rvalue));
                case op_logOr: return compileExpressionInner(n->lvalue) || compileExpressionInner(n->rvalue);
                case op_logOrEq: return compileExpressionInner(n->lvalue).operator_logicalOrAssign(compileExpressionInner(n->rvalue));
                case op_assign: return compileExpressionInner(n->lvalue) = compileExpressionInner(n->rvalue);
                case op_compare: return compileExpressionInner(n->lvalue) == compileExpressionInner(n->rvalue);
                case op_lessThan: return compileExpressionInner(n->lvalue) < compileExpressionInner(n->rvalue);
                case op_lessThanEq: return compileExpressionInner(n->lvalue) <= compileExpressionInner(n->rvalue);
                case op_greaterThan: return compileExpressionInner(n->lvalue) > compileExpressionInner(n->rvalue);
                case op_greaterThanEq: return compileExpressionInner(n->lvalue) >= compileExpressionInner(n->rvalue);
                case op_preInc: return ++compileExpressionInner(n->lvalue);
                case op_postInc: return compileExpressionInner(n->lvalue)++;
                case op_preDec: return --compileExpressionInner(n->lvalue);
                case op_postDec: return compileExpressionInner(n->lvalue)--;
                case op_negate: return -compileExpressionInner(n->lvalue);
                case op_index: return compileExpressionInner(n->lvalue)[compileExpressionInner(n->rvalue)];
                case op_conditional: return compileConditionalExpr(n);
                case op_member: return compileMemberExpr(n);
                case op_new: {
                    DataType* type = resolveTypeSpecifier(n->data_type);

                    Array<Value> args;
                    ast_node* p = n->parameters;
                    while (p) {
                        args.push(compileExpression(p));
                        p = p->next;
                    }

                    return constructObject(type, args);
                }
                case op_placementNew: {
                    DataType* type = resolveTypeSpecifier(n->data_type);

                    Array<Value> args;
                    ast_node* p = n->parameters;
                    while (p) {
                        args.push(compileExpression(p));
                        p = p->next;
                    }

                    Value dest = compileExpressionInner(n->lvalue);
                    constructObject(dest, type, args);
                    return dest;
                }
                case op_call: {
                    Array<Value> args;
                    ast_node* p = n->parameters;
                    while (p) {
                        args.push(compileExpression(p));
                        p = p->next;
                    }

                    return compileExpressionInner(n->lvalue)(args);
                }
            }

            return currentFunction()->getPoison();
        }
        Value Compiler::compileExpression(ast_node* n) {
            enterNode(n);
            scope().enter();

            Value out = compileExpressionInner(n);
            
            scope().exit(out);
            exitNode();

            return out;
        }
        void Compiler::compileIfStatement(ast_node* n) {
            FunctionDef* cf = currentFunction();
            enterNode(n);

            scope().enter();
            Value cond = compileExpression(n->cond);
            Value condBool = cond.convertedTo(m_ctx->getTypes()->getType<bool>());
            scope().exit(condBool);

            auto branch = add(ir_branch).op(condBool).label(cf->label());
            
            scope().enter();
            compileAny(n->body);
            scope().exit();

            if (n->else_body) {
                auto jump = add(ir_jump);

                branch.label(cf->label());

                
                scope().enter();
                compileAny(n->else_body);
                scope().exit();

                jump.label(cf->label());
            } else branch.label(cf->label());

            exitNode();
        }
        void Compiler::compileReturnStatement(ast_node* n) {
            enterNode(n);
            DataType* retTp = currentFunction()->getReturnType();

            if (n->body) {
                Value ret = compileExpression(n->body);

                if (retTp) {
                    add(ir_ret).op(ret.convertedTo(retTp));
                } else {
                    currentFunction()->setReturnType(ret.getType());
                    add(ir_ret).op(ret);
                }

                exitNode();
                return;
            }

            if (retTp) {
                // todo: errors
                add(ir_ret).op(currentFunction()->getPoison());
                exitNode();
                return;
            }

            currentFunction()->setReturnType(m_ctx->getTypes()->getType<void>());
            add(ir_ret);
            exitNode();
        }
        void Compiler::compileSwitchStatement(ast_node* n) {
            // todo
        }
        void Compiler::compileThrow(ast_node* n) {
            // todo
        }
        void Compiler::compileTryBlock(ast_node* n) {
            // todo
        }
        Value& Compiler::compileVarDecl(ast_node* n) {
            enterNode(n);
            Value* v = nullptr;
            Value init = n->initializer ? compileExpression(n->initializer) : currentFunction()->getPoison();
            DataType* dt = nullptr;

            if (n->body->data_type) dt = resolveTypeSpecifier(n->body->data_type);
            else {
                if (!n->initializer) {
                    // todo: error (unable to determine type)
                    return currentFunction()->getPoison();
                }

                dt = init.getType();
            }

            if (inInitFunction()) {
                u32 slot = m_output->getModule()->addData(n->body->str(), dt, private_access);
                v = &currentFunction()->val(n->body->str(), slot);
            } else {
                v = &currentFunction()->val(n->body->str(), dt);
            }

            if (n->initializer) {
                if (dt->getInfo().is_primitive) *v = init.convertedTo(dt);
                else {
                    Value initWith = init;
                    const DataType* initWithTp = initWith.getType();

                    constructObject(*v, { initWith });

                    // call constructor
                    Array<Function *> ctors = dt->findMethods("constructor", nullptr, &initWithTp, 1, fm_skip_implicit_args);
                    if (ctors.size() == 1) {
                        generateCall(ctors[0], { initWith }, v);
                    } else if (ctors.size() > 1) {
                        u8 strictC = 0;
                        ffi::Function* func = nullptr;
                        for (u32 i = 0;i < ctors.size();i++) {
                            const auto& args = ctors[i]->getSignature()->getArguments();
                            for (u32 a = 0;a < args.size();a++) {
                                if (args[a].isImplicit()) continue;
                                if (args[a].dataType->isEqualTo(initWithTp)) {
                                    if (args.size() > a + 1) {
                                        // Function has more arguments which are not explicitly required
                                        break;
                                    }

                                    strictC++;
                                    func = ctors[i];
                                }
                            }
                        }
                        
                        if (strictC == 1) {
                            generateCall(func, { initWith }, v);
                        }

                        // todo: Error, ambiguous call
                    } else {
                        // todo: Error, no matches
                    }
                }
            } else if (!dt->getInfo().is_primitive) {
                Array<Function *> ctors = dt->findMethods("constructor", nullptr, nullptr, 0, fm_skip_implicit_args);
                if (ctors.size() == 1) {
                    generateCall(ctors[0], {}, v);
                } else if (ctors.size() > 1) {
                    // todo: Error, ambiguous call
                } else {
                    // todo: Error, no default constructor
                }
            }

            exitNode();
            return *v;
        }
        void Compiler::compileObjectDecompositor(ast_node* n) {
            enterNode(n);
            FunctionDef* cf = currentFunction();

            Value source = compileExpression(n->initializer);
            
            ast_node* p = n->body;
            while (p) {
                enterNode(p);
                Value init = source.getProp(p->str());

                DataType* dt = p->data_type ? resolveTypeSpecifier(p->data_type) : init.getType();
                Value* v = nullptr;

                if (inInitFunction()) {
                    u32 slot = m_output->getModule()->addData(n->body->str(), dt, private_access);
                    v = &currentFunction()->val(p->str(), slot);
                } else {
                    v = &currentFunction()->val(p->str(), dt);
                }

                if (dt->getInfo().is_primitive) *v = init.convertedTo(dt);
                else {
                    Value initWith = init;
                    const DataType* initWithTp = initWith.getType();

                    constructObject(*v, { initWith });

                    // call constructor
                    Array<Function *> ctors = dt->findMethods("constructor", nullptr, &initWithTp, 1, fm_skip_implicit_args);
                    if (ctors.size() == 1) {
                        generateCall(ctors[0], { initWith }, v);
                    } else if (ctors.size() > 1) {
                        u8 strictC = 0;
                        ffi::Function* func = nullptr;
                        for (u32 i = 0;i < ctors.size();i++) {
                            const auto& args = ctors[i]->getSignature()->getArguments();
                            for (u32 a = 0;a < args.size();a++) {
                                if (args[a].isImplicit()) continue;
                                if (args[a].dataType->isEqualTo(initWithTp)) {
                                    if (args.size() > a + 1) {
                                        // Function has more arguments which are not explicitly required
                                        break;
                                    }

                                    strictC++;
                                    func = ctors[i];
                                }
                            }
                        }
                        
                        if (strictC == 1) {
                            generateCall(func, { initWith }, v);
                        }

                        // todo: Error, ambiguous call
                    } else {
                        // todo: Error, no matches
                    }
                }

                exitNode();
                p = p->next;
            }

            exitNode();
        }
        void Compiler::compileLoop(ast_node* n) {
            FunctionDef* cf = currentFunction();
            enterNode(n);
            Scope& s = scope().enter();

            // initializer will either be expression or variable decl
            if (n->initializer) compileAny(n->initializer);

            label_id loop_begin = cf->label();
            m_lsStack.push({
                true,       // is_loop
                loop_begin, // loop_begin
                {},         // pending_end_label
                &s          // outer_scope
            });

            if (n->flags.defer_cond == 0) {
                scope().enter();
                Value cond = compileExpression(n->cond);
                Value condBool = cond.convertedTo(m_ctx->getTypes()->getType<bool>());
                scope().exit(condBool);

                m_lsStack.last().pending_end_label.push(add(ir_branch).op(condBool).label(cf->label()));

                if (n->modifier) {
                    scope().enter();
                    compileExpression(n->modifier);
                    scope().exit();
                }

                scope().enter();
                compileAny(n->body);
                scope().exit();

                add(ir_jump).label(loop_begin);
            } else {
                if (n->modifier) {
                    scope().enter();
                    compileExpression(n->modifier);
                    scope().exit();
                }

                scope().enter();
                compileAny(n->body);
                scope().exit();

                scope().enter();
                Value cond = compileExpression(n->cond);
                Value condBool = cond.convertedTo(m_ctx->getTypes()->getType<bool>());
                scope().exit(condBool);

                m_lsStack.last().pending_end_label.push(add(ir_branch).op(condBool).label(loop_begin));
            }

            label_id loop_end = cf->label();
            m_lsStack.last().pending_end_label.each([loop_end](InstructionRef& i) {
                i.label(loop_end);
            });
            m_lsStack.pop();

            scope().exit();
            exitNode();
        }
        void Compiler::compileContinue(ast_node* n) {
            if (m_lsStack.size() == 0) {
                // todo: error
                return;
            }

            auto& ls = m_lsStack.last();
            if (!ls.is_loop) {
                // todo: error
                return;
            }

            scope().emitScopeExitInstructions(*ls.outer_scope);
            add(ir_jump).label(ls.loop_begin);
        }
        void Compiler::compileBreak(ast_node* n) {
            if (m_lsStack.size() == 0) {
                // todo: error
                return;
            }
            auto& ls = m_lsStack.last();

            scope().emitScopeExitInstructions(*ls.outer_scope);
            ls.pending_end_label.push(add(ir_jump));
        }
        void Compiler::compileBlock(ast_node* n) {
            scope().enter();

            n = n->body;
            while (n) {
                compileAny(n);
                n = n->next;
            }

            scope().exit();
        }
        void Compiler::compileAny(ast_node* n) {
            switch (n->tp) {
                case nt_break: return compileBreak(n);
                case nt_class: {
                    if (!inInitFunction()) {
                        // todo: errors
                        return;
                    }

                    compileClass(n);
                    return;
                }
                case nt_continue: return compileContinue(n);
                case nt_export: {
                    if (!inInitFunction()) {
                        // todo: errors
                        return;
                    }

                    return compileExport(n);
                }
                case nt_expression: return (void)compileExpression(n);
                case nt_function: {
                    if (!inInitFunction()) {
                        // todo: errors
                        return;
                    }

                    compileFunction(n);
                    return;
                }
                case nt_if: return compileIfStatement(n);
                case nt_loop: return compileLoop(n);
                case nt_return: return compileReturnStatement(n);
                case nt_scoped_block: return compileBlock(n);
                case nt_switch: return compileSwitchStatement(n);
                case nt_throw: return compileThrow(n);
                case nt_try: return compileTryBlock(n);
                case nt_type: {
                    if (!inInitFunction()) {
                        // todo: errors
                        return;
                    }

                    compileType(n);
                    return;
                }
                case nt_variable: {
                    if (n->body->tp == nt_object_decompositor) {
                        compileObjectDecompositor(n);
                        return;
                    }

                    compileVarDecl(n);
                    return;
                }
                default: break;
            }
        }
    };
};