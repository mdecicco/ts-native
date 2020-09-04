#include <compiler/context.h>
#include <compiler/function.h>
#include <compiler/data_type.h>

#include <instruction_array.h>
#include <compile_log.h>
#include <parse.h>
#include <source_map.h>

using namespace std;

namespace gjs {
	void compile_context::clear_last_member_info() {
		do_store_member_info = false;
		last_member_or_method.is_set = false;
		last_member_or_method.name = "";
		last_member_or_method.subject = nullptr;
		last_member_or_method.type = nullptr;
		last_member_or_method.method = nullptr;
	}

	void compile_context::add(instruction& i, ast_node* because) {
		address addr = out->size();
		(*out) += i;

		/*
		std::string ln = "";
		u32 wscount = 0;
		bool reachedText = false;
		for (u32 c = 0;c < because->start.lineText.length();c++) {
			if (isspace(because->start.lineText[c]) && !reachedText) wscount++;
			else {
				reachedText = true;
				ln += because->start.lineText[c];
			}
		}
		printf("%s\n", ln.c_str());
		for (u32 c = 0;c < because->start.col - wscount;c++) printf(" ");
		printf("^ ");
		printf("%s\n\n", i.to_string().c_str());
		//*/

		map->append(because);
	}

	data_type* compile_context::type(const string& name) {
		for (u16 i = 0;i < types.size();i++) {
			if (types[i]->name == name) return types[i];
		}
		return nullptr;
	}

	data_type* compile_context::type(ast_node* node) {
		data_type* t = type(*node);
		if (t && node->data_type) {
			if (!t->requires_subtype) {
				log->err(format("Type '%s' does not accept a sub-type", t->name.c_str()), node->data_type);
			} else {
				data_type* st = type(node->data_type);
				data_type* ct = type(t->name + "<" + st->name + ">");
				if (!ct) {
					// combined type must be created
					ct = new data_type(t->name + "<" + st->name + ">");
					ct->requires_subtype = true;
					ct->type_id = hash(ct->name);
					ct->size = t->size;
					ct->actual_size = t->actual_size;
					ct->built_in = t->built_in;
					ct->ctor = t->ctor;
					ct->dtor = t->dtor;
					ct->props = t->props;
					ct->methods = t->methods;
					ct->base_type = t;
					ct->sub_type = st;
					types.push_back(ct);
				}

				return ct;
			}
		}
		return t;
	}

	func* compile_context::function(const string& name) {
		for (u16 i = 0;i < funcs.size();i++) {
			if (funcs[i]->name == name) return funcs[i];
		}
		return nullptr;
	}
};