#include <compiler/context.h>
#include <compiler/function.h>
#include <parser/ast.h>
#include <context.h>
#include <vm_type.h>
#include <vm_function.h>
#include <errors.h>
#include <warnings.h>

namespace gjs {
	using exc = error::exception;
	using ec = error::ecode;
	using wc = warning::wcode;

	namespace compile {
		context::context() {
			next_reg_id = 0;
			env = nullptr;
			input = nullptr;
			new_types = nullptr;
			rel_node = nullptr;
		}

		var context::imm(u64 u) {
			return var(this, u);
		}

		var context::imm(i64 i) {
			return var(this, i);
		}

		var context::imm(f32 f) {
			return var(this, f);
		}

		var context::imm(f64 d) {
			return var(this, d);
		}

		var context::imm(const std::string& s) {
			return var(this, s);
		}

		var context::new_var(vm_type* type, const std::string& name) {
			return var();
		}

		var context::new_var(vm_type* type) {
			return var();
		}

		var context::dyn_var(vm_type* type, const std::string& name) {
			return var();
		}

		var context::dyn_var(vm_type* type) {
			return var();
		}

		var context::clone_var(var v) {
			return var();
		}

		var context::empty_var(vm_type* type) {
			return var(this, next_reg_id++, type);
		}

		var context::dummy_var(vm_type* type, const std::string& name) {
			var v = var();
			v.m_ctx = this;
			v.m_instantiation = rel_node->ref;
			v.m_name = name;
			v.m_type = type;
			return v;
		}

		var context::dummy_var(vm_type* type) {
			var v = var();
			v.m_ctx = this;
			v.m_instantiation = rel_node->ref;
			v.m_type = type;
			return v;
		}

		vm_function* context::function(const std::string& name, vm_type* ret, const std::vector<vm_type*>& args) {
			std::vector<vm_function*> matches;

			for (u16 f = 0;f < new_functions.size();f++) {
				// match name
				vm_function* func = new_functions[f];

				if (func->name != name) continue;

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
				log()->err(ec::c_ambiguous_function, rel_node->ref, name.c_str(), name.c_str(), arg_tp_str(args).c_str(), ret->name.c_str());
				return nullptr;
			}

			if (matches.size() == 1) {
				return matches[0];
			}

			log()->err(ec::c_no_such_function, rel_node->ref, name.c_str(), arg_tp_str(args).c_str(), ret->name.c_str());
			return nullptr;
		}

		vm_type* context::type(const std::string& name) {
			vm_type* t = env->types()->get(hash(name));
			if (t) return t;
			t = new_types->get(name);
			return t;
		}

		tac_instruction& context::add(operation op) {
			code.push_back(tac_instruction(op, node()->ref));
			return code[code.size() - 1];
		}
		
		void context::push_node(parse::ast* node) {
			rel_node = node;
		}

		void context::pop_node() {
		}

		parse::ast* context::node() {
			return rel_node;
		}

		compile_log* context::log() {
			return env->compiler()->log();
		}
	};
};