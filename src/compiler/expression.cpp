#include <compiler/expression.h>
#include <compiler/context.h>
#include <compiler/variable.h>
#include <compiler/function.h>
#include <compiler/data_type.h>
#include <vm_function.h>
#include <vm_type.h>
#include <compile_log.h>
#include <instruction_array.h>
#include <instruction_encoder.h>
#include <instruction.h>
#include <register.h>
#include <bind.h>
#include <robin_hood.h>

#define is_fp(r) (r >= vmr::f0 && r <= vmr::f15)
#define is_arg(r) (r >= vmr::a0 && r <= vmr::a7)

using namespace std;

namespace gjs {
	using vmr = vm_register;
	using vmi = vm_instruction;
	using op = ast_node::operation_type;
	using nt = ast_node::node_type;
	
	#define eval_fd(o) out->imm.f_32 = (l->imm.f_32 ##o r->imm.f_64)
	#define eval_ff(o) out->imm.f_32 = (l->imm.f_32 ##o r->imm.f_32)
	#define eval_fi(o) out->imm.f_32 = (l->imm.f_32 ##o r->imm.i   )
	#define eval_dd(o) out->imm.f_64 = (l->imm.f_64 ##o r->imm.f_64)
	#define eval_df(o) out->imm.f_64 = (l->imm.f_64 ##o r->imm.f_32)
	#define eval_di(o) out->imm.f_64 = (l->imm.f_64 ##o r->imm.i   )
	#define eval_id(o) out->imm.i    = (l->imm.i    ##o r->imm.f_64)
	#define eval_ii(o) out->imm.i    = (l->imm.i    ##o r->imm.i   )
	#define eval_if(o) out->imm.i    = (l->imm.i    ##o r->imm.f_32)
	#define eval(o)													\
		if (l->type->is_floating_point) {							\
			if (l->type->size == sizeof(f64)) {						\
				if (r->type->is_floating_point) {					\
					if (r->type->size == sizeof(f64)) eval_fd(o);	\
					else eval_ff(o);								\
				} else eval_fi(o);									\
			} else {												\
				if (r->type->is_floating_point) {					\
					if (r->type->size == sizeof(f64)) eval_dd(o);	\
					else eval_df(o);								\
				} else eval_di(o);									\
			}														\
		} else {													\
			if (r->type->is_floating_point) {						\
				if (r->type->size == sizeof(f64)) eval_id(o);		\
				else eval_if(o);									\
			} else eval_ii(o);										\
		}

	void evaluate_arithmetic_op_maybe_fp(compile_context& ctx, var* l, var* r, var* out, ast_node* because, op operation) {
		if (out->is_reg) ctx.cur_func->registers.free(out->loc.reg);
		out->is_reg = false;
		out->is_imm = true;

		switch (operation) {
			case op::add: { eval(+); break; }
			case op::sub: { eval(-); break; }
			case op::mul: { eval(*); break; }
			case op::div: { eval(/); break; }
			case op::less: { eval(<); break; }
			case op::greater: { eval(>); break; }
			case op::lessEq: { eval(<=); break; }
			case op::greaterEq: { eval(>=); break; }
			case op::notEq: { eval(!=); break; }
			case op::isEq: { eval(==); break; }
			case op::addEq:
			case op::subEq:
			case op::mulEq:
			case op::divEq:
			case op::postInc:
			case op::postDec: {
				ctx.log->err("Expression must have a modifiable lvalue", because);
				break;
			}
			case op::preInc:
			case op::preDec: {
				ctx.log->err("Expression must have a modifiable rvalue", because);
				break;
			}
		}
	}

	void evaluate_arithmetic_op(compile_context& ctx, var* l, var* r, var* out, ast_node* because, op operation) {
		if (out->is_reg) ctx.cur_func->registers.free(out->loc.reg);
		out->is_reg = false;
		out->is_imm = true;
		switch (operation) {
			case op::shiftLeft: { eval_ii(<<); break; }
			case op::shiftRight: { eval_ii(>>); break; }
			case op::land: { eval_ii(&&); break; }
			case op::lor: { eval_ii(||); break; }
			case op::band: { eval_ii(&); break; }
			case op::bor: { eval_ii(|); break; }
			case op::bxor: { eval_ii(^); break; }
			case op::shiftLeftEq:
			case op::shiftRightEq:
			case op::landEq:
			case op::lorEq:
			case op::bandEq:
			case op::borEq:
			case op::bxorEq: {
				ctx.log->err("Expression must have a modifiable lvalue", because);
			}
		}
	}

