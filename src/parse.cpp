#include <parse.h>
#include <parse_utils.h>
#include <context.h>

using namespace std;

namespace gjs {
	using token = tokenizer::token;

	struct parse_frame {
		vector<ast_node*> variable_declarations;
		vector<ast_node*> type_declarations;
	};

	struct parse_context {
		vm_context* env;
		ast_node* root;
		stack<ast_node*> node_path;
		vector<parse_frame> frames;

		// set before parsing static/const variables, unset after
		bool var_is_static;
		bool var_is_const;

		ast_node* find_variable(const string& name) {
			for(auto i = frames.rbegin();i != frames.rend();i++) {
				parse_frame& f = *i;
				for (u32 i = 0;i < f.variable_declarations.size();i++) {
					if (string(*f.variable_declarations[i]->identifier) == name) return f.variable_declarations[i];
				}
			}

			return nullptr;
		}

		ast_node* find_type(const string& name) {
			for(auto i = frames.rbegin();i != frames.rend();i++) {
				parse_frame& f = *i;
				for (u32 i = 0;i < f.type_declarations.size();i++) {
					if (string(*f.type_declarations[i]->identifier) == name) return f.type_declarations[i];
				}
			}

			return nullptr;
		}
	};

	ast_node* parse_next(parse_context& ctx, tokenizer& t);
	ast_node* identifier(tokenizer& t, token& tk);
	void init_tokenizer(tokenizer& t);


	// https://craftinginterpreters.com/parsing-expressions.html
	struct expr_parser {
		expr_parser(parse_context& _ctx, tokenizer& _t, const token& expr) : ctx(_ctx), base_t(_t), t(expr.text) {
			t.file = _t.file;
			init_tokenizer(t);
			base = expr;
			cur_token = 0;
		}

		void get_tokens() {
			try {
				while (!t.at_end()) {
					token tok = t.any_token();
					if (tok) tokens.push_back(tok);
					else break;
				}
			} catch (const parse_exception& e) {
				u32 col = e.col;
				if (e.line == 0) col += base.col;
				throw parse_exception(
					t.file,
					e.text,
					base_t.lines[e.line + base.line],
					e.line + base.line,
					col
				);
			}
		}

		ast_node* parse() {
			get_tokens();
			if (tokens.size() == 0) return nullptr;

			try {
				return expr();
			} catch (const parse_exception& e) {
				u32 col = e.col;
				if (e.line == 0) col += base.col;

				throw parse_exception(
					t.file,
					e.text,
					base_t.lines[e.line + base.line],
					e.line + base.line,
					col
				);
			}

			return nullptr;
		}
		
		void consume() { cur_token++; }

		bool at_end() { return cur_token == tokens.size(); }

		token current() {
			if (cur_token >= tokens.size()) return { 0, 0, "" };
			return tokens[cur_token];
		}

		bool match(const vector<string>& tokens) {
			token cur = current();
			for (u8 i = 0;i < tokens.size();i++) {
				if (cur.text == tokens[i]) return true;
			}

			return false;
		}

		ast_node* expr() {
			return assign();
		}

		ast_node* assign() {
			ast_node* ret = conditional();
			token cur = current();
			while (match({ "=", "+=", "-=", "*=", "/=", "%=", "<<=", ">>=", "&=", "^=", "|=", "&&=", "||=" })) {
				consume();

				ast_node* op = new ast_node();
				op->type = ast_node::node_type::operation;
				if (cur.text == "=") op->op = ast_node::operation_type::eq;
				else if (cur.text == "+=") op->op = ast_node::operation_type::addEq;
				else if (cur.text == "-=") op->op = ast_node::operation_type::subEq;
				else if (cur.text == "*=") op->op = ast_node::operation_type::mulEq;
				else if (cur.text == "/=") op->op = ast_node::operation_type::divEq;
				else if (cur.text == "%=") op->op = ast_node::operation_type::modEq;
				else if (cur.text == "<<=") op->op = ast_node::operation_type::shiftLeftEq;
				else if (cur.text == ">>=") op->op = ast_node::operation_type::shiftRightEq;
				else if (cur.text == "&=") op->op = ast_node::operation_type::bandEq;
				else if (cur.text == "^=") op->op = ast_node::operation_type::bxorEq;
				else if (cur.text == "|=") op->op = ast_node::operation_type::borEq;
				else if (cur.text == "&&=") op->op = ast_node::operation_type::landEq;
				else if (cur.text == "||=") op->op = ast_node::operation_type::lorEq;
				op->lvalue = ret;
				op->rvalue = conditional();
				if (!op->rvalue) {
					throw parse_exception(
						"Expected expression for rvalue",
						t, cur
					);
				}

				ret = op;
				set_node_src(op, cur);
				cur = current();
			}

			return ret;
		}

		ast_node* conditional() {
			ast_node* ret = logical_or();

			token cur = current();
			if (match({ "?" })) {
				consume();
				ast_node* cond = new ast_node();
				cond->type = ast_node::node_type::conditional;
				cond->condition = ret;
				cond->lvalue = expr();
				if (!cond->lvalue) {
					throw parse_exception(
						"Expected expression for conditional 'true' case",
						t, cur
					);
				}

				if (!match({ ":" })) {
					throw parse_exception(
						"Expected ':' for conditional case separator",
						t, cur
					);
				}

				cond->rvalue = expr();
				if (!cond->rvalue) {
					throw parse_exception(
						"Expected expression for conditional 'false' case",
						t, cur
					);
				}

				set_node_src(cond, cur);
				ret = cond;
			}

			return ret;
		}

		ast_node* logical_or() {
			ast_node* ret = logical_and();
			token cur = current();
			while (match({ "||" })) {
				consume();

				ast_node* op = new ast_node();
				op->type = ast_node::node_type::operation;
				op->op = ast_node::operation_type::lor;
				op->lvalue = ret;
				op->rvalue = logical_and();
				if (!op->rvalue) {
					throw parse_exception(
						"Expected expression for rvalue",
						t, cur
					);
				}

				ret = op;
				set_node_src(op, cur);
				cur = current();
			}

			return ret;
		}

