#include <compile.h>
#include <context.h>
#include <parse.h>
#include <instruction_array.h>
#include <instruction_encoder.h>
#include <register.h>
#include <vm_state.h>

using namespace std;

namespace gjs {
	using nt = ast_node::node_type;
	using op = ast_node::operation_type;
	using vmr = vm_register;
	using vmi = vm_instruction;
	static const char* op_symbol[] = {
		"invalid",
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
		"- (negate)",
		"& (address)",
		"* (dereference)",
		"."
	};

	#define is_fp(r) (r >= vmr::f0 && r <= vmr::f15)
	#define is_arg(r) (r >= vmr::a0 && r <= vmr::a7)

	compile_exception::compile_exception(const std::string& _file, const string& _text, const string& _lineText, u32 _line, u32 _col) {
		file = _file;
		text = _text;
		lineText = _lineText;
		line = _line;
		col = _col;
	}

	compile_exception::compile_exception(const std::string& text, ast_node* node) : compile_exception(node->start.file, text, node->start.lineText, node->start.line, node->start.col) {
	}

	compile_exception::~compile_exception() { }



	void compile_log::err(const std::string& text, const std::string& file, const std::string& lineText, u32 line, u32 col) {
		errors.push_back({ file, text, lineText, line, col });
	}

	void compile_log::err(const std::string& text, ast_node* node) {
		err(node->start.file, text, node->start.lineText, node->start.line, node->start.col);
	}

	void compile_log::warn(const std::string& text, const std::string& file, const std::string& lineText, u32 line, u32 col) {
		warnings.push_back({ file, text, lineText, line, col });
	}

	void compile_log::warn(const std::string& text, ast_node* node) {
		warn(node->start.file, text, node->start.lineText, node->start.line, node->start.col);
	}


	struct compile_context;
	struct func;
	struct data_type;
	void compile_function(compile_context& ctx, ast_node* node, func* out);
	void compile_node(compile_context& ctx, ast_node* node);
	void compile_variable_declaration(compile_context& ctx, ast_node* node);



	struct func {
		struct stack_slot {
			address addr;
			u32 size;
			bool used;
		};

		struct arg_info {
			string name;
			data_type* type;
			vmr loc;
		};

		struct var {
			var() {
				ctx = nullptr;
				is_variable = false;
				no_auto_free = false;
				is_reg = false;
				is_imm = false;
				type = nullptr;
				count = 1;
				total_ref_count = 0;
				ref_count = 0;
			}

			compile_context* ctx;
			bool is_variable;
			bool no_auto_free;
			bool is_reg;
			bool is_imm;
			string name;
			data_type* type;
			u32 count;
			u16 total_ref_count;
			u16 ref_count;
			union {
				vmr reg;
				address stack_addr;
			} loc;
			union {
				integer i;
				decimal d;
			} imm;

			address move_to_stack(ast_node* because, integer offset = 0);

			address move_to_stack(ast_node* because, vmr offset);

			vmr move_to_register(ast_node* because, integer offset = 0);

			vmr move_to_register(ast_node* because, vmr offset);

			vmr to_reg(ast_node* because, integer offset = 0);

			vmr to_reg(ast_node* because, vmr offset);

			address to_stack(ast_node* because, integer offset = 0);

			address to_stack(ast_node* because, vmr offset);

			// stack offset has no effect if the loc doesn't point to a stack address
			void move_to(vmr reg, ast_node* because, integer stack_offset = 0);

			void move_to(vmr reg, ast_node* because, vmr stack_offset);

			// same as move_to, but doesn't affect this var or free its register or stack address
			// and doesn't check if reg is available, it will overwrite the value stored in reg
			// and not mark reg as in-use. Used exclusively for passing arguments to functions.
			// As such, overwritten registers will either be restored to their previous values
			// after the call is made or weren't in use anyway
			void store_in(vmr reg, ast_node* because, integer stack_offset = 0);

			void store_in(vmr reg, ast_node* because, vmr stack_offset);
		};

		struct scope {
			robin_hood::unordered_map<string, var*> vars;
		};

		func() {
			registers.a = {
				vmr::a0, vmr::a1, vmr::a2, vmr::a3, vmr::a4, vmr::a5, vmr::a6, vmr::a7
			};

			registers.gp = {
				vmr::s0, vmr::s1, vmr::s2, vmr::s3, vmr::s4, vmr::s5, vmr::s6, vmr::s7
			};

			registers.fp = {
				vmr::f0 , vmr::f1 , vmr::f2 , vmr::f3 , vmr::f4 , vmr::f5 , vmr::f6 , vmr::f7 ,
				vmr::f8 , vmr::f9 , vmr::f10, vmr::f11, vmr::f12, vmr::f13, vmr::f14, vmr::f15
			};

			scopes.push_back(scope());

			return_reg = vmr::register_count;
			return_type = nullptr;
			root = nullptr;
			auto_free_consumed_vars = true;
			stack.parent = this;
			registers.parent = this;
		}

		func(compile_context& ctx, vm_function* f);

		string name;
		data_type* return_type;
		vmr return_reg;
		vector<arg_info> args;
		u64 entry;
		vector<scope> scopes;
		ast_node* root;
		bool auto_free_consumed_vars;
		
		struct {
			func* parent;
			vector<stack_slot> slots;

			address allocate(u32 size) {
				if (parent->auto_free_consumed_vars) parent->free_consumed_vars();
				address sz = 0;
				for (u32 i = 0;i < slots.size();i++) {
					if (slots[i].size == size && !slots[i].used) {
						slots[i].used = true;
						return slots[i].addr;
					}
					sz += slots[i].size;
				}

				slots.push_back({ sz, size, true });
				return sz;
			}

			void free(address addr) {
				for (u32 i = 0;i < slots.size();i++) {
					if (slots[i].addr == addr) {
						slots[i].used = false;
						return;
					}
				}
			}

			u32 size() {
				u32 sz = 0;
				for (u32 i = 0;i < slots.size();i++) sz += slots[i].size;
				return sz;
			}
		} stack;

		struct {
			func* parent;
			vector<vmr> a;
			vector<vmr> gp;
			vector<vmr> fp;
			vector<vmr> used;

			vmr allocate_arg() {
				if (parent->auto_free_consumed_vars) parent->free_consumed_vars();
				if (a.size() == 0) return vmr::register_count;
				vmr r = a.front();
				a.erase(a.begin());
				used.push_back(r);
				return r;
			}

			vmr allocate_gp() {
				if (parent->auto_free_consumed_vars) parent->free_consumed_vars();
				if (gp.size() == 0) return vmr::register_count;
				vmr r = gp.back();
				gp.pop_back();
				used.push_back(r);
				return r;
			}

			vmr allocate_fp() {
				if (parent->auto_free_consumed_vars) parent->free_consumed_vars();
				if (fp.size() == 0) return vmr::register_count;
				vmr r = fp.back();
				fp.pop_back();
				used.push_back(r);
				return r;
			}

			void free(vmr reg) {
				for (u8 i = 0;i < used.size();i++) {
					if (used[i] == reg) {
						used.erase(used.begin() + i);
						break;
					}
				}
				if (reg >= vmr::s0 && reg <= vmr::s7) gp.push_back(reg);
				else if (reg >= vmr::a0 && reg <= vmr::a7) a.push_back(reg);
				else fp.push_back(reg);
			}
		} registers;

		var* allocate(compile_context& ctx, ast_node* decl, bool is_arg = false);

		var* allocate(compile_context& ctx, data_type* type);

		var* imm(compile_context& ctx, integer i);

		var* imm(compile_context& ctx, decimal d);

		var* imm(compile_context& ctx, char* s);

		var* zero(compile_context& ctx, data_type* type);

