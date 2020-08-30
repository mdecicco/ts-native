#pragma once
#include <types.h>

namespace gjs {
	class vm_context;
	class vm_type;
	void set_builtin_context(vm_context* ctx);
	void init_context(vm_context* ctx);
	
	void* script_allocate(uinteger size);
	void script_free(void* ptr);
	void script_copymem(void* to, void* from, uinteger size);

	class script_array {
		public:
			script_array(vm_type* type);
			~script_array();

			f32 operator[](u32 idx);
	};
};