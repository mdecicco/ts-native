#pragma once
#include <instruction.h>
#include <instruction_encoder.h>

namespace gjs {
	class vm_allocator;

	class instruction_array {
		public:
			instruction_array(vm_allocator* allocator);
			~instruction_array();

			void operator += (const instruction_encoder& i);
			void operator += (instruction i);

			inline instruction operator[](u32 index) const { return m_instructions[index]; }
			inline void set(u32 index, const instruction_encoder& i) { m_instructions[index] = i; }
			inline void set(u32 index, instruction i) { m_instructions[index] = i; }
			inline u32 size() const { return m_count; }

		protected:
			vm_allocator* m_allocator;
			instruction* m_instructions;
			u32 m_count;
			u32 m_capacity;
	};
};
