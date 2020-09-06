#include <compiler/data_type.h>
#include <compiler/function.h>
#include <compiler/context.h>
#include <compile_log.h>
#include <parse.h>
#include <bind.h>

using namespace std;

namespace gjs {
	using nt = ast_node::node_type;

	data_type::data_type(const std::string& _name, bool _built_in) :
		name(_name), built_in(_built_in), is_floating_point(false), is_unsigned(false), ctor(nullptr),
		dtor(nullptr), type(nullptr), size(0), actual_size(0), is_primitive(false), requires_subtype(false),
		base_type(nullptr), sub_type(nullptr), type_id(hash(_name))
	{ }

	data_type::data_type(compile_context& ctx, ast_node* node) {
		type_id = hash(*node->identifier);
		ctor = nullptr;
		dtor = nullptr;
		type = nullptr;
		built_in = false;
		is_floating_point = false;
		is_unsigned = false;
		is_primitive = false;
		requires_subtype = false;
		base_type = nullptr;
		sub_type = nullptr;
		name = *node->identifier;
		ast_node* n = node->body;
		actual_size = 0;
		size = sizeof(void*);

		ctx.types.push_back(this);
		while (n) {
			if (n->type == nt::variable_declaration) {
				// class
				data_type* t = ctx.type(*n->data_type);
				props.push_back({
					bind::pf_none,
					*n->identifier,
					t,
					u32(actual_size),
					nullptr,
					nullptr
				});
				actual_size += t->size;
			} else if (n->type == nt::format_property) {
				// format
				data_type* t = ctx.type(*n->data_type);
				props.push_back({
					bind::pf_none,
					*n->identifier,
					t,
					u32(actual_size),
					nullptr,
					nullptr
				});
				actual_size += t->size;
			}
			n = n->next;
		}

		if (node->type == nt::class_declaration) {
			if (node->constructor) {
				func* m = new func();
				m->this_type = this;
				m->is_ctor = true;
				compile_function(ctx, node->constructor, m);
				m->name = name + "::construct";
				ctor = m;
				ctx.funcs.push_back(m);
			}

			if (node->destructor) {
				func* m = new func();
				m->this_type = this;
				m->is_dtor = true;
				compile_function(ctx, node->destructor, m);
				m->name = name + "::destruct";
				dtor = m;
				ctx.funcs.push_back(m);
			}

			n = node->body;
			while (n) {
				if (n->type == nt::function_declaration) {
					func* m = new func();
					m->this_type = this;
					compile_function(ctx, n, m);
					methods.push_back(m);

					if (m->name.find_first_of("operator") != string::npos) {
						// determine required argument count
						vector<string> parts = split(m->name, " \t\n");
						u32 required_argc = 0;
						bool casting_op = ctx.type(parts[1]) != nullptr;
						if (!casting_op) {
							if (parts[1] != "++" && parts[1] != "--" && parts[1] != "!") {
								// non-casting binary operator
								required_argc = 1;
							}
						}

						u32 argc = 0;
						ast_node* a = n->arguments ? n->arguments->body : nullptr;
						while (a) { argc++; a = a->next; }

						if (argc != required_argc) {
							if (required_argc == 0) {
								if (casting_op) ctx.log->err("Casting operators operators must take no arguments", n);
								else ctx.log->err("Unary operators must take no arguments", n);
							} else ctx.log->err("Non-casting binary operators must take exactly one argument", n);
						}
					}

					m->name = name + "::" + m->name;
					ctx.funcs.push_back(m);
				}
				n = n->next;
			}
		}
	}

	bool data_type::equals(data_type* rhs) {
		return name == rhs->name;
	}

	// <type>::operator <to type>()
	func* data_type::cast_to_func(data_type* to) {
		return method("operator " + to->name);
	}

	// <type>::operator=(<from type> rhs)
	func* data_type::set_from_func(data_type* from) {
		return nullptr;
	}

	data_type::property* data_type::prop(const std::string& _name) {
		for (u32 p = 0;p < props.size();p++) {
			if (props[p].name == _name) return &props[p];
		}
		return nullptr;
	}

	func* data_type::method(const std::string& _name) {
		for (u32 m = 0;m < methods.size();m++) {
			if (_name.find_first_of(' ') != string::npos) {
				// probably an operator
				vector<string> mparts = split(split(methods[m]->name, ":")[1], " \t\n\r");
				vector<string> sparts = split(_name, " \t\n\r");
				if (mparts.size() != sparts.size()) continue;
				bool matched = true;
				for (u32 i = 0;matched && i < mparts.size();i++) {
					matched = mparts[i] == sparts[i];
				}
				if (matched) return methods[m];
			}
			string& bt_name = base_type ? base_type->name : name;
			if (methods[m]->name == bt_name + "::" + _name) return methods[m];
		}
		return nullptr;
	}
};