	void arithmetic_op_maybe_fp(compile_context& ctx, var* l, var* r, var* o, ast_node* because, op operation) {
		if (!o || (!l && !r)) return;

		// vmi::null -> swap lvalue and rvalue and use f_reg/f_imm or reg/imm or u_reg/imm
		// vmi(-1) -> unsupported
		static vmi possible_arr[][12] = {
			// d_reg/d_imm, d_imm/d_reg, d_reg/d_reg, f_reg/f_imm , f_imm/f_reg, f_reg/f_reg, reg/imm   , imm/reg   , reg/reg  , u_reg/imm , imm/u_reg  , u_reg/u_reg
			{ vmi::daddi  , vmi::null  , vmi::dadd  , vmi::faddi  , vmi::null  , vmi::fadd  , vmi::addi , vmi::null , vmi::add , vmi::addui, vmi::null  , vmi::addu }, // add
			{ vmi::dsubi  , vmi::dsubir, vmi::dsub  , vmi::fsubi  , vmi::fsubir, vmi::fsub  , vmi::subi , vmi::subir, vmi::sub , vmi::subui, vmi::subuir, vmi::subu }, // sub
			{ vmi::dmuli  , vmi::null  , vmi::dmul  , vmi::fmuli  , vmi::null  , vmi::fmul  , vmi::muli , vmi::null , vmi::mul , vmi::mului, vmi::null  , vmi::mulu }, // mul
			{ vmi::ddivi  , vmi::ddivir, vmi::ddiv  , vmi::fdivi  , vmi::fdivir, vmi::fdiv  , vmi::divi , vmi::divir, vmi::div , vmi::divui, vmi::divuir, vmi::divu }, // div
			{ vmi::dlti   , vmi(-1)	   , vmi::dlt   , vmi::flti   , vmi(-1)	   , vmi::flt   , vmi::lti  , vmi(-1)   , vmi::lt  , vmi::lti  , vmi(-1)    , vmi::lt   }, // less
			{ vmi::dgti   , vmi(-1)	   , vmi::dgt   , vmi::fgti   , vmi(-1)	   , vmi::fgt   , vmi::gti  , vmi(-1)   , vmi::gt  , vmi::gti  , vmi(-1)    , vmi::gt   }, // greater
			{ vmi::dltei  , vmi(-1)	   , vmi::dlte  , vmi::fltei  , vmi(-1)	   , vmi::flte  , vmi::ltei , vmi(-1)   , vmi::lte , vmi::ltei , vmi(-1)    , vmi::lte  }, // lessEq
			{ vmi::dgtei  , vmi(-1)	   , vmi::dgte  , vmi::fgtei  , vmi(-1)	   , vmi::fgte  , vmi::gtei , vmi(-1)   , vmi::gte , vmi::gtei , vmi(-1)    , vmi::gte  }, // greaterEq
			{ vmi::dncmpi , vmi::null  , vmi::dncmp , vmi::fncmpi , vmi::null  , vmi::fncmp , vmi::ncmpi, vmi::null , vmi::ncmp, vmi::ncmpi, vmi::null  , vmi::ncmp }, // notEq
			{ vmi::dcmpi  , vmi::null  , vmi::dcmp  , vmi::fcmpi  , vmi::null  , vmi::fcmp  , vmi::cmpi , vmi::null , vmi::cmp , vmi::cmpi , vmi::null  , vmi::cmp  }, // isEq1
		};
		static robin_hood::unordered_map<op, u32> possible_map = {
			{ op::add		, 0 },
			{ op::sub		, 1 },
			{ op::mul		, 2 },
			{ op::div		, 3 },
			{ op::addEq		, 0 },
			{ op::subEq		, 1 },
			{ op::mulEq		, 2 },
			{ op::divEq		, 3 },
			{ op::preInc	, 0 },
			{ op::preDec	, 1 },
			{ op::postInc	, 0 },
			{ op::postDec	, 1 },
			{ op::less      , 4 },
			{ op::greater   , 5 },
			{ op::lessEq    , 6 },
			{ op::greaterEq , 7 },
			{ op::notEq     , 8 },
			{ op::isEq      , 9 },
			{ op::eq		, 0 }
		};
		vmi* possible = possible_arr[possible_map[operation]];
		
		var* rhs = cast(ctx, r, l ? l : o, because);

		vmr lvr = l && !l->is_imm ? l->to_reg(because) : vmr::zero;
		if (!rhs->is_imm) rhs->to_reg(because);

		bool op_is_fpr = (l && is_fp(lvr)) || is_fp(o->to_reg(because));
		bool op_is_f64 = op_is_fpr ? ((l && l->type->size == sizeof(f64)) || (o->type->size == sizeof(f64))) : false;
		bool lv_is_imm = l && l->is_imm;
		bool rv_is_imm = rhs && rhs->is_imm;
		bool op_is_uns = (l && l->type->type && l->type->type->is_unsigned) || (o->type->type && o->type->type->is_unsigned);

		if (lv_is_imm && rv_is_imm) {
			// just do the math here and set output to an imm
			evaluate_arithmetic_op_maybe_fp(ctx, l, rhs, o, because, operation);
			return;
		}


		vmi i;
		if (op_is_fpr) {
			if (op_is_f64) {
				if (lv_is_imm) i = possible[1];
				else if (rv_is_imm) i = possible[0];
				else i = possible[2];
			} else {
				if (lv_is_imm) i = possible[4];
				else if (rv_is_imm) i = possible[3];
				else i = possible[5];
			}
		} else {
			if (op_is_uns) {
				if (lv_is_imm) i = possible[10];
				else if (rv_is_imm) i = possible[9];
				else i = possible[11];
			} else {
				if (lv_is_imm) i = possible[7];
				else if (rv_is_imm) i = possible[6];
				else i = possible[8];
			}
		}

		bool already_errored = false;
		if (i == vmi(-1)) {
			ctx.log->err("Unsupported operation", because);
			already_errored = true;
		}

		bool swap = false;
		if (i == vmi::null) {
			swap = true;
			if (op_is_fpr) {
				if (op_is_f64) i = possible[0];
				else i = possible[3];
			}
			else {
				if (op_is_uns) i = possible[9];
				else i = possible[6];
			}
		}

		if (i == vmi(-1)) {
			if (!already_errored) ctx.log->err("Unsupported operation", because);
		}

		instruction inst = encode(i).operand(o->to_reg(because));

		if (swap) {
			inst.operand(rhs->to_reg(because));
			if (lv_is_imm) {
				if (op_is_fpr) {
					if (l->type->size == sizeof(f64)) inst.operand(l->imm.f_64);
					else inst.operand(l->imm.f_32);
				}
				else inst.operand(l->imm.i);
			} else inst.operand(lvr);
		} else {
			inst.operand(lvr);
			if (rv_is_imm) {
				if (op_is_fpr) {
					if (rhs->type->size == sizeof(f64)) inst.operand(rhs->imm.f_64);
					else inst.operand(rhs->imm.f_32);
				}
				else inst.operand(rhs->imm.i);
			} else inst.operand(rhs->to_reg(because));
		}

		ctx.add(inst, because);

		if (rhs != r) ctx.cur_func->free(rhs);
	}

