#include <tsn/common/Context.h>
#include <tsn/common/Module.hpp>
#include <tsn/ffi/DataTypeRegistry.h>
#include <tsn/bind/bind.hpp>
#include <tsn/compiler/Compiler.h>
#include <tsn/compiler/FunctionDef.hpp>
#include <tsn/ffi/FunctionRegistry.hpp>
#include <tsn/interfaces/IFunctionHolder.hpp>

#include <utils/Array.hpp>
#include <utils/Math.hpp>

using namespace tsn::ffi;
using namespace tsn::compiler;
using namespace utils;

namespace tsn {
    template <typename T>
    void BindVec2(Module* m, const char* name) {
        using V = vec2<T>;

        auto t = bind<V>(m, name);
        t.prop("x", &V::x, public_access);
        t.prop("y", &V::y, public_access);

        t.ctor(+[](InlineCodeGenContext* ctx){
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_vset).op(*self).op(cf->imm<T>(0.0));
        }, public_access);
        t.ctor<T, T>(+[](InlineCodeGenContext* ctx){
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_store).op(args[0]).op(*self).op(cf->imm<u32>(offsetof(V, x)));
            cf->add(ir_store).op(args[1]).op(*self).op(cf->imm<u32>(offsetof(V, y)));
        }, public_access);
        t.ctor<T>(+[](InlineCodeGenContext* ctx){
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_vset).op(*self).op(args[0]);
        }, public_access);
        t.ctor<const V&>(+[](InlineCodeGenContext* ctx){
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;
            
            cf->add(ir_vset).op(*self).op(args[0]);
        }, public_access);

        t.prop<T>("length", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_vmag).op(*result).op(*self);
        }, nullptr, public_access);
        t.prop<T>("lengthSq", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_vmagsq).op(*result).op(*self);
        }, nullptr,  public_access);
        t.method<T, const V&>("dot", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_vdot).op(*result).op(*self).op(args[0]);
        }, public_access);
        t.method<void>("normalize", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto self = ctx->selfPointer;

            cf->add(ir_vnorm).op(*self);
        }, public_access);
        t.prop<V>("normal", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto self = ctx->selfPointer;
            cf->add(ir_vset).op(*result).op(*self);
            cf->add(ir_vnorm).op(*result);
        }, nullptr, public_access);
        t.method("toString", +[](V* self) {
            return String::Format("%f, %f", self->x, self->y);
        }, public_access);

        t.method<V, const V&>("operator+", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_vset).op(*result).op(*self);
            cf->add(ir_vadd).op(*result).op(args[0]);
        }, public_access);
        t.method<V, T>("operator+", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_vset).op(*result).op(*self);
            cf->add(ir_vadd).op(*result).op(args[0]);
        }, public_access);
        t.method<void, const V&>("operator+=", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_vadd).op(*self).op(args[0]);
        }, public_access);
        t.method<void, T>("operator+=", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_vadd).op(*self).op(args[0]);
        }, public_access);

        t.method<V, const V&>("operator-", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_vset).op(*result).op(*self);
            cf->add(ir_vsub).op(*result).op(args[0]);
        }, public_access);
        t.method<V, T>("operator-", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_vset).op(*result).op(*self);
            cf->add(ir_vsub).op(*result).op(args[0]);
        }, public_access);
        t.method<void, const V&>("operator-=", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_vadd).op(*self).op(args[0]);
        }, public_access);
        t.method<void, T>("operator-=", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_vsub).op(*self).op(args[0]);
        }, public_access);

        t.method<V, const V&>("operator*", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_vset).op(*result).op(*self);
            cf->add(ir_vmul).op(*result).op(args[0]);
        }, public_access);
        t.method<V, T>("operator*", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_vset).op(*result).op(*self);
            cf->add(ir_vmul).op(*result).op(args[0]);
        }, public_access);
        t.method<void, const V&>("operator*=", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_vmul).op(*self).op(args[0]);
        }, public_access);
        t.method<void, T>("operator*=", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_vmul).op(*self).op(args[0]);
        }, public_access);

        t.method<V, const V&>("operator/", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_vset).op(*result).op(*self);
            cf->add(ir_vdiv).op(*result).op(args[0]);
        }, public_access);
        t.method<V, T>("operator/", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_vset).op(*result).op(*self);
            cf->add(ir_vdiv).op(*result).op(args[0]);
        }, public_access);
        t.method<void, const V&>("operator/=", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_vdiv).op(*self).op(args[0]);
        }, public_access);
        t.method<void, T>("operator/=", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_vdiv).op(*self).op(args[0]);
        }, public_access);

        t.method<V, const V&>("operator%", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_vset).op(*result).op(*self);
            cf->add(ir_vmod).op(*result).op(args[0]);
        }, public_access);
        t.method<V, T>("operator%", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_vset).op(*result).op(*self);
            cf->add(ir_vmod).op(*result).op(args[0]);
        }, public_access);
        t.method<void, const V&>("operator%=", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_vmod).op(*self).op(args[0]);
        }, public_access);
        t.method<void, T>("operator%=", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_vmod).op(*self).op(args[0]);
        }, public_access);

        t.method<V>("operator-", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_vset).op(*result).op(*self);
            cf->add(ir_vneg).op(*result);
        }, public_access);


        t.finalize();
    }
    
    template <typename T>
    void BindVec3(Module* m, const char* name) {
        using V = vec3<T>;

        auto t = bind<V>(m, name);
        t.prop("x", &V::x, public_access);
        t.prop("y", &V::y, public_access);
        t.prop("z", &V::z, public_access);

        t.ctor(+[](InlineCodeGenContext* ctx){
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_vset).op(*self).op(cf->imm<T>(0.0));
        }, public_access);
        t.ctor<T, T, T>(+[](InlineCodeGenContext* ctx){
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_store).op(args[0]).op(*self).op(cf->imm<u32>(offsetof(V, x)));
            cf->add(ir_store).op(args[1]).op(*self).op(cf->imm<u32>(offsetof(V, y)));
            cf->add(ir_store).op(args[2]).op(*self).op(cf->imm<u32>(offsetof(V, z)));
        }, public_access);
        t.ctor<T>(+[](InlineCodeGenContext* ctx){
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_vset).op(*self).op(args[0]);
        }, public_access);
        t.ctor<const V&>(+[](InlineCodeGenContext* ctx){
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;
            
            cf->add(ir_vset).op(*self).op(args[0]);
        }, public_access);
        t.ctor<const vec2<T>&, T>(+[](InlineCodeGenContext* ctx){
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;
            
            cf->add(ir_store).op(args[0].getProp("x")).op(*self).op(cf->imm<u32>(offsetof(V, x)));
            cf->add(ir_store).op(args[0].getProp("y")).op(*self).op(cf->imm<u32>(offsetof(V, y)));
            cf->add(ir_store).op(args[1]).op(*self).op(cf->imm<u32>(offsetof(V, z)));
        }, public_access);

        t.prop<T>("length", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_vmag).op(*result).op(*self);
        }, nullptr, public_access);
        t.prop<T>("lengthSq", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_vmagsq).op(*result).op(*self);
        }, nullptr, public_access);
        t.method<V, const V&>("cross", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_vcross).op(*result).op(*self).op(args[0]);
        }, public_access);
        t.method<T, const V&>("dot", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_vdot).op(*result).op(*self).op(args[0]);
        }, public_access);
        t.method<void>("normalize", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto self = ctx->selfPointer;

            cf->add(ir_vnorm).op(*self);
        }, public_access);
        t.prop<V>("normal", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto self = ctx->selfPointer;
            cf->add(ir_vset).op(*result).op(*self);
            cf->add(ir_vnorm).op(*result);
        }, nullptr, public_access);
        t.method("toString", +[](V* self) {
            return String::Format("%f, %f, %f", self->x, self->y, self->z);
        }, public_access);

        t.method<V, const V&>("operator+", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_vset).op(*result).op(*self);
            cf->add(ir_vadd).op(*result).op(args[0]);
        }, public_access);
        t.method<V, T>("operator+", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_vset).op(*result).op(*self);
            cf->add(ir_vadd).op(*result).op(args[0]);
        }, public_access);
        t.method<void, const V&>("operator+=", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_vadd).op(*self).op(args[0]);
        }, public_access);
        t.method<void, T>("operator+=", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_vadd).op(*self).op(args[0]);
        }, public_access);

        t.method<V, const V&>("operator-", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_vset).op(*result).op(*self);
            cf->add(ir_vsub).op(*result).op(args[0]);
        }, public_access);
        t.method<V, T>("operator-", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_vset).op(*result).op(*self);
            cf->add(ir_vsub).op(*result).op(args[0]);
        }, public_access);
        t.method<void, const V&>("operator-=", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_vadd).op(*self).op(args[0]);
        }, public_access);
        t.method<void, T>("operator-=", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_vsub).op(*self).op(args[0]);
        }, public_access);

        t.method<V, const V&>("operator*", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_vset).op(*result).op(*self);
            cf->add(ir_vmul).op(*result).op(args[0]);
        }, public_access);
        t.method<V, T>("operator*", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_vset).op(*result).op(*self);
            cf->add(ir_vmul).op(*result).op(args[0]);
        }, public_access);
        t.method<void, const V&>("operator*=", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_vmul).op(*self).op(args[0]);
        }, public_access);
        t.method<void, T>("operator*=", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_vmul).op(*self).op(args[0]);
        }, public_access);

        t.method<V, const V&>("operator/", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_vset).op(*result).op(*self);
            cf->add(ir_vdiv).op(*result).op(args[0]);
        }, public_access);
        t.method<V, T>("operator/", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_vset).op(*result).op(*self);
            cf->add(ir_vdiv).op(*result).op(args[0]);
        }, public_access);
        t.method<void, const V&>("operator/=", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_vdiv).op(*self).op(args[0]);
        }, public_access);
        t.method<void, T>("operator/=", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_vdiv).op(*self).op(args[0]);
        }, public_access);

        t.method<V, const V&>("operator%", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_vset).op(*result).op(*self);
            cf->add(ir_vmod).op(*result).op(args[0]);
        }, public_access);
        t.method<V, T>("operator%", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_vset).op(*result).op(*self);
            cf->add(ir_vmod).op(*result).op(args[0]);
        }, public_access);
        t.method<void, const V&>("operator%=", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_vmod).op(*self).op(args[0]);
        }, public_access);
        t.method<void, T>("operator%=", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_vmod).op(*self).op(args[0]);
        }, public_access);

        t.method<V>("operator-", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_vset).op(*result).op(*self);
            cf->add(ir_vneg).op(*result);
        }, public_access);

        t.prop<vec2<T>>("xx", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto self = ctx->selfPointer;
            
            Value x = self->getProp("x");
            cf->add(ir_vset).op(*result).op(x);
        }, nullptr, public_access);

        t.prop<vec2<T>>("xy", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto self = ctx->selfPointer;
            
            Value x = self->getProp("x");
            cf->add(ir_store).op(x).op(*result).op(cf->imm<u32>(offsetof(vec2<T>, x)));
            Value y = self->getProp("y");
            cf->add(ir_store).op(y).op(*result).op(cf->imm<u32>(offsetof(vec2<T>, y)));
        }, nullptr, public_access);

        t.prop<vec2<T>>("xz", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto self = ctx->selfPointer;
            
            Value x = self->getProp("x");
            cf->add(ir_store).op(x).op(*result).op(cf->imm<u32>(offsetof(vec2<T>, x)));
            Value y = self->getProp("z");
            cf->add(ir_store).op(y).op(*result).op(cf->imm<u32>(offsetof(vec2<T>, y)));
        }, nullptr, public_access);

        t.prop<vec2<T>>("yx", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto self = ctx->selfPointer;
            
            Value x = self->getProp("y");
            cf->add(ir_store).op(x).op(*result).op(cf->imm<u32>(offsetof(vec2<T>, x)));
            Value y = self->getProp("x");
            cf->add(ir_store).op(y).op(*result).op(cf->imm<u32>(offsetof(vec2<T>, y)));
        }, nullptr, public_access);

        t.prop<vec2<T>>("yy", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto self = ctx->selfPointer;
            
            Value x = self->getProp("y");
            cf->add(ir_vset).op(*result).op(x);
        }, nullptr, public_access);

        t.prop<vec2<T>>("yz", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto self = ctx->selfPointer;
            
            Value x = self->getProp("y");
            cf->add(ir_store).op(x).op(*result).op(cf->imm<u32>(offsetof(vec2<T>, x)));
            Value y = self->getProp("z");
            cf->add(ir_store).op(y).op(*result).op(cf->imm<u32>(offsetof(vec2<T>, y)));
        }, nullptr, public_access);

        t.prop<vec2<T>>("zx", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto self = ctx->selfPointer;
            
            Value x = self->getProp("z");
            cf->add(ir_store).op(x).op(*result).op(cf->imm<u32>(offsetof(vec2<T>, x)));
            Value y = self->getProp("x");
            cf->add(ir_store).op(y).op(*result).op(cf->imm<u32>(offsetof(vec2<T>, y)));
        }, nullptr, public_access);

        t.prop<vec2<T>>("zy", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto self = ctx->selfPointer;
            
            Value x = self->getProp("z");
            cf->add(ir_store).op(x).op(*result).op(cf->imm<u32>(offsetof(vec2<T>, x)));
            Value y = self->getProp("y");
            cf->add(ir_store).op(y).op(*result).op(cf->imm<u32>(offsetof(vec2<T>, y)));
        }, nullptr, public_access);

        t.prop<vec2<T>>("zz", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto self = ctx->selfPointer;
            
            Value x = self->getProp("z");
            cf->add(ir_vset).op(*result).op(x);
        }, nullptr, public_access);

        t.prop<vec3<T>>("xxx", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto self = ctx->selfPointer;
            
            Value x = self->getProp("x");
            cf->add(ir_vset).op(*result).op(x);
        }, nullptr, public_access);

        t.prop<vec3<T>>("yyy", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto self = ctx->selfPointer;
            
            Value x = self->getProp("y");
            cf->add(ir_vset).op(*result).op(x);
        }, nullptr, public_access);

        t.prop<vec3<T>>("zzz", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto self = ctx->selfPointer;
            
            Value x = self->getProp("z");
            cf->add(ir_vset).op(*result).op(x);
        }, nullptr, public_access);

        t.staticMethod("HSV", &vec3<T>::HSV, public_access);

        t.finalize();
    }
    
    template <typename T>
    void BindVec4(Module* m, const char* name) {
        using V = vec4<T>;

        auto t = bind<V>(m, name);
        t.prop("x", &V::x, public_access);
        t.prop("y", &V::y, public_access);
        t.prop("z", &V::z, public_access);
        t.prop("w", &V::w, public_access);

        t.ctor(+[](InlineCodeGenContext* ctx){
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_vset).op(*self).op(cf->imm<T>(0.0));
        }, public_access);
        t.ctor<T, T, T, T>(+[](InlineCodeGenContext* ctx){
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_store).op(args[0]).op(*self).op(cf->imm<u32>(offsetof(V, x)));
            cf->add(ir_store).op(args[1]).op(*self).op(cf->imm<u32>(offsetof(V, y)));
            cf->add(ir_store).op(args[2]).op(*self).op(cf->imm<u32>(offsetof(V, z)));
            cf->add(ir_store).op(args[3]).op(*self).op(cf->imm<u32>(offsetof(V, w)));
        }, public_access);
        t.ctor<T>(+[](InlineCodeGenContext* ctx){
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_vset).op(*self).op(args[0]);
        }, public_access);
        t.ctor<const V&>(+[](InlineCodeGenContext* ctx){
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;
            
            cf->add(ir_vset).op(*self).op(args[0]);
        }, public_access);
        t.ctor<const vec3<T>&, T>(+[](InlineCodeGenContext* ctx){
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;
            
            cf->add(ir_store).op(args[0].getProp("x")).op(*self).op(cf->imm<u32>(offsetof(V, x)));
            cf->add(ir_store).op(args[0].getProp("y")).op(*self).op(cf->imm<u32>(offsetof(V, y)));
            cf->add(ir_store).op(args[0].getProp("z")).op(*self).op(cf->imm<u32>(offsetof(V, z)));
            cf->add(ir_store).op(args[1]).op(*self).op(cf->imm<u32>(offsetof(V, w)));
        }, public_access);

        t.prop<T>("length", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_vmag).op(*result).op(*self);
        }, nullptr, public_access);
        t.prop<T>("lengthSq", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_vmagsq).op(*result).op(*self);
        }, nullptr, public_access);
        t.method<V, const V&>("cross", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_vcross).op(*result).op(*self).op(args[0]);
        }, public_access);
        t.method<T, const V&>("dot", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_vdot).op(*result).op(*self).op(args[0]);
        }, public_access);
        t.method<void>("normalize", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto self = ctx->selfPointer;

            cf->add(ir_vnorm).op(*self);
        }, public_access);
        t.prop<V>("normal", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto self = ctx->selfPointer;
            cf->add(ir_vset).op(*result).op(*self);
            cf->add(ir_vnorm).op(*result);
        }, nullptr, public_access);
        t.method("toString", +[](V* self) {
            return String::Format("%f, %f, %f, %f", self->x, self->y, self->z, self->w);
        }, public_access);

        t.method<V, const V&>("operator+", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_vset).op(*result).op(*self);
            cf->add(ir_vadd).op(*result).op(args[0]);
        }, public_access);
        t.method<V, T>("operator+", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_vset).op(*result).op(*self);
            cf->add(ir_vadd).op(*result).op(args[0]);
        }, public_access);
        t.method<void, const V&>("operator+=", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_vadd).op(*self).op(args[0]);
        }, public_access);
        t.method<void, T>("operator+=", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_vadd).op(*self).op(args[0]);
        }, public_access);

        t.method<V, const V&>("operator-", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_vset).op(*result).op(*self);
            cf->add(ir_vsub).op(*result).op(args[0]);
        }, public_access);
        t.method<V, T>("operator-", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_vset).op(*result).op(*self);
            cf->add(ir_vsub).op(*result).op(args[0]);
        }, public_access);
        t.method<void, const V&>("operator-=", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_vadd).op(*self).op(args[0]);
        }, public_access);
        t.method<void, T>("operator-=", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_vsub).op(*self).op(args[0]);
        }, public_access);

        t.method<V, const V&>("operator*", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_vset).op(*result).op(*self);
            cf->add(ir_vmul).op(*result).op(args[0]);
        }, public_access);
        t.method<V, T>("operator*", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_vset).op(*result).op(*self);
            cf->add(ir_vmul).op(*result).op(args[0]);
        }, public_access);
        t.method<void, const V&>("operator*=", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_vmul).op(*self).op(args[0]);
        }, public_access);
        t.method<void, T>("operator*=", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_vmul).op(*self).op(args[0]);
        }, public_access);

        t.method<V, const V&>("operator/", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_vset).op(*result).op(*self);
            cf->add(ir_vdiv).op(*result).op(args[0]);
        }, public_access);
        t.method<V, T>("operator/", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_vset).op(*result).op(*self);
            cf->add(ir_vdiv).op(*result).op(args[0]);
        }, public_access);
        t.method<void, const V&>("operator/=", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_vdiv).op(*self).op(args[0]);
        }, public_access);
        t.method<void, T>("operator/=", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_vdiv).op(*self).op(args[0]);
        }, public_access);

        t.method<V, const V&>("operator%", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_vset).op(*result).op(*self);
            cf->add(ir_vmod).op(*result).op(args[0]);
        }, public_access);
        t.method<V, T>("operator%", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_vset).op(*result).op(*self);
            cf->add(ir_vmod).op(*result).op(args[0]);
        }, public_access);
        t.method<void, const V&>("operator%=", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_vmod).op(*self).op(args[0]);
        }, public_access);
        t.method<void, T>("operator%=", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_vmod).op(*self).op(args[0]);
        }, public_access);

        t.method<V>("operator-", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_vset).op(*result).op(*self);
            cf->add(ir_vneg).op(*result);
        }, public_access);

        t.prop<vec2<T>>("xx", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto self = ctx->selfPointer;
            
            Value x = self->getProp("x");
            cf->add(ir_vset).op(*result).op(x);
        }, nullptr, public_access);

        t.prop<vec2<T>>("xy", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto self = ctx->selfPointer;
            
            Value x = self->getProp("x");
            cf->add(ir_store).op(x).op(*result).op(cf->imm<u32>(offsetof(vec2<T>, x)));
            Value y = self->getProp("y");
            cf->add(ir_store).op(y).op(*result).op(cf->imm<u32>(offsetof(vec2<T>, y)));
        }, nullptr, public_access);

        t.prop<vec2<T>>("xz", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto self = ctx->selfPointer;
            
            Value x = self->getProp("x");
            cf->add(ir_store).op(x).op(*result).op(cf->imm<u32>(offsetof(vec2<T>, x)));
            Value y = self->getProp("z");
            cf->add(ir_store).op(y).op(*result).op(cf->imm<u32>(offsetof(vec2<T>, y)));
        }, nullptr, public_access);

        t.prop<vec2<T>>("xw", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto self = ctx->selfPointer;
            
            Value x = self->getProp("x");
            cf->add(ir_store).op(x).op(*result).op(cf->imm<u32>(offsetof(vec2<T>, x)));
            Value y = self->getProp("w");
            cf->add(ir_store).op(y).op(*result).op(cf->imm<u32>(offsetof(vec2<T>, y)));
        }, nullptr, public_access);

        t.prop<vec2<T>>("yx", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto self = ctx->selfPointer;
            
            Value x = self->getProp("y");
            cf->add(ir_store).op(x).op(*result).op(cf->imm<u32>(offsetof(vec2<T>, x)));
            Value y = self->getProp("x");
            cf->add(ir_store).op(y).op(*result).op(cf->imm<u32>(offsetof(vec2<T>, y)));
        }, nullptr, public_access);

        t.prop<vec2<T>>("yy", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto self = ctx->selfPointer;
            
            Value x = self->getProp("y");
            cf->add(ir_vset).op(*result).op(x);
        }, nullptr, public_access);

        t.prop<vec2<T>>("yz", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto self = ctx->selfPointer;
            
            Value x = self->getProp("y");
            cf->add(ir_store).op(x).op(*result).op(cf->imm<u32>(offsetof(vec2<T>, x)));
            Value y = self->getProp("z");
            cf->add(ir_store).op(y).op(*result).op(cf->imm<u32>(offsetof(vec2<T>, y)));
        }, nullptr, public_access);

        t.prop<vec2<T>>("yw", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto self = ctx->selfPointer;
            
            Value x = self->getProp("y");
            cf->add(ir_store).op(x).op(*result).op(cf->imm<u32>(offsetof(vec2<T>, x)));
            Value y = self->getProp("w");
            cf->add(ir_store).op(y).op(*result).op(cf->imm<u32>(offsetof(vec2<T>, y)));
        }, nullptr, public_access);

        t.prop<vec2<T>>("zx", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto self = ctx->selfPointer;
            
            Value x = self->getProp("z");
            cf->add(ir_store).op(x).op(*result).op(cf->imm<u32>(offsetof(vec2<T>, x)));
            Value y = self->getProp("x");
            cf->add(ir_store).op(y).op(*result).op(cf->imm<u32>(offsetof(vec2<T>, y)));
        }, nullptr, public_access);

        t.prop<vec2<T>>("zy", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto self = ctx->selfPointer;
            
            Value x = self->getProp("z");
            cf->add(ir_store).op(x).op(*result).op(cf->imm<u32>(offsetof(vec2<T>, x)));
            Value y = self->getProp("y");
            cf->add(ir_store).op(y).op(*result).op(cf->imm<u32>(offsetof(vec2<T>, y)));
        }, nullptr, public_access);

        t.prop<vec2<T>>("zz", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto self = ctx->selfPointer;
            
            Value x = self->getProp("z");
            cf->add(ir_vset).op(*result).op(x);
        }, nullptr, public_access);

        t.prop<vec2<T>>("zw", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto self = ctx->selfPointer;
            
            Value x = self->getProp("z");
            cf->add(ir_store).op(x).op(*result).op(cf->imm<u32>(offsetof(vec2<T>, x)));
            Value y = self->getProp("w");
            cf->add(ir_store).op(y).op(*result).op(cf->imm<u32>(offsetof(vec2<T>, y)));
        }, nullptr, public_access);

        t.prop<vec3<T>>("xxx", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto self = ctx->selfPointer;
            
            Value x = self->getProp("x");
            cf->add(ir_vset).op(*result).op(x);
        }, nullptr, public_access);

        t.prop<vec3<T>>("yyy", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto self = ctx->selfPointer;
            
            Value x = self->getProp("y");
            cf->add(ir_vset).op(*result).op(x);
        }, nullptr, public_access);

        t.prop<vec3<T>>("zzz", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto self = ctx->selfPointer;
            
            Value x = self->getProp("z");
            cf->add(ir_vset).op(*result).op(x);
        }, nullptr, public_access);

        t.prop<vec3<T>>("www", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto self = ctx->selfPointer;
            
            Value x = self->getProp("w");
            cf->add(ir_vset).op(*result).op(x);
        }, nullptr, public_access);

        t.staticMethod("HSV", &vec3<T>::HSV, public_access);


        t.finalize();
    }

    template <typename T>
    void BindQuat(Module* m, const char* name) {
        using Q = quat<T>;
        using V = vec3<T>;

        auto t = bind<Q>(m, name);
        t.prop("axis", &Q::axis, public_access);
        t.prop("angle", &Q::angle, public_access);

        t.ctor(+[](InlineCodeGenContext* ctx){
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_vset).op(*self).op(cf->imm<T>(0.0));
            cf->add(ir_store).op(cf->imm<T>(0.0)).op(*self).op(cf->imm<u32>(offsetof(Q, angle)));
        }, public_access);
        t.ctor<T, T, T, T>(+[](InlineCodeGenContext* ctx){
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_store).op(args[0]).op(*self).op(cf->imm<u32>(offsetof(Q, axis) + offsetof(V, x)));
            cf->add(ir_store).op(args[1]).op(*self).op(cf->imm<u32>(offsetof(Q, axis) + offsetof(V, y)));
            cf->add(ir_store).op(args[2]).op(*self).op(cf->imm<u32>(offsetof(Q, axis) + offsetof(V, z)));
            cf->add(ir_store).op(args[3]).op(*self).op(cf->imm<u32>(offsetof(Q, angle)));
        }, public_access);

        t.method<Q, const Q&>("operator*", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            auto xa = self->getProp("axis").getProp("x");
            auto ya = self->getProp("axis").getProp("y");
            auto za = self->getProp("axis").getProp("z");
            auto wa = self->getProp("angle");

            auto xb = args[0].getProp("axis").getProp("x");
            auto yb = args[0].getProp("axis").getProp("y");
            auto zb = args[0].getProp("axis").getProp("z");
            auto wb = args[0].getProp("angle");

            auto xr = (wa * xb + wb * xa + ya * zb) - za * yb;
            auto yr = (wa * yb + wb * ya + za * xb) - xa * zb;
            auto zr = (wa * zb + wb * za + xa * yb) - ya * xb;
            auto wr = ((wa * wb - xa * xb) - ya * yb) - za * zb;

            cf->add(ir_store).op(xr).op(*result).op(cf->imm<u32>(offsetof(Q, axis) + offsetof(V, x)));
            cf->add(ir_store).op(yr).op(*result).op(cf->imm<u32>(offsetof(Q, axis) + offsetof(V, y)));
            cf->add(ir_store).op(zr).op(*result).op(cf->imm<u32>(offsetof(Q, axis) + offsetof(V, z)));
            cf->add(ir_store).op(wr).op(*result).op(cf->imm<u32>(offsetof(Q, angle)));
        }, public_access);

        t.method<void, const Q&>("operator*=", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            auto xa = self->getProp("axis").getProp("x");
            auto ya = self->getProp("axis").getProp("y");
            auto za = self->getProp("axis").getProp("z");
            auto wa = self->getProp("angle");

            auto xb = args[0].getProp("axis").getProp("x");
            auto yb = args[0].getProp("axis").getProp("y");
            auto zb = args[0].getProp("axis").getProp("z");
            auto wb = args[0].getProp("angle");

            auto xr = (wa * xb + wb * xa + ya * zb) - za * yb;
            auto yr = (wa * yb + wb * ya + za * xb) - xa * zb;
            auto zr = (wa * zb + wb * za + xa * yb) - ya * xb;
            auto wr = ((wa * wb - xa * xb) - ya * yb) - za * zb;

            cf->add(ir_store).op(xr).op(*self).op(cf->imm<u32>(offsetof(Q, axis) + offsetof(V, x)));
            cf->add(ir_store).op(yr).op(*self).op(cf->imm<u32>(offsetof(Q, axis) + offsetof(V, y)));
            cf->add(ir_store).op(zr).op(*self).op(cf->imm<u32>(offsetof(Q, axis) + offsetof(V, z)));
            cf->add(ir_store).op(wr).op(*self).op(cf->imm<u32>(offsetof(Q, angle)));
        }, public_access);

        t.method<T, const Q&>("dot", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            cf->add(ir_vdot).op(*result).op(*self).op(args[0]);
            *result += self->getProp("angle") * args[0].getProp("angle");
        }, public_access);

        t.method<V, const V&>("operator*", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            // result = axis * (axis.dot(rhs) * 2.0)
            Value axis = self->getProp("axis");
            Value axisDotRhs = cf->val<T>();
            cf->add(ir_vdot).op(axisDotRhs).op(axis).op(args[0]);
            axisDotRhs *= cf->imm<T>(2.0);

            cf->add(ir_vset).op(*result).op(axis);
            cf->add(ir_vmul).op(*result).op(axisDotRhs);

            Value tmp0 = cf->stack<V>();

            // result += rhs * (((angle * angle) * 2.0) - 1.0)
            cf->add(ir_vset).op(tmp0).op(args[0]);

            Value angle = self->getProp("angle");
            Value tmp1 = ((angle * angle) * cf->imm<T>(2.0)) - cf->imm<T>(1.0);
            cf->add(ir_vmul).op(tmp0).op(tmp1);
            cf->add(ir_vadd).op(*result).op(tmp0);

            // result += axis.cross(rhs) * (angle * 2.0)
            Value tmp2 = angle * cf->imm<T>(2.0);
            cf->add(ir_vcross).op(tmp0).op(axis).op(args[0]);
            cf->add(ir_vmul).op(tmp0).op(tmp2);
            cf->add(ir_vadd).op(*result).op(tmp0);
        }, public_access);

        t.staticMethod<Q, const V&, T>("fromAxisAngle", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            auto sin = c->getContext()->getFunctions()->findFunctions<T, T>("sin", fm_strict | fm_skip_implicit_args)[0];
            auto cos = c->getContext()->getFunctions()->findFunctions<T, T>("cos", fm_strict | fm_skip_implicit_args)[0];
            Value ha = args[1] * cf->imm<T>(0.5);
            Value sa = c->generateCall(sin, { ha });
            Value ca = c->generateCall(cos, { ha });

            Value axis = result->getProp("axis");
            cf->add(ir_vset).op(axis).op(args[0]);
            cf->add(ir_vmul).op(axis).op(sa);
            cf->add(ir_store).op(ca).op(*result).op(cf->imm<u32>(offsetof(Q, angle)));
        }, public_access);

        t.staticMethod<Q, T, T, T, T>("fromAxisAngle", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            auto sin = c->getContext()->getFunctions()->findFunctions<T, T>("sin", fm_strict | fm_skip_implicit_args)[0];
            auto cos = c->getContext()->getFunctions()->findFunctions<T, T>("cos", fm_strict | fm_skip_implicit_args)[0];
            Value ha = args[3] * cf->imm<T>(0.5);
            Value sa = c->generateCall(sin, { ha });
            Value ca = c->generateCall(cos, { ha });

            Value axis = result->getProp("axis");
            cf->add(ir_store).op(args[0]).op(axis).op(cf->imm<u32>(offsetof(V, x)));
            cf->add(ir_store).op(args[1]).op(axis).op(cf->imm<u32>(offsetof(V, y)));
            cf->add(ir_store).op(args[2]).op(axis).op(cf->imm<u32>(offsetof(V, z)));
            cf->add(ir_vmul).op(axis).op(sa);
            cf->add(ir_store).op(ca).op(*result).op(cf->imm<u32>(offsetof(Q, angle)));
        }, public_access);

        t.method("toString", +[](Q* self) {
            return String::Format("%f, %f, %f, %f", self->axis.x, self->axis.y, self->axis.z, self->angle);
        }, public_access);

        t.finalize();
    }

    template <typename T>
    void BindMat2(Module* m, const char* name) {
        using M = mat2<T>;
        using V = vec2<T>;

        auto t = bind<M>(m, name);
        t.prop("x", &M::x, public_access);
        t.prop("y", &M::y, public_access);

        t.ctor(+[](InlineCodeGenContext* ctx){
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto self = ctx->selfPointer;

            cf->add(ir_store).op(cf->imm<T>(1.0)).op(*self).op(cf->imm<u32>(offsetof(M, x) + offsetof(V, x)));
            cf->add(ir_store).op(cf->imm<T>(0.0)).op(*self).op(cf->imm<u32>(offsetof(M, x) + offsetof(V, y)));
            cf->add(ir_store).op(cf->imm<T>(0.0)).op(*self).op(cf->imm<u32>(offsetof(M, y) + offsetof(V, x)));
            cf->add(ir_store).op(cf->imm<T>(1.0)).op(*self).op(cf->imm<u32>(offsetof(M, y) + offsetof(V, y)));
        }, public_access);

        t.ctor<T, T, T, T>(+[](InlineCodeGenContext* ctx){
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            Value x = self->getProp("x");
            cf->add(ir_store).op(args[0]).op(x).op(cf->imm<u32>(offsetof(vec2<T>, x)));
            cf->add(ir_store).op(args[1]).op(x).op(cf->imm<u32>(offsetof(vec2<T>, y)));
            
            Value y = self->getProp("y");
            cf->add(ir_store).op(args[2]).op(y).op(cf->imm<u32>(offsetof(vec2<T>, x)));
            cf->add(ir_store).op(args[3]).op(y).op(cf->imm<u32>(offsetof(vec2<T>, y)));
        }, public_access);

        t.ctor<const V&, const V&>(+[](InlineCodeGenContext* ctx){
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            Value x = self->getProp("x");
            cf->add(ir_vset).op(x).op(args[0]);
            
            Value y = self->getProp("y");
            cf->add(ir_vset).op(y).op(args[2]);
        }, public_access);

        t.ctor<const M&>(+[](InlineCodeGenContext* ctx){
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            Value x = self->getProp("x");
            Value rx = args[0].getProp("x");
            cf->add(ir_vset).op(x).op(rx);
            
            Value y = self->getProp("y");
            Value ry = args[0].getProp("y");
            cf->add(ir_vset).op(y).op(ry);
        }, public_access);

        t.method<M, const M&>("operator*", +[](InlineCodeGenContext* ctx){
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto self = ctx->selfPointer;
            auto result = ctx->resultStorage;
            auto args = ctx->arguments;

            // transpose rhs to utilize vector dot product instructions later
            Value t = args[0].getProp("transposed");

            Value tx = t.getProp("x");
            Value ty = t.getProp("y");

            Value tmp = cf->val<T>();

            Value ax = self->getProp("x");
            cf->add(ir_vdot).op(tmp).op(ax).op(tx);
            cf->add(ir_store).op(tmp).op(*result).op(cf->imm<u32>(offsetof(M, x) + offsetof(V, x)));
            cf->add(ir_vdot).op(tmp).op(ax).op(ty);
            cf->add(ir_store).op(tmp).op(*result).op(cf->imm<u32>(offsetof(M, x) + offsetof(V, y)));

            Value ay = self->getProp("y");
            cf->add(ir_vdot).op(tmp).op(ay).op(tx);
            cf->add(ir_store).op(tmp).op(*result).op(cf->imm<u32>(offsetof(M, y) + offsetof(vec2<T>, x)));
            cf->add(ir_vdot).op(tmp).op(ay).op(ty);
            cf->add(ir_store).op(tmp).op(*result).op(cf->imm<u32>(offsetof(M, y) + offsetof(vec2<T>, y)));
        }, public_access);

        t.method<void, const M&>("operator*=", +[](InlineCodeGenContext* ctx){
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto self = ctx->selfPointer;
            auto args = ctx->arguments;

            // transpose rhs to utilize vector dot product instructions later
            Value t = args[0].getProp("transposed");

            Value tx = t.getProp("x");
            Value ty = t.getProp("y");

            Value ax = self->getProp("x");
            Value tmpX = cf->val<T>();
            Value tmpY = cf->val<T>();
            cf->add(ir_vdot).op(tmpX).op(ax).op(tx);
            cf->add(ir_vdot).op(tmpY).op(ax).op(ty);
            cf->add(ir_store).op(tmpX).op(ax).op(cf->imm<u32>(offsetof(V, x)));
            cf->add(ir_store).op(tmpY).op(ax).op(cf->imm<u32>(offsetof(V, y)));

            Value ay = self->getProp("y");
            cf->add(ir_vdot).op(tmpX).op(ay).op(tx);
            cf->add(ir_vdot).op(tmpY).op(ay).op(ty);
            cf->add(ir_store).op(tmpX).op(ay).op(cf->imm<u32>(offsetof(V, x)));
            cf->add(ir_store).op(tmpY).op(ay).op(cf->imm<u32>(offsetof(V, y)));
        }, public_access);

        t.method<V, const V&>("operator*", +[](InlineCodeGenContext* ctx){
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto self = ctx->selfPointer;
            auto result = ctx->resultStorage;
            auto args = ctx->arguments;

            // transpose rhs to utilize vector dot product instructions later
            Value t = self->getProp("transposed");

            Value tx = t.getProp("x");
            Value ty = t.getProp("y");

            Value tmp = cf->val<T>();

            cf->add(ir_vdot).op(tmp).op(tx).op(args[0]);
            cf->add(ir_store).op(tmp).op(*result).op(cf->imm<u32>(offsetof(V, x)));
            cf->add(ir_vdot).op(tmp).op(ty).op(args[0]);
            cf->add(ir_store).op(tmp).op(*result).op(cf->imm<u32>(offsetof(V, y)));
        }, public_access);

        t.prop<M>("transposed", +[](InlineCodeGenContext* ctx){
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto self = ctx->selfPointer;
            auto result = ctx->resultStorage;
            auto args = ctx->arguments;

            Value bx = self->getProp("x");
            Value by = self->getProp("y");
            Value bxx = bx.getProp("x");
            Value bxy = bx.getProp("y");
            Value byx = by.getProp("x");
            Value byy = by.getProp("y");

            cf->add(ir_store).op(bxx).op(*result).op(cf->imm<u32>(offsetof(M, x) + offsetof(V, x)));
            cf->add(ir_store).op(byx).op(*result).op(cf->imm<u32>(offsetof(M, x) + offsetof(V, y)));
            cf->add(ir_store).op(bxy).op(*result).op(cf->imm<u32>(offsetof(M, y) + offsetof(V, x)));
            cf->add(ir_store).op(byy).op(*result).op(cf->imm<u32>(offsetof(M, y) + offsetof(V, y)));

        }, nullptr, public_access);

        t.method("toString", +[](M* self) {
            return String::Format("[%f, %f], [%f, %f]", self->x.x, self->x.y, self->y.x, self->y.y);
        }, public_access);

        t.finalize();
    }

    void BindMath(Context* ctx) {
        Module* m = ctx->createHostModule("math");

        m->addData("PI", 3.141592653589793);
        m->addData("E", 2.718281828459045);

        bind<f32, f32>(m, "radians", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto args = ctx->arguments;

            cf->add(ir_fmul).op(*result).op(args[0]).op(cf->imm<f32>(0.0174533f));
        });

        bind<f64, f64>(m, "radians", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto args = ctx->arguments;

            cf->add(ir_fmul).op(*result).op(args[0]).op(cf->imm<f64>(0.0174533));
        });

        bind<f32, f32>(m, "degrees", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto args = ctx->arguments;

            cf->add(ir_fmul).op(*result).op(args[0]).op(cf->imm<f32>(57.2958f));
        });

        bind<f64, f64>(m, "degrees", +[](InlineCodeGenContext* ctx) {
            auto c = ctx->compiler;
            auto cf = c->currentFunction();
            auto result = ctx->resultStorage;
            auto args = ctx->arguments;

            cf->add(ir_fmul).op(*result).op(args[0]).op(cf->imm<f64>(57.2958));
        });

        bind<f32, f32>(m, "sin", sinf);
        bind<f64, f64>(m, "sin", sin);
        bind<f32, f32>(m, "sinh", sinhf);
        bind<f64, f64>(m, "sinh", sinh);
        bind<f32, f32>(m, "asin", asinf);
        bind<f64, f64>(m, "asin", asin);
        bind<f32, f32>(m, "asinh", asinhf);
        bind<f64, f64>(m, "asinh", asinh);

        bind<f32, f32>(m, "cos", cosf);
        bind<f64, f64>(m, "cos", cos);
        bind<f32, f32>(m, "cosh", coshf);
        bind<f64, f64>(m, "cosh", cosh);
        bind<f32, f32>(m, "acos", acosf);
        bind<f64, f64>(m, "acos", acos);
        bind<f32, f32>(m, "acosh", acoshf);
        bind<f64, f64>(m, "acosh", acosh);

        bind<f32, f32>(m, "tan", tanf);
        bind<f64, f64>(m, "tan", tan);
        bind<f32, f32>(m, "tanh", tanhf);
        bind<f64, f64>(m, "tanh", tanh);
        bind<f32, f32>(m, "atan", atanf);
        bind<f64, f64>(m, "atan", atan);
        bind<f32, f32>(m, "atan", atanhf);
        bind<f64, f64>(m, "atan", atanh);
        bind<f32, f32, f32>(m, "atan2", atan2f);
        bind<f64, f64, f64>(m, "atan2", atan2);

        bind<f32, f32>(m, "ceil", ceilf);
        bind<f64, f64>(m, "ceil", ceil);
        bind<f32, f32>(m, "floor", floorf);
        bind<f64, f64>(m, "floor", floor);
        bind<f32, f32>(m, "exp", expf);
        bind<f64, f64>(m, "exp", exp);
        bind<f32, f32>(m, "exp2", exp2f);
        bind<f64, f64>(m, "exp2", exp2);
        bind<f32, f32>(m, "log", logf);
        bind<f64, f64>(m, "log", log);
        bind<f32, f32>(m, "log2", log2f);
        bind<f64, f64>(m, "log2", log2);
        bind<f32, f32>(m, "log10", log10f);
        bind<f64, f64>(m, "log10", log10);
        bind<f32, f32, f32>(m, "pow", powf);
        bind<f64, f64, f64>(m, "pow", pow);
        bind<f32, f32>(m, "sqrt", sqrtf);
        bind<f64, f64>(m, "sqrt", sqrt);
        bind<f32, f32>(m, "abs", fabs);
        bind<f64, f64>(m, "abs", abs);
        bind<i32, i32>(m, "abs", abs);
        bind<i64, i64>(m, "abs", abs);

        BindVec2<f32>(m, "vec2f");
        BindVec2<f64>(m, "vec2d");
        BindVec3<f32>(m, "vec3f");
        BindVec3<f64>(m, "vec3d");
        BindVec4<f32>(m, "vec4f");
        BindVec4<f64>(m, "vec4d");
        BindQuat<f32>(m, "quatf");
        BindQuat<f64>(m, "quatd");
        BindMat2<f32>(m, "mat2f");
        BindMat2<f64>(m, "mat2d");
    }
};