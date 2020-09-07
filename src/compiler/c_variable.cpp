#include <compiler/variable.h>
#include <compiler/context.h>
#include <compiler/function.h>
#include <parser/ast.h>
#include <errors.h>
#include <warnings.h>
#include <context.h>
#include <vm_type.h>
#include <vm_function.h>
#include <robin_hood.h>

namespace gjs {
	using exc = error::exception;
	using ec = error::ecode;
	using wc = warning::wcode;

	namespace compile {
		bool has_valid_conversion(vm_type* from, vm_type* to) {
			return false;
		}


		var::var(context* ctx, u64 u) {
			m_ctx = ctx;
			m_is_imm = true;
			m_instantiation = ctx->rel_node->ref;
			m_is_ptr = false;
			m_reg_id = -1;
			m_imm.u = u;
			m_type = ctx->type("u64");
		}

		var::var(context* ctx, i64 i) {
			m_ctx = ctx;
			m_is_imm = true;
			m_instantiation = ctx->rel_node->ref;
			m_is_ptr = false;
			m_reg_id = -1;
			m_imm.i = i;
			m_type = ctx->type("i64");
		}

		var::var(context* ctx, f32 f) {
			m_ctx = ctx;
			m_is_imm = true;
			m_instantiation = ctx->rel_node->ref;
			m_is_ptr = false;
			m_reg_id = -1;
			m_imm.f = f;
			m_type = ctx->type("f32");
		}

		var::var(context* ctx, f64 d) {
			m_ctx = ctx;
			m_is_imm = true;
			m_instantiation = ctx->rel_node->ref;
			m_is_ptr = false;
			m_reg_id = -1;
			m_imm.d = d;
			m_type = ctx->type("f64");
		}

		var::var(context* ctx, const std::string& s) {
			m_ctx = ctx;
			m_is_imm = true;
			m_instantiation = ctx->rel_node->ref;
			m_is_ptr = true;
			m_reg_id = -1;
			m_type = ctx->type("string");
		}

		var::var(context* ctx, u32 reg_id, vm_type* type) {
			m_ctx = ctx;
			m_is_imm = false;
			m_instantiation = ctx->rel_node->ref;
			m_is_ptr = !type->is_primitive;
			m_reg_id = reg_id;
			m_type = type;
		}

		var::var() {
			m_ctx = nullptr;
			m_is_imm = false;
			m_is_ptr = false;
			m_reg_id = -1;
			m_type = nullptr;
			m_imm.u = 0;
		}

		var::var(const var& v) {
			m_ctx = v.m_ctx;
			m_is_imm = v.m_is_imm;
			m_instantiation = v.m_instantiation;
			m_is_ptr = v.m_is_ptr;
			m_reg_id = v.m_reg_id;
			m_imm = v.m_imm;
			m_type = v.m_type;
		}

		var::~var() {
		}




		u64 var::size() const {
			return m_type->size;
		}

		bool var::has_prop(const std::string& prop) const {
			for (u16 i = 0;i < m_type->properties.size();i++) {
				if (m_type->properties[i].name == prop) return true;
			}

			return false;
		}

		var var::prop(const std::string& prop) const {
			for (u16 i = 0;i < m_type->properties.size();i++) {
				auto& p = m_type->properties[i];
				if (p.name == prop) {
					var ret = m_ctx->empty_var(p.type);
					if (p.flags & bind::pf_static) {
						m_ctx->add(operation::eq).operand(ret).operand(m_ctx->imm(p.offset));
					} else {
						m_ctx->add(operation::uadd).operand(ret).operand(*this).operand(m_ctx->imm(p.offset));
					}
					m_ctx->add(operation::load).operand(ret).operand(ret);
					return ret;
				}
			}

			throw exc(ec::c_no_such_property, m_ctx->rel_node->ref, m_type->name.c_str(), prop.c_str());
			return var();
		}

		var var::prop_ptr(const std::string& prop) const {
			for (u16 i = 0;i < m_type->properties.size();i++) {
				auto& p = m_type->properties[i];
				if (p.name == prop) {
					if (p.flags & bind::pf_static) {
						return m_ctx->imm(p.offset);
					} else {
						var ret = m_ctx->empty_var(m_ctx->type("u64"));
						m_ctx->add(operation::uadd).operand(ret).operand(*this).operand(m_ctx->imm(p.offset));
						return ret;
					}
				}
			}

			throw exc(ec::c_no_such_property, m_ctx->rel_node->ref, m_type->name.c_str(), prop.c_str());
			return var();
		}