	void arithmetic_op(compile_context& ctx, var* l, var* r, var* o, ast_node* because, op operation) {
		if (!o) return;
		static vmi possible_arr[][6] = {
			// reg/imm  , imm/reg   , reg/reg  , u_reg/imm , imm/u_reg  , u_reg/u_reg
			{ vmi::sli  , vmi::slir , vmi::sl  , vmi::sli  , vmi::slir  , vmi::sl   }, // shiftLeft
			{ vmi::sri  , vmi::srir , vmi::sr  , vmi::sri  , vmi::srir  , vmi::sr   }, // shiftRight
			{ vmi::andi , vmi::null , vmi::and , vmi::andi , vmi::null  , vmi::and  }, // land
			{ vmi::ori  , vmi::null , vmi::or  , vmi::ori  , vmi::null  , vmi::or   }, // lor
			{ vmi::bandi, vmi::null , vmi::band, vmi::bandi, vmi::null  , vmi::band }, // band
			{ vmi::bori , vmi::null , vmi::bor , vmi::bori , vmi::null  , vmi::bor  }, // bor
			{ vmi::xori , vmi::null , vmi::xor , vmi::xori , vmi::null  , vmi::xor  }, // bxor
		};
		static robin_hood::unordered_map<op, u32> possible_map = {
			{ op::shiftLeft		, 0 },
			{ op::shiftRight	, 1 },
			{ op::land			, 2 },
			{ op::lor			, 3 },
			{ op::band			, 4 },
			{ op::bor			, 5 },
			{ op::bxor			, 6 },
			{ op::shiftLeftEq	, 0 },
			{ op::shiftRightEq	, 1 },
			{ op::landEq		, 2 },
			{ op::lorEq			, 3 },
			{ op::bandEq		, 4 },
			{ op::borEq			, 5 },
			{ op::bxorEq		, 6 },
		};
		vmi* possible = possible_arr[possible_map[operation]];

		var* rhs = cast(ctx, r, l ? l : o, because);

		vmr lvr = l && !l->is_imm ? l->to_reg(because) : vmr::zero;
		if (!rhs->is_imm) rhs->to_reg(because);

		bool op_is_fp = (l && is_fp(lvr)) || is_fp(o->to_reg(because));
		bool lv_is_im = l && l->is_imm;
		bool rv_is_im = rhs && rhs->is_imm;
		bool op_is_us = (l && l->type->type && l->type->type->is_unsigned) || (o->type->type && o->type->type->is_unsigned);

		if (lv_is_im && rv_is_im) {
			// just do the math here and set output to an imm
			evaluate_arithmetic_op(ctx, l, rhs, o, because, operation);
			return;
		}


		vmi i;
		if (op_is_fp) {
			if (lv_is_im) i = possible[1];
			else if (rv_is_im) i = possible[0];
			else i = possible[2];
		} else {
			if (op_is_us) {
				if (lv_is_im) i = possible[7];
				else if (rv_is_im) i = possible[6];
				else i = possible[8];
			} else {
				if (lv_is_im) i = possible[4];
				else if (rv_is_im) i = possible[3];
				else i = possible[5];
			}
		}

		bool swap = false;
		if (i == vmi::null) {
			swap = true;
			if (op_is_fp) i = possible[0];
			else {
				if (op_is_us) i = possible[6];
				else i = possible[3];
			}
		}

		instruction inst = encode(i).operand(o->to_reg(because));

		if (swap) {
			inst.operand(rhs->to_reg(because));
			if (lv_is_im) {
				if (op_is_fp) {
					if (l->type->size == sizeof(f64)) inst.operand(l->imm.f_64);
					else inst.operand(l->imm.f_32);
				} else inst.operand(l->imm.i);
			} else inst.operand(lvr);
		} else {
			inst.operand(lvr);
			if (rv_is_im) {
				if (op_is_fp) {
					if (rhs->type->size == sizeof(f64)) inst.operand(rhs->imm.f_64);
					else inst.operand(rhs->imm.f_32);
				} else inst.operand(rhs->imm.i);
			} else inst.operand(rhs->to_reg(because));
		}

		ctx.add(inst, because);

		if (rhs != r) ctx.cur_func->free(rhs);
	}

	var* compile_complex_operation(compile_context& ctx, var* lv, var* rv, ast_node* node, var* dest) {
		static robin_hood::unordered_map<op, const char*> opStr = {
			{ op::add, "+" },
			{ op::sub, "-" },
			{ op::mul, "*" },
			{ op::div, "/" },
			{ op::mod, "%" },
			{ op::shiftLeft, "<<" },
			{ op::shiftRight, ">>" },
			{ op::land, "&&" },
			{ op::lor, "||" },
			{ op::band, "&" },
			{ op::bor, "|" },
			{ op::bxor, "^" },
			{ op::addEq, "+=" },
			{ op::subEq, "-=" },
			{ op::mulEq, "*=" },
			{ op::divEq, "/=" },
			{ op::modEq, "%=" },
			{ op::shiftLeftEq, "<<=" },
			{ op::shiftRightEq, ">>=" },
			{ op::landEq, "&&=" },
			{ op::lorEq, "||=" },
			{ op::bandEq, "&=" },
			{ op::borEq, "|=" },
			{ op::bxorEq, "^=" },
			{ op::preInc, "++" },
			{ op::preDec, "--" },
			{ op::postInc, "++" },
			{ op::postDec, "--" },
			{ op::less, "<" },
			{ op::greater, ">" },
			{ op::lessEq, "<=" },
			{ op::greaterEq, ">=" },
			{ op::notEq, "!=" },
			{ op::isEq, "==" },
			{ op::eq, "=" },
			{ op::not, "!" },
			{ op::index, "[]" },
		};

		// todo: in case of post-inc/dec, clone object before calling the operator and use clone as return value

		var* opOf = lv ? lv : rv;
		var* param = lv ? rv : nullptr;

		func* opFunc = lv->type->method(format("operator %s", opStr[node->op]));
		var* ret = nullptr;
		if (opFunc) {
			vector<var*> args = { opOf };
			if (param) args.push_back(param);
			ret = call(ctx, opFunc, node, args, opOf->type);
			if (ret) {
				if (dest) {
					ret->store_in(dest->to_reg(node), node);
					ctx.cur_func->free(ret);
					ret = dest;
				}
			}
		} else {
			if (param) ctx.log->err(format("Type '%s' has no '%s' operator which accepts a parameter of type '%s'", opOf->type->name.c_str(), opStr[node->op], param->type->name.c_str()), node);
			else ctx.log->err(format("Type '%s' has no '%s' operator", opOf->type->name.c_str(), opStr[node->op]), node);
		}

		return ret;
	}