		ast_node* logical_and() {
			ast_node* ret = binary_or();
			token cur = current();
			while (match({ "&&" })) {
				consume();

				ast_node* op = new ast_node();
				op->type = ast_node::node_type::operation;
				op->op = ast_node::operation_type::land;
				op->lvalue = ret;
				op->rvalue = binary_or();
				if (!op->rvalue) {
					throw parse_exception(
						"Expected expression for rvalue",
						t, cur
					);
				}

				ret = op;
				set_node_src(op, cur);
				cur = current();
			}

			return ret;
		}

		ast_node* binary_or() {
			ast_node* ret = binary_xor();
			token cur = current();
			while (match({ "|" })) {
				consume();

				ast_node* op = new ast_node();
				op->type = ast_node::node_type::operation;
				op->op = ast_node::operation_type::bor;
				op->lvalue = ret;
				op->rvalue = binary_xor();
				if (!op->rvalue) {
					throw parse_exception(
						"Expected expression for rvalue",
						t, cur
					);
				}

				ret = op;
				set_node_src(op, cur);
				cur = current();
			}

			return ret;
		}

		ast_node* binary_xor() {
			ast_node* ret = binary_and();
			token cur = current();
			while (match({ "^" })) {
				consume();

				ast_node* op = new ast_node();
				op->type = ast_node::node_type::operation;
				op->op = ast_node::operation_type::bxor;
				op->lvalue = ret;
				op->rvalue = binary_and();
				if (!op->rvalue) {
					throw parse_exception(
						"Expected expression for rvalue",
						t, cur
					);
				}

				ret = op;
				set_node_src(op, cur);
				cur = current();
			}

			return ret;
		}

		ast_node* binary_and() {
			ast_node* ret = equality();
			token cur = current();
			while (match({ "&" })) {
				consume();

				ast_node* op = new ast_node();
				op->type = ast_node::node_type::operation;
				op->op = ast_node::operation_type::band;
				op->lvalue = ret;
				op->rvalue = equality();
				if (!op->rvalue) {
					throw parse_exception(
						"Expected expression for rvalue",
						t, cur
					);
				}

				ret = op;
				set_node_src(op, cur);
				cur = current();
			}

			return ret;
		}

		ast_node* equality() {
			ast_node* ret = comp();
			token cur = current();
			while (match({ "!=", "==" })) {
				consume();

				ast_node* op = new ast_node();
				op->type = ast_node::node_type::operation;
				if (cur.text == "!=") op->op = ast_node::operation_type::notEq;
				else op->op = ast_node::operation_type::isEq;
				op->lvalue = ret;
				op->rvalue = comp();
				if (!op->rvalue) {
					throw parse_exception(
						"Expected expression for rvalue",
						t, cur
					);
				}

				ret = op;
				set_node_src(op, cur);
				cur = current();
			}

			return ret;
		}

		ast_node* comp() {
			ast_node* ret = shift();
			token cur = current();
			while (match({ "<", "<=", ">", ">=" })) {
				consume();

				ast_node* op = new ast_node();
				op->type = ast_node::node_type::operation;
				if (cur.text == "<") op->op = ast_node::operation_type::less;
				else if (cur.text == "<=") op->op = ast_node::operation_type::lessEq;
				else if (cur.text == ">") op->op = ast_node::operation_type::greater;
				else op->op = ast_node::operation_type::greaterEq;
				op->lvalue = ret;
				op->rvalue = shift();
				if (!op->rvalue) {
					throw parse_exception(
						"Expected expression for rvalue",
						t, cur
					);
				}

				ret = op;
				set_node_src(op, cur);
				cur = current();
			}

			return ret;
		}

		ast_node* shift() {
			ast_node* ret = add();
			token cur = current();
			while (match({ "<<", ">>" })) {
				consume();

				ast_node* op = new ast_node();
				op->type = ast_node::node_type::operation;
				if (cur.text == "<<") op->op = ast_node::operation_type::shiftLeft;
				else op->op = ast_node::operation_type::shiftRight;
				op->lvalue = ret;
				op->rvalue = add();
				if (!op->rvalue) {
					throw parse_exception(
						"Expected expression for rvalue",
						t, cur
					);
				}

				ret = op;
				set_node_src(op, cur);
				cur = current();
			}

			return ret;
		}

		ast_node* add() {
			ast_node* ret = mult();
			token cur = current();
			while (match({ "+", "-" })) {
				consume();

				ast_node* op = new ast_node();
				op->type = ast_node::node_type::operation;
				if (cur.text == "+") op->op = ast_node::operation_type::add;
				else op->op = ast_node::operation_type::sub;
				op->lvalue = ret;
				op->rvalue = mult();
				if (!op->rvalue) {
					throw parse_exception(
						"Expected expression for rvalue",
						t, cur
					);
				}

				ret = op;
				set_node_src(op, cur);
				cur = current();
			}

			return ret;
		}

		ast_node* mult() {
			ast_node* ret = unary();
			token cur = current();
			while (match({ "*", "/", "%" })) {
				consume();

				ast_node* op = new ast_node();
				op->type = ast_node::node_type::operation;
				if (cur.text == "*") op->op = ast_node::operation_type::mul;
				else if (cur.text == "/") op->op = ast_node::operation_type::div;
				else op->op = ast_node::operation_type::mod;
				op->lvalue = ret;
				op->rvalue = unary();
				if (!op->rvalue) {
					throw parse_exception(
						"Expected expression for rvalue",
						t, cur
					);
				}

				ret = op;
				set_node_src(op, cur);
				cur = current();
			}

			return ret;
		}

		ast_node* unary() {
			token cur = current();
			if (match({ "!", "-", "--", "++" })) {
				consume();

				ast_node* op = new ast_node();
				op->type = ast_node::node_type::operation;
				if (cur.text == "!") op->op = ast_node::operation_type::not;
				else if (cur.text == "-") op->op = ast_node::operation_type::negate;
				else if (cur.text == "--") op->op = ast_node::operation_type::preDec;
				else if (cur.text == "++") op->op = ast_node::operation_type::preInc;
				op->rvalue = unary();
				if (!op->rvalue || (op->rvalue->type != ast_node::node_type::identifier && op->rvalue->op != ast_node::operation_type::member)) {
					throw parse_exception(
						"Expected identifier or member expression for rvalue",
						t, cur
					);
				}

				set_node_src(op, cur);
				return op;
			}

			return postfix();
		}