		void free(var* v) {
			if (v->is_reg) registers.free(v->loc.reg);
			else stack.free(v->loc.stack_addr);

			for (u8 i = 0;i < scopes.size();i++) {
				for (auto s = scopes[i].vars.begin();s != scopes[i].vars.end();++s) {
					if (s->second == v) {
						scopes[i].vars.erase(s);
						delete v;
						return;
					}
				}
			}

			// exception
		}

		void free_consumed_vars() {
			for (u8 i = 0;i < scopes.size();i++) {
				for (auto s = scopes[i].vars.begin();s != scopes[i].vars.end();++s) {
					var* v = s->getSecond();
					if (!v->is_variable || v->no_auto_free) continue;
					if (v->ref_count == v->total_ref_count) free(v);
				}
			}
		}

		var* get(ast_node* identifier) {
			for (u8 i = 0;i < scopes.size();i++) {
				if (scopes[i].vars.count(*identifier) == 0) continue;
				var* v = scopes[i].vars[*identifier];
				v->ref_count++;
				return v;
			}

			// exception
			return nullptr;
		}

		void push_scope() {
			scopes.push_back(scope());
		}

		void pop_scope() {
			scope& s = scopes.back();
			vector<var*> vars;
			for (auto i = s.vars.begin();i != s.vars.end();++i) {
				vars.push_back(i->getSecond());
			}
			for (u32 i = 0;i < vars.size();i++) free(vars[i]);
			scopes.pop_back();
		}

		bool in_use(vmr reg) {
			for (u8 i = 0;i < registers.used.size();i++) {
				if (registers.used[i] == reg) return true;
			}
			return false;
		}
	};

	using var = func::var;

	struct data_type {
		data_type(const string& _name, bool _built_in = false) : name(_name), built_in(_built_in), is_floating_point(false), is_unsigned(false) { }
		data_type(compile_context& ctx, ast_node* node);

		bool equals(data_type* rhs) {
			return name == rhs->name;
		}

		// <type>::operator <to type>()
		func* cast_to_func(data_type* to) {
			return nullptr;
		}

		// <type>::operator=(<from type> rhs)
		func* set_from_func(data_type* from) {
			return nullptr;
		}

		string name;
		bool built_in;
		bool is_floating_point;
		bool is_unsigned;
		struct property {
			u8 flags;
			string name;
			data_type* type;
			u32 offset;
			func* getter;
			func* setter;
		};

		property* prop(const string& _name) {
			for (u32 p = 0;p < props.size();p++) {
				if (props[p].name == _name) return &props[p];
			}
			return nullptr;
		}

		func* method(const string& _name) {
			for (u32 m = 0;m < methods.size();m++) {
				if (methods[m]->name == name + "::" + _name) return methods[m];
			}
			return nullptr;
		}

		u32 size;
		vector<property> props;
		vector<func*> methods;
		vm_type* type;
	};

	struct compile_context {
		vm_context* ctx;
		compile_log log;
		instruction_array* out;
		vector<data_type*> types;
		vector<func*> funcs;
		func* cur_func;
		source_map* map;

		// used when compiling expressions involving type properties
		bool last_member_was_pointer;

		// used when compiling expressions involving type methods
		func* last_type_method;

		void add(instruction i, ast_node* because) {
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
			printf("%s\n\n", instruction_to_string(i).c_str());
			*/

			map->append(because);
		}

		data_type* type(const string& name) {
			for (u16 i = 0;i < types.size();i++) {
				if (types[i]->name == name) return types[i];
			}
			return nullptr;
		}

		data_type* type(ast_node* node) {
			return type(*node);
		}

		func* function(const string& name) {
			for (u16 i = 0;i < funcs.size();i++) {
				if (funcs[i]->name == name) return funcs[i];
			}
			return nullptr;
		}
	};

	func::func(compile_context& ctx, vm_function* f) {
		name = f->name;
		return_type = ctx.type(f->signature.return_type->name);
		for (u8 a = 0;a < f->signature.arg_types.size();a++) {
			vmr loc = f->is_host ? f->signature.arg_locs[a] : vmr::register_count;
			args.push_back({ "", ctx.type(f->signature.arg_types[a]->name), loc });
		}
		entry = f->is_host ? f->access.wrapped->address : f->access.entry;
		return_reg = f->signature.return_loc;
	}

	var* func::allocate(compile_context& ctx, ast_node* decl, bool is_arg) {
		if (auto_free_consumed_vars) free_consumed_vars();

		// todo: figure out what to do for big args
		u32 count = 1;
		string name = *decl->identifier;
		data_type* type = ctx.type(decl->data_type);
		if (type->size > 8 || count > 1) {
			scopes[scopes.size() - 1].vars[name] = new var();
			var& l = *scopes[scopes.size() - 1].vars[name];
			l.is_variable = true;
			l.name = name;
			l.ctx = &ctx;
			l.type = type;
			l.count = count;
			l.is_reg = false;
			l.is_imm = false;
			l.ref_count = 1; // declaration counts as a ref
			l.loc.stack_addr = stack.allocate(type->size * count);
			return &l;
		}

		vmr reg;
		if (type->is_floating_point) reg = registers.allocate_fp();
		else {
			if (!is_arg) reg = registers.allocate_gp();
			else reg = registers.allocate_arg();
		}

		scopes[scopes.size() - 1].vars[name] = new var();
		var& l = *scopes[scopes.size() - 1].vars[name];
		l.is_variable = true;
		l.name = name;
		l.ctx = &ctx;
		l.type = type;
		l.count = count;
		l.is_imm = false;
		l.ref_count = 1; // declaration counts as a ref
		if (reg != vmr::register_count) {
			l.is_reg = true;
			l.loc.reg = reg;
		} else {
			l.is_reg = false;
			l.loc.stack_addr = stack.allocate(type->size);
		}
		return &l;
	}

	static u32 anon_var_id = 0;
	func::var* func::allocate(compile_context& ctx, data_type* type) {
		if (auto_free_consumed_vars) free_consumed_vars();

		u32 count = 1;
		string name = format("__anon_%d", anon_var_id++);
		if (type->size > 8 || count > 1) {
			scopes[scopes.size() - 1].vars[name] = new var();
			var& l = *scopes[scopes.size() - 1].vars[name];
			l.is_variable = false;
			l.ctx = &ctx;
			l.type = type;
			l.count = count;
			l.is_reg = false;
			l.is_imm = false;
			l.loc.stack_addr = stack.allocate(type->size * count);
			return &l;
		}

		vmr reg;
		if (type->is_floating_point) reg = registers.allocate_fp();
		else reg = registers.allocate_gp();

		scopes[scopes.size() - 1].vars[name] = new var();
		var& l = *scopes[scopes.size() - 1].vars[name];
		l.is_variable = false;
		l.ctx = &ctx;
		l.type = type;
		l.count = count;
		l.is_imm = false;
		if (reg != vmr::register_count) {
			l.is_reg = true;
			l.loc.reg = reg;
		} else {
			l.is_reg = false;
			l.loc.stack_addr = stack.allocate(type->size);
		}
		return &l;
	}

	var* func::imm(compile_context& ctx, integer i) {
		string name = format("__anon_%d", anon_var_id++);
		var* l = scopes[scopes.size() - 1].vars[name] = new var();
		l->is_variable = false;
		l->ctx = &ctx;
		l->type = ctx.type("i32");
		l->count = 1;
		l->is_reg = false;
		l->is_imm = true;
		l->imm.i = i;
		return l;
	}

	var* func::imm(compile_context& ctx, decimal d) {
		string name = format("__anon_%d", anon_var_id++);
		var* l = scopes[scopes.size() - 1].vars[name] = new var();
		l->is_variable = false;
		l->ctx = &ctx;
		l->type = ctx.type("f32");
		l->count = 1;
		l->is_reg = false;
		l->is_imm = true;
		l->imm.d = d;
		return l;
	}

