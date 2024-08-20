#include <tsn/builtin/Builtin.h>
#include <tsn/common/Context.h>
#include <tsn/common/Module.h>
#include <tsn/ffi/DataTypeRegistry.h>
#include <tsn/ffi/FunctionRegistry.hpp>
#include <tsn/interfaces/IFunctionHolder.hpp>
#include <tsn/bind/bind.hpp>
#include <tsn/compiler/Logger.h>
#include <tsn/compiler/Compiler.h>
#include <tsn/compiler/FunctionDef.hpp>

#include <utils/Array.hpp>

using namespace tsn::ffi;
using namespace tsn::compiler;
using namespace utils;

namespace tsn {
    #define DATA_OFFSET 0
    #define REFS_OFFSET sizeof(void*)
    #define EXTERNAL_OFFSET (sizeof(void*) * 2)

    void BindPointer(Context* ctx) {
        TemplateType* ptr = new TemplateType(ctx->getGlobal(), "Pointer", "::Pointer", 1, +[](ffi::DataType* tp, ffi::DataTypeExtender* e, Pipeline* p) {
            Context* ctx = tp->getSource()->getContext();
            DataTypeRegistry* treg = ctx->getTypes();

            auto ptype = tp->getTemplateArguments()[0];

            auto& info = e->info();
            info.size = sizeof(void*) + sizeof(u32*) + sizeof(bool);
            info.is_pod = 0;
            info.is_trivially_constructible = 0;
            info.is_trivially_copyable = 0;
            info.is_trivially_destructible = 0;

            e->addProperty("_data", ptype, DATA_OFFSET, vf_pointer | vf_rw, private_access);
            e->addProperty("_refs", treg->getUInt32(), REFS_OFFSET, vf_pointer | vf_rw, private_access);
            e->addProperty("_external", treg->getBoolean(), EXTERNAL_OFFSET, vf_rw, private_access);
            e->addMethod<void>("constructor", +[](compiler::InlineCodeGenContext* ctx) {
                auto c = ctx->compiler;
                auto cf = c->currentFunction();
                auto self = ctx->selfPointer;
                auto args = ctx->arguments;

                cf->add(ir_store).op(cf->getNull()).op(*self).op(cf->imm<u32>(DATA_OFFSET));
                cf->add(ir_store).op(cf->getNull()).op(*self).op(cf->imm<u32>(REFS_OFFSET));
                cf->add(ir_store).op(cf->imm(false)).op(*self).op(cf->imm<u32>(EXTERNAL_OFFSET));
            });
            e->addMethod<void, tsn::null_t>("constructor", +[](compiler::InlineCodeGenContext* ctx) {
                auto c = ctx->compiler;
                auto cf = c->currentFunction();
                auto self = ctx->selfPointer;
                auto args = ctx->arguments;

                cf->add(ir_store).op(cf->getNull()).op(*self).op(cf->imm<u32>(DATA_OFFSET));
                cf->add(ir_store).op(cf->getNull()).op(*self).op(cf->imm<u32>(REFS_OFFSET));
                cf->add(ir_store).op(cf->imm(false)).op(*self).op(cf->imm<u32>(EXTERNAL_OFFSET));
            });
            e->addMethod(
                "constructor",
                treg->getVoid(), false,
                {
                    { ptype->getInfo().is_primitive ? arg_type::value : arg_type::pointer, ptype }
                },
                +[](compiler::InlineCodeGenContext* ctx) {
                    auto c = ctx->compiler;
                    auto cf = c->currentFunction();
                    auto self = ctx->selfPointer;
                    auto targs = self->getType()->getTemplateArguments();
                    auto args = ctx->arguments;

                    Function* newMem = c->getContext()->getModule("<host>/memory.tsn")->allFunctions().find([](const ffi::Function* fn) {
                        return fn && fn->getName() == "newMem";
                    });

                    Value data = c->generateCall(newMem, { cf->imm<u32>(targs[0]->getInfo().size) });
                    c->constructObject(data, targs[0], { args[0] });
                    cf->add(ir_store).op(data).op(*self).op(cf->imm<u32>(DATA_OFFSET));

                    Value refs = c->generateCall(newMem, { cf->imm<u32>(sizeof(u32)) });
                    cf->add(ir_store).op(cf->imm<u32>(1)).op(refs);
                    cf->add(ir_store).op(refs).op(*self).op(cf->imm<u32>(REFS_OFFSET));
                    cf->add(ir_store).op(cf->imm(false)).op(*self).op(cf->imm<u32>(EXTERNAL_OFFSET));
                }
            );
            e->addMethod(
                "constructor",
                treg->getVoid(), false,
                {
                    {arg_type::pointer, tp }
                },
                +[](compiler::InlineCodeGenContext* ctx) {
                    auto c = ctx->compiler;
                    auto cf = c->currentFunction();
                    auto self = ctx->selfPointer;
                    auto targs = self->getType()->getTemplateArguments();
                    auto args = ctx->arguments;

                    Value data = args[0].getProp("_data");
                    c->constructObject(data, targs[0], { args[0] });
                    cf->add(ir_store).op(data).op(*self).op(cf->imm<u32>(DATA_OFFSET));

                    Value refs = args[0].getProp("_refs");
                    Value refsVal = *refs;
                    refsVal++;
                    cf->add(ir_store).op(refsVal).op(refs);
                    cf->add(ir_store).op(refs).op(*self).op(cf->imm<u32>(REFS_OFFSET));
                    cf->add(ir_store).op(cf->imm(false)).op(*self).op(cf->imm<u32>(EXTERNAL_OFFSET));
                }
            );

            auto release = +[](compiler::InlineCodeGenContext* ctx) {
                auto c = ctx->compiler;
                auto cf = c->currentFunction();
                auto self = ctx->selfPointer;
                auto targs = self->getType()->getTemplateArguments();
                auto args = ctx->arguments;

                Function* freeMem = c->getContext()->getModule("<host>/memory.tsn")->allFunctions().find([](const ffi::Function* fn) {
                    return fn && fn->getName() == "freeMem";
                });

                // Check if the pointer is not owned
                Value isExternal = self->getProp("_external");
                auto b1 = cf->add(ir_branch).op(isExternal);
                label_id checkRefsLabel = cf->label();

                // Check if _refs is not null
                Value refs = self->getProp("_refs");
                Value refsNotNull = cf->val<bool>();
                cf->add(ir_uneq).op(refsNotNull).op(refs).op(cf->getNull());

                auto b2 = cf->add(ir_branch).op(refsNotNull);
                label_id decRefLabel = cf->label();

                // Decrease ref count and check if it has reached zero
                Value nextRefCount = *refs;
                nextRefCount--;

                Value refsIsZero = cf->val<bool>();
                cf->add(ir_ueq).op(refsIsZero).op(nextRefCount).op(cf->imm<u32>(0));
                auto b3 = cf->add(ir_branch).op(refsIsZero);

                label_id doFreeLabel = cf->label();

                // Free the memory
                c->generateCall(freeMem, { refs });

                Function* dtor = targs[0]->getDestructor();
                if (dtor) c->generateCall(dtor, {}, self);

                Value data = self->getProp("_data");
                c->generateCall(freeMem, { data });

                label_id doNotFreeLabel = cf->label();

                b1.label(checkRefsLabel).label(doNotFreeLabel);
                b2.label(decRefLabel).label(doNotFreeLabel);
                b3.label(doFreeLabel).label(doNotFreeLabel);

                // Reset the properties to null
                cf->add(ir_store).op(cf->getNull()).op(*self).op(cf->imm<u32>(DATA_OFFSET));
                cf->add(ir_store).op(cf->getNull()).op(*self).op(cf->imm<u32>(REFS_OFFSET));
                cf->add(ir_store).op(cf->imm(false)).op(*self).op(cf->imm<u32>(EXTERNAL_OFFSET));
            };

            e->setDestructor(release);
            e->addMethod<void>("release", release);
            e->addMethod<void, tsn::null_t>("operator=", release);
            e->addMethod(
                "operator=",
                treg->getVoid(), false,
                {
                    { ptype->getInfo().is_primitive ? arg_type::value : arg_type::pointer, ptype }
                },
                +[](compiler::InlineCodeGenContext* ctx) {
                    auto c = ctx->compiler;
                    auto cf = c->currentFunction();
                    auto self = ctx->selfPointer;
                    auto targs = self->getType()->getTemplateArguments();
                    auto args = ctx->arguments;

                    self->callMethod("release", c->getContext()->getTypes()->getVoid(), {});

                    Function* newMem = c->getContext()->getModule("<host>/memory.tsn")->allFunctions().find([](const ffi::Function* fn) {
                        return fn && fn->getName() == "newMem";
                    });

                    Value data = c->generateCall(newMem, { cf->imm<u32>(targs[0]->getInfo().size) });
                    c->constructObject(data, targs[0], { args[0] });
                    cf->add(ir_store).op(data).op(*self).op(cf->imm<u32>(DATA_OFFSET));

                    Value refs = c->generateCall(newMem, { cf->imm<u32>(sizeof(u32)) });
                    cf->add(ir_store).op(cf->imm<u32>(1)).op(refs);
                    cf->add(ir_store).op(refs).op(*self).op(cf->imm<u32>(REFS_OFFSET));
                    cf->add(ir_store).op(cf->imm(false)).op(*self).op(cf->imm<u32>(EXTERNAL_OFFSET));
                }
            );
            e->addMethod(
                "operator=",
                treg->getVoid(), false,
                {
                    {arg_type::pointer, tp }
                },
                +[](compiler::InlineCodeGenContext* ctx) {
                    auto c = ctx->compiler;
                    auto cf = c->currentFunction();
                    auto self = ctx->selfPointer;
                    auto targs = self->getType()->getTemplateArguments();
                    auto args = ctx->arguments;

                    self->callMethod("release", c->getContext()->getTypes()->getVoid(), {});

                    Value data = args[0].getProp("_data");
                    c->constructObject(data, targs[0], { args[0] });
                    cf->add(ir_store).op(data).op(*self).op(cf->imm<u32>(DATA_OFFSET));

                    Value refs = args[0].getProp("_refs");
                    Value refsVal = *refs;
                    refsVal++;
                    cf->add(ir_store).op(refsVal).op(refs);
                    cf->add(ir_store).op(refs).op(*self).op(cf->imm<u32>(REFS_OFFSET));
                    cf->add(ir_store).op(cf->imm(false)).op(*self).op(cf->imm<u32>(EXTERNAL_OFFSET));
                }
            );
            e->addMethod<bool, null_t>("operator==", +[](compiler::InlineCodeGenContext* ctx) {
                auto c = ctx->compiler;
                auto cf = c->currentFunction();
                auto self = ctx->selfPointer;
                auto result = ctx->resultStorage;

                Value data = self->getProp("_data");
                cf->add(ir_ueq).op(*result).op(data).op(cf->getNull());
            });
            e->addMethod<bool, null_t>("operator!=", +[](compiler::InlineCodeGenContext* ctx) {
                auto c = ctx->compiler;
                auto cf = c->currentFunction();
                auto self = ctx->selfPointer;
                auto result = ctx->resultStorage;

                Value data = self->getProp("_data");
                cf->add(ir_uneq).op(*result).op(data).op(cf->getNull());
            });
            e->addMethod<bool>("operator boolean", +[](compiler::InlineCodeGenContext* ctx) {
                auto c = ctx->compiler;
                auto cf = c->currentFunction();
                auto self = ctx->selfPointer;
                auto result = ctx->resultStorage;

                Value data = self->getProp("_data");
                cf->add(ir_uneq).op(*result).op(data).op(cf->getNull());
            });
            e->addProperty(
                "references",
                ctx->getTypes()->getUInt32(),
                +[](compiler::InlineCodeGenContext* ctx) {
                    auto c = ctx->compiler;
                    auto cf = c->currentFunction();
                    auto self = ctx->selfPointer;
                    auto targs = self->getType()->getTemplateArguments();
                    auto args = ctx->arguments;
                    auto result = ctx->resultStorage;

                    // Check if the pointer is not owned
                    Value isExternal = self->getProp("_external");
                    auto b1 = cf->add(ir_branch).op(isExternal);
                    label_id checkRefsLabel = cf->label();

                    // Check if _refs is not null
                    Value refs = args[0].getProp("_refs");
                    Value refsNotNull = cf->val<bool>();
                    cf->add(ir_uneq).op(refsNotNull).op(refs).op(cf->getNull());
                    auto b2 = cf->add(ir_branch).op(refsNotNull);
                    
                    label_id returnRefCountLabel = cf->label();
                    cf->add(ir_load).op(*result).op(*self).op(cf->imm<u32>(REFS_OFFSET));
                    auto j1 = cf->add(ir_jump);

                    label_id returnZeroLabel = cf->label();
                    cf->add(ir_assign).op(*result).op(cf->imm<u32>(0));

                    label_id endLabel = cf->label();

                    b1.label(checkRefsLabel).label(returnZeroLabel);
                    b2.label(returnRefCountLabel).label(returnZeroLabel);
                    j1.label(endLabel);
                },
                nullptr
            );

            return true;
        });
        
        ctx->getGlobal()->addForeignType(ptr);
        ctx->getTypes()->addForeignType(ptr);
        
        ctx->getPipeline()->specializeTemplate(ptr, { ctx->getTypes()->getInt8() });
        ctx->getPipeline()->specializeTemplate(ptr, { ctx->getTypes()->getUInt8() });
        ctx->getPipeline()->specializeTemplate(ptr, { ctx->getTypes()->getInt16() });
        ctx->getPipeline()->specializeTemplate(ptr, { ctx->getTypes()->getUInt16() });
        ctx->getPipeline()->specializeTemplate(ptr, { ctx->getTypes()->getInt32() });
        ctx->getPipeline()->specializeTemplate(ptr, { ctx->getTypes()->getUInt32() });
        ctx->getPipeline()->specializeTemplate(ptr, { ctx->getTypes()->getInt64() });
        ctx->getPipeline()->specializeTemplate(ptr, { ctx->getTypes()->getUInt64() });
        ctx->getPipeline()->specializeTemplate(ptr, { ctx->getTypes()->getFloat32() });
        ctx->getPipeline()->specializeTemplate(ptr, { ctx->getTypes()->getFloat64() });
        ctx->getPipeline()->specializeTemplate(ptr, { ctx->getTypes()->getVoidPtr() });
        ctx->getPipeline()->specializeTemplate(ptr, { ctx->getTypes()->getString() });
        ctx->getPipeline()->specializeTemplate(ptr, { ctx->getTypes()->getBoolean() });
    }
};