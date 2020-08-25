#pragma once
#include <types.h>
#include <parse_utils.h>
#include <string>
#include <vector>

namespace gjs {
	struct ast_node {
		enum class node_type {
			root = 0,
			empty,
			variable_declaration,
			function_declaration,
			class_declaration,
			format_declaration,
			variable_initializer,
			format_property,
			function_arguments,
			if_statement,
			for_loop,
			while_loop,
			do_while_loop,
			import_statement,
			export_statement,
			return_statement,
			object,
			call,
			expression,
			conditional,
			constant,
			identifier,
			type_identifier,
			operation,
			context_type,
			context_function
		};
		enum class constant_type {
			none,
			integer,
			decimal,
			string
		};
		enum class operation_type {
			invalid = 0,
			add,
			sub,
			mul,
			div,
			mod,
			shiftLeft,
			shiftRight,
			land,
			lor,
			band,
			bor,
			bxor,
			addEq,
			subEq,
			mulEq,
			divEq,
			modEq,
			shiftLeftEq,
			shiftRightEq,
			landEq,
			lorEq,
			bandEq,
			borEq,
			bxorEq,
			preInc,
			preDec,
			postInc,
			postDec,
			less,
			greater,
			lessEq,
			greaterEq,
			notEq,
			isEq,
			eq,
			not,
			negate,
			addr,
			at,
			member
		};

		struct source_location {
			std::string file;
			std::string lineText;
			u32 line;
			u32 col;
		};

		ast_node();
		~ast_node();
		void debug_print(u32 tab_level, bool dontIndentFirst = false, bool inArray = false);

		node_type type;
		source_location start;
		source_location end;
		ast_node* next;

		ast_node* data_type;
		ast_node* identifier;

		ast_node* body;
		ast_node* else_body;
		ast_node* property;
		ast_node* arguments;

		ast_node* initializer;
		ast_node* lvalue;
		ast_node* rvalue;
		ast_node* callee;
		ast_node* constructor;
		ast_node* destructor;
		ast_node* condition;
		ast_node* modifier;

		constant_type c_type;
		union {
			i64 i;
			double d;
			char* s;
		} value;

		inline void set(i64 v) { value.i = v; }
		inline void set(double v) { value.d = v; }
		inline void set(const std::string& v) {
			value.s = new char[v.length() + 1];
			memset(value.s, 0, v.length() + 1);
			snprintf(value.s, v.length() + 1, "%s", v.c_str());
		}
		inline void src(const tokenizer& t, const tokenizer::token& tk) {
			start.lineText = t.lines[tk.line];
			start.file = tk.file;
			start.line = tk.line;
			start.col = tk.col;
		}

		operator std::string() const { return value.s; }
		operator i64() { return value.i; }
		operator double() { return value.d; }

		// per-type values
		operation_type op;
		bool is_const;
		bool is_static;
	};

	class vm_context;
	ast_node* parse_source(vm_context* ctx, const std::string& file, const std::string& source);
};