		ast_node* postfix() {
			ast_node* ret = call();
			token cur = current();
			if (match({ "++", "--" })) {
				if (!ret || (ret->type != ast_node::node_type::identifier && ret->op != ast_node::operation_type::member)) {
					throw parse_exception(
						"Expected identifier or member expression for lvalue",
						t, cur
					);
				}

				consume();
				ast_node* op = new ast_node();
				op->type = ast_node::node_type::operation;
				if (cur.text == "--") op->op = ast_node::operation_type::postDec;
				else op->op = ast_node::operation_type::postInc;
				op->lvalue = ret;
				set_node_src(op, cur);
				return op;
			}

			return ret;
		}

		// function call, new
		ast_node* call() {
			ast_node* ret = member();
			token cur = current();
			while (match({ "(" })) {
				if (!ret || (ret->type != ast_node::node_type::identifier && ret->op != ast_node::operation_type::member)) {
					throw parse_exception(
						"Expected identifier or member expression for callee",
						t, cur
					);
				}
				consume();

				ast_node* c = new ast_node();
				c->type = ast_node::node_type::call;
				c->callee = ret;
				c->arguments = expr();
				while(match({ "," })) {
					if (!c->arguments) {
						throw parse_exception(
							"Expected expression for function argument",
							t, current()
						);
					}

					token comma = current();
					consume();
					ast_node* n = c->arguments;
					while (n->next) n = n->next;
					n->next = expr();
					if (!n->next) {
						throw parse_exception(
							"Expected expression for function argument",
							t, comma
						);
					}
				}
				if (!match({ ")" })) {
					throw parse_exception(
						"Expected ')' to close function argument list",
						t, cur
					);
				}
				consume();

				ret = c;
				set_node_src(c, cur);
				cur = current();
			}

			return ret;
		}

		ast_node* member() {
			ast_node* ret = primary();
			token cur = current();
			while (match({ ".", "[" })) {
				consume();

				ast_node* op = new ast_node();
				op->type = ast_node::node_type::operation;
				op->op = ast_node::operation_type::member;
				op->lvalue = ret;
				if (cur.text == "[") {
					op->rvalue = primary();
					if (!op->rvalue) {
						throw parse_exception(
							"Expected expression for member accessor value",
							t, cur
						);
					}
				} else {
					if (t.is_identifier(current().text)) {
						op->rvalue = identifier(t, current());
						set_node_src(op->rvalue, current());
						consume();
					} else {
						throw parse_exception(
							"Expected identifier for member accessor name",
							t, cur
						);
					}
				}

				if (cur.text == "[") {
					if (!match({ "]" })) {
						throw parse_exception(
							"Expected ']' to close member accessor",
							t, cur
						);
					}
					consume();
				}

				ret = op;
				set_node_src(op, cur);
				cur = current();
			}

			return ret;
		}

		ast_node* primary() {
			ast_node* n = nullptr;

			n = parse_identifier(current());
			if (n) {
				consume();
				return n;
			}
			
			n = parse_boolean(current());
			if (n) {
				consume();
				return n;
			}
			
			n = parse_number(current());
			if (n) {
				consume();
				return n;
			}

			n = parse_string(current());
			if (n) {
				consume();
				return n;
			}

			if (match({ "(" })) {
				consume();
				n = expr();
				if(!match({ ")" })) {
					throw parse_exception(
						"Expected ')' to end expression group",
						t, current()
					);
				}
				consume();
			}

			return n;
		}

		ast_node* parse_identifier(const token& tok) {
			if (tok.text == "this" || t.is_identifier(tok.text)) {
				if (tok.text != "this" && !ctx.find_variable(tok.text)) {
					throw parse_exception(
						format("Use of undeclared identifier '%s'", tok.text.c_str()),
						t, tok
					);
				}
				ast_node* n = new ast_node();
				n->type = ast_node::node_type::identifier;
				n->set(tok.text);
				set_node_src(n, tok);
				return n;
			}

			return nullptr;
		}

		ast_node* parse_boolean(const token& tok) {
			if (tok.text != "true" && tok.text != "false") return nullptr;

			ast_node* node = new ast_node();
			node->type = ast_node::node_type::constant;
			node->c_type = ast_node::constant_type::integer;
			node->value.i = tok.text == "true" ? 1 : 0;
			set_node_src(node, tok);
			return node;
		}

		ast_node* parse_number(const token& tok) {
			if (tok.text == "null") {
				ast_node* node = new ast_node();
				node->type = ast_node::node_type::constant;
				node->c_type = ast_node::constant_type::integer;
				node->value.i = 0;
				set_node_src(node, tok);
				return node;
			}

			if (!t.is_number(tok.text)) return nullptr;
			ast_node* node = new ast_node();
			node->type = ast_node::node_type::constant;
			if (tok.text.find_first_of('.') != string::npos) {
				node->c_type = ast_node::constant_type::decimal;
				node->value.d =	atof(tok.text.c_str());
			} else {
				node->c_type = ast_node::constant_type::integer;
				node->value.i =	atoi(tok.text.c_str());
			}
			set_node_src(node, tok);
			return node;
		}

		ast_node* parse_string(const token& tok) {
			if (tok.text[0] != '\'' && tok.text[0] != '"') return nullptr;
			ast_node* node = new ast_node();
			node->type = ast_node::node_type::constant;
			node->c_type = ast_node::constant_type::string;
			node->set(tok.text.substr(1, tok.text.length() - 2));
			set_node_src(node, tok);
			return node;
		}

		void set_node_src(ast_node* node, const token& tok) {
			u32 col = tok.col;
			if (tok.line == 0) col += base.col;
			node->start.lineText = base_t.lines[base.line + tok.line];
			node->start.col = col;
			node->start.line = base.line + tok.line;
			node->start.file = tok.file;
		}

		vector<token> tokens;
		u32 cur_token;
		parse_context& ctx;
		tokenizer& base_t;
		tokenizer t;
		token base;
	};



