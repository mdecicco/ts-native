#pragma once
#include <types.h>
#include <vm_state.h>
#include <instruction_array.h>

namespace gjs {
	class vm_allocator;
	class vm_context;
	class vm {
		public:
			vm(vm_context* ctx, vm_allocator* alloc, u32 stack_size, u32 mem_size);
			~vm();

			void execute(const instruction_array& code, address entry);

			vm_allocator* alloc;
			vm_state state;

		protected:
			void jit(const instruction_array& code);
			vm_context* m_ctx;
			u32 m_stack_size;
	};
};

