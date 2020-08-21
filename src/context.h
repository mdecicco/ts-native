#pragma once
#include <robin_hood.h>
#include <string>

namespace gjs {
	struct wrapped_function;
	struct wrapped_class;

	class vm_state;
	class vm_context {
		public:
			vm_context();
			~vm_context();

			void add(const std::string& name, wrapped_function* func);
			void add(const std::string& name, wrapped_class* cls);

			wrapped_function* function(const std::string& name);
			wrapped_class* prototype(const std::string& name);
			vm_state* state() { return m_state; }


		protected:
			robin_hood::unordered_map<std::string, wrapped_function*> m_host_functions;
			robin_hood::unordered_map<std::string, wrapped_class*> m_host_classes;
			vm_state* m_state;
	};
};
