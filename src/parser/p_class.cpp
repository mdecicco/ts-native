#include <parser/parse.h>
#include <parser/context.h>
#include <parser/ast.h>
#include <errors.h>

namespace gjs {
	using tt = lex::token_type;
	using exc = error::exception;
	using ec = error::ecode;
	using token = lex::token;

	namespace parse {
		using nt = ast::node_type;

		ast* class_declaration(context& ctx) {
            if (!ctx.match({ "class" })) throw exc(ec::p_expected_specific_keyword, ctx.current().src, "class");

			ast* c = new ast();
			c->type = nt::class_declaration;
			c->src(ctx.current());
			ctx.consume();
			c->identifier = identifier(ctx);

			if (ctx.match({ tt::semicolon })) {
				ctx.consume();
				return c;
			}

			if (!ctx.match({ tt::open_block })) throw exc(ec::p_expected_char, ctx.current().src, '{');
			ctx.path.push(c);

			auto add_to_body = [c](ast* node) {
				if (!c->body) c->body = node;
				else {
					ast* n = c->body;
					while (n->next) n = n->next;
					n->next = node;
				}
			};

			while (!ctx.at_end() && !ctx.match({ tt::close_block })) {
				if (ctx.match({ "constructor" })) {
					ast* f = new ast();
					f->type = nt::function_declaration;
					f->identifier = new ast();
					f->identifier->type = nt::identifier;
					f->identifier->set("constructor");
					f->identifier->src(ctx.current());
					f->data_type = new ast();
					f->data_type->type = nt::type_identifier;
					f->data_type->set(std::string(*c->identifier));
					f->data_type->src(ctx.current());
					f->src(ctx.current());
					ctx.consume();

					ctx.path.push(f);
					f->arguments = argument_list(ctx);
					if (!ctx.match({ tt::semicolon })) {
						f->body = any(ctx);
					} else ctx.consume();
					ctx.path.pop();

					c->constructor = f;
					continue;
				}

				if (ctx.match({ "destructor" })) {
					ast* f = new ast();
					f->type = nt::function_declaration;
					f->identifier = new ast();
					f->identifier->type = nt::identifier;
					f->identifier->set("destructor");
					f->identifier->src(ctx.current());
					f->data_type = new ast();
					f->data_type->type = nt::type_identifier;
					f->data_type->set(std::string(*c->identifier));
					f->data_type->src(ctx.current());
					f->src(ctx.current());
					ctx.consume();

					ctx.path.push(f);
					f->arguments = argument_list(ctx);
					if (!ctx.match({ tt::semicolon })) {
						f->body = any(ctx);
					} else ctx.consume();
					ctx.path.pop();

					c->destructor = f;
					continue;
				}

				if (ctx.match({ "const" })) {
					ast* decl = new ast();
					decl->type = nt::class_property;
					decl->src(ctx.current());
					decl->is_const = true;
					ctx.consume();
					decl->data_type = type_identifier(ctx);
					decl->identifier = identifier(ctx);

					if (!ctx.match({ tt::semicolon })) throw exc(ec::p_expected_char, ctx.current().src, ';');
					ctx.consume();
					add_to_body(decl);
					continue;
				}
				
				if (ctx.match({ "static" })) {
					ast* decl = new ast();
					decl->type = nt::class_property;
					decl->src(ctx.current());
					decl->is_static = true;
					ctx.consume();
					decl->data_type = type_identifier(ctx);
					decl->identifier = identifier(ctx);

					if (!ctx.match({ tt::semicolon })) throw exc(ec::p_expected_char, ctx.current().src, ';');
					ctx.consume();
					add_to_body(decl);
					continue;
				}

				if (ctx.match({ tt::identifier })) {
					if (ctx.is_typename(ctx.current().text)) {
						if (ctx.pattern({ tt::identifier, tt::identifier, tt::open_parenth })) {
							if (!ctx.is_typename(ctx.current().text)) {
								throw exc(ec::p_expected_class_prop_or_meth, ctx.current().src);
							}

							// like 'void print(...' or 'i32 hello(...'
							add_to_body(function_declaration(ctx));
							continue;
						} else if (ctx.pattern({ tt::identifier, tt::keyword, tt::operation, tt::open_parenth }) && ctx.pattern_s({ "", "operator" })) {
							if (!ctx.is_typename(ctx.current().text)) {
								throw exc(ec::p_expected_class_prop_or_meth, ctx.current().src);
							}

							// like 'i32 operator += (...' or 'vec3f operator * (...'
							add_to_body(function_declaration(ctx));
							continue;
						} else if (ctx.pattern({ tt::identifier, tt::identifier, tt::semicolon })) {
							if (!ctx.is_typename(ctx.current().text)) {
								throw exc(ec::p_expected_class_prop_or_meth, ctx.current().src);
							}

							// like 'i32 x;' or 'f32 y;'
							ast* decl = new ast();
							decl->type = nt::class_property;
							decl->src(ctx.current());
							decl->data_type = type_identifier(ctx);
							decl->identifier = identifier(ctx);
							ctx.consume(); // ;
							add_to_body(decl);
							continue;
						} 

						throw exc(ec::p_expected_class_prop_or_meth, ctx.current().src);
					} else {
						throw exc(ec::p_unexpected_identifier, ctx.current().src, ctx.current().text.c_str());
					}
					continue;
				}

				if (ctx.match({ tt::keyword })) {
					if (ctx.pattern({ tt::keyword, tt::identifier, tt::open_parenth }) && ctx.match({ "operator" })) {
						if (!ctx.is_typename(ctx.tokens[ctx.cur_token + 1].text)) {
							throw exc(ec::p_expected_class_prop_or_meth, ctx.current().src);
						}

						// like 'operator i32(...' or 'operator vec2f(...'
						add_to_body(function_declaration(ctx));
						continue;
					}
					
					throw exc(ec::p_unexpected_keyword, ctx.current().src, ctx.current().text.c_str());
				}
			}

			if (!ctx.match({ tt::close_block })) {
				throw exc(ec::p_unexpected_eof, c->ref, "class body");
			}

			ctx.path.pop();
			return nullptr;
		}

		ast* format_declaration(context& ctx) {
            if (!ctx.match({ "format" })) {
                throw exc(ec::p_expected_specific_keyword, ctx.current(), "format");
            }
			
			ast* c = new ast();
			c->type = nt::format_declaration;
			c->src(ctx.current());
			ctx.consume();
			c->identifier = identifier(ctx);
			
			if (!ctx.match({ tt::open_block })) throw exc(ec::p_expected_char, ctx.current(), '{');

			bool expect_end = false;
			while (!ctx.at_end() && !ctx.match({ tt::close_block })) {
				if (expect_end) {
					throw exc(ec::p_expected_char, ctx.current(), '}');
				}

				ast* p = new ast();
				p->type = nt::format_property;
				p->data_type = type_identifier(ctx);
				p->ref = p->data_type->ref;

				if (!ctx.match({ tt::colon })) throw exc(ec::p_expected_char, ctx.current(), ':');
				ctx.consume();
				p->identifier = identifier(ctx);

				if (!c->body) c->body = p;
				else {
					ast* n = c->body;
					while (n->next) n = n->next;
					n->next = p;
				}

				if (ctx.match({ tt::comma })) ctx.consume();
				else expect_end = true;
			}

			if (!ctx.match({ tt::close_block })) {
				throw exc(ec::p_unexpected_eof, c->ref, "format body");
			}

			return nullptr;
		};
	};
};