	ast_node::ast_node() {
		type = node_type::root;
		start = { "", "", 0, 0 };
		end = { "", "", 0, 0 };
		next = nullptr;
		data_type = nullptr;
		identifier = nullptr;
		body = nullptr;
		else_body = nullptr;
		property = nullptr;
		arguments = nullptr;
		initializer = nullptr;
		lvalue = nullptr;
		rvalue = nullptr;
		callee = nullptr;
		constructor = nullptr;
		destructor = nullptr;
		condition = nullptr;
		modifier = nullptr;
		c_type = constant_type::none;
		op = operation_type::invalid;
		is_const = false;
		is_static = false;
		memset(&value, 0, sizeof(value));
	}

	ast_node::~ast_node() {
		if (value.s && c_type == constant_type::string) delete [] value.s;
		if (next) delete next;
		if (data_type) delete data_type;
		if (identifier) delete identifier;
		if (body) delete body;
		if (else_body) delete else_body;
		if (property) delete property;
		if (arguments) delete arguments;
		if (initializer) delete initializer;
		if (lvalue) delete lvalue;
		if (rvalue) delete rvalue;
		if (callee) delete callee;
		if (constructor) delete constructor;
		if (destructor) delete destructor;
		if (condition) delete condition;
		if (modifier) delete modifier;
	}

	void ast_node::debug_print(u32 tab_level, bool dontIndentFirst, bool inArray) {
		static const char* type_names[] = {
			"root",
			"empty",
			"variable_declaration",
			"function_declaration",
			"class_declaration",
			"format_declaration",
			"variable_initializer",
			"format_property",
			"function_arguments",
			"if_statement",
			"for_loop",
			"while_loop",
			"do_while_loop",
			"import_statement",
			"export_statement",
			"return_statement",
			"object",
			"call",
			"expression",
			"conditional",
			"constant",
			"identifier",
			"type_identifier",
			"operation",
			"context_type",
			"context_function"
		};
		static const char* op_names[] = {
			"invalid",
			"add",
			"sub",
			"mul",
			"div",
			"mod",
			"shiftLeft",
			"shiftRight",
			"land",
			"lor",
			"band",
			"bor",
			"bxor",
			"addEq",
			"subEq",
			"mulEq",
			"divEq",
			"modEq",
			"shiftLeftEq",
			"shiftRightEq",
			"landEq",
			"lorEq",
			"bandEq",
			"borEq",
			"bxorEq",
			"preInc",
			"preDec",
			"postInc",
			"postDec",
			"less",
			"greater",
			"lessEq",
			"greaterEq",
			"notEq",
			"isEq",
			"eq",
			"not",
			"negate",
			"addr",
			"at",
			"member"
		};
		auto tab = [tab_level](u32 offset) {
			for (u32 i = 0;i < tab_level + offset;i++) printf("    ");
		};

		if (!dontIndentFirst) tab(0);
		u32 indent = 1;
		printf("{\n");

		tab(indent);
		printf("node_type: '%s',\n", type_names[u32(type)]);

		tab(indent);
		printf("source: { line: %d, col: %d, file: '%s' },\n", start.line + 1, start.col, start.file.c_str());

		if (type == node_type::variable_declaration) {
			tab(indent);
			printf("is_const: %s,\n", is_const ? "true" : "false");
			tab(indent);
			printf("is_static: %s,\n", is_static ? "true" : "false");
		}

		if (op != operation_type::invalid) {
			tab(indent);
			printf("operation: '%s',\n", op_names[u32(op)]);
		}

		switch(c_type) {
			case constant_type::integer: { tab(indent); printf("value: %lld,\n", value.i); break; }
			case constant_type::decimal: { tab(indent); printf("value: %f,\n", value.d); break; }
			case constant_type::string: { tab(indent); printf("value: '%s',\n", value.s); break; }
			default: break;
		}

		if (type == node_type::identifier || type == node_type::type_identifier) {
			tab(indent);
			printf("value: '%s',\n", value.s);
		}

		auto child = [tab_level, tab, indent](ast_node* n, const char* name) {
			tab(indent);
			printf("%s: ", name);
			if (n->next) printf("[\n");
			n->debug_print(tab_level + indent + (n->next ? 1 : 0), n->next == nullptr);
			if (n->next) {
				tab(indent);
				printf("],\n");
			}
		};

		if (data_type) {
			tab(indent);
			printf("data_type: ");
			data_type->debug_print(tab_level + indent, true);
		}
		if (identifier) child(identifier, "identifier");
		if (property) child(property, "property");
		if (arguments) child(arguments, "arguments");
		if (lvalue) child(lvalue, "lvalue");
		if (rvalue) child(rvalue, "rvalue");
		if (callee) child(callee, "callee");
		if (constructor) child(constructor, "constructor");
		if (destructor) child(destructor, "destructor");
		if (condition) child(condition, "condition");
		if (modifier) child(modifier, "modifier");
		if (initializer) child(initializer, "initializer");
		if (else_body) child(else_body, "else_body");
		if (body) child(body, "body");

		tab(0);
		printf("},\n");
		if (next) next->debug_print(tab_level, false, true);
	}



	ast_node* type_identifier(tokenizer& t, token& tk) {
		ast_node* n = new ast_node();
		n->type = ast_node::node_type::type_identifier;
		n->src(t, tk);
		n->set(tk.text);
		return n;
	}

	ast_node* identifier(tokenizer& t, token& tk) {
		ast_node* n = new ast_node();
		n->type = ast_node::node_type::identifier;
		n->src(t, tk);
		n->set(tk.text);
		return n;
	}



	ast_node* parse_data_type(parse_context& ctx, tokenizer& t) {
		token type = t.identifier(false);
		if (type) {
			if (ctx.find_type(type.text)) return type_identifier(t, type);
			else throw parse_exception("Expected type name", t, type);
		} else {
			throw parse_exception("Expected type name", t);
			return nullptr;
		}

		return nullptr;
	}

	ast_node* parse_new_identifier(parse_context& ctx, tokenizer& t, bool expected = true) {
		token id = t.identifier(expected);
		if (id) {
			if (ctx.find_type(id.text)) throw parse_exception(format("'%s' is already defined", id.text.c_str()), t, id);
			else if (ctx.find_variable(id.text)) throw parse_exception(format("'%s' is already defined", id.text.c_str()), t, id);
			else return identifier(t, id);
		} else if (expected) {
			throw parse_exception("Expected identifier", t);
			return nullptr;
		}

		return nullptr;
	}

