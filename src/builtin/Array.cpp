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
    #define LENGTH_OFFSET 0   // must match Array<...>::m_size
    #define CAPACITY_OFFSET 4 // must match Array<...>::m_capacity
    #define DATA_OFFSET 8     // must match Array<...>::m_data

    // todo: take advantage of trivial copyability

    void BindArray(Context* ctx) {
        TemplateType* ptr = new TemplateType(ctx->getGlobal(), "Array", "::Array", 1, +[](ffi::DataType* tp, ffi::DataTypeExtender* e, Pipeline* p) {
            Context* ctx = tp->getSource()->getContext();
            DataTypeRegistry* treg = ctx->getTypes();

            auto ptype = tp->getTemplateArguments()[0];

            auto& info = e->info();
            info.size = sizeof(Array<u32>);
            info.is_pod = 0;
            info.is_trivially_constructible = 0;
            info.is_trivially_copyable = 0;
            info.is_trivially_destructible = 0;

            e->addProperty("_length", treg->getUInt32(), LENGTH_OFFSET, vf_rw, private_access);
            e->addProperty("_capacity", treg->getUInt32(), CAPACITY_OFFSET, vf_rw, private_access);
            e->addProperty("_data", treg->getVoidPtr(), DATA_OFFSET, vf_pointer | vf_rw, private_access);
            e->addMethod<void>("constructor", +[](compiler::InlineCodeGenContext* ctx) {
                auto c = ctx->compiler;
                auto cf = c->currentFunction();
                auto self = ctx->selfPointer;
                auto args = ctx->arguments;

                cf->add(ir_store).op(cf->imm<u32>(0)).op(*self).op(cf->imm<u32>(LENGTH_OFFSET));
                cf->add(ir_store).op(cf->imm<u32>(0)).op(*self).op(cf->imm<u32>(CAPACITY_OFFSET));
                cf->add(ir_store).op(cf->getNull()).op(*self).op(cf->imm<u32>(DATA_OFFSET));
            });
            e->addMethod<void, u32>("constructor", +[](compiler::InlineCodeGenContext* ctx) {
                auto c = ctx->compiler;
                auto cf = c->currentFunction();
                auto self = ctx->selfPointer;
                auto targs = self->getType()->getTemplateArguments();
                auto args = ctx->arguments;

                Function* newMem = c->getContext()->getModule("<host>/memory.tsn")->allFunctions().find([](const ffi::Function* fn) {
                    return fn && fn->getName() == "newMem";
                });

                Value memSize = args[0] * cf->imm<u32>(targs[0]->getInfo().size);
                Value mem = c->generateCall(newMem, { memSize });

                cf->add(ir_store).op(cf->imm<u32>(0)).op(*self).op(cf->imm<u32>(LENGTH_OFFSET));
                cf->add(ir_store).op(args[0]).op(*self).op(cf->imm<u32>(CAPACITY_OFFSET));
                cf->add(ir_store).op(mem).op(*self).op(cf->imm<u32>(DATA_OFFSET));
            });
            e->addMethod(
                "constructor",
                ctx->getTypes()->getVoid(),
                false,
                {
                    { arg_type::pointer, tp }
                },
                +[](compiler::InlineCodeGenContext* ctx) {
                    auto c = ctx->compiler;
                    auto cf = c->currentFunction();
                    auto self = ctx->selfPointer;
                    auto targs = self->getType()->getTemplateArguments();
                    auto treg = c->getContext()->getTypes();
                    auto args = ctx->arguments;

                    Function* newMem = c->getContext()->getModule("<host>/memory.tsn")->allFunctions().find([](const ffi::Function* fn) {
                        return fn && fn->getName() == "newMem";
                    });

                    Value length = args[0].getProp("_length");
                    Value typeSize = cf->imm<u32>(targs[0]->getInfo().size);
                    Value memSize = length * typeSize;
                    Value mem = c->generateCall(newMem, { memSize });

                    cf->add(ir_store).op(length).op(*self).op(cf->imm<u32>(LENGTH_OFFSET));
                    cf->add(ir_store).op(length).op(*self).op(cf->imm<u32>(CAPACITY_OFFSET));
                    cf->add(ir_store).op(mem).op(*self).op(cf->imm<u32>(DATA_OFFSET));

                    Value selfData = self->getProp("_data");
                    Value rhsData = args[0].getProp("_data");

                    // loop init
                    Value i = cf->val<u32>();
                    cf->add(ir_assign).op(i).op(cf->imm<u32>(0));

                    // loop condition
                    label_id condLabel = cf->label();
                    Value cond = i < length;
                    auto b1 = cf->add(ir_branch).op(cond).label(cf->label());

                    // loop body
                    Value newE = selfData + (i * typeSize);
                    newE.setType(targs[0]);
                    newE.getFlags().is_pointer = 1;

                    if (targs[0]->getInfo().is_primitive) {
                        Value rhsE = rhsData + (i * typeSize);
                        rhsE.setType(targs[0]);
                        rhsE.getFlags().is_pointer = 1;
                        rhsE = *rhsE;
                        cf->add(ir_store).op(rhsE).op(newE);
                    } else {
                        Value rhsE = rhsData + (i * typeSize);
                        rhsE.setType(targs[0]);
                        rhsE.getFlags().is_pointer = 1;
                        c->constructObject(newE, targs[0], { rhsE });
                    }

                    // loop end
                    cf->add(ir_uinc).op(i);
                    cf->add(ir_jump).label(condLabel);

                    // after loop
                    b1.label(cf->label());
                }
            );
            e->setDestructor(
                +[](compiler::InlineCodeGenContext* ctx) {
                    auto c = ctx->compiler;
                    auto cf = c->currentFunction();
                    auto self = ctx->selfPointer;
                    auto targs = self->getType()->getTemplateArguments();
                    auto treg = c->getContext()->getTypes();
                    auto args = ctx->arguments;

                    self->callMethod("clear", c->getContext()->getTypes()->getVoid(), {});
                }
            );

            e->addMethod<void>("clear", +[](compiler::InlineCodeGenContext* ctx) {
                auto c = ctx->compiler;
                auto cf = c->currentFunction();
                auto self = ctx->selfPointer;
                auto targs = self->getType()->getTemplateArguments();
                auto treg = c->getContext()->getTypes();
                auto args = ctx->arguments;

                Function* freeMem = c->getContext()->getModule("<host>/memory.tsn")->allFunctions().find([](const ffi::Function* fn) {
                    return fn && fn->getName() == "freeMem";
                });

                Value data = self->getProp("_data");

                Function* dtor = targs[0]->getDestructor();
                if (dtor) {
                    Value length = args[0].getProp("_length");
                    Value typeSize = cf->imm<u32>(targs[0]->getInfo().size);

                    // loop init
                    Value i = cf->val<u32>();
                    cf->add(ir_assign).op(i).op(cf->imm<u32>(0));

                    // loop condition
                    label_id condLabel = cf->label();
                    Value cond = i < length;
                    auto b1 = cf->add(ir_branch).op(cond).label(cf->label());

                    // loop body
                    Value ele = data + (i * typeSize);
                    ele.setType(targs[0]);
                    ele.getFlags().is_pointer = 1;

                    c->generateCall(dtor, {}, &ele);

                    // loop end
                    cf->add(ir_uinc).op(i);
                    cf->add(ir_jump).label(condLabel);

                    // after loop
                    b1.label(cf->label());
                }

                Value dataNotNull = cf->val<bool>();
                cf->add(ir_uneq).op(dataNotNull).op(data).op(cf->getNull());
                auto b1 = cf->add(ir_branch).op(dataNotNull).label(cf->label());

                Value mem = c->generateCall(freeMem, { data.convertedTo(treg->getVoidPtr()) });

                cf->add(ir_store).op(cf->getNull()).op(*self).op(cf->imm<u32>(DATA_OFFSET));

                b1.label(cf->label());

                cf->add(ir_store).op(cf->imm<u32>(0)).op(*self).op(cf->imm<u32>(LENGTH_OFFSET));
                cf->add(ir_store).op(cf->imm<u32>(0)).op(*self).op(cf->imm<u32>(CAPACITY_OFFSET));
            });

            e->addMethod<void, u32>("reserve", +[](compiler::InlineCodeGenContext* ctx) {
                auto c = ctx->compiler;
                auto cf = c->currentFunction();
                auto self = ctx->selfPointer;
                auto targs = self->getType()->getTemplateArguments();
                auto treg = c->getContext()->getTypes();
                auto args = ctx->arguments;

                Function* newMem = nullptr;
                Function* freeMem = nullptr;
                
                c->getContext()->getModule("<host>/memory.tsn")->allFunctions().some([&newMem, &freeMem](ffi::Function* fn) {
                    if (fn && fn->getName() == "newMem") newMem = fn;
                    if (fn && fn->getName() == "freeMem") freeMem = fn;

                    return newMem && freeMem;
                });
                
                Value typeSize = cf->imm<u32>(targs[0]->getInfo().size);
                Value newCapacity = self->getProp("_capacity") + args[0];
                Value memSize = newCapacity * typeSize;
                Value newData = c->generateCall(newMem, { memSize });
                Value oldData = self->getProp("_data");
                Value length = self->getProp("_length");

                // loop init
                Value i = cf->val<u32>();
                cf->add(ir_assign).op(i).op(cf->imm<u32>(0));

                // loop condition
                label_id condLabel = cf->label();
                Value cond = i < length;
                auto b1 = cf->add(ir_branch).op(cond).label(cf->label());

                // loop body
                Value newE = newData + (i * typeSize);
                newE.setType(targs[0]);
                newE.getFlags().is_pointer = 1;

                if (targs[0]->getInfo().is_primitive) {
                    Value oldE = oldData + (i * typeSize);
                    oldE.setType(targs[0]);
                    oldE.getFlags().is_pointer = 1;
                    oldE = *oldE;
                    cf->add(ir_store).op(oldE).op(newE);
                } else {
                    Value oldE = oldData + (i * typeSize);
                    oldE.setType(targs[0]);
                    oldE.getFlags().is_pointer = 1;

                    c->constructObject(newE, targs[0], { oldE });

                    Function* dtor = targs[0]->getDestructor();
                    if (dtor) c->generateCall(dtor, {}, &oldE);
                }

                // loop end
                cf->add(ir_uinc).op(i);
                cf->add(ir_jump).label(condLabel);

                // after loop
                b1.label(cf->label());
                
                // free data if it's not null
                Value dataNotNull = cf->val<bool>();
                cf->add(ir_uneq).op(dataNotNull).op(oldData).op(cf->getNull());
                auto b2 = cf->add(ir_branch).op(dataNotNull).label(cf->label());

                c->generateCall(freeMem, { oldData.convertedTo(treg->getVoidPtr()) });

                // after free data if it's not null
                b2.label(cf->label());

                cf->add(ir_store).op(newCapacity).op(*self).op(cf->imm<u32>(CAPACITY_OFFSET));
                cf->add(ir_store).op(newData).op(*self).op(cf->imm<u32>(DATA_OFFSET));
            });

            e->addMethod(
                "operator=",
                ctx->getTypes()->getVoid(),
                false,
                {
                    { arg_type::pointer, tp }
                },
                +[](compiler::InlineCodeGenContext* ctx) {
                    auto c = ctx->compiler;
                    auto cf = c->currentFunction();
                    auto self = ctx->selfPointer;
                    auto targs = self->getType()->getTemplateArguments();
                    auto treg = c->getContext()->getTypes();
                    auto args = ctx->arguments;

                    Function* newMem = c->getContext()->getModule("<host>/memory.tsn")->allFunctions().find([](const ffi::Function* fn) {
                        return fn && fn->getName() == "newMem";
                    });

                    self->callMethod("clear", treg->getVoid(), {});

                    Value length = args[0].getProp("_length");
                    Value typeSize = cf->imm<u32>(targs[0]->getInfo().size);
                    Value memSize = length * typeSize;
                    Value mem = c->generateCall(newMem, { memSize });

                    cf->add(ir_store).op(length).op(*self).op(cf->imm<u32>(LENGTH_OFFSET));
                    cf->add(ir_store).op(length).op(*self).op(cf->imm<u32>(CAPACITY_OFFSET));
                    cf->add(ir_store).op(mem).op(*self).op(cf->imm<u32>(DATA_OFFSET));

                    Value selfData = self->getProp("_data");
                    Value rhsData = args[0].getProp("_data");

                    // loop init
                    Value i = cf->val<u32>();
                    cf->add(ir_assign).op(i).op(cf->imm<u32>(0));

                    // loop condition
                    label_id condLabel = cf->label();
                    Value cond = i < length;
                    auto b1 = cf->add(ir_branch).op(cond).label(cf->label());

                    // loop body
                    Value newE = selfData + (i * typeSize);
                    newE.setType(targs[0]);
                    newE.getFlags().is_pointer = 1;

                    if (targs[0]->getInfo().is_primitive) {
                        Value rhsE = rhsData + (i * typeSize);
                        rhsE.setType(targs[0]);
                        rhsE.getFlags().is_pointer = 1;
                        rhsE = *rhsE;
                        cf->add(ir_store).op(rhsE).op(newE);
                    } else {
                        Value rhsE = rhsData + (i * typeSize);
                        rhsE.setType(targs[0]);
                        rhsE.getFlags().is_pointer = 1;
                        c->constructObject(newE, targs[0], { rhsE });
                    }

                    // loop end
                    cf->add(ir_uinc).op(i);
                    cf->add(ir_jump).label(condLabel);

                    // after loop
                    b1.label(cf->label());
                }
            );

            e->addMethod(
                "push",
                ctx->getTypes()->getVoid(),
                false,
                {
                    { ptype->getInfo().is_primitive ? arg_type::value : arg_type::pointer, ptype }
                },
                +[](compiler::InlineCodeGenContext* ctx) {
                    auto c = ctx->compiler;
                    auto cf = c->currentFunction();
                    auto self = ctx->selfPointer;
                    auto targs = self->getType()->getTemplateArguments();
                    auto treg = c->getContext()->getTypes();
                    auto args = ctx->arguments;

                    Value length = self->getProp("_length");
                    Value capacity = self->getProp("_capacity");

                    // check if resize necessary
                    Value atCapacity = length == capacity;
                    auto b1 = cf->add(ir_branch).op(atCapacity).label(cf->label());

                    // check if capacity is zero, and resize to 16 if so
                    Value isEmpty = length == cf->imm<u32>(0);
                    auto b2 = cf->add(ir_branch).op(isEmpty).label(cf->label());

                    // capacity is zero
                    self->callMethod("reserve", treg->getVoid(), { cf->imm<u32>(16) });

                    // jump to after resize
                    auto j1 = cf->add(ir_jump);

                    // capacity is not zero
                    b2.label(cf->label());
                    Value reserveAmount = capacity / cf->imm<u32>(2);
                    self->callMethod("reserve", treg->getVoid(), { reserveAmount });

                    // after resize
                    label_id afterResize = cf->label();
                    b1.label(afterResize);
                    j1.label(afterResize);

                    // finally, push the element
                    Value typeSize = cf->imm<u32>(targs[0]->getInfo().size);
                    Value newE = self->getProp("_data") + (length * typeSize);
                    newE.setType(targs[0]);
                    newE.getFlags().is_pointer = 1;

                    if (targs[0]->getInfo().is_primitive) {
                        cf->add(ir_store).op(args[0]).op(newE);
                    } else {
                        c->constructObject(newE, targs[0], { args[0] });
                    }

                    // increment length
                    Value newLength = length + cf->imm<u32>(1);
                    cf->add(ir_store).op(newLength).op(*self).op(cf->imm<u32>(LENGTH_OFFSET));
                }
            );

            e->addMethod(
                "operator[]",
                ptype,
                true,
                {
                    { arg_type::value, ctx->getTypes()->getUInt32() }
                },
                +[](compiler::InlineCodeGenContext* ctx) {
                    auto c = ctx->compiler;
                    auto cf = c->currentFunction();
                    auto self = ctx->selfPointer;
                    auto targs = self->getType()->getTemplateArguments();
                    auto treg = c->getContext()->getTypes();
                    auto args = ctx->arguments;
                    auto result = ctx->resultStorage;

                    // todo: exception on out of range
                    // ... also exceptions in general

                    Value typeSize = cf->imm<u32>(targs[0]->getInfo().size);
                    Value data = self->getProp("_data");
                    Value offset = (args[0] * typeSize);
                    cf->add(ir_uadd).op(*result).op(data).op(offset);

                    /*
                    - Ensure that when a pointer to a non-primitive is assigned
                        to a variable the copy construction still happens
                    */
                }
            )->getFlags().return_pointer_non_nullable = 1;

            e->addProperty(
                "length",
                ctx->getTypes()->getUInt32(),
                +[](compiler::InlineCodeGenContext* ctx) {
                    auto c = ctx->compiler;
                    auto cf = c->currentFunction();
                    auto self = ctx->selfPointer;
                    auto targs = self->getType()->getTemplateArguments();
                    auto args = ctx->arguments;
                    auto result = ctx->resultStorage;

                    cf->add(ir_load).op(*result).op(*self).op(cf->imm<u32>(LENGTH_OFFSET));
                },
                nullptr
            );
            e->addProperty(
                "capacity",
                ctx->getTypes()->getUInt32(),
                +[](compiler::InlineCodeGenContext* ctx) {
                    auto c = ctx->compiler;
                    auto cf = c->currentFunction();
                    auto self = ctx->selfPointer;
                    auto targs = self->getType()->getTemplateArguments();
                    auto args = ctx->arguments;
                    auto result = ctx->resultStorage;

                    cf->add(ir_load).op(*result).op(*self).op(cf->imm<u32>(CAPACITY_OFFSET));
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