#pragma once
#include <template_utils.hpp>
#include <types.h>

#include <string>
#include <vector>
#include <robin_hood.h>

namespace gjs {
	namespace bind {
		struct wrapped_class;
	};

	class vm_context;
	class vm_type;

	class type_manager {
		public:
			type_manager(vm_context* ctx);
			~type_manager();

			vm_type* get(const std::string& internal_name);

			vm_type* get(u32 id);

			template <typename T>
			vm_type* get() {
				return get(base_type_name<T>());
			}

			vm_type* add(const std::string& name, const std::string& internal_name);

			vm_type* finalize_class(bind::wrapped_class* wrapped);

			std::vector<vm_type*> all();

		protected:
			friend class vm_function;
			vm_context* m_ctx;
			robin_hood::unordered_map<std::string, vm_type*> m_types;
			robin_hood::unordered_map<u32, vm_type*> m_types_by_id;
	};

	class vm_function;
	class vm_type {
		public:
			std::string name;
			std::string internal_name;
			u32 size;
			bool is_primitive;
			bool is_unsigned;
			bool is_floating_point;
			bool is_builtin;
			bool requires_subtype;

			struct property {
				u8 flags;
				std::string name;
				vm_type* type;
				u64 offset;
				vm_function* getter;
				vm_function* setter;
			};

			vm_type* base_type;
			vm_type* sub_type;
			std::vector<property> properties;
			vm_function* constructor;
			vm_function* destructor;
			std::vector<vm_function*> methods;

			inline u32 id() const { return m_id; }

		protected:
			friend class type_manager;
			bind::wrapped_class* m_wrapped;
			u32 m_id;
			vm_type();
			~vm_type();
	};

	struct subtype_t {
		template<typename T>
		T get() {
		}

		void* data;
	};
};