	var* func::imm(compile_context& ctx, char* s) {
		/*
		string name = format("__anon_%d", anon_var_id++);
		scopes[scopes.size() - 1].vars[name] = var();
		var& l = scopes[scopes.size() - 1].vars[name];
		l.is_variable = false;
		l.ctx = &ctx;
		l.type = ctx.type("i32");
		l.count = 1;
		l.is_reg = false;
		l.is_imm = true;
		l.imm.d = d;
		return &l;
		*/
		// todo: move string literals to vm memory and store address in value as an immediate
		return nullptr;
	}

	var* func::zero(compile_context& ctx, data_type* type) {
		string name = format("__anon_%d", anon_var_id++);
		var* l = scopes[scopes.size() - 1].vars[name] = new var();
		l->is_variable = false;
		l->ctx = &ctx;
		l->type = ctx.type("i32");
		l->count = 1;
		l->is_reg = false;
		l->is_imm = true;
		l->loc.reg = vmr::zero;
		return l;
	}

	address var::move_to_stack(ast_node* because, integer offset) {
		if (!is_reg && !is_imm) return loc.stack_addr;
		vmi store;
		switch (type->size) {
			case 1: { store = vmi::st8; break; }
			case 2: { store = vmi::st16; break; }
			case 4: { store = vmi::st32; break; }
			case 8: { store = vmi::st64; break; }
		}
		address addr = ctx->cur_func->stack.allocate(type->size);
		ctx->add(
			encode(store).operand(loc.reg).operand(vmr::sp).operand(uinteger(addr) + offset),
			because
		);

		ctx->cur_func->registers.free(loc.reg);
		is_reg = false;
		loc.stack_addr = addr;

		return addr;
	}

	address var::move_to_stack(ast_node* because, vmr offset) {
		if (!is_reg && !is_imm) return loc.stack_addr;
		vmi store;
		switch (type->size) {
			case 1: { store = vmi::st8; break; }
			case 2: { store = vmi::st16; break; }
			case 4: { store = vmi::st32; break; }
			case 8: { store = vmi::st64; break; }
		}
		address addr = ctx->cur_func->stack.allocate(type->size);
		ctx->add(
			encode(vmi::addui).operand(vmr::v0).operand(offset).operand((uinteger)addr),
			because
		);

		ctx->add(
			encode(store).operand(loc.reg).operand(vmr::v0).operand(0),
			because
		);

		ctx->cur_func->registers.free(loc.reg);
		is_reg = false;
		loc.stack_addr = addr;

		return addr;
	}

	vmr var::move_to_register(ast_node* because, integer offset) {
		vmr reg;
		if (type->is_floating_point) reg = ctx->cur_func->registers.allocate_fp();
		else reg = ctx->cur_func->registers.allocate_gp();
		ctx->cur_func->registers.free(reg);

		move_to(reg, because, offset);

		return reg;
	}

	vmr var::move_to_register(ast_node* because, vmr offset) {
		vmr reg;
		if (type->is_floating_point) reg = ctx->cur_func->registers.allocate_fp();
		else reg = ctx->cur_func->registers.allocate_gp();
		ctx->cur_func->registers.free(reg);

		move_to(reg, because, offset);

		return reg;
	}

	vmr var::to_reg(ast_node* because, integer offset) {
		if (is_reg) return loc.reg;
		return move_to_register(because, offset);
	}

	vmr var::to_reg(ast_node* because, vmr offset) {
		if (is_reg) return loc.reg;
		return move_to_register(because, offset);
	}

	address func::var::to_stack(ast_node* because, integer offset) {
		if (!is_reg) return loc.stack_addr;
		return move_to_stack(because, offset);
	}

	address func::var::to_stack(ast_node* because, vmr offset) {
		if (!is_reg) return loc.stack_addr;
		return move_to_stack(because, offset);
	}

	void var::move_to(vmr reg, ast_node* because, integer stack_offset) {
		bool found = false;
		if (is_fp(reg)) {
			for (u32 i = 0;i < ctx->cur_func->registers.fp.size();i++) {
				if (ctx->cur_func->registers.fp[i] == reg) {
					found = true;
					ctx->cur_func->registers.fp.erase(ctx->cur_func->registers.fp.begin() + i);
					break;
				}
			}
		} else if (is_arg(reg)) {
			for (u32 i = 0;i < ctx->cur_func->registers.a.size();i++) {
				if (ctx->cur_func->registers.a[i] == reg) {
					found = true;
					ctx->cur_func->registers.a.erase(ctx->cur_func->registers.a.begin() + i);
					break;
				}
			}
		} else {
			for (u32 i = 0;i < ctx->cur_func->registers.gp.size();i++) {
				if (ctx->cur_func->registers.gp[i] == reg) {
					found = true;
					ctx->cur_func->registers.gp.erase(ctx->cur_func->registers.gp.begin() + i);
					break;
				}
			}
		}

		if (!found) {
			throw compile_exception("Register not available for var to be moved to", because);
		}

		ctx->cur_func->registers.used.push_back(reg);
		if (is_reg) {
			ctx->add(
				encode(vmi::add).operand(reg).operand(loc.reg).operand(vmr::zero),
				because
			);
			ctx->cur_func->registers.free(loc.reg);
		} else if (is_imm) {
			if (!type->is_floating_point) {
				ctx->add(
					encode(vmi::addi).operand(reg).operand(vmr::zero).operand(imm.i),
					because
				);
			} else {
				ctx->add(
					encode(vmi::faddi).operand(reg).operand(vmr::zero).operand(imm.d),
					because
				);
			}
		} else {
			vmi load;
			switch (type->size) {
				case 1: { load = vmi::ld8; break; }
				case 2: { load = vmi::ld16; break; }
				case 4: { load = vmi::ld32; break; }
				case 8: { load = vmi::ld64; break; }
			}
			ctx->add(
				encode(load).operand(reg).operand(vmr::sp).operand((integer)loc.stack_addr + stack_offset),
				because
			);
			ctx->cur_func->stack.free(loc.stack_addr);
		}

		loc.reg = reg;
		is_reg = true;
		is_imm = false;
	}

	void var::move_to(vmr reg, ast_node* because, vmr stack_offset) {
		bool found = false;
		if (is_fp(reg)) {
			for (u32 i = 0;i < ctx->cur_func->registers.fp.size();i++) {
				if (ctx->cur_func->registers.fp[i] == reg) {
					found = true;
					ctx->cur_func->registers.fp.erase(ctx->cur_func->registers.fp.begin() + i);
					break;
				}
			}
		} else if (is_arg(reg)) {
			for (u32 i = 0;i < ctx->cur_func->registers.a.size();i++) {
				if (ctx->cur_func->registers.a[i] == reg) {
					found = true;
					ctx->cur_func->registers.a.erase(ctx->cur_func->registers.a.begin() + i);
					break;
				}
			}
		} else {
			for (u32 i = 0;i < ctx->cur_func->registers.gp.size();i++) {
				if (ctx->cur_func->registers.gp[i] == reg) {
					found = true;
					ctx->cur_func->registers.gp.erase(ctx->cur_func->registers.gp.begin() + i);
					break;
				}
			}
		}

		if (!found) {
			throw compile_exception("Register not available for var to be moved to", because);
		}

		ctx->cur_func->registers.used.push_back(reg);
		if (is_reg) {
			ctx->add(
				encode(vmi::add).operand(reg).operand(loc.reg).operand(vmr::zero),
				because
			);
			ctx->cur_func->registers.free(loc.reg);
		} else if (is_imm) {
			if (!type->is_floating_point) {
				ctx->add(
					encode(vmi::addi).operand(reg).operand(vmr::zero).operand(imm.i),
					because
				);
			} else {
				ctx->add(
					encode(vmi::faddi).operand(reg).operand(vmr::zero).operand(imm.d),
					because
				);
			}
		} else {
			vmi load;
			switch (type->size) {
				case 1: { load = vmi::ld8; break; }
				case 2: { load = vmi::ld16; break; }
				case 4: { load = vmi::ld32; break; }
				case 8: { load = vmi::ld64; break; }
			}
			ctx->add(
				encode(vmi::addu).operand(vmr::v0).operand(vmr::sp).operand(stack_offset),
				because
			);

			ctx->add(
				encode(load).operand(reg).operand(vmr::v0).operand((integer)loc.stack_addr),
				because
			);
			ctx->cur_func->stack.free(loc.stack_addr);
		}

		loc.reg = reg;
		is_reg = true;
		is_imm = false;
	}

