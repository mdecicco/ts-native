#pragma once
#include <instruction.h>
#include <instruction_encoder.h>

namespace gjs {
	class vm_allocator;

	class instruction_array {
		public:
			instruction_array(vm_allocator* allocator);
			~instruction_array();

			void operator += (const instruction& i);

			inline instruction operator[](u32 index) const { return m_instructions[index]; }
			inline void set(u64 index, const instruction& i) { m_instructions[index] = i; }
			inline u64 size() const { return m_count; }
			void remove(u64 from, u64 to);
			void remove(u64 index);

		protected:
			vm_allocator* m_allocator;
			instruction* m_instructions;
			u64 m_count;
			u64 m_capacity;
	};
};