		vm_function* var::method(const std::string& _name, vm_type* ret, const std::vector<vm_type*>& args) const {
			std::vector<vm_function*> matches;

			for (u16 m = 0;m < m_type->methods.size();m++) {
				// match name
				vm_function* func = nullptr;
				if (_name.find_first_of(' ') != std::string::npos) {
					// probably an operator
					std::vector<std::string> mparts = split(split(m_type->methods[m]->name, ":")[1], " \t\n\r");
					std::vector<std::string> sparts = split(_name, " \t\n\r");
					if (mparts.size() != sparts.size()) continue;
					bool matched = true;
					for (u32 i = 0;matched && i < mparts.size();i++) {
						matched = mparts[i] == sparts[i];
					}
					if (matched) func = m_type->methods[m];
				}
				std::string& bt_name = m_type->base_type ? m_type->base_type->name : m_type->name;
				if (m_type->methods[m]->name == bt_name + "::" + _name) func = m_type->methods[m];

				if (!func) continue;

				// match return type
				if (!has_valid_conversion(func->signature.return_type, ret)) continue;
				bool ret_tp_strict = func->signature.return_type->id() != ret->id();

				// match argument types
				if (func->signature.arg_types.size() != args.size()) continue;

				// prefer strict type matches
				bool match = true;
				for (u8 i = 0;i < args.size();i++) {
					if (func->signature.arg_types[i]->id() != args[i]->id()) {
						match = false;
						break;
					}
				}

				if (match && ret_tp_strict) return func;

				if (!match) {
					// check if the arguments are at least convertible
					match = true;
					for (u8 i = 0;i < args.size();i++) {
						if (!has_valid_conversion(args[i], func->signature.arg_types[i])) {
							match = false;
							break;
						}
					}

					if (!match) continue;
				}

				matches.push_back(func);
			}

			if (matches.size() > 1) {
				m_ctx->log()->err(ec::c_ambiguous_method, m_ctx->rel_node->ref, _name.c_str(), m_type->name.c_str(), arg_tp_str(args).c_str(), ret->name.c_str());
				return nullptr;
			}

			if (matches.size() == 1) {
				return matches[0];
			}

			m_ctx->log()->err(ec::c_no_such_method, m_ctx->rel_node->ref, m_type->name.c_str(), _name.c_str(), arg_tp_str(args).c_str(), ret->name.c_str());
			return nullptr;
		}

		bool var::convertible_to(vm_type* tp) const {
			return has_valid_conversion(m_type, tp);
		}

		var var::convert(vm_type* tp) const {
			return var();
		}

		std::string var::to_string() const {
			return "";
		}



		using ot = parse::ast::operation_type;

		static const char* ot_str[] = {
			"invlaid",
			"+",
			"-",
			"*",
			"/",
			"%",
			"<<",
			">>",
			"&&",
			"||",
			"&",
			"|",
			"^",
			"+=",
			"-=",
			"*=",
			"/=",
			"%=",
			"<<=",
			">>=",
			"&&=",
			"||=",
			"&=",
			"|=",
			"^=",
			"++",
			"--",
			"++",
			"--",
			"<",
			">",
			"<=",
			">=",
			"!=",
			"==",
			"=",
			"!",
			"-"
			"invlaid",
			"[]",
			"invalid",
			"invalid"
		};