	void var::store_in(vmr reg, ast_node* because, integer stack_offset) {
		if (is_reg) {
			ctx->add(
				encode(vmi::add).operand(reg).operand(loc.reg).operand(vmr::zero),
				because
			);
		} else if (is_imm) {
			if (!type->is_floating_point) {
				ctx->add(
					encode(vmi::addi).operand(reg).operand(vmr::zero).operand(imm.i),
					because
				);
			} else {
				ctx->add(
					encode(vmi::faddi).operand(reg).operand(vmr::zero).operand(imm.d),
					because
				);
			}
		} else {
			vmi load;
			switch (type->size) {
				case 1: { load = vmi::ld8; break; }
				case 2: { load = vmi::ld16; break; }
				case 4: { load = vmi::ld32; break; }
				case 8: { load = vmi::ld64; break; }
			}
			ctx->add(
				encode(load).operand(reg).operand(vmr::sp).operand((integer)loc.stack_addr + stack_offset),
				because
			);
		}
	}

	void var::store_in(vmr reg, ast_node* because, vmr stack_offset) {
		if (is_reg) {
			ctx->add(
				encode(vmi::add).operand(reg).operand(loc.reg).operand(vmr::zero),
				because
			);
		} else if (is_imm) {
			if (!type->is_floating_point) {
				ctx->add(
					encode(vmi::addi).operand(reg).operand(vmr::zero).operand(imm.i),
					because
				);
			} else {
				ctx->add(
					encode(vmi::faddi).operand(reg).operand(vmr::zero).operand(imm.d),
					because
				);
			}
		} else {
			vmi load;
			switch (type->size) {
				case 1: { load = vmi::ld8; break; }
				case 2: { load = vmi::ld16; break; }
				case 4: { load = vmi::ld32; break; }
				case 8: { load = vmi::ld64; break; }
			}
			ctx->add(
				encode(vmi::add).operand(vmr::v0).operand(vmr::sp).operand(stack_offset),
				because
			);

			ctx->add(
				encode(load).operand(reg).operand(vmr::v0).operand((integer)loc.stack_addr),
				because
			);
		}
	}

	var* call(compile_context& ctx, func* to, ast_node* because, const vector<var*>& args) {
		// move arguments to the right place, or a temporary place until vars have been
		// backed up in the stack
		vector<pair<vmr, vmr>> arg_pairs;
		for (u8 a = 0;a < args.size();a++) {
			vmr dest = to->args[a].loc;
			if (args[a]->is_reg && dest == args[a]->loc.reg) continue; // arg is already in the right place

			if (!ctx.cur_func->in_use(dest)) args[a]->store_in(dest, because);
			else {
				// current register of arg[a] shouldn't be rewritten between now and the call
				//vmr tmp;
				//if (args[a]->type->is_floating_point) tmp = ctx.cur_func->registers.allocate_fp();
				//else tmp = ctx.cur_func->registers.allocate_gp();
				//args[a]->store_in(tmp, because);
				//arg_pairs.push_back(pair<vmr, vmr>(dest, tmp));
				arg_pairs.push_back(pair<vmr, vmr>(dest, args[a]->to_reg(because)));
			}
		}

		// free any used up variables that don't need to be
		// stored in the stack, if not in a loop
		if (ctx.cur_func->auto_free_consumed_vars) ctx.cur_func->free_consumed_vars();

		// keep track of where vars should be restored to, if they refer to registers
		vector <pair<var*, vmr>> restore_pairs;
		for (u32 s = 0;s < ctx.cur_func->scopes.size();s++) {
			for (auto i = ctx.cur_func->scopes[s].vars.begin();i != ctx.cur_func->scopes[s].vars.end();++i) {
				var* v = i->getSecond();
				if (v->is_reg) {
					restore_pairs.push_back(pair<var*, vmr>(v, v->loc.reg));
				}
			}
		}

		// allocate var for return value, if necessary
		var* ret = nullptr;
		vmr ret_reg;
		if (to->return_type->name != "void") {
			ret = ctx.cur_func->allocate(ctx, to->return_type);
			if (to->return_reg == vmr::register_count) {
				// function must either be calling itself (and compiler hasn't seen a return statement yet)
				// or script writer didn't put a return statement and an error was emitted. Assign register
				// automatically
				if (to->return_type->is_floating_point) to->return_reg = to->registers.allocate_fp();
				else to->return_reg = to->registers.allocate_gp();

				// then free it since it's not actually in use until the statement is reached
				to->registers.free(to->return_reg);
			}
			ret_reg = ret->to_reg(because);
		}

		// back up register vars to the stack
		for (u8 i = 0;i < restore_pairs.size();i++) {
			restore_pairs[i].first->to_stack(because);
		}

		// move remaining args that couldn't be moved before
		for (u8 i = 0;i < arg_pairs.size();i++) {
			vmi assign = is_fp(arg_pairs[i].first) ? vmi::fadd : vmi::add;
			ctx.add(
				encode(assign).operand(arg_pairs[i].first).operand(arg_pairs[i].second).operand(vmr::zero),
				because
			);
			ctx.cur_func->registers.free(arg_pairs[i].second);
		}

		// backup $ra
		address ra_addr = ctx.cur_func->stack.allocate(4);
		ctx.add(
			encode(vmi::st32).operand(vmr::ra).operand(vmr::sp).operand((integer)ra_addr),
			because
		);

		// move stack pointer
		if (ctx.cur_func->stack.size() > 0) {
			ctx.add(
				encode(vmi::addi).operand(vmr::sp).operand(vmr::sp).operand((integer)ctx.cur_func->stack.size()),
				because
			);
		}

		// make the call
		ctx.add(
			encode(vmi::jal).operand(to->entry),
			because
		);

		// move stack pointer back
		if (ctx.cur_func->stack.size() > 0) {
			ctx.add(
				encode(vmi::subi).operand(vmr::sp).operand(vmr::sp).operand((integer)ctx.cur_func->stack.size()),
				because
			);
		}

		// store return value if necessary
		if (to->return_type->name != "void") {
			if (ret_reg != to->return_reg) {
				ctx.add(
					encode(vmi::add).operand(ret_reg).operand(to->return_reg).operand(vmr::zero),
					because
				);
			}
		}

		// restore $ra
		ctx.cur_func->stack.free(ra_addr);
		ctx.add(
			encode(vmi::ld32).operand(vmr::ra).operand(vmr::sp).operand((integer)ra_addr),
			because
		);

		// restore register vars
		for (u8 i = 0;i < restore_pairs.size();i++) {
			restore_pairs[i].first->move_to(restore_pairs[i].second, because);
		}
		
		return ret;
	}



	data_type::data_type(compile_context& ctx, ast_node* node) {
		name = *node->identifier;
		ast_node* n = node->body;
		size_t offset = 0;
		while (n) {
			if (n->type == nt::variable_declaration) {
				// class
				data_type* t = ctx.type(*node->data_type);
				props.push_back({
					bind::pf_none,
					*node->identifier,
					t,
					u32(offset),
					nullptr,
					nullptr
				});
				offset += t->size;
			} else if (n->type == nt::format_property) {
				// format
				data_type* t = ctx.type(*node->data_type);
				props.push_back({
					bind::pf_none,
					*node->identifier,
					t,
					u32(offset),
					nullptr,
					nullptr
				});
				offset += t->size;
			} else if (n->type == nt::function_declaration) {
				ctx.funcs.push_back(new func());
				compile_function(ctx, n, ctx.funcs[ctx.funcs.size() - 1]);
				methods.push_back(ctx.funcs[ctx.funcs.size() - 1]);
				func* m = methods[methods.size() - 1];
				m->name = name + "::" + m->name;
			}
			n = n->next;
		}
	}

