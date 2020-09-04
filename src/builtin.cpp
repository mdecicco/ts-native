#include <vm_function.h>
#include <vm_type.h>
#include <builtin.h>
#include <vm_type.h>
#include <context.h>
#include <bind.h>

namespace gjs {
	// todo: thread_id:ctx map
	static vm_context* ctx = nullptr;

	void set_builtin_context(vm_context* _ctx) {
		ctx = _ctx;
	}

	void init_context(vm_context* ctx) {
		vm_type* tp = nullptr;

		tp = nullptr;
		tp = ctx->types()->add("i64", typeid(i64).name());
		tp->is_primitive = true;
		tp->is_builtin = true;
		tp->size = sizeof(i64);

		tp = nullptr;
		tp = ctx->types()->add("u64", typeid(u64).name());
		tp->is_primitive = true;
		tp->is_unsigned = true;
		tp->is_builtin = true;
		tp->size = sizeof(u64);

		tp = nullptr;
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
		tp->is_builtin = true;
		tp->size = 0;

		tp = ctx->types()->add("data", "void*");
		tp->is_builtin = true;
		tp->size = sizeof(void*);
		tp->is_unsigned = true;

		tp = ctx->types()->add("string", "char");
		tp->is_builtin = true;
		tp->size = sizeof(char*);

		ctx->bind<subtype_t>("__subtype__").finalize();

		auto arr = ctx->bind<script_array>("array");
		arr.constructor<vm_type*>();
		arr.method("push", &script_array::push);
		arr.method("operator []", &script_array::operator[]);
		arr.prop(std::string("length"), &script_array::length, bind::property_flags::pf_read_only);
		tp = arr.finalize();
		tp->requires_subtype = true;
		tp->is_builtin = true;


		ctx->bind(script_allocate, "alloc");
		ctx->bind(script_free, "free");
		ctx->bind(script_copymem, "memcopy");
	}

	void* script_allocate(uinteger size) {
		// todo: allocate from context memory
		void* mem = malloc(size);
		return mem;
	}

	void script_free(void* ptr) {
		free(ptr);
	}

	void script_copymem(void* to, void* from, uinteger size) {
		memmove(to, from, size);
	}


	script_array::script_array(vm_type* type) : m_size(0), m_count(0), m_capacity(0), m_type(type) {
	}

	script_array::~script_array() {
	}

	void script_array::push(subtype_t* elem) {
		if (m_capacity == 0) {
			m_data = new u8[32 * m_type->size];
			m_capacity = 32;
		}
		memcpy(m_data + (m_count * m_type->size), &elem->data, m_type->size);
		m_count++;
	}

	subtype_t* script_array::operator[](u32 idx) {
		return (subtype_t*)(m_data + (idx * m_type->size));
	}

	u32 script_array::length() {
		return m_count;
	}
};