	var* compile_expression(compile_context& ctx, ast_node* node, var* dest) {
		if (node->type == nt::identifier) {
			if (!dest) return ctx.cur_func->get(ctx, node);
			ctx.cur_func->get(ctx, node)->store_in(dest->to_reg(node), node);
			return dest;
		}
		else if (node->type == nt::constant) {
			var* imm = nullptr;
			switch (node->c_type) {
				case ast_node::constant_type::integer: { imm = ctx.cur_func->imm(ctx, (integer)node->value.i); break; }
				case ast_node::constant_type::f32: { imm = ctx.cur_func->imm(ctx, node->value.f_32); break; }
				case ast_node::constant_type::f64: { imm = ctx.cur_func->imm(ctx, node->value.f_64); break; }
				case ast_node::constant_type::string: { imm = ctx.cur_func->imm(ctx, node->value.s); break; }
			}
			if (dest) {
				var* v = imm;
				if (!dest->type->equals(v->type)) {
					v = cast(ctx, v, dest, node);
				}
				v->store_in(dest->to_reg(node), node);
				ctx.cur_func->free(imm);
				if (v != imm) ctx.cur_func->free(v);
				return dest;
			}
			return imm;
		}
		else if (node->type == nt::conditional) {
			var* cond = compile_expression(ctx, node->condition, nullptr);
			u64 baddr = ctx.out->size();
			vmr condreg = cond->to_reg(node);
			ctx.cur_func->free(cond);

			// if cond == 0: goto address of rvalue expression
			ctx.add(
				encode(vmi::beqz).operand(condreg),
				node
			);

			// otherwise, continue to lvalue expression
			var* result = compile_expression(ctx, node->lvalue, dest);

			// then jump past rvalue expression so result isn't overwritten with rvalue
			u64 tjmpaddr = ctx.out->size();
			ctx.add(
				encode(vmi::jmp),
				node->lvalue
			);

			// update branch failure address
			ctx.out->set(baddr, encode(vmi::beqz).operand(condreg).operand(ctx.out->size()));

			compile_expression(ctx, node->rvalue, result);

			// update post-lvalue expression jump address
			ctx.out->set(tjmpaddr, encode(vmi::jmp).operand(ctx.out->size()));

			if (dest) {
				var* v = result;
				if (!dest->type->equals(v->type)) {
					v = cast(ctx, v, dest, node);
				}
				v->store_in(dest->to_reg(node), node);
				ctx.cur_func->free(result);
				if (v != result) ctx.cur_func->free(v);
				return dest;
			}

			return result;
		}

		bool assignsRv = false;
		bool assignsLv = false;
		if (node->type == nt::operation) {
			switch(node->op) {
				case op::addEq:
				case op::subEq:
				case op::mulEq:
				case op::divEq:
				case op::modEq:
				case op::shiftLeftEq:
				case op::shiftRightEq:
				case op::landEq:
				case op::lorEq:
				case op::bandEq:
				case op::borEq:
				case op::bxorEq:
				case op::postInc:
				case op::postDec:
				case op::eq: { assignsLv = true; break; }
				case op::preInc:
				case op::preDec: { assignsRv = true; break; }
				default: {
					break;
				}
			}
		}

		var* lvalue = nullptr;
		var* rvalue = nullptr;
		if (node->lvalue) {
			if (assignsLv) ctx.do_store_member_info = true;
			if (node->lvalue->type == nt::operation || node->lvalue->type == nt::call) lvalue = compile_expression(ctx, node->lvalue, nullptr);
			else if (node->lvalue->type == nt::identifier) lvalue = ctx.cur_func->get(ctx, node->lvalue);
			else if (node->lvalue->type == nt::constant) {
				switch (node->lvalue->c_type) {
					case ast_node::constant_type::integer: { lvalue = ctx.cur_func->imm(ctx, (integer)node->lvalue->value.i); break; }
					case ast_node::constant_type::f32: { lvalue = ctx.cur_func->imm(ctx, (f32)node->lvalue->value.f_32); break; }
					case ast_node::constant_type::f64: { lvalue = ctx.cur_func->imm(ctx, (f64)node->lvalue->value.f_64); break; }
					case ast_node::constant_type::string: { lvalue = ctx.cur_func->imm(ctx, node->lvalue->value.s); break; }
				}
			}
			if (assignsLv) ctx.do_store_member_info = false;
		}
		if (node->rvalue) {
			if (assignsRv) ctx.do_store_member_info = true;
			if (node->rvalue->type == nt::operation || node->rvalue->type == nt::call) rvalue = compile_expression(ctx, node->rvalue, nullptr);
			else if (node->rvalue->type == nt::identifier) {
				if (node->op != op::member) rvalue = ctx.cur_func->get(ctx, node->rvalue);
			}
			else if (node->rvalue->type == nt::constant) {
				switch (node->rvalue->c_type) {
					case ast_node::constant_type::integer: { rvalue = ctx.cur_func->imm(ctx, (integer)node->rvalue->value.i); break; }
					case ast_node::constant_type::f32: { rvalue = ctx.cur_func->imm(ctx, (f32)node->rvalue->value.f_32); break; }
					case ast_node::constant_type::f64: { rvalue = ctx.cur_func->imm(ctx, (f64)node->rvalue->value.f_64); break; }
					case ast_node::constant_type::string: { rvalue = ctx.cur_func->imm(ctx, node->rvalue->value.s); break; }
				}
			}
			if (assignsRv) ctx.do_store_member_info = false;
		}

		var* ret = nullptr;

		if (node->type == nt::call) {
			if (node->callee->type == nt::operation) {
				if (node->callee->op == op::member) {
					ctx.clear_last_member_info();
					ctx.do_store_member_info = true;
					var* this_obj = compile_expression(ctx, node->callee, nullptr);
					ctx.do_store_member_info = false;
					if (this_obj) this_obj->no_auto_free = true;
					if (ctx.last_member_or_method.method) {
						vector<var*> args;
						vector<bool> free_arg;

						if (ctx.last_member_or_method.method->is_thiscall) {
							args.push_back(this_obj);
							free_arg.push_back(!this_obj->is_variable);
						}

						ast_node* arg = node->arguments;
						while (arg) {
							args.push_back(compile_expression(ctx, arg, nullptr));
							free_arg.push_back(!args[args.size() - 1]->is_variable);
							arg = arg->next;
						}
						ret = call(ctx, ctx.last_member_or_method.method, node, args, this_obj ? this_obj->type : nullptr);
						for (u32 i = 0;i < args.size();i++) {
							if (free_arg[i]) ctx.cur_func->free(args[i]);
						}
					} else ctx.log->err("Expression does not result in a callable entity", node->callee);
				} else ctx.log->err("Expression does not result in a callable entity", node->callee);
			} else {
				func* t = ctx.function(*node->callee);
				vector<var*> args;
				vector<bool> free_arg;
				ast_node* arg = node->arguments;
				while (arg) {
					args.push_back(compile_expression(ctx, arg, nullptr));
					free_arg.push_back(!args[args.size() - 1]->is_variable);
					arg = arg->next;
				}
				ret = call(ctx, t, node, args);
				for (u32 i = 0;i < args.size();i++) {
					if (free_arg[i]) ctx.cur_func->free(args[i]);
				}
			}
			
			if (dest) {
				if (!ret) ctx.log->err("Cannot assign variable to result of function with no return value", node);
				else {
					var* v = ret;
					if (!dest->type->equals(v->type)) {
						v = cast(ctx, v, dest, node);
					}
					v->store_in(dest->to_reg(node), node);
					ctx.cur_func->free(ret);
					if (v != ret) ctx.cur_func->free(v);
					return dest;
				}
			}

			return ret;
		} else {
			bool auto_free = ctx.cur_func->auto_free_consumed_vars;
			ctx.cur_func->auto_free_consumed_vars = false;
			bool assigned = false;

			bool maybe_complex = (lvalue && !lvalue->type->is_primitive) || (!lvalue && rvalue && !rvalue->type->is_primitive);
			if (maybe_complex && node->op != op::member) {
				ret = compile_complex_operation(ctx, lvalue, rvalue, node, dest);
			} else {
				switch (node->op) {
					case op::invalid: { break; }
					case op::add: {
						var* result = dest ? dest : ctx.cur_func->allocate(ctx, lvalue->type);
						arithmetic_op_maybe_fp(ctx, lvalue, rvalue, result, node, node->op);
						ret = result;
						break;
					}
					case op::sub: {
						var* result = dest ? dest : ctx.cur_func->allocate(ctx, lvalue->type);
						arithmetic_op_maybe_fp(ctx, lvalue, rvalue, result, node, node->op);
						ret = result;
						break;
					}
					case op::mul: {
						var* result = dest ? dest : ctx.cur_func->allocate(ctx, lvalue->type);
						arithmetic_op_maybe_fp(ctx, lvalue, rvalue, result, node, node->op);
						ret = result;
						break;
					}
					case op::div: {
						var* result = dest ? dest : ctx.cur_func->allocate(ctx, lvalue->type);
						arithmetic_op_maybe_fp(ctx, lvalue, rvalue, result, node, node->op);
						ret = result;
						break;
					}
					case op::mod: {
						/*
						var* result = dest ? dest : ctx.cur_func->allocate(ctx, lvalue->type);
						arithmetic_op(ctx, lvalue, rvalue, result, node, node->op);
						return result;
						*/
						var* result = dest ? dest : ctx.cur_func->allocate(ctx, lvalue->type);
						ret = result;
						break;
					}
					case op::shiftLeft: {
						var* result = dest ? dest : ctx.cur_func->allocate(ctx, lvalue->type);
						arithmetic_op(ctx, lvalue, rvalue, result, node, node->op);
						ret = result;
						break;
					}
					case op::shiftRight: {
						var* result = dest ? dest : ctx.cur_func->allocate(ctx, lvalue->type);
						arithmetic_op(ctx, lvalue, rvalue, result, node, node->op);
						ret = result;
						break;
					}
					case op::land: {
						var* result = dest ? dest : ctx.cur_func->allocate(ctx, lvalue->type);
						arithmetic_op(ctx, lvalue, rvalue, result, node, node->op);
						ret = result;
						break;
					}
					case op::lor: {
						var* result = dest ? dest : ctx.cur_func->allocate(ctx, lvalue->type);
						arithmetic_op(ctx, lvalue, rvalue, result, node, node->op);
						ret = result;
						break;
					}
					case op::band: {
						var* result = dest ? dest : ctx.cur_func->allocate(ctx, lvalue->type);
						arithmetic_op(ctx, lvalue, rvalue, result, node, node->op);
						ret = result;
						break;
					}
					case op::bor: {
						var* result = dest ? dest : ctx.cur_func->allocate(ctx, lvalue->type);
						arithmetic_op(ctx, lvalue, rvalue, result, node, node->op);
						ret = result;
						break;
					}
					case op::bxor: {
						var* result = dest ? dest : ctx.cur_func->allocate(ctx, lvalue->type);
						arithmetic_op(ctx, lvalue, rvalue, result, node, node->op);
						ret = result;
						break;
					}
					case op::addEq: {
						arithmetic_op_maybe_fp(ctx, lvalue, rvalue, lvalue, node, node->op);
						ret = lvalue;
						assigned = true;
						break;
					}
					case op::subEq: {
						arithmetic_op_maybe_fp(ctx, lvalue, rvalue, lvalue, node, node->op);
						ret = lvalue;
						assigned = true;
						break;
					}
					case op::mulEq: {
						arithmetic_op_maybe_fp(ctx, lvalue, rvalue, lvalue, node, node->op);
						ret = lvalue;
						assigned = true;
						break;
					}
					case op::divEq: {
						arithmetic_op_maybe_fp(ctx, lvalue, rvalue, lvalue, node, node->op);
						ret = lvalue;
						assigned = true;
						break;
					}
					case op::modEq: {
						/*
						arithmetic_op(ctx, lvalue, rvalue, lvalue, node, node->op);
						return result;
						*/
						var* result = dest ? dest : ctx.cur_func->allocate(ctx, lvalue->type);
						ret = result;
						assigned = true;
						break;
					}
					case op::shiftLeftEq: {
						arithmetic_op(ctx, lvalue, rvalue, lvalue, node, node->op);
						ret = lvalue;
						assigned = true;
						break;
					}
					case op::shiftRightEq: {
						arithmetic_op(ctx, lvalue, rvalue, lvalue, node, node->op);
						ret = lvalue;
						assigned = true;
						break;
					}
					case op::landEq: {
						arithmetic_op(ctx, lvalue, rvalue, lvalue, node, node->op);
						ret = lvalue;
						assigned = true;
						break;
					}
					case op::lorEq: {
						arithmetic_op(ctx, lvalue, rvalue, lvalue, node, node->op);
						ret = lvalue;
						assigned = true;
						break;
					}
					case op::bandEq: {
						arithmetic_op(ctx, lvalue, rvalue, lvalue, node, node->op);
						ret = lvalue;
						assigned = true;
						break;
					}
					case op::borEq: {
						arithmetic_op(ctx, lvalue, rvalue, lvalue, node, node->op);
						ret = lvalue;
						assigned = true;
						break;
					}
					case op::bxorEq: {
						arithmetic_op(ctx, lvalue, rvalue, lvalue, node, node->op);
						ret = lvalue;
						assigned = true;
						break;
					}
					case op::preInc: {
						var* tmp;
						if (rvalue->type->is_floating_point) tmp = ctx.cur_func->imm(ctx, 1.0f);
						else tmp = ctx.cur_func->imm(ctx, i64(1));

						arithmetic_op_maybe_fp(ctx, rvalue, tmp, rvalue, node, node->op);
						if (dest) rvalue->store_in(dest->to_reg(node), node);

						ctx.cur_func->free(tmp);
						ret = dest ? dest : rvalue;
						assigned = true;
						break;
					}
					case op::preDec: {
						var* tmp;
						if (rvalue->type->is_floating_point) tmp = ctx.cur_func->imm(ctx, 1.0f);
						else tmp = ctx.cur_func->imm(ctx, i64(1));

						arithmetic_op_maybe_fp(ctx, rvalue, tmp, rvalue, node, node->op);
						if (dest) rvalue->store_in(dest->to_reg(node), node);

						ctx.cur_func->free(tmp);
						ret = dest ? dest : rvalue;
						assigned = true;
						break;
					}
					case op::postInc: {
						var* result = dest ? dest : ctx.cur_func->allocate(ctx, lvalue->type);
						var* tmp;
						if (lvalue->type->is_floating_point) tmp = ctx.cur_func->imm(ctx, 1.0f);
						else tmp = ctx.cur_func->imm(ctx, i64(1));

						lvalue->store_in(result->to_reg(node), node);
						arithmetic_op_maybe_fp(ctx, lvalue, tmp, lvalue, node, node->op);

						ctx.cur_func->free(tmp);
						ret = result;
						assigned = true;
						break;
					}
					case op::postDec: {
						var* result = dest ? dest : ctx.cur_func->allocate(ctx, lvalue->type);
						var* tmp;
						if (lvalue->type->is_floating_point) tmp = ctx.cur_func->imm(ctx, 1.0f);
						else tmp = ctx.cur_func->imm(ctx, i64(1));

						lvalue->store_in(result->to_reg(node), node);
						arithmetic_op_maybe_fp(ctx, lvalue, tmp, lvalue, node, node->op);

						ctx.cur_func->free(tmp);
						ret = result;
						assigned = true;
						break;
					}
					case op::less: {
						var* result = dest ? dest : ctx.cur_func->allocate(ctx, ctx.type("bool"));
						var* lv = lvalue;
						var* rv = rvalue;
						op oper = node->op;
						if (lv->is_imm && !rv->is_imm) {
							swap(lv, rv);
							oper = op::greaterEq;
						}
						arithmetic_op_maybe_fp(ctx, lv, rv, result, node, oper);
						ret = result;
						break;
					}
					case op::greater: {
						var* result = dest ? dest : ctx.cur_func->allocate(ctx, ctx.type("bool"));
						var* lv = lvalue;
						var* rv = rvalue;
						op oper = node->op;
						if (lv->is_imm && !rv->is_imm) {
							swap(lv, rv);
							oper = op::lessEq;
						}
						arithmetic_op_maybe_fp(ctx, lv, rv, result, node, oper);
						ret = result;
						break;
					}
					case op::lessEq: {
						var* result = dest ? dest : ctx.cur_func->allocate(ctx, ctx.type("bool"));
						var* lv = lvalue;
						var* rv = rvalue;
						op oper = node->op;
						if (lv->is_imm && !rv->is_imm) {
							swap(lv, rv);
							oper = op::greater;
						}
						arithmetic_op_maybe_fp(ctx, lv, rv, result, node, oper);
						ret = result;
						break;
					}
					case op::greaterEq: {
						var* result = dest ? dest : ctx.cur_func->allocate(ctx, ctx.type("bool"));
						var* lv = lvalue;
						var* rv = rvalue;
						op oper = node->op;
						if (lv->is_imm && !rv->is_imm) {
							swap(lv, rv);
							oper = op::less;
						}
						arithmetic_op_maybe_fp(ctx, lv, rv, result, node, oper);
						ret = result;
						break;
					}
					case op::notEq: {
						var* result = dest ? dest : ctx.cur_func->allocate(ctx, ctx.type("bool"));
						arithmetic_op_maybe_fp(ctx, lvalue, rvalue, result, node, node->op);
						ret = result;
						break;
					}
					case op::isEq: {
						var* result = dest ? dest : ctx.cur_func->allocate(ctx, ctx.type("bool"));
						arithmetic_op_maybe_fp(ctx, lvalue, rvalue, result, node, node->op);
						ret = result;
						break;
					}
					case op::eq: {
						arithmetic_op_maybe_fp(ctx, nullptr, rvalue, lvalue, node, node->op);
						if (dest) lvalue->store_in(dest->to_reg(node), node);
						ret = dest ? dest : lvalue;
						assigned = true;
						break;
					}
					case op::not: {
						var* result = dest ? dest : ctx.cur_func->allocate(ctx, ctx.type("bool"));
						arithmetic_op_maybe_fp(ctx, nullptr, lvalue, result, node, node->op);
						ret = result;
						break;
					}
					case op::negate: {
						// todo: use bitwise operations instead of multiplication
						var* result = dest ? dest : ctx.cur_func->allocate(ctx, rvalue->type);
						var* tmp = nullptr;
						if (rvalue->type->is_floating_point) tmp = ctx.cur_func->imm(ctx, -1.0f);
						else tmp = ctx.cur_func->imm(ctx, i64(-1));
						arithmetic_op_maybe_fp(ctx, rvalue, tmp, result, node, node->op);
						ctx.cur_func->free(tmp);
						break;
					}
					case op::addr: {
						// todo: remove
						var* result = dest ? dest : ctx.cur_func->allocate(ctx, lvalue->type);
						ret = result;
						break;
					}
					case op::at: {
						// todo: remove
						var* result = dest ? dest : ctx.cur_func->allocate(ctx, lvalue->type);
						ret = result;
						break;
					}
					case op::member: {
						if (!lvalue) {
							data_type* t = ctx.type(node->lvalue);
							data_type::property* prop = t->prop(*node->rvalue);
							if (prop) {
								if (ctx.do_store_member_info) {
									ctx.last_member_or_method.is_set = true;
									ctx.last_member_or_method.name = *node->rvalue;
									ctx.last_member_or_method.subject = nullptr;
									ctx.last_member_or_method.type = t;
									ctx.last_member_or_method.method = nullptr;
								}

								if (!(prop->flags & bind::property_flags::pf_static)) {
									ctx.log->err(format("Expected static method or property of type '%s'", t->name.c_str()), node->rvalue);
								} else {
									var* pval = ctx.cur_func->allocate(ctx, prop->type);
									
									if (prop->flags ^ bind::pf_pointer) {
										if (prop->type->built_in) {
											// load value
											vmi ld;
											switch (prop->type->size) {
												case 1: { ld = vmi::ld8; break; }
												case 2: { ld = vmi::ld16; break; }
												case 4: { ld = vmi::ld32; break; }
												case 8: { ld = vmi::ld64; break; }
											}
											ctx.add(
												encode(ld).operand(pval->to_reg(node)).operand(vmr::zero).operand(prop->offset),
												node
											);
										} else {
											// get pointer to property in pval
											ctx.add(
												encode(vmi::addui).operand(pval->to_reg(node)).operand(vmr::zero).operand(prop->offset),
												node
											);
										}
									}
									else {
										if (prop->type->built_in) {
											// load pointer
											vmi ld;
											switch (sizeof(void*)) {
												case 1: { ld = vmi::ld8; break; }
												case 2: { ld = vmi::ld16; break; }
												case 4: { ld = vmi::ld32; break; }
												case 8: { ld = vmi::ld64; break; }
											}
											ctx.add(
												encode(ld).operand(pval->to_reg(node)).operand(vmr::zero).operand(prop->offset),
												node
											);

											// load value pointed to by pval
											switch (prop->type->size) {
												case 1: { ld = vmi::ld8; break; }
												case 2: { ld = vmi::ld16; break; }
												case 4: { ld = vmi::ld32; break; }
												case 8: { ld = vmi::ld64; break; }
											}
											ctx.add(
												encode(ld).operand(pval->to_reg(node)).operand(pval->to_reg(node)),
												node
											);
										} else {
											// load pointer
											vmi ld;
											switch (sizeof(void*)) {
												case 1: { ld = vmi::ld8; break; }
												case 2: { ld = vmi::ld16; break; }
												case 4: { ld = vmi::ld32; break; }
												case 8: { ld = vmi::ld64; break; }
											}
											ctx.add(
												encode(ld).operand(pval->to_reg(node)).operand(vmr::zero).operand(prop->offset),
												node
											);
										}
									}

									if (dest) {
										if (!prop->type->equals(dest->type)) {
											var* converted = cast(ctx, pval, dest, node);
											converted->store_in(dest->to_reg(node), node);
											if (converted != pval) ctx.cur_func->free(converted);
										} else pval->store_in(dest->to_reg(node), node);

										ctx.cur_func->free(pval);
									}
									var* result = dest ? dest : pval;
									ret = result;
								}

								return ret;
							} else {
								func* sf = t->method(*node->rvalue);
								if (!sf || sf->is_thiscall) {
									ctx.log->err(format("Expected static method or property of type '%s'", t->name.c_str()), node->rvalue);
								} else {
									if (ctx.do_store_member_info) {
										ctx.last_member_or_method.is_set = true;
										ctx.last_member_or_method.name = *node->rvalue;
										ctx.last_member_or_method.subject = nullptr;
										ctx.last_member_or_method.type = t;
										ctx.last_member_or_method.method = sf;
									}
								}

								return nullptr;
							}
						}

						data_type::property* prop = lvalue->type->prop(*node->rvalue);
						if (prop) {
							var* pval = nullptr;
							if (ctx.do_store_member_info) {
								ctx.last_member_or_method.is_set = true;
								ctx.last_member_or_method.name = *node->rvalue;
								ctx.last_member_or_method.subject = lvalue;
								ctx.last_member_or_method.type = lvalue->type;
								ctx.last_member_or_method.method = nullptr;
							}

							if (prop->getter) {
								pval = call(ctx, prop->getter, node->rvalue, { lvalue }, lvalue->type);
							} else {
								pval = ctx.cur_func->allocate(ctx, prop->type); 

								if (prop->flags ^ bind::pf_pointer) {
									if (prop->type->built_in) {
										// load value at lvalue + n bytes
										vmi ld;
										switch (prop->type->size) {
											case 1: { ld = vmi::ld8; break; }
											case 2: { ld = vmi::ld16; break; }
											case 4: { ld = vmi::ld32; break; }
											case 8: { ld = vmi::ld64; break; }
										}
										ctx.add(
											encode(ld).operand(pval->to_reg(node)).operand(lvalue->to_reg(node)).operand(prop->offset),
											node
										);
									} else {
										// get pointer to property in pval
										ctx.add(
											encode(vmi::addui).operand(pval->to_reg(node)).operand(lvalue->to_reg(node)).operand(prop->offset),
											node
										);
									}
								}
								else {
									if (prop->type->built_in) {
										// load pointer at lvalue + n bytes
										vmi ld;
										switch (sizeof(void*)) {
											case 1: { ld = vmi::ld8; break; }
											case 2: { ld = vmi::ld16; break; }
											case 4: { ld = vmi::ld32; break; }
											case 8: { ld = vmi::ld64; break; }
										}
										ctx.add(
											encode(ld).operand(pval->to_reg(node)).operand(lvalue->to_reg(node)).operand(prop->offset),
											node
										);

										// load value pointed to by pval
										switch (prop->type->size) {
											case 1: { ld = vmi::ld8; break; }
											case 2: { ld = vmi::ld16; break; }
											case 4: { ld = vmi::ld32; break; }
											case 8: { ld = vmi::ld64; break; }
										}
										ctx.add(
											encode(ld).operand(pval->to_reg(node)).operand(pval->to_reg(node)),
											node
										);
									} else {
										// load pointer at lvalue + n bytes
										vmi ld;
										switch (sizeof(void*)) {
											case 1: { ld = vmi::ld8; break; }
											case 2: { ld = vmi::ld16; break; }
											case 4: { ld = vmi::ld32; break; }
											case 8: { ld = vmi::ld64; break; }
										}
										ctx.add(
											encode(ld).operand(pval->to_reg(node)).operand(lvalue->to_reg(node)).operand(prop->offset),
											node
										);
									}
								}
							}

							if (dest) {
								if (!prop->type->equals(dest->type)) {
									var* converted = cast(ctx, pval, dest, node);
									converted->store_in(dest->to_reg(node), node);
									if (converted != pval) ctx.cur_func->free(converted);
								} else pval->store_in(dest->to_reg(node), node);

								ctx.cur_func->free(pval);
							}
							var* result = dest ? dest : pval;
							ret = result;
						}
						else {
							func* method = lvalue->type->method(*node->rvalue);
							if (method) {
								ret = lvalue;
								if (ctx.do_store_member_info) {
									ctx.last_member_or_method.is_set = true;
									ctx.last_member_or_method.name = *node->rvalue;
									ctx.last_member_or_method.subject = lvalue;
									ctx.last_member_or_method.type = lvalue->type;
									ctx.last_member_or_method.method = method;
								}
							} else {
								string n = *node->rvalue;
								ctx.log->err(format("'%s' is not a property or method of type '%s'", n.c_str(), lvalue->type->name.c_str()), node);
							}
						}
						break;
					}
					case op::index: {
						ctx.log->err(format("Type '%s' has no [] operator", lvalue->type->name.c_str()), node);
						break;
					}
					case op::newObj: {
						data_type* tp = ctx.type(node->data_type);
						func* getmem = ctx.function("alloc");
						var* sz = ctx.cur_func->imm(ctx, (integer)tp->actual_size);
						ret = call(ctx, getmem, node, { sz });
						ret->type = tp;
						ctx.cur_func->free(sz);

						if (tp->ctor) {
							vector<var*> args = { ret };
							vector<bool> free_arg = { true };

							if (tp->requires_subtype) {
								if (tp->sub_type) {
									args.push_back(ctx.cur_func->imm(ctx, (i64)tp->sub_type->type_id));
									free_arg.push_back(true);
								} else {
									ctx.log->err(format("Type '%s' can not be constructed without a sub-type", tp->name.c_str()), node);

									// Prevent errors that would stem from this
									args.push_back(ctx.cur_func->imm(ctx, (i64)0));
									free_arg.push_back(true);
								}
							}

							ast_node* arg = node->arguments;
							while (arg) {
								args.push_back(compile_expression(ctx, arg, nullptr));
								free_arg.push_back(!args[args.size() - 1]->is_variable);
								arg = arg->next;
							}
							ret = call(ctx, tp->ctor, node, args);
							ret->type = tp;
							for (u32 i = 0;i < args.size();i++) {
								if (free_arg[i]) ctx.cur_func->free(args[i]);
							}
						}

						if (dest) {
							var* v = ret;
							if (!dest->type->equals(v->type)) {
								v = cast(ctx, v, dest, node);
							}
							v->store_in(dest->to_reg(node), node);
							ctx.cur_func->free(ret);
							if (v != ret) ctx.cur_func->free(v);
							ctx.cur_func->auto_free_consumed_vars = auto_free;
							return dest;
						}
						break;
					}
					case op::stackObj: {
						data_type* tp = ctx.type(node->data_type);
						var* obj = ctx.cur_func->allocate_stack_var(ctx, tp, node);

						if (tp->ctor) {
							vector<var*> args = { obj };
							vector<bool> free_arg = { true };

							if (tp->requires_subtype) {
								if (tp->sub_type) {
									args.push_back(ctx.cur_func->imm(ctx, (i64)tp->sub_type->type_id));
									free_arg.push_back(true);
								} else {
									ctx.log->err(format("Type '%s' can not be constructed without a sub-type", tp->name.c_str()), node);

									// Prevent errors that would stem from this
									args.push_back(ctx.cur_func->imm(ctx, (i64)0));
									free_arg.push_back(true);
								}
							}

							ast_node* arg = node->arguments;
							while (arg) {
								args.push_back(compile_expression(ctx, arg, nullptr));
								free_arg.push_back(!args[args.size() - 1]->is_variable);
								arg = arg->next;
							}
							ret = call(ctx, tp->ctor, node, args);
							obj->move_stack_reference(ret);
							ret->type = tp;

							for (u32 i = 0;i < args.size();i++) {
								if (free_arg[i]) ctx.cur_func->free(args[i]);
							}
						} else ret = obj;

						if (dest) {
							var* v = ret;
							if (!dest->type->equals(v->type)) {
								v = cast(ctx, v, dest, node);
							}
							v->store_in(dest->to_reg(node), node);
							ret->move_stack_reference(dest);
							ctx.cur_func->free(ret);
							if (v != ret) ctx.cur_func->free(v);
							ctx.cur_func->auto_free_consumed_vars = auto_free;
							return dest;
						}
					}
				}
			}

			ast_node* node_assigned = node->lvalue ? node->lvalue : node->rvalue;
			if ((assignsLv || assignsRv) && (node_assigned->op == op::member || node_assigned->op == op::index) && ret) {
				// store result
				if (node_assigned->op == op::member) {
					data_type::property* prop = ctx.last_member_or_method.type->prop(ctx.last_member_or_method.name);
					if (prop->getter) {
						// require setter
						if (prop->setter) {
							var* result = call(ctx, prop->setter, node, { ctx.last_member_or_method.subject, ret }, ctx.last_member_or_method.type);
							if (result) {
								if (!ret->is_variable) ctx.cur_func->free(ret);
								ret = result;
							}
						} else {
							ctx.log->err(format("Cannot assign property '%s' of '%s' which has a getter but no setter", prop->name.c_str(), ctx.last_member_or_method.type->name.c_str()), node);
						}
					} else {
						// set the property directly
					}
				} else {
					// todo: set $v1 to pointer to memory if [] operator returns pointer/reference. Raise flag in ctx
					// if flag not raised: value is unassignable
				}
			}

			ctx.cur_func->auto_free_consumed_vars = auto_free;
		}

		if (lvalue && !lvalue->is_variable && lvalue != ret && lvalue != ctx.last_member_or_method.subject) {
			ctx.cur_func->free_stack_object(lvalue, node);
			ctx.cur_func->free(lvalue);
		}
		if (rvalue && !rvalue->is_variable && rvalue != ret && rvalue != ctx.last_member_or_method.subject) {
			ctx.cur_func->free_stack_object(rvalue, node);
			ctx.cur_func->free(rvalue);
		}

		return ret;
	}
};