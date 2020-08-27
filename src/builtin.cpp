#include <builtin.h>
#include <context.h>

namespace gjs {
	// todo: thread_id:ctx map
	static vm_context* ctx = nullptr;

	void set_builtin_context(vm_context* _ctx) {
		ctx = _ctx;
	}

	void init_context(vm_context* ctx) {
		vm_type* tp = nullptr;
		tp = ctx->types()->add("i32", typeid(i32).name());
		tp->is_primitive = true;
		tp->is_builtin = true;
		tp->size = sizeof(i32);

		tp = nullptr;
		tp = ctx->types()->add("u32", typeid(u32).name());
		tp->is_primitive = true;
		tp->is_unsigned = true;
		tp->is_builtin = true;
		tp->size = sizeof(u32);

		tp = nullptr;
		tp = ctx->types()->add("i16", typeid(i16).name());
		tp->is_primitive = true;
		tp->is_builtin = true;
		tp->size = sizeof(i16);

		tp = nullptr;
		tp = ctx->types()->add("u16", typeid(u16).name());
		tp->is_primitive = true;
		tp->is_unsigned = true;
		tp->is_builtin = true;
		tp->size = sizeof(u16);

		tp = nullptr;
		tp = ctx->types()->add("i8", typeid(i8).name());
		tp->is_primitive = true;
		tp->is_builtin = true;
		tp->size = sizeof(i8);

		tp = nullptr;
		tp = ctx->types()->add("u8", typeid(u8).name());
		tp->is_primitive = true;
		tp->is_unsigned = true;
		tp->is_builtin = true;
		tp->size = sizeof(u8);

		tp = ctx->types()->add("f32", typeid(f32).name());
		tp->is_primitive = true;
		tp->is_floating_point = true;
		tp->is_builtin = true;
		tp->size = sizeof(f32);

		tp = ctx->types()->add("f64", typeid(f64).name());
		tp->is_primitive = true;
		tp->is_floating_point = true;
		tp->is_builtin = true;
		tp->size = sizeof(f64);

		tp = ctx->types()->add("bool", typeid(bool).name());
		tp->is_primitive = true;
		tp->is_builtin = true;
		tp->size = sizeof(bool);

		tp = ctx->types()->add("void", "void");
		tp->is_primitive = false;
		tp->is_builtin = true;
		tp->size = 0;

		tp = ctx->types()->add("data", "void*");
		tp->is_primitive = false;
		tp->is_builtin = true;
		tp->size = sizeof(void*);
		tp->is_unsigned = true;

		tp = ctx->types()->add("string", "char");
		tp->is_primitive = false;
		tp->is_builtin = true;
		tp->size = sizeof(char*);

		ctx->bind(script_allocate, "alloc");
		ctx->bind(script_free, "free");
	}

	void* script_allocate(u32 size) {
		// todo: allocate from context memory
		void* mem = malloc(size);
		return mem;
	}

	void script_free(void* ptr) {
		free(ptr);
	}
};