	ast_node* parse_body(parse_context& ctx, tokenizer& t) {
		ast_node* body = nullptr;
		ast_node* node = parse_next(ctx, t);
		while (node != nullptr) {
			if (!body) body = node;
			else {
				ast_node* n = body;
				while (n->next) n = n->next;
				n->next = node;
			}
			node = parse_next(ctx, t);
		}

		return body;
	}

	ast_node* parse_expression(parse_context& ctx, tokenizer& t, bool expected = true) {
		token expr = t.expression(false);
		if (expected && !expr) {
			throw parse_exception("Expected expression", t);
			return nullptr;
		}

		expr_parser p(ctx, t, expr);
		return p.parse();

		return nullptr;
	}

	ast_node* parse_variable_declaration(parse_context& ctx, tokenizer& t) {
		ast_node* type = parse_data_type(ctx, t);
		if (string(*type) == "void") {
			throw parse_exception(
				t.file,
				"Variables can not be declared void",
				type->start.lineText, 
				type->start.line,
				type->start.col
			);
			return nullptr;
		}

		parse_frame& frame = ctx.frames[ctx.frames.size() - 1];
		ast_node* parent = ctx.node_path.top();

		ast_node* vdecl = new ast_node();
		vdecl->start = type->start;

		vdecl->type = ast_node::node_type::variable_declaration;
		vdecl->data_type = type;

		token name = t.identifier();
		ast_node* prevDecl = ctx.find_type(name.text);
		if (!prevDecl) prevDecl = ctx.find_variable(name.text);
		if (prevDecl) {
			throw parse_exception(
				format("Name '%s' was already defined", name.text.c_str()),
				t, name
			);
			return nullptr;
		}

		vdecl->identifier = identifier(t, name);

		token eq = t.keyword(false, "=");
		if (eq) {
			if (ctx.node_path.top()->type == ast_node::node_type::class_declaration && !ctx.var_is_static) {
				throw parse_exception(
					"Non-static class members must be initialized in the constructor",
					t, eq
				);
				return nullptr;
			}
			ast_node* init = new ast_node();
			init->type = ast_node::node_type::variable_initializer;
			init->src(t, eq);
			init->body = parse_expression(ctx, t);
			vdecl->initializer = init;
		}

		frame.variable_declarations.push_back(vdecl);
		return vdecl;
	}

	ast_node* parse_function_declaration_arguments(parse_context& ctx, tokenizer& t) {
		token start = t.character('(', true);

		ast_node* arguments = new ast_node();
		arguments->type = ast_node::node_type::function_arguments;
		arguments->src(t, start);

		token end;
		bool expectEnd = false;
		while (!(bool)(end = t.character(')', expectEnd))) {
			token type = t.keyword(false);
			if (type) {
				if (type == "const") {
					throw parse_exception(
						"Function arguments can't be declared const",
						t, type
					);
					return nullptr;
				}
				else if (type == "static") {
					throw parse_exception(
						"Function arguments can't be declared static",
						t, type
					);
					return nullptr;
				}

				throw parse_exception(
					format("Unexpected keyword '%s'", type.text.c_str()),
					t, type
				);
				return nullptr;
			}
			else if ((bool)(type = t.identifier(false))) {
				if (!ctx.find_type(type.text)) throw parse_exception("Expected type name", t, type);
			} else {
				throw parse_exception("Expected type name", t);
				return nullptr;
			}

			token name = t.identifier();
			ast_node* prevDecl = ctx.find_type(name.text);
			if (!prevDecl) prevDecl = ctx.find_variable(name.text);
			if (prevDecl) {
				throw parse_exception(
					format("Name '%s' was already defined", name.text.c_str()),
					t, name
				);
				return nullptr;
			}

			ast_node* arg = new ast_node();
			arg->type = ast_node::node_type::variable_declaration;
			arg->src(t, type);

			arg->data_type = type_identifier(t, type);

			arg->identifier = identifier(t, name);

			if (arguments->body) {
				ast_node* n = arguments->body;
				while (n->next) n = n->next;
				n->next = arg;
			} else arguments->body = arg;

			ctx.frames[ctx.frames.size() - 1].variable_declarations.push_back(arg);

			expectEnd = !t.character(',', false);
		}

		if (!end) {
			throw parse_exception("Expected ')'", t);
			return nullptr;
		}

		return arguments;
	}

