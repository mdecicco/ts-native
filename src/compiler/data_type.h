#pragma once
#include <string>
#include <vector>
#include <types.h>

namespace gjs {
	class vm_type;
	struct func;
	struct ast_node;
	struct compile_context;

	struct data_type {
		struct property {
			u8 flags;
			std::string name;
			data_type* type;
			u64 offset;
			func* getter;
			func* setter;
		};

		std::string name;
		bool built_in;
		bool is_floating_point;
		bool is_unsigned;
		bool is_primitive;
		bool accepts_subtype;

		// the following two sizes are equal for primitive types
		// for structures, classes, the size is sizeof(void*)
		u64 actual_size;
		u64 size;
		std::vector<property> props;
		std::vector<func*> methods;
		func* ctor;
		func* dtor;
		data_type* base_type;
		data_type* sub_type;
		vm_type* type;

		data_type(const std::string& _name, bool _built_in = false);
		data_type(compile_context& ctx, ast_node* node);

		bool equals(data_type* rhs);
		func* cast_to_func(data_type* to);
		func* set_from_func(data_type* from);

		property* prop(const std::string& _name);
		func* method(const std::string& _name);
	};
};