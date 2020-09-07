#include <compiler/context.h>
#include <compiler/compile.h>
#include <parser/ast.h>

namespace gjs {
	using namespace parse;
	using nt = ast::node_type;
	using ot = ast::operation_type;
	using ct = ast::constant_type;

	namespace compile {
		bool is_assignment(ast* expr) {
			if (expr->type == nt::operation) {
				switch (expr->op) {
					case ot::addEq:
                    case ot::subEq:
                    case ot::mulEq:
                    case ot::divEq:
					case ot::modEq:
					case ot::shiftLeftEq:
					case ot::shiftRightEq:
					case ot::landEq:
					case ot::lorEq:
					case ot::bandEq:
					case ot::borEq:
					case ot::bxorEq:
					case ot::preInc:
					case ot::preDec:
					case ot::postInc:
					case ot::postDec:
					case ot::eq: return true;
					default: return false;
				}
				return false;
			}
		}

		var operation(context& ctx, ast* n) {
			ctx.push_node(n);
			var ret = ctx.null_var();
			ctx.pop_node();

			switch (n->op) {
				case ot::invalid: {
					break;
				}
				case ot::add:			return expression(ctx, n->lvalue) + expression(ctx, n->rvalue);
				case ot::sub:			return expression(ctx, n->lvalue) - expression(ctx, n->rvalue);
				case ot::mul:			return expression(ctx, n->lvalue) * expression(ctx, n->rvalue);
				case ot::div:			return expression(ctx, n->lvalue) / expression(ctx, n->rvalue);
				case ot::mod:			return expression(ctx, n->lvalue) % expression(ctx, n->rvalue);
				case ot::band:			return expression(ctx, n->lvalue) & expression(ctx, n->rvalue);
				case ot::bor:			return expression(ctx, n->lvalue) | expression(ctx, n->rvalue);
				case ot::bxor:			return expression(ctx, n->lvalue) ^ expression(ctx, n->rvalue);
				case ot::less:			return expression(ctx, n->lvalue) < expression(ctx, n->rvalue);
				case ot::greater:		return expression(ctx, n->lvalue) > expression(ctx, n->rvalue);
				case ot::shiftLeft:		return expression(ctx, n->lvalue) << expression(ctx, n->rvalue);
				case ot::shiftRight:	return expression(ctx, n->lvalue) >> expression(ctx, n->rvalue);
				case ot::land:			return expression(ctx, n->lvalue) && expression(ctx, n->rvalue);
				case ot::lor:			return expression(ctx, n->lvalue) || expression(ctx, n->rvalue);
				case ot::lessEq:		return expression(ctx, n->lvalue) <= expression(ctx, n->rvalue);
				case ot::greaterEq:		return expression(ctx, n->lvalue) >= expression(ctx, n->rvalue);
				case ot::notEq:			return expression(ctx, n->lvalue) != expression(ctx, n->rvalue);
				case ot::isEq:			return expression(ctx, n->lvalue) == expression(ctx, n->rvalue);
				case ot::not:			return !expression(ctx, n->rvalue);
				case ot::negate:		return -expression(ctx, n->rvalue);
				case ot::addEq:			{ ret = expression(ctx, n->lvalue) += expression(ctx, n->rvalue); break; }
				case ot::subEq:			{ ret = expression(ctx, n->lvalue) -= expression(ctx, n->rvalue); break; }
				case ot::mulEq:			{ ret = expression(ctx, n->lvalue) *= expression(ctx, n->rvalue); break; }
				case ot::divEq:			{ ret = expression(ctx, n->lvalue) /= expression(ctx, n->rvalue); break; }
				case ot::modEq:			{ ret = expression(ctx, n->lvalue) %= expression(ctx, n->rvalue); break; }
				case ot::bandEq:		{ ret = expression(ctx, n->lvalue) &= expression(ctx, n->rvalue); break; }
				case ot::borEq:			{ ret = expression(ctx, n->lvalue) |= expression(ctx, n->rvalue); break; }
				case ot::bxorEq:		{ ret = expression(ctx, n->lvalue) ^= expression(ctx, n->rvalue); break; }
				case ot::shiftLeftEq:	{ ret = expression(ctx, n->lvalue) <<= expression(ctx, n->rvalue); break; }
				case ot::shiftRightEq:	{ ret = expression(ctx, n->lvalue) >>= expression(ctx, n->rvalue); break; }
				case ot::preInc:		{ ret = ++expression(ctx, n->rvalue); break; }
				case ot::preDec:		{ ret = --expression(ctx, n->rvalue); break; }
				case ot::postInc:		{ ret = expression(ctx, n->lvalue)++; break; }
				case ot::postDec:		{ ret = expression(ctx, n->lvalue)--; break; }
				case ot::landEq:		{ ret = expression(ctx, n->lvalue).operator_landEq(expression(ctx, n->rvalue)); break; }
				case ot::lorEq:			{ ret = expression(ctx, n->lvalue).operator_lorEq(expression(ctx, n->rvalue)); break; }
				case ot::eq:			{ ret = expression(ctx, n->lvalue).operator_eq(expression(ctx, n->rvalue)); break; }
				case ot::member: {
					break;
				}
				case ot::index: {
					break;
				}
				case ot::newObj: {
					break;
				}
				case ot::stackObj: {
					break;
				}
			}

			return ret;
		}

		var expression(context& ctx, ast* n) {
			switch (n->type) {
				case nt::call: return function_call(ctx, n);
				case nt::identifier: {
					ctx.push_node(n);
					var v = ctx.get_var(*n);
					ctx.pop_node();
					return v;
				}
				case nt::constant: {
					ctx.push_node(n);
					switch (n->c_type) {
						case ct::integer: {
							var v = ctx.imm(n->value.i);
							ctx.pop_node();
							return v;
						}
						case ct::f32: {
							var v = ctx.imm(n->value.f_32);
							ctx.pop_node();
							return v;
						}
						case ct::f64: {
							var v = ctx.imm(n->value.f_64);
							ctx.pop_node();
							return v;
						}
						case ct::string: {
							var v = ctx.imm(n->value.s);
							ctx.pop_node();
							return v;
						}
					}
				}
				case nt::conditional: {
					ctx.push_node(n);
					auto& b = ctx.add(operation::branch).operand(expression(ctx, n->condition));
					var tmp = expression(ctx, n->lvalue);
					var result = ctx.empty_var(tmp.type());
					ctx.add(operation::eq).operand(result).operand(tmp);
					auto& j = ctx.add(operation::jump);
					b.operand(ctx.imm(ctx.code.size()));
					ctx.add(operation::eq).operand(result).operand(expression(ctx, n->rvalue).convert(tmp.type()));
					j.operand(ctx.imm(ctx.code.size()));
					
					ctx.pop_node();
					return result;
				}
				case nt::operation: return operation(ctx, n);
				default: {
				}
			}
		}
	};
};