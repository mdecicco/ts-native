#include <compiler/context.h>
#include <compiler/function.h>
#include <compiler/data_type.h>

#include <instruction_array.h>
#include <parse.h>
#include <source_map.h>

using namespace std;

namespace gjs {
	void compile_context::add(instruction& i, ast_node* because) {
		address addr = out->size();
		(*out) += i;

		///*
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
		return type(*node);
	}

	func* compile_context::function(const string& name) {
		for (u16 i = 0;i < funcs.size();i++) {
			if (funcs[i]->name == name) return funcs[i];
		}
		return nullptr;
	}
};