		operation get_op(ot op, vm_type* tp) {
			using o = operation;
			if (!tp->is_primitive) return o::null;

			static o possible_arr[][4] = {
				{ o::iadd , o::uadd , o::dadd , o::fadd  },
				{ o::isub , o::usub , o::dsub , o::fsub  },
				{ o::imul , o::umul , o::dmul , o::fmul  },
				{ o::idiv , o::udiv , o::ddiv , o::fdiv  },
				{ o::imod , o::umod , o::dmod , o::fmod  },
				{ o::ilt  , o::ult  , o::dlt  , o::flt   },
				{ o::igt  , o::ugt  , o::dgt  , o::fgt   },
				{ o::ilte , o::ulte , o::dlte , o::flte  },
				{ o::igte , o::ugte , o::dgte , o::fgte  },
				{ o::incmp, o::uncmp, o::dncmp, o::fncmp },
				{ o::icmp , o::ucmp , o::dcmp , o::fcmp  },
				{ o::sl   , o::sl   , o::null , o::null  },
				{ o::sr   , o::sr   , o::null , o::null  },
				{ o::land , o::land , o::null , o::null  },
				{ o::lor  , o::lor  , o::null , o::null  },
				{ o::band , o::band , o::null , o::null  },
				{ o::bor  , o::bor  , o::null , o::null  },
				{ o::bxor , o::bxor , o::null , o::null  }
			};
			static robin_hood::unordered_map<ot, u8> possible_map = {
				{ ot::add       , 0  },
				{ ot::sub       , 1  },
				{ ot::mul       , 2  },
				{ ot::div       , 3  },
				{ ot::mod       , 4  },
				{ ot::less      , 5  },
				{ ot::greater   , 6  },
				{ ot::lessEq    , 7  },
				{ ot::greaterEq , 8  },
				{ ot::notEq     , 9  },
				{ ot::isEq      , 10 },
				{ ot::shiftLeft , 11 },
				{ ot::shiftRight, 12 },
				{ ot::land      , 13 },
				{ ot::lor       , 14 },
				{ ot::band      , 15 },
				{ ot::bor       , 16 },
				{ ot::bxor      , 17 }
			};

			if (!tp->is_floating_point) {
				if (!tp->is_unsigned) {
					return possible_arr[possible_map[op]][0];
				} else {
					return possible_arr[possible_map[op]][1];
				}
			} else {
				if (tp->size == sizeof(f64)) {
					return possible_arr[possible_map[op]][2];
				} else {
					return possible_arr[possible_map[op]][3];
				}
			}

			return o::null;
		}

		var do_bin_op(context* ctx, const var& a, const var& b, ot _op) {
			operation op = get_op(_op, a.type());
			if ((u8)op) {
				var ret = ctx->new_var(a.type());
				ctx->add(op).operand(ret).operand(a).operand(b.convert(a.type()));
				return ret;
			}
			else {
				vm_function* f = a.method(std::string("operator ") + ot_str[(u8)_op], a.type(), { b.type() });
				if (f) {
					return call(*ctx, f, { a, b.convert(f->signature.arg_types[0]) });
				}
			}

			return ctx->dummy_var(a.type());
		}

		var var::operator + (const var& rhs) const {
			return do_bin_op(m_ctx, *this, rhs, ot::add);
		}

		var var::operator - (const var& rhs) const {
			return do_bin_op(m_ctx, *this, rhs, ot::sub);
		}

		var var::operator * (const var& rhs) const {
			return do_bin_op(m_ctx, *this, rhs, ot::mul);
		}

		var var::operator / (const var& rhs) const {
			return do_bin_op(m_ctx, *this, rhs, ot::div);
		}

		var var::operator % (const var& rhs) const {
			return do_bin_op(m_ctx, *this, rhs, ot::mod);
		}

		var var::operator << (const var& rhs) const {
			return do_bin_op(m_ctx, *this, rhs, ot::shiftLeft);
		}

		var var::operator >> (const var& rhs) const {
			return do_bin_op(m_ctx, *this, rhs, ot::shiftRight);
		}

		var var::operator && (const var& rhs) const {
			return do_bin_op(m_ctx, *this, rhs, ot::land);
		}

		var var::operator || (const var& rhs) const {
			return do_bin_op(m_ctx, *this, rhs, ot::lor);
		}

		var var::operator & (const var& rhs) const {
			return do_bin_op(m_ctx, *this, rhs, ot::band);
		}

		var var::operator | (const var& rhs) const {
			return do_bin_op(m_ctx, *this, rhs, ot::bor);
		}

		var var::operator ^ (const var& rhs) const {
			return do_bin_op(m_ctx, *this, rhs, ot::bxor);
		}

		var var::operator += (const var& rhs) const {
			var result = do_bin_op(m_ctx, *this, rhs, ot::addEq);
			operator_eq(result);
			return result;
		}

		var var::operator -= (const var& rhs) const {
			var result = do_bin_op(m_ctx, *this, rhs, ot::subEq);
			operator_eq(result);
			return result;
		}