	ast_node* parse_function_declaration(parse_context& ctx, tokenizer& t) {
		ast_node* func = nullptr;
		ast_node* return_type = parse_data_type(ctx, t);
		token name = t.identifier();

		// check if there was a declaration without a body
		ast_node* existing = ctx.find_variable(name.text);
		if (existing && existing->type == ast_node::node_type::function_declaration && !existing->body) {
			// function has a forward declaration
			func = existing;

			// make sure the return type is the same
			if (string(*return_type) != string(*existing->data_type)) {
				throw parse_exception(
					t.file,
					format("Definition for function '%s' has different return type than its declaration", name.text.c_str()),
					return_type->start.lineText,
					return_type->start.line,
					return_type->start.col
				);
			}

			// make sure argument list is the same (in terms of data types and length)
			ast_node* this_arg = parse_function_declaration_arguments(ctx, t);
			ast_node* that_arg = existing->arguments;
			bool same = (!this_arg) == (!that_arg);
			if (same && this_arg) {
				this_arg = this_arg->body;
				that_arg = that_arg->body;

				while (this_arg && that_arg) {
					if (string(*this_arg->data_type) != string(*that_arg->data_type)) {
						delete return_type;
						throw parse_exception(
							t.file,
							format("Definition for function '%s' has different arguments than its declaration", name.text.c_str()),
							this_arg->start.lineText,
							this_arg->start.line,
							this_arg->start.col
						);
					}

					this_arg = this_arg->next;
					that_arg = that_arg->next;
				}

				same = (!this_arg) == (!that_arg);
			}

			if (!same) {
				throw parse_exception(
					t.file,
					format("Definition for function '%s' has different arguments than its declaration", name.text.c_str()),
					return_type->start.lineText,
					return_type->start.line,
					return_type->start.col
				);
			}

			delete return_type;
		} else {
			// function was either not forward-declared, was defined at the declaration, or some other entity
			// is using this name

			func = new ast_node();
			func->type = ast_node::node_type::function_declaration;

			func->data_type = return_type;
			func->start = func->data_type->start;

			ast_node* prevDecl = ctx.find_type(name.text);
			if (!prevDecl) prevDecl = ctx.find_variable(name.text);
			if (prevDecl) {
				throw parse_exception(
					format("Name '%s' was already defined", name.text.c_str()),
					t, name
				);
				return nullptr;
			}

			func->identifier = identifier(t, name);

			ctx.frames[ctx.frames.size() - 1].variable_declarations.push_back(func);
			ctx.node_path.push(func);
			ctx.frames.push_back(parse_frame());
			func->arguments = parse_function_declaration_arguments(ctx, t);
		}

		t.backup_state();
		if (t.semicolon(false)) {
			t.restore_state();
			if (existing) {
				throw parse_exception(
					format("Function '%s' already has a forward declaration", name.text.c_str()),
					t, name
				);
			}
			// just the declaration
			ctx.frames.pop_back();
			ctx.node_path.pop();
			return func;
		}
		t.restore_state();

		if (existing) {
			// function definition for forward declared function
			// push arguments onto parse frame
			ctx.node_path.push(func);
			ctx.frames.push_back(parse_frame());
			if (func->arguments && func->arguments->body) {
				ast_node* arg = func->arguments->body;
				while (arg) {
					ctx.frames[ctx.frames.size() - 1].variable_declarations.push_back(arg);
					arg = arg->next;
				}
			}
		}

		t.character('{');
		func->body = parse_body(ctx, t);
		if (!t.character('}')) {
			throw parse_exception(
				t.file,
				"Expected '}'",
				t.lines[t.lines.size() - 1],
				t.lines.size() - 1,
				t.lines[t.lines.size() - 1].size() - 1
			);
			return nullptr;
		}

		ctx.frames.pop_back();
		ctx.node_path.pop();

		if (existing) {
			// parser needs to know this was a success, but returning func
			// would result in a cyclic linked list of ast_node.
			ast_node* ret = new ast_node();
			ret->type = ast_node::node_type::empty;
			ret->start = func->data_type->start;
			return ret;
		}
		return func;
	}

	ast_node* parse_declaration(parse_context& ctx, tokenizer& t) {
		// next token /will/ be a data type 
		t.backup_state();
		if (!t.keyword(false)) t.identifier();

		// now at either a variable name or a function name
		t.identifier();

		// now at either ';' (uninitialized variable), '=' (initialized variable), or '(' (function definition)
		if (t.character('(', false)) {
			t.restore_state();
			return parse_function_declaration(ctx, t);
		} else {
			t.restore_state();
			return parse_variable_declaration(ctx, t);
		}

		return nullptr;
	}

	ast_node* parse_const_declaration(parse_context& ctx, tokenizer& t) {
		ctx.var_is_const = true;
		ast_node* decl = parse_variable_declaration(ctx, t);
		decl->is_const = true;
		ctx.var_is_const = false;
		return decl;
	}

	ast_node* parse_static_declaration(parse_context& ctx, tokenizer& t) {
		ctx.var_is_static = true;
		ast_node* decl = parse_variable_declaration(ctx, t);
		decl->is_static = true;
		ctx.var_is_static = false;
		return decl;
	}

	ast_node* parse_format_definition(parse_context& ctx, tokenizer& t) {
		token start = t.keyword(true, "format");

		ast_node* n = new ast_node();
		n->type = ast_node::node_type::format_declaration;
		n->src(t, start);

		n->identifier = parse_new_identifier(ctx, t);

		t.keyword(true, "=");

		token open = t.character('{');

		token end;
		bool expectEnd = false;
		while (!(bool)(end = t.character('}', expectEnd))) {
			token name = t.identifier();
			ast_node* op = n->body;
			while (op) {
				if (string(*op->identifier) == name.text) {
					throw parse_exception(
						format("Name '%s' was already defined in format", name.text.c_str()),
						t, name
					);
					return nullptr;
				}
				op = op->next;
			}

			t.character(':');

			ast_node* type = parse_data_type(ctx, t);
			if (string(*type) == "void") {
				throw parse_exception(
					t.file,
					"Format properties can not be declared void",
					type->start.lineText,
					type->start.line,
					type->start.col
				);
				return nullptr;
			}

			ast_node* prop = new ast_node();
			prop->start = type->start;
			prop->type = ast_node::node_type::format_property;
			prop->data_type = type;
			prop->identifier = identifier(t, name);

			if (n->body) {
				ast_node* p = n->body;
				while (p->next) p = p->next;
				p->next = prop;
			} else n->body = prop;

			expectEnd = !t.character(',', false);
		}

		if (!n->body) {
			throw parse_exception("Object formats must not be empty", t, open);
			return nullptr;
		}

		if (!end) {
			throw parse_exception("Expected '}'", t);
			return nullptr;
		}

		ctx.frames[ctx.frames.size() - 1].type_declarations.push_back(n);

		return n;
	}

	ast_node* parse_class_definition(parse_context& ctx, tokenizer& t) {
		token start = t.keyword(true, "class");

		if (ctx.node_path.size() > 1) {
			throw parse_exception("Classes may only be defined in the global scope", t, start);
		}

		ast_node* c = new ast_node();
		c->type = ast_node::node_type::class_declaration;
		c->src(t, start);
		c->identifier = parse_new_identifier(ctx, t);
		ctx.frames[ctx.frames.size() - 1].type_declarations.push_back(c);
		ctx.node_path.push(c);
		ctx.frames.push_back(parse_frame());

		// no inheritance yet

		token open = t.character('{');
		token end;
		while (!(bool)(end = t.character('}', false))) {
			token tok = t.keyword(false, "constructor");
			if (tok) {
				if (c->constructor) {
					throw parse_exception("Unexpected class constructor", t, tok);
					return nullptr;
				}

				ast_node* func = new ast_node();
				func->type = ast_node::node_type::function_declaration;
				func->src(t, tok);

				ctx.node_path.push(func);
				ctx.frames.push_back(parse_frame());
				func->arguments = parse_function_declaration_arguments(ctx, t);

				t.character('{');
				func->body = parse_body(ctx, t);
				if (!t.character('}')) {
					throw parse_exception(
						t.file,
						"Expected '}'",
						t.lines[t.lines.size() - 1],
						t.lines.size() - 1,
						t.lines[t.lines.size() - 1].size() - 1
					);
					return nullptr;
				}
				ctx.frames.pop_back();
				ctx.node_path.pop();
				c->constructor = func;
				continue;
			}

			tok = t.keyword(false, "destructor");
			if (tok) {
				if (c->destructor) {
					throw parse_exception("Unexpected class destructor", t, tok);
					return nullptr;
				}
				ast_node* func = new ast_node();
				func->type = ast_node::node_type::function_declaration;
				func->src(t, tok);

				ctx.node_path.push(func);
				ctx.frames.push_back(parse_frame());
				func->arguments = parse_function_declaration_arguments(ctx, t);

				t.character('{');
				func->body = parse_body(ctx, t);
				if (!t.character('}')) {
					throw parse_exception(
						t.file,
						"Expected '}'",
						t.lines[t.lines.size() - 1],
						t.lines.size() - 1,
						t.lines[t.lines.size() - 1].size() - 1
					);
					return nullptr;
				}
				ctx.frames.pop_back();
				ctx.node_path.pop();
				c->destructor = func;
				continue;
			}

			// at the moment, classes can only have constructors, destructors, methods, and properties
			// so this must be a method or property declaration

			t.backup_state();
			token first = t.keyword(false);
			t.restore_state();
			ast_node* decl = nullptr;
			if (first) {
				if (first == "const") decl = parse_const_declaration(ctx, t);
				else if (first == "static") decl = parse_static_declaration(ctx, t);
			}
			if (!decl) {
				t.backup_state();
				first = t.identifier(false);
				t.restore_state();
				if (first) {
					if (ctx.find_type(first.text)) decl = parse_declaration(ctx, t);
				}
			}
			
			if (!decl) throw parse_exception("Expected class method or property declaration", t);
			else if (decl->type == ast_node::node_type::variable_declaration) t.semicolon();

			if (!c->body) c->body = decl;
			else {
				ast_node* n = c->body;
				while (n->next) n = n->next;
				n->next = decl;
			}
		}

		if (!c->body) {
			throw parse_exception("Class bodiess must not be empty", t, open);
			return nullptr;
		}

		if (!end) {
			throw parse_exception("Expected '}'", t);
			return nullptr;
		}

		ctx.frames.pop_back();
		ctx.node_path.pop();
		return c;
	}

	ast_node* parse_if_statement(parse_context& ctx, tokenizer& t) {
		token start = t.keyword(true, "if");
		t.character('(');

		ast_node* stmt = new ast_node();
		stmt->type = ast_node::node_type::if_statement;
		stmt->src(t, start);
		ctx.node_path.push(stmt);

		stmt->condition = parse_expression(ctx, t);

		t.character(')');

		ctx.frames.push_back(parse_frame());

		if (t.character('{', false)) {
			stmt->body = parse_body(ctx, t);
			if (!t.character('}')) {
				throw parse_exception(
					"Expected '}'",
					t
				);
				return nullptr;
			}
		} else stmt->body = parse_next(ctx, t);

		ctx.frames.pop_back();

		if (t.keyword(false, "else")) {
			ctx.frames.push_back(parse_frame());

			if (t.character('{', false)) {
				stmt->body = parse_body(ctx, t);
				if (!t.character('}')) {
					throw parse_exception(
						"Expected '}'",
						t
					);
					return nullptr;
				}
			} else stmt->else_body = parse_next(ctx, t);

			ctx.frames.pop_back();
		}

		ctx.node_path.pop();
		return stmt;
	}

	ast_node* parse_for_loop(parse_context& ctx, tokenizer& t) {
		token start = t.keyword(true, "for");

		ast_node* l = new ast_node();
		l->type = ast_node::node_type::for_loop;
		l->src(t, start);
		ctx.frames.push_back(parse_frame());
		ctx.node_path.push(l);

		t.character('(');
		if (!t.character(';', false)) {
			l->initializer = parse_variable_declaration(ctx, t);
			t.character(';');
		}

		if (!t.character(';', false)) {
			l->condition = parse_expression(ctx, t);
			t.character(';');
		}

		if (!t.character(')', false)) {
			l->modifier = parse_expression(ctx, t);
		}
		t.character(')');
		
		if (t.character('{', false)) {
			l->body = parse_body(ctx, t);
			if (!t.character('}')) {
				throw parse_exception(
					"Expected '}'",
					t
				);
				return nullptr;
			}
		} else l->body = parse_next(ctx, t);

		ctx.node_path.pop();
		ctx.frames.pop_back();
		return l;
	}

	ast_node* parse_while_loop(parse_context& ctx, tokenizer& t) {
		token start = t.keyword(true, "while");

		ast_node* l = new ast_node();
		l->type = ast_node::node_type::while_loop;
		l->src(t, start);
		ctx.frames.push_back(parse_frame());
		ctx.node_path.push(l);

		t.character('(');
		l->condition = parse_expression(ctx, t);
		t.character(')');

		if (t.character('{', false)) {
			l->body = parse_body(ctx, t);
			if (!t.character('}')) {
				throw parse_exception(
					"Expected '}'",
					t
				);
				return nullptr;
			}
		} else l->body = parse_next(ctx, t);

		ctx.node_path.pop();
		ctx.frames.pop_back();
		return l;
	}

	ast_node* parse_do_while_loop(parse_context& ctx, tokenizer& t) {
		token start = t.keyword(true, "do");

		ast_node* l = new ast_node();
		l->type = ast_node::node_type::do_while_loop;
		l->src(t, start);
		ctx.frames.push_back(parse_frame());
		ctx.node_path.push(l);

		if (t.character('{', false)) {
			l->body = parse_body(ctx, t);
			if (!t.character('}')) {
				throw parse_exception(
					"Expected '}'",
					t
				);
				return nullptr;
			}
		} else l->body = parse_next(ctx, t);

		t.keyword(true, "while");
		t.character('(');
		l->condition = parse_expression(ctx, t);
		t.character(')');

		ctx.node_path.pop();
		ctx.frames.pop_back();
		return l;
	}