	var* cast(compile_context& ctx, var* from, var* to, ast_node* because) {
		if (from->type->equals(to->type)) return from;
		if (from->type->name == "bool" || to->type->name == "bool") return from;

		if (!from->type->built_in || (from->type->name == "string" || to->type->name == "string")) {
			func* cast_func = from->type->cast_to_func(to->type);
			if (!cast_func) {
				ctx.log.err(format("No conversion from '%s' to '%s' found", from->type->name.c_str(), to->type->name.c_str()), because);
				return from;
			}

			return call(ctx, cast_func, because, { from });
		}

		if (from->type->name == "void") {
			ctx.log.err(format("Cannot convert from 'void' to '%s'", to->type->name.c_str()), because);
			return from;
		}
		if (to->type->name == "void") {
			ctx.log.err(format("Cannot convert from '%s' to 'void'", to->type->name.c_str()), because);
			return from;
		}

		// only types remaining are integral

		if (!from->type->is_floating_point) {
			// to must be floating point
			if (from->is_imm) return ctx.cur_func->imm(ctx, (decimal)from->imm.i);
			var* tmp = ctx.cur_func->allocate(ctx, to->type);
			ctx.add(
				encode(vmi::mtfp).operand(from->to_reg(because)).operand(tmp->to_reg(because)),
				because
			);
			ctx.add(
				encode(vmi::ctf).operand(tmp->to_reg(because)),
				because
			);
			return tmp;
		} else {
			if (from->is_imm) return ctx.cur_func->imm(ctx, (integer)from->imm.d);
			var* tmp = ctx.cur_func->allocate(ctx, to->type);
			vmr treg = ctx.cur_func->registers.allocate_fp();
			ctx.add(
				encode(vmi::fadd).operand(treg).operand(from->to_reg(because)).operand(vmr::zero),
				because
			);
			ctx.add(
				encode(vmi::cti).operand(treg),
				because
			);
			ctx.add(
				encode(vmi::mffp).operand(treg).operand(tmp->to_reg(because)),
				because
			);
			ctx.cur_func->registers.free(treg);
			return tmp;
		}

		ctx.log.err(format("No conversion from '%s' to '%s' found", from->type->name.c_str(), to->type->name.c_str()), because);
		return from;
	}