		var var::operator *= (const var& rhs) const {
			var result = do_bin_op(m_ctx, *this, rhs, ot::mulEq);
			operator_eq(result);
			return result;
		}

		var var::operator /= (const var& rhs) const {
			var result = do_bin_op(m_ctx, *this, rhs, ot::divEq);
			operator_eq(result);
			return result;
		}

		var var::operator %= (const var& rhs) const {
			var result = do_bin_op(m_ctx, *this, rhs, ot::modEq);
			operator_eq(result);
			return result;
		}

		var var::operator <<= (const var& rhs) const {
			var result = do_bin_op(m_ctx, *this, rhs, ot::shiftLeftEq);
			operator_eq(result);
			return result;
		}

		var var::operator >>= (const var& rhs) const {
			var result = do_bin_op(m_ctx, *this, rhs, ot::shiftRightEq);
			operator_eq(result);
			return result;
		}

		var var::operator_landEq (const var& rhs) const {
			var result = do_bin_op(m_ctx, *this, rhs, ot::landEq);
			operator_eq(result);
			return result;
		}

		var var::operator_lorEq (const var& rhs) const {
			var result = do_bin_op(m_ctx, *this, rhs, ot::lorEq);
			operator_eq(result);
			return result;
		}

		var var::operator &= (const var& rhs) const {
			var result = do_bin_op(m_ctx, *this, rhs, ot::bandEq);
			operator_eq(result);
			return result;
		}

		var var::operator |= (const var& rhs) const {
			var result = do_bin_op(m_ctx, *this, rhs, ot::borEq);
			operator_eq(result);
			return result;
		}

		var var::operator ^= (const var& rhs) const {
			var result = do_bin_op(m_ctx, *this, rhs, ot::bxorEq);
			operator_eq(result);
			return result;
		}

		var var::operator ++ () const {
			var result = do_bin_op(m_ctx, *this, m_ctx->imm(i64(1)), ot::add);
			operator_eq(result);
			return result;
		}

		var var::operator -- () const {
			var result = do_bin_op(m_ctx, *this, m_ctx->imm(i64(1)), ot::sub);
			operator_eq(result);
			return result;
		}

		var var::operator ++ (int) const {
			var clone = m_ctx->clone_var(*this);
			var result = do_bin_op(m_ctx, *this, m_ctx->imm(i64(1)), ot::add);
			operator_eq(result);
			return result;
		}

		var var::operator -- (int) const {
			var clone = m_ctx->clone_var(*this);
			var result = do_bin_op(m_ctx, *this, m_ctx->imm(i64(1)), ot::sub);
			operator_eq(result);
			return result;
		}

		var var::operator < (const var& rhs) const {
			return do_bin_op(m_ctx, *this, rhs, ot::less);
		}

		var var::operator > (const var& rhs) const {
			return do_bin_op(m_ctx, *this, rhs, ot::greater);
		}

		var var::operator <= (const var& rhs) const {
			return do_bin_op(m_ctx, *this, rhs, ot::lessEq);
		}

		var var::operator >= (const var& rhs) const {
			return do_bin_op(m_ctx, *this, rhs, ot::greaterEq);
		}

		var var::operator != (const var& rhs) const {
			return do_bin_op(m_ctx, *this, rhs, ot::notEq);
		}

		var var::operator == (const var& rhs) const {
			return do_bin_op(m_ctx, *this, rhs, ot::isEq);
		}

		var var::operator ! () const {
			return do_bin_op(m_ctx, *this, m_ctx->imm(i64(0)), ot::isEq);
		}

		var var::operator - () const {
			m_ctx->add(operation::neg).operand(*this);
			return m_ctx->clone_var(*this);
		}

		var var::operator_eq (const var& rhs) const {
			m_ctx->add(operation::eq).operand(*this).operand(rhs.convert(m_type));
			return m_ctx->clone_var(*this);
		}

		var var::operator_idx (vm_type* ret_tp, const var& rhs) const {
			vm_function* f = method("operator []", ret_tp, { rhs.type() });
			if (f) {
				return call(*m_ctx, f, { *this, rhs.convert(f->signature.arg_types[0]) });
			}

			return m_ctx->dummy_var(ret_tp);
		}
	};
};