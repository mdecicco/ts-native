#pragma once
#include <register.h>

namespace gjs {
	class vm_memory {
		public:
			vm_memory(u32 size);
			~vm_memory();

			inline u8* operator[](u32 addr) { return &m_data[addr]; }
			inline u32 size() const { return m_size; }

		protected:
			u8* m_data;
			u32 m_size;
	};

	class vm_state {
		public:
			vm_state(u32 memory_size);
			~vm_state() { }

			vm_memory memory;
			u64 registers[u8(vm_register::register_count)];
	};
};