	ast_node* parse_return(parse_context& ctx, tokenizer& t) {
		token start = t.keyword(true, "return");
		ast_node* r = new ast_node();
		r->type = ast_node::node_type::return_statement;
		r->src(t, start);

		t.backup_state();
		if (!t.semicolon(false)) {
			t.commit_state();
			r->body = parse_expression(ctx, t);
		} else t.restore_state();

		return r;
	}

	ast_node* parse_next(parse_context& ctx, tokenizer& t) {
		ast_node* r = nullptr;

		t.backup_state();
		token first = t.keyword(false);
		t.restore_state();
		if (first) {
			// declaration
			if (first == "const") r = parse_const_declaration(ctx, t);
			else if (first == "static") r = parse_static_declaration(ctx, t);
			
			// statement
			else if (first == "if") r = parse_if_statement(ctx, t);
			else if (first == "for") r = parse_for_loop(ctx, t);
			else if (first == "while") r = parse_while_loop(ctx, t);
			else if (first == "do") r = parse_do_while_loop(ctx, t);
			else if (first == "return") r = parse_return(ctx, t);

			// definition
			else if (first == "format") r = parse_format_definition(ctx, t);
			else if (first == "class") r = parse_class_definition(ctx, t);

			// expression
			else if (first == "this") r = parse_expression(ctx, t, true);
			else if (first == "null") r = parse_expression(ctx, t, true);
			else if (first == "new") r = parse_expression(ctx, t, true);
			else {
				throw parse_exception(
					format("Unexpected token '%s'", first.text.c_str()),
					t, first
				);
			}

			bool semicolon_optional = false;
			if (r->type == ast_node::node_type::function_declaration) semicolon_optional = true;
			if (r->type == ast_node::node_type::format_declaration) semicolon_optional = true;
			if (r->type == ast_node::node_type::class_declaration) semicolon_optional = true;
			if (r->type == ast_node::node_type::if_statement) semicolon_optional = true;
			if (r->type == ast_node::node_type::for_loop) semicolon_optional = true;
			if (r->type == ast_node::node_type::while_loop) semicolon_optional = true;
			if (r->type == ast_node::node_type::do_while_loop) semicolon_optional = true;
			if (r->type == ast_node::node_type::empty) semicolon_optional = true;
			t.semicolon(!semicolon_optional);
			return r;
		}

		t.backup_state();
		first = t.identifier(false);
		t.restore_state();
		if (first) {
			if (ctx.find_type(first.text)) r = parse_declaration(ctx, t);
			else if (ctx.find_variable(first.text)) r = parse_expression(ctx, t);
			else {
				throw parse_exception(
					format("Undefined identifier '%s'", first.text.c_str()),
					t, first
				);
			}
			bool semicolon_optional = false;
			if (r->type == ast_node::node_type::function_declaration && r->body) semicolon_optional = true;
			if (r->type == ast_node::node_type::empty) semicolon_optional = true;
			t.semicolon(!semicolon_optional);
			return r;
		}

		t.backup_state();
		if (t.character('(', false)) {
			t.restore_state();
			r = parse_expression(ctx, t);
		}

		t.semicolon(r != nullptr);

		return r;
	}

	void init_tokenizer(tokenizer& t) {
		t.specify_keyword("if");
		t.specify_keyword("else");
		t.specify_keyword("while");
		t.specify_keyword("for");
		t.specify_keyword("do");
		t.specify_keyword("class");
		t.specify_keyword("format");
		t.specify_keyword("return");
		t.specify_keyword("this");
		t.specify_keyword("const");
		t.specify_keyword("static");
		t.specify_keyword("null");
		t.specify_keyword("new");
		t.specify_keyword("constructor");
		t.specify_keyword("destructor");
		t.specify_keyword("++");
		t.specify_keyword("--");
		t.specify_keyword("+");
		t.specify_keyword("-");
		t.specify_keyword("*");
		t.specify_keyword("/");
		t.specify_keyword("%");
		t.specify_keyword("&");
		t.specify_keyword("|");
		t.specify_keyword("^");
		t.specify_keyword("+=");
		t.specify_keyword("-=");
		t.specify_keyword("*=");
		t.specify_keyword("/=");
		t.specify_keyword("%=");
		t.specify_keyword("&=");
		t.specify_keyword("|=");
		t.specify_keyword("^=");
		t.specify_keyword("&&");
		t.specify_keyword("||");
		t.specify_keyword(">>");
		t.specify_keyword("<<");
		t.specify_keyword(">>=");
		t.specify_keyword("<<=");
		t.specify_keyword("<");
		t.specify_keyword("<=");
		t.specify_keyword(">");
		t.specify_keyword(">=");
		t.specify_keyword("!");
		t.specify_keyword("=");
		t.specify_keyword("==");
	}

	ast_node* parse_source(vm_context* env, const std::string& file, const string& source) {
		tokenizer t(source);
		t.file = file;
		if (t.lines.size() == 0) return nullptr;
		init_tokenizer(t);

		parse_context ctx;
		ctx.env = env;
		ctx.root = new ast_node();
		ctx.root->start.file = file;
		ctx.node_path.push(ctx.root);
		ctx.frames.push_back(parse_frame());

		vector<vm_type*> types = env->types()->all();
		for (u32 i = 0;i < types.size();i++) {
			ast_node* t = new ast_node();
			t->type = ast_node::node_type::context_type;
			t->identifier = new ast_node();
			t->identifier->type = ast_node::node_type::identifier;
			t->identifier->set(types[i]->name);

			ctx.frames[0].type_declarations.push_back(t);
		}

		vector<vm_function*> funcs = env->all_functions();
		for (u32 i = 0;i < funcs.size();i++) {
			ast_node* f = new ast_node();
			f->type = ast_node::node_type::context_function;
			f->identifier = new ast_node();
			f->identifier->type = ast_node::node_type::identifier;
			f->identifier->set(funcs[i]->name);

			ctx.frames[0].variable_declarations.push_back(f);
		}

		ast_node* node = parse_next(ctx, t);
		ctx.root->body = node;
		while (node != nullptr) {
			ast_node* n = ctx.root->body;
			while (n->next) n = n->next;

			node = parse_next(ctx, t);
			n->next = node;
		}

		ctx.node_path.pop();
		ctx.frames.pop_back();
		return ctx.root;
	}
};