	void arithmetic_op_maybe_fp(compile_context& ctx, var* l, var* r, var* o, ast_node* because, op operation) {
		// vmi::null -> swap lvalue and rvalue and use f_reg/f_imm or reg/imm or u_reg/imm
		// vmi(-1) -> unsupported
		static vmi possible_arr[][9] = {
			// f_reg/f_imm, f_imm/f_reg, f_reg/f_reg, reg/imm   , imm/reg   , reg/reg  , u_reg/imm , imm/u_reg  , u_reg/u_reg
			{ vmi::faddi  , vmi::null  , vmi::fadd  , vmi::addi , vmi::null , vmi::add , vmi::addui, vmi::null  , vmi::addu }, // add
			{ vmi::fsubi  , vmi::fsubir, vmi::fsub  , vmi::subi , vmi::subir, vmi::sub , vmi::subui, vmi::subuir, vmi::subu }, // sub
			{ vmi::fmuli  , vmi::null  , vmi::fmul  , vmi::muli , vmi::null , vmi::mul , vmi::mului, vmi::null  , vmi::mulu }, // mul
			{ vmi::fdivi  , vmi::fdivir, vmi::fdiv  , vmi::divi , vmi::divir, vmi::div , vmi::divui, vmi::divuir, vmi::divu }, // div
			{ vmi::flti   , vmi(-1)	   , vmi::flt   , vmi::lti  , vmi(-1)   , vmi::lt  , vmi::lti  , vmi(-1)    , vmi::lt   }, // less
			{ vmi::fgti   , vmi(-1)	   , vmi::fgt   , vmi::gti  , vmi(-1)   , vmi::gt  , vmi::gti  , vmi(-1)    , vmi::gt   }, // greater
			{ vmi::fltei  , vmi(-1)	   , vmi::flte  , vmi::ltei , vmi(-1)   , vmi::lte , vmi::ltei , vmi(-1)    , vmi::lte  }, // lessEq
			{ vmi::fgtei  , vmi(-1)	   , vmi::fgte  , vmi::gtei , vmi(-1)   , vmi::gte , vmi::gtei , vmi(-1)    , vmi::gte  }, // greaterEq
			{ vmi::fncmpi , vmi::null  , vmi::fncmp , vmi::ncmpi, vmi::null , vmi::ncmp, vmi::ncmpi, vmi::null  , vmi::ncmp }, // notEq
			{ vmi::fcmpi  , vmi::null  , vmi::fcmp  , vmi::cmpi , vmi::null , vmi::cmp , vmi::cmpi , vmi::null  , vmi::cmp  }, // isEq1
		};
		static unordered_map<op, u32> possible_map = {
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
			{ op::isEq      , 9 }
		};
		vmi* possible = possible_arr[possible_map[operation]];
		
		var* rhs = l ? cast(ctx, r, l, because) : r;

		vmr lvr = l ? l->to_reg(because) : vmr::zero;
		if (!rhs->is_imm) rhs->to_reg(because);

		bool op_is_fp = (l && is_fp(lvr)) || is_fp(o->to_reg(because));
		bool lv_is_im = l && l->is_imm;
		bool rv_is_im = rhs && rhs->is_imm;
		bool op_is_us = (l && l->type->type && l->type->type->is_unsigned) || (o->type->type && o->type->type->is_unsigned);

		if (lv_is_im && rv_is_im) {
			// just do the math here and set output to an imm
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

		bool already_errored = false;
		if (i == vmi(-1)) {
			ctx.log.err("Unsupported operation", because);
			already_errored = true;
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

		if (i == vmi(-1)) {
			if (!already_errored) ctx.log.err("Unsupported operation", because);
		}

		instruction_encoder inst = encode(i).operand(o->to_reg(because));

		if (swap) {
			inst.operand(rhs->to_reg(because));
			if (lv_is_im) {
				if (op_is_fp) inst.operand(l->imm.d);
				else inst.operand(l->imm.i);
			} else inst.operand(lvr);
		} else {
			inst.operand(lvr);
			if (rv_is_im) {
				if (op_is_fp) inst.operand(rhs->imm.d);
				else inst.operand(rhs->imm.i);
			} else inst.operand(rhs->to_reg(because));
		}

		ctx.add(inst, because);

		if (rhs != r) ctx.cur_func->free(rhs);
	}

	void arithmetic_op(compile_context& ctx, var* l, var* r, var* o, ast_node* because, op operation) {
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
		static unordered_map<op, u32> possible_map = {
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

		var* rhs = l ? cast(ctx, r, l, because) : r;

		vmr lvr = l ? l->to_reg(because) : vmr::zero;
		if (!rhs->is_imm) rhs->to_reg(because);

		bool op_is_fp = (l && is_fp(lvr)) || is_fp(o->to_reg(because));
		bool lv_is_im = l && l->is_imm;
		bool rv_is_im = rhs && rhs->is_imm;
		bool op_is_us = (l && l->type->type && l->type->type->is_unsigned) || (o->type->type && o->type->type->is_unsigned);

		if (lv_is_im && rv_is_im) {
			// just do the math here and set output to an imm
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

		instruction_encoder inst = encode(i).operand(o->to_reg(because));

		if (swap) {
			inst.operand(rhs->to_reg(because));
			if (lv_is_im) {
				if (op_is_fp) inst.operand(l->imm.d);
				else inst.operand(l->imm.i);
			} else inst.operand(lvr);
		} else {
			inst.operand(lvr);
			if (rv_is_im) {
				if (op_is_fp) inst.operand(rhs->imm.d);
				else inst.operand(rhs->imm.i);
			} else inst.operand(rhs->to_reg(because));
		}

		ctx.add(inst, because);

		if (rhs != r) ctx.cur_func->free(rhs);
	}


	u16 count_references(ast_node* node, const string& identifier, bool ignoreNext = false) {
		if (!node) return 0;

		u16 refs = 0;
		if (node->type == nt::identifier) return (string(*node) == identifier) ? 1 : 0;

		if (!ignoreNext) refs += count_references(node->next, identifier);
		refs += count_references(node->data_type, identifier);
		refs += count_references(node->identifier, identifier);
		refs += count_references(node->arguments, identifier);
		refs += count_references(node->body, identifier);
		refs += count_references(node->else_body, identifier);
		refs += count_references(node->property, identifier);
		refs += count_references(node->initializer, identifier);
		refs += count_references(node->lvalue, identifier);
		refs += count_references(node->rvalue, identifier);
		refs += count_references(node->callee, identifier);
		refs += count_references(node->constructor, identifier);
		refs += count_references(node->destructor, identifier);
		refs += count_references(node->condition, identifier);
		refs += count_references(node->modifier, identifier);

		return refs;
	}

	void compile_function(compile_context& ctx, ast_node* node, func* out) {
		out->name = *node->identifier;
		out->return_type = ctx.type(*node->data_type);
		out->entry = ctx.out->size();
		out->root = node;

		ctx.cur_func = out;

		ctx.add(encode(vmi::null), node);

		ast_node* arg = node->arguments ? node->arguments->body : nullptr;
		while (arg) {
			func::var* var = ctx.cur_func->allocate(ctx, arg, true);
			var->total_ref_count = count_references(node, var->name, true);
			out->args.push_back({ *arg->identifier, ctx.type(arg->data_type), var->to_reg(arg) });
			arg = arg->next;
		}

		ast_node* n = node->body;
		while (n) {
			compile_node(ctx, n);
			n = n->next;
		}

		ctx.cur_func = nullptr;
	}

	var* compile_expression(compile_context& ctx, ast_node* node, var* dest) {
		if (node->type == nt::identifier) {
			if (!dest) return ctx.cur_func->get(node);
			ctx.cur_func->get(node)->store_in(dest->to_reg(node), node);
			return dest;
		} else if (node->type == nt::constant) {
			var* imm = nullptr;
			switch (node->c_type) {
				case ast_node::constant_type::integer: { imm = ctx.cur_func->imm(ctx, (integer)node->value.i); break; }
				case ast_node::constant_type::decimal: { imm = ctx.cur_func->imm(ctx, (decimal)node->value.d); break; }
				case ast_node::constant_type::string: { imm = ctx.cur_func->imm(ctx, node->value.s); break; }
			}
			if (dest) {
				imm->store_in(dest->to_reg(node), node);
				ctx.cur_func->free(imm);
				return dest;
			}
			return imm;
		}

		var* lvalue = nullptr;
		var* rvalue = nullptr;
		if (node->lvalue) {
			if (node->lvalue->type == nt::operation || node->lvalue->type == nt::call) lvalue = compile_expression(ctx, node->lvalue, nullptr);
			else if (node->lvalue->type == nt::identifier) lvalue = ctx.cur_func->get(node->lvalue);
			else if (node->lvalue->type == nt::constant) {
				switch (node->lvalue->c_type) {
					case ast_node::constant_type::integer: { lvalue = ctx.cur_func->imm(ctx, (integer)node->lvalue->value.i); break; }
					case ast_node::constant_type::decimal: { lvalue = ctx.cur_func->imm(ctx, (decimal)node->lvalue->value.d); break; }
					case ast_node::constant_type::string: { lvalue = ctx.cur_func->imm(ctx, node->lvalue->value.s); break; }
				}
			}
		}
		if (node->rvalue) {
			if (node->rvalue->type == nt::operation || node->rvalue->type == nt::call) rvalue = compile_expression(ctx, node->rvalue, nullptr);
			else if (node->rvalue->type == nt::identifier) {
				if (node->op != op::member) rvalue = ctx.cur_func->get(node->rvalue);
			}
			else if (node->rvalue->type == nt::constant) {
				switch (node->rvalue->c_type) {
					case ast_node::constant_type::integer: { rvalue = ctx.cur_func->imm(ctx, (integer)node->rvalue->value.i); break; }
					case ast_node::constant_type::decimal: { rvalue = ctx.cur_func->imm(ctx, (decimal)node->rvalue->value.d); break; }
					case ast_node::constant_type::string: { rvalue = ctx.cur_func->imm(ctx, node->rvalue->value.s); break; }
				}
			}
		}

		var* ret = nullptr;

		if (node->type == nt::call) {
			if (node->callee->type == nt::operation) {
				if (node->callee->op == op::member) {
					var* this_obj = compile_expression(ctx, node->callee, nullptr);
					this_obj->no_auto_free = true;
					if (ctx.last_type_method) {
						vector<var*> args = { this_obj };
						vector<bool> free_arg = { true };
						ast_node* arg = node->arguments;
						while (arg) {
							args.push_back(compile_expression(ctx, arg, nullptr));
							free_arg.push_back(!args[args.size() - 1]->is_variable);
							arg = arg->next;
						}
						ret = call(ctx, ctx.last_type_method, node, args);
						for (u32 i = 0;i < args.size();i++) {
							if (free_arg[i]) ctx.cur_func->free(args[i]);
						}
					}
				}
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
		} else {
			bool auto_free = ctx.cur_func->auto_free_consumed_vars;
			ctx.cur_func->auto_free_consumed_vars = false;
			bool assigned = false;
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
					var* result = dest ? dest : ctx.cur_func->allocate(ctx, rvalue->type);
					var* tmp;
					if (rvalue->type->is_floating_point) tmp = ctx.cur_func->imm(ctx, 1.0f);
					else tmp = ctx.cur_func->imm(ctx, 1);

					rvalue->store_in(result->to_reg(node), node);
					arithmetic_op_maybe_fp(ctx, rvalue, tmp, rvalue, node, node->op);

					ctx.cur_func->free(tmp);
					ret = result;
					assigned = true;
					break;
				}
				case op::preDec: {
					var* result = dest ? dest : ctx.cur_func->allocate(ctx, rvalue->type);
					var* tmp;
					if (rvalue->type->is_floating_point) tmp = ctx.cur_func->imm(ctx, 1.0f);
					else tmp = ctx.cur_func->imm(ctx, 1);

					rvalue->store_in(result->to_reg(node), node);
					arithmetic_op_maybe_fp(ctx, rvalue, tmp, rvalue, node, node->op);

					ctx.cur_func->free(tmp);
					ret = result;
					assigned = true;
					break;
				}
				case op::postInc: {
					var* tmp;
					if (lvalue->type->is_floating_point) tmp = ctx.cur_func->imm(ctx, 1.0f);
					else tmp = ctx.cur_func->imm(ctx, 1);

					arithmetic_op_maybe_fp(ctx, lvalue, tmp, lvalue, node, node->op);
					if (dest) lvalue->store_in(dest->to_reg(node), node);

					ctx.cur_func->free(tmp);
					ret = dest ? dest : lvalue;
					assigned = true;
					break;
				}
				case op::postDec: {
					var* tmp;
					if (lvalue->type->is_floating_point) tmp = ctx.cur_func->imm(ctx, 1.0f);
					else tmp = ctx.cur_func->imm(ctx, 1);

					arithmetic_op_maybe_fp(ctx, lvalue, tmp, lvalue, node, node->op);
					if (dest) lvalue->store_in(dest->to_reg(node), node);

					ctx.cur_func->free(tmp);
					ret = dest ? dest : lvalue;
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
					var* tmp = ctx.cur_func->imm(ctx, (integer)0);
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
					else tmp = ctx.cur_func->imm(ctx, -1);
					arithmetic_op_maybe_fp(ctx, rvalue, tmp, result, node, node->op);
					ctx.cur_func->free(tmp);
					break;
				}
				case op::addr: {
					var* result = dest ? dest : ctx.cur_func->allocate(ctx, lvalue->type);
					ret = result;
					break;
				}
				case op::at: {
					var* result = dest ? dest : ctx.cur_func->allocate(ctx, lvalue->type);
					ret = result;
					break;
				}
				case op::member: {
					data_type::property* prop = lvalue->type->prop(*node->rvalue);
					if (prop) {
						var* pval = ctx.cur_func->allocate(ctx, prop->type);
						// store pointer to property in v1 in case value should be stored later
						ctx.add(
							encode(vmi::addui).operand(vmr::v1).operand(lvalue->to_reg(node)).operand((integer)prop->offset),
							node
						);

						if (prop->flags ^ bind::pf_object_pointer) {
							ctx.last_member_was_pointer = false;
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
									encode(ld).operand(pval->to_reg(node)).operand(vmr::v1).operand(0),
									node
								);
							} else {
								// get pointer to property in pval
								ctx.add(
									encode(vmi::addui).operand(pval->to_reg(node)).operand(vmr::v1).operand(0),
									node
								);
							}

							ctx.last_type_method = nullptr;
						} else {
							ctx.last_member_was_pointer = true;
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
									encode(ld).operand(pval->to_reg(node)).operand(vmr::v1).operand(0),
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
									encode(ld).operand(pval->to_reg(node)).operand(pval->to_reg(node)).operand(0),
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
									encode(ld).operand(pval->to_reg(node)).operand(vmr::v1).operand(0),
									node
								);
							}
						}

						var* result = dest ? dest : pval;
						if (!result->type->equals(prop->type)) {
							// try to cast
						}
						ret = result;
					}
					else {
						func* method = lvalue->type->method(*node->rvalue);
						if (method) {
							ctx.last_type_method = method;
							ret = lvalue;
						} else {
							ctx.last_type_method = nullptr;
							string n = *node->rvalue;
							ctx.log.err(format("'%s' is not a property or method of type '%s'", n.c_str(), lvalue->type->name.c_str()), node);
						}
					}
					break;
				}
			}

			var* var_assigned = lvalue ? lvalue : rvalue;
			ast_node* node_assigned = node->lvalue ? node->lvalue : node->rvalue;
			if (assigned && node_assigned->op == op::member) {
				// v1 will hold a pointer to the member
				if (var_assigned->type->built_in) {
					if (ctx.last_member_was_pointer) {
						// load pointer stored at address in v1 (and put it in v1)
						vmi ld;
						switch (sizeof(void*)) {
							case 1: { ld = vmi::ld8; break; }
							case 2: { ld = vmi::ld16; break; }
							case 4: { ld = vmi::ld32; break; }
							case 8: { ld = vmi::ld64; break; }
						}
						ctx.add(
							encode(ld).operand(vmr::v1).operand(vmr::v1).operand(0),
							node
						);


						// store value at address in v1
						vmi st;
						switch(var_assigned->type->size) {
							case 1: { st = vmi::st8; break; }
							case 2: { st = vmi::st16; break; }
							case 4: { st = vmi::st32; break; }
							case 8: { st = vmi::st64; break; }
						}
						ctx.add(
							encode(st).operand(ret->to_reg(node)).operand(vmr::v1).operand(0),
							node
						);
					} else {
						// store value at address in v1
						vmi st;
						switch(var_assigned->type->size) {
							case 1: { st = vmi::st8; break; }
							case 2: { st = vmi::st16; break; }
							case 4: { st = vmi::st32; break; }
							case 8: { st = vmi::st64; break; }
						}
						ctx.add(
							encode(st).operand(ret->to_reg(node)).operand(vmr::v1).operand(0),
							node
						);
					}
				} else {
					// store pointer to value at address in v1
					vmi st;
					switch(sizeof(void*)) {
						case 1: { st = vmi::st8; break; }
						case 2: { st = vmi::st16; break; }
						case 4: { st = vmi::st32; break; }
						case 8: { st = vmi::st64; break; }
					}
					ctx.add(
						encode(st).operand(ret->to_reg(node)).operand(vmr::v1).operand(0),
						node
					);
				}
			}

			ctx.cur_func->auto_free_consumed_vars = auto_free;
		}

		if (lvalue && !lvalue->is_variable && lvalue != ret) ctx.cur_func->free(lvalue);
		if (rvalue && !rvalue->is_variable && rvalue != ret) ctx.cur_func->free(rvalue);

		return ret;
	}

	void compile_variable_declaration(compile_context& ctx, ast_node* node) {
		func::var* var = ctx.cur_func->allocate(ctx, node);
		var->total_ref_count = count_references(ctx.cur_func->root, var->name);

		if (node->initializer) compile_expression(ctx, node->initializer->body, var);
	}

	void compile_if_statement(compile_context& ctx, ast_node* node) {
		func::var* cond = compile_expression(ctx, node->condition, nullptr);
		address branchAddr = ctx.out->size();
		ctx.add(
			encode(vmi::bneqz).operand(cond->to_reg(node)).operand(0),
			node
		);
		ctx.cur_func->free(cond);
		ast_node* n = node->body;
		ctx.cur_func->push_scope();
		while (n) {
			compile_node(ctx, n);
			n = n->next;
		}

		// update branch fail address
		instruction branch = (*ctx.out)[branchAddr];
		ctx.out->set(
			branchAddr,
			encode(decode_instruction(branch)).operand(decode_operand_1(branch)).operand((integer)ctx.out->size())
		);

		n = node->else_body;
		while (n) {
			compile_node(ctx, n);
			n = n->next;
		}
		ctx.cur_func->pop_scope();
	}

	void compile_for_loop(compile_context& ctx, ast_node* node) {
		ctx.cur_func->push_scope();
		ctx.cur_func->auto_free_consumed_vars = false;
		
		if (node->initializer) compile_variable_declaration(ctx, node->initializer);
		address loop_cond = ctx.out->size();
		address branch_addr = 0;
		if (node->condition) {
			func::var* cond = compile_expression(ctx, node->condition, nullptr);
			branch_addr = ctx.out->size();
			ctx.add(
				encode(vmi::bneqz).operand(cond->to_reg(node->condition)).operand(0),
				node->condition
			);
			ctx.cur_func->free(cond);
		}

		if (node->modifier) {
			func::var* mod = compile_expression(ctx, node->modifier, nullptr);
			ctx.cur_func->free(mod);
		}

		ast_node* n = node->body;
		while (n) {
			compile_node(ctx, n);
			n = n->next;
		}

		ctx.add(
			encode(vmi::jmp).operand((u64)loop_cond),
			node
		);

		if (node->condition) {
			// update branch fail address
			instruction branch = (*ctx.out)[branch_addr];
			ctx.out->set(
				branch_addr,
				encode(decode_instruction(branch)).operand(decode_operand_1(branch)).operand((uinteger)ctx.out->size())
			);
		}

		ctx.cur_func->auto_free_consumed_vars = true;
		ctx.cur_func->pop_scope();
	}

	void compile_while_loop(compile_context& ctx, ast_node* node) {
		ctx.cur_func->auto_free_consumed_vars = false;
		address condAddr = ctx.out->size();
		func::var* cond = compile_expression(ctx, node->condition, nullptr);
		address branchAddr = ctx.out->size();
		ctx.add(
			encode(vmi::bneqz).operand(cond->to_reg(node)).operand(0),
			node
		);
		ctx.cur_func->free(cond);
		ast_node* n = node->body;
		ctx.cur_func->push_scope();
		while (n) {
			compile_node(ctx, n);
			n = n->next;
		}

		ctx.add(
			encode(vmi::jmp).operand((uinteger)condAddr),
			node
		);

		// update branch fail address
		instruction branch = (*ctx.out)[branchAddr];
		ctx.out->set(
			branchAddr,
			encode(decode_instruction(branch)).operand(decode_operand_1(branch)).operand((uinteger)ctx.out->size())
		);

		n = node->else_body;
		while (n) {
			compile_node(ctx, n);
			n = n->next;
		}
		ctx.cur_func->auto_free_consumed_vars = true;
		ctx.cur_func->pop_scope();
	}

	void compile_do_while_loop(compile_context& ctx, ast_node* node) {
		ctx.cur_func->auto_free_consumed_vars = false;
		address startAddr = ctx.out->size();
		ctx.cur_func->push_scope();
		ast_node* n = node->body;
		while (n) {
			compile_node(ctx, n);
			n = n->next;
		}

		func::var* cond = compile_expression(ctx, node->condition, nullptr);
		ctx.add(
			encode(vmi::bneqz).operand(cond->to_reg(node)).operand((uinteger)ctx.out->size() + 1),
			node
		);
		ctx.cur_func->free(cond);

		ctx.add(
			encode(vmi::jmp).operand((uinteger)startAddr),
			node->condition
		);

		ctx.cur_func->auto_free_consumed_vars = true;
		ctx.cur_func->pop_scope();
	}

	void compile_node(compile_context& ctx, ast_node* node) {
		switch(node->type) {
			case nt::empty: break;
			case nt::variable_declaration: { compile_variable_declaration(ctx, node); break; }
			case nt::function_declaration: {
				ctx.funcs.push_back(new func());
				compile_function(ctx, node, ctx.funcs[ctx.funcs.size() - 1]);
				break;
			}
			case nt::class_declaration: { ctx.types.push_back(new data_type(ctx, node)); break; }
			case nt::format_declaration: { ctx.types.push_back(new data_type(ctx, node)); break; }
			case nt::variable_initializer: break;
			case nt::format_property: break;
			case nt::function_arguments: break;
			case nt::if_statement: { compile_if_statement(ctx, node); break; }
			case nt::for_loop: { compile_for_loop(ctx, node); break; }
			case nt::while_loop: { compile_while_loop(ctx, node); break; }
			case nt::do_while_loop: { compile_while_loop(ctx, node); break; }
			case nt::import_statement: break;
			case nt::export_statement: break;
			case nt::return_statement: {
				if (node->body) {
					var* ret = compile_expression(ctx, node->body, nullptr);
					if (ret) {
						if (ctx.cur_func->return_reg == vmr::register_count) {
							ctx.cur_func->return_reg = ret->to_reg(node);
						} else {
							ret->store_in(ctx.cur_func->return_reg, node);
						}
						if (!ret->is_variable) ctx.cur_func->free(ret);
					}
				}

				ctx.add(
					encode(vmi::jmpr).operand(vmr::ra),
					node
				);
				break;
			}
			case nt::object: { break; }
			case nt::call: {
				var* tmp = compile_expression(ctx, node, nullptr);
				if (tmp) ctx.cur_func->free(tmp);
				break;
			}
			case nt::expression: {
				var* tmp = compile_expression(ctx, node, nullptr);
				if (tmp && !tmp->is_variable) ctx.cur_func->free(tmp);
				break;
			}
			case nt::conditional: { break; }
			case nt::constant: { break; }
			case nt::identifier: { break; }
			case nt::type_identifier: { break; }
			case nt::operation: {
				var* tmp = compile_expression(ctx, node, nullptr);
				if (tmp && !tmp->is_variable) ctx.cur_func->free(tmp);
				break;
			}
			case nt::context_function: { break; }
			case nt::context_type: { break; }
			default: {
				throw compile_exception("Invalid AST node type", node);
			}
		}
	}

	void init_context(compile_context& ctx) {
		data_type* t = nullptr;

		vector<vm_type*> types = ctx.ctx->types()->all();
		for (u32 i = 0;i < types.size();i++) {
			ctx.types.push_back(new data_type(types[i]->name));
			t = ctx.types[ctx.types.size() - 1];
			t->size = types[i]->size;
			t->type = types[i];
			t->built_in = types[i]->is_builtin;
			t->is_floating_point = types[i]->is_floating_point;
			t->is_unsigned = types[i]->is_unsigned;
			if (!t->built_in || t->name == "string") {
				// object pointer
				t->size = sizeof(void*);
			}
		}

		vector<vm_function*> funcs = ctx.ctx->all_functions();
		for (u32 i = 0;i < funcs.size();i++) {
			ctx.funcs.push_back(new func(ctx, funcs[i]));
		}

		// type methods and properties must come after functions are added
		for (u32 i = 0;i < types.size();i++) {
			data_type* t = ctx.type(types[i]->name);

			for (u16 p = 0;p < t->type->properties.size();p++) {
				vm_type::property& prop = t->type->properties[p];
				t->props.push_back({
					prop.flags,
					prop.name,
					ctx.type(prop.type->name),
					prop.offset,
					prop.getter ? ctx.function(prop.getter->name) : nullptr,
					prop.setter ? ctx.function(prop.setter->name) : nullptr
				});
			}

			for (u16 m = 0;m < t->type->methods.size();m++) {
				t->methods.push_back(ctx.function(t->type->methods[m]->name));
			}
		}
	}

	void compile_ast(vm_context* vctx, ast_node* tree, instruction_array* out, source_map* map) {
		if (!tree) {
			throw compile_exception("No AST to compile", "", "", 0, 0);
		}

		compile_context ctx;
		ctx.ctx = vctx;
		ctx.map = map;
		ctx.out = out;
		init_context(ctx);

		ast_node* n = tree->body;
		if (!n) throw compile_exception("AST has no body", tree);
		while (n) {
			compile_node(ctx, n);
			n = n->next;
		}

		vector<data_type*> new_types;
		for (u32 i = 0;i < ctx.types.size();i++) {
			data_type* t = ctx.types[i];
			if (t->type) continue; // already in context

			vm_type* tp = vctx->types()->add(t->name, t->name);
			t->type = tp;
			for (u32 p = 0;p < t->props.size();p++) {
				data_type::property& sprop = t->props[p];
				vm_type::property prop;
				prop.name = sprop.name;
				prop.offset = sprop.offset;
				prop.flags = sprop.flags;
				tp->properties.push_back(prop);
			}
			new_types.push_back(t);
		}

		for (u32 i = 0;i < ctx.funcs.size();i++) {
			func* sf = ctx.funcs[i];
			vm_function* f = new vm_function(vctx, sf->name, sf->entry);
			for (u8 a = 0;a < sf->args.size();a++) {
				f->signature.arg_types.push_back(sf->args[a].type->type);
				f->signature.arg_locs.push_back(sf->args[a].loc);
			}
			f->signature.return_loc = sf->return_reg;
			f->signature.return_type = sf->return_type->type;
			vctx->add(f);
		}

		for (u32 i = 0;i < new_types.size();i++) {
			data_type* t = new_types[i];
			vm_type* tp = t->type;
			for (u32 p = 0;p < t->props.size();p++) {
				data_type::property& sprop = t->props[p];
				vm_type::property& tprop = tp->properties[p];
				tprop.getter = sprop.getter ? vctx->function(sprop.getter->entry) : nullptr;
				tprop.setter = sprop.setter ? vctx->function(sprop.setter->entry) : nullptr;
				tprop.type = sprop.type->type;
			}

			for (u32 m = 0;m < t->methods.size();m++) {
				func* f = t->methods[m];
				if (f->name == t->name + "::constructor") tp->constructor = vctx->function(f->entry);
				else if (f->name == t->name + "::destructor") tp->destructor = vctx->function(f->entry);
				else {
					// todo: operators, converters
					tp->methods.push_back(vctx->function(f->entry));
				}
			}
		}
	}
};