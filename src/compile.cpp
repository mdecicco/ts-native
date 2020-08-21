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
	void compile_function(compile_context& ctx, ast_node* node, func& out);
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
				is_reg = false;
				is_imm = false;
				type = nullptr;
				count = 1;
				total_ref_count = 0;
				ref_count = 0;
			}

			compile_context* ctx;
			bool is_variable;
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
			robin_hood::unordered_map<string, var> vars;
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

		string name;
		data_type* return_type;
		vmr return_reg;
		vector<arg_info> args;
		address entry;
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

		void free(var* v) {
			if (v->is_reg) registers.free(v->loc.reg);
			else stack.free(v->loc.stack_addr);

			for (u8 i = 0;i < scopes.size();i++) {
				for (auto s = scopes[i].vars.begin();s != scopes[i].vars.end();++s) {
					if ((&s->getSecond()) == v) {
						scopes[i].vars.erase(s);
						return;
					}
				}
			}

			// exception
		}

		void free_consumed_vars() {
			for (u8 i = 0;i < scopes.size();i++) {
				for (auto s = scopes[i].vars.begin();s != scopes[i].vars.end();++s) {
					var* v = &s->getSecond();
					if (!v->is_variable) continue;
					if (v->ref_count == v->total_ref_count) free(v);
				}
			}
		}

		var* get(ast_node* identifier) {
			for (u8 i = 0;i < scopes.size();i++) {
				if (scopes[i].vars.count(*identifier) == 0) continue;
				var* v = &scopes[i].vars[*identifier];
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
				vars.push_back(&i->getSecond());
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
		data_type(const string& _name, bool _built_in = false) : name(_name), built_in(_built_in) { }
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
		struct prop {
			string name;
			data_type* type;
			u32 offset;
		};

		u32 size;
		vector<prop> props;
		vector<func> methods;
	};

	struct source_map {
		vector<string> files;
		vector<string> lines;
		struct elem {
			u16 file;
			u32 line;
			u32 col;
		};
		struct src_info {
			string file;
			string lineText;
			u32 line;
			u32 col;
		};

		// 1:1 map, instruction address : elem
		vector<elem> map;

		void append(ast_node* node) {
			elem e;
			bool found_file = false;
			bool found_line = false;
			for (u16 i = 0;i < files.size();i++) {
				if (files[i] == node->start.file) {
					e.file = i;
					found_file = true;
				}
			}
			for (u32 i = 0;i < lines.size();i++) {
				if (lines[i] == node->start.lineText) {
					e.line = i;
					found_line = true;
				}
			}
			if (!found_file) {
				e.file = files.size();
				files.push_back(node->start.file);
			}
			if (!found_line) {
				e.line = lines.size();
				lines.push_back(node->start.lineText);
			}

			e.col = node->start.col;

			map.push_back(e);
		}

		src_info get(address addr) {
			elem e = map[addr];
			return { files[e.file], lines[e.line], e.line, e.col };
		}
	};

	struct compile_context {
		vm_context* ctx;
		source_map map;
		compile_log log;
		instruction_array* out;
		vector<data_type> types;
		vector<func> funcs;
		func* cur_func;

		void add(instruction i, ast_node* because) {
			address addr = out->size();
			(*out) += i;

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

			map.append(because);
		}

		data_type* type(const string& name) {
			for (u16 i = 0;i < types.size();i++) {
				if (types[i].name == name) return &types[i];
			}
			return nullptr;
		}

		data_type* type(ast_node* node) {
			return type(*node);
		}

		func* function(const string& name) {
			for (u16 i = 0;i < funcs.size();i++) {
				if (funcs[i].name == name) return &funcs[i];
			}
			return nullptr;
		}
	};



	var* func::allocate(compile_context& ctx, ast_node* decl, bool is_arg) {
		if (auto_free_consumed_vars) free_consumed_vars();

		// todo: figure out what to do for big args
		u32 count = 1;
		string name = *decl->identifier;
		data_type* type = ctx.type(decl->data_type);
		if (type->size > 8 || count > 1) {
			scopes[scopes.size() - 1].vars[name] = var();
			var& l = scopes[scopes.size() - 1].vars[name];
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
		if (type->name == "decimal") reg = registers.allocate_fp();
		else {
			if (!is_arg) reg = registers.allocate_gp();
			else reg = registers.allocate_arg();
		}

		scopes[scopes.size() - 1].vars[name] = var();
		var& l = scopes[scopes.size() - 1].vars[name];
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
			scopes[scopes.size() - 1].vars[name] = var();
			var& l = scopes[scopes.size() - 1].vars[name];
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
		if (type->name == "decimal") reg = registers.allocate_fp();
		else reg = registers.allocate_gp();

		scopes[scopes.size() - 1].vars[name] = var();
		var& l = scopes[scopes.size() - 1].vars[name];
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
		scopes[scopes.size() - 1].vars[name] = var();
		var& l = scopes[scopes.size() - 1].vars[name];
		l.is_variable = false;
		l.ctx = &ctx;
		l.type = ctx.type("integer");
		l.count = 1;
		l.is_reg = false;
		l.is_imm = true;
		l.imm.i = i;
		return &l;
	}

	var* func::imm(compile_context& ctx, decimal d) {
		string name = format("__anon_%d", anon_var_id++);
		scopes[scopes.size() - 1].vars[name] = var();
		var& l = scopes[scopes.size() - 1].vars[name];
		l.is_variable = false;
		l.ctx = &ctx;
		l.type = ctx.type("decimal");
		l.count = 1;
		l.is_reg = false;
		l.is_imm = true;
		l.imm.d = d;
		return &l;
	}

	var* func::imm(compile_context& ctx, char* s) {
		/*
		string name = format("__anon_%d", anon_var_id++);
		scopes[scopes.size() - 1].vars[name] = var();
		var& l = scopes[scopes.size() - 1].vars[name];
		l.is_variable = false;
		l.ctx = &ctx;
		l.type = ctx.type("integer");
		l.count = 1;
		l.is_reg = false;
		l.is_imm = true;
		l.imm.d = d;
		return &l;
		*/
		// todo: move string literals to vm memory and store address in value as an immediate
		return nullptr;
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
			encode(store).operand(loc.reg).operand(vmr::sp).operand(integer(addr) + offset),
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
			encode(vmi::addi).operand(vmr::v0).operand(offset).operand((integer)addr),
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
		if (type->name == "decimal") reg = ctx->cur_func->registers.allocate_fp();
		else reg = ctx->cur_func->registers.allocate_gp();
		ctx->cur_func->registers.free(reg);

		move_to(reg, because, offset);

		return reg;
	}

	vmr var::move_to_register(ast_node* because, vmr offset) {
		vmr reg;
		if (type->name == "decimal") reg = ctx->cur_func->registers.allocate_fp();
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
			if (type->name == "integer") {
				ctx->add(
					encode(vmi::add).operand(reg).operand(vmr::zero).operand(imm.i),
					because
				);
			} else {
				ctx->add(
					encode(vmi::fadd).operand(reg).operand(vmr::zero).operand(imm.d),
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
			if (type->name == "integer") {
				ctx->add(
					encode(vmi::add).operand(reg).operand(vmr::zero).operand(imm.i),
					because
				);
			} else {
				ctx->add(
					encode(vmi::fadd).operand(reg).operand(vmr::zero).operand(imm.d),
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
			if (type->name == "integer") {
				ctx->add(
					encode(vmi::add).operand(reg).operand(vmr::zero).operand(imm.i),
					because
				);
			} else {
				ctx->add(
					encode(vmi::fadd).operand(reg).operand(vmr::zero).operand(imm.d),
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
			if (type->name == "integer") {
				ctx->add(
					encode(vmi::add).operand(reg).operand(vmr::zero).operand(imm.i),
					because
				);
			} else {
				ctx->add(
					encode(vmi::fadd).operand(reg).operand(vmr::zero).operand(imm.d),
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

	var* call(compile_context& ctx, func* to, ast_node* because, vector<var*> args) {
		// keep track of where vars should be restored to, if they refer to registers
		vector <pair<var*, vmr>> restore_pairs;
		for (u32 s = 0;s < ctx.cur_func->scopes.size();s++) {
			for (auto i = ctx.cur_func->scopes[s].vars.begin();i != ctx.cur_func->scopes[s].vars.end();++i) {
				var* v = &i->getSecond();
				if (v->is_reg) {
					restore_pairs.push_back(pair<var*, vmr>(v, v->loc.reg));
				}
			}
		}

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
				//if (args[a]->type->name == "decimal") tmp = ctx.cur_func->registers.allocate_fp();
				//else tmp = ctx.cur_func->registers.allocate_gp();
				//args[a]->store_in(tmp, because);
				//arg_pairs.push_back(pair<vmr, vmr>(dest, tmp));
				arg_pairs.push_back(pair<vmr, vmr>(dest, args[a]->to_reg(because)));
			}
		}

		// back up register vars to the stack
		for (u32 s = 0;s < ctx.cur_func->scopes.size();s++) {
			for (auto i = ctx.cur_func->scopes[s].vars.begin();i != ctx.cur_func->scopes[s].vars.end();++i) {
				var* v = &i->getSecond();
				if (v->is_reg) {
					v->to_stack(because);
				}
			}
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

		// move stack pointer
		if (ctx.cur_func->stack.size() > 0) {
			ctx.add(
				encode(vmi::addi).operand(vmr::sp).operand(vmr::sp).operand((integer)ctx.cur_func->stack.size()),
				because
			);
		}

		// make the call
		ctx.add(
			encode(vmi::jal).operand((integer)to->entry),
			because
		);

		// move stack pointer back
		if (ctx.cur_func->stack.size() > 0) {
			ctx.add(
				encode(vmi::subi).operand(vmr::sp).operand(vmr::sp).operand((integer)ctx.cur_func->stack.size()),
				because
			);
		}

		// restore register vars
		for (u8 i = 0;i < restore_pairs.size();i++) {
			restore_pairs[i].first->move_to(restore_pairs[i].second, because);
		}

		// allocate var for return value, if necessary
		var* ret = nullptr;

		if (to->return_type->name != "void") {
			ret = ctx.cur_func->allocate(ctx, to->return_type);
			if (to->return_reg == vmr::register_count) {
				// function must either be calling itself (and compiler hasn't seen a return statement yet)
				// or script writer didn't put a return statement and an error was emitted. Assign register
				// automatically
				if (to->return_type->name == "decimal") to->return_reg = to->registers.allocate_fp();
				else to->return_reg = to->registers.allocate_gp();

				// then free it since it's not actually in use until the statement is reached
				to->registers.free(to->return_reg);
			}

			vmr ret_reg = ret->to_reg(because);
			if (ret_reg != to->return_reg) {
				ctx.add(
					encode(vmi::add).operand(ret_reg).operand(to->return_reg).operand(vmr::zero),
					because
				);
			}
		}

		return ret;
	}



	data_type::data_type(compile_context& ctx, ast_node* node) {
		name = *node->identifier;
		ast_node* n = node->body;
		while (n) {
			if (n->type == nt::variable_declaration) {
				// class
				data_type* t = ctx.type(*node->data_type);
				props.push_back({ *node->identifier, t, t->size });
			} else if (n->type == nt::format_property) {
				// format
				data_type* t = ctx.type(*node->data_type);
				props.push_back({ *node->identifier, t, t->size });
			} else if (n->type == nt::function_declaration) {
				methods.push_back(func());
				compile_function(ctx, n, methods[methods.size() - 1]);
			}
			n = n->next;
		}
	}

	var* cast(compile_context& ctx, var* from, var* to, ast_node* because) {
		if (from->type->equals(to->type)) return from;

		if (!from->type->built_in) {
			func* cast_func = from->type->cast_to_func(to->type);
			if (!cast_func) {
				ctx.log.err(format("No conversion from '%s' to '%s' found", from->type->name.c_str(), to->type->name.c_str()), because);
				return from;
			}

			return call(ctx, cast_func, because, { from });
		}

		if (from->type->name == "string" || to->type->name == "string") return from;
		if (from->type->name == "function") {
			ctx.log.err(format("Cannot convert from 'function' to '%s'", to->type->name.c_str()), because);
			return from;
		}
		if (to->type->name == "function") {
			ctx.log.err(format("Cannot convert from '%s' to 'function'", to->type->name.c_str()), because);
			return from;
		}
		if (from->type->name == "void") {
			ctx.log.err(format("Cannot convert from 'void' to '%s'", to->type->name.c_str()), because);
			return from;
		}
		if (to->type->name == "void") {
			ctx.log.err(format("Cannot convert from '%s' to 'void'", to->type->name.c_str()), because);
			return from;
		}
		if (from->type->name == "integer") {
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
		}
		if (from->type->name == "decimal") {
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

	// possible[0] = float <op> immediate
	// possible[1] = float <op> register
	// possible[2] = fixed <op> immediate
	// possible[3] = fixed <op> register
	void arithmetic_op_maybe_fp(compile_context& ctx, var* l, var* r, var* o, ast_node* because, vmi possible[4]) {
		var* rhs = cast(ctx, r, l, because);

		l->to_reg(because);
		if (!rhs->is_imm) rhs->to_reg(because);

		vmi i;

		if (is_fp(l->loc.reg)) i = rhs->is_imm ? possible[0] : possible[1];
		else i = rhs->is_imm ? possible[2] : possible[3];

		instruction_encoder inst = encode(i).operand(o->to_reg(because)).operand(l->loc.reg);

		if (rhs->is_imm) {
			if (is_fp(l->loc.reg)) inst.operand(rhs->imm.d);
			else inst.operand(rhs->imm.i);
		} else inst.operand(rhs->loc.reg);

		ctx.add(inst, because);

		if (rhs != r) ctx.cur_func->free(rhs);
	}

	// possible[0] = fixed <op> immediate
	// possible[1] = fixed <op> register
	void arithmetic_op(compile_context& ctx, var* l, var* r, var* o, ast_node* because, vmi possible[2]) {
		var* rhs = cast(ctx, r, l, because);

		l->to_reg(because);
		if (!rhs->is_imm) rhs->to_reg(because);

		vmi i = rhs->is_imm ? possible[0] : possible[1];

		instruction_encoder inst = encode(i).operand(o->to_reg(because)).operand(l->loc.reg);

		if (rhs->is_imm) inst.operand(rhs->imm.i);
		else inst.operand(rhs->loc.reg);

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
		refs += count_references(node->literal, identifier);
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

	void compile_function(compile_context& ctx, ast_node* node, func& out) {
		out.name = *node->identifier;
		out.return_type = ctx.type(*node->data_type);
		out.entry = ctx.out->size() + 1;
		out.root = node;

		ctx.cur_func = &out;

		ctx.add(encode(vmi::null), node);

		ast_node* arg = node->arguments ? node->arguments->body : nullptr;
		while (arg) {
			func::var* var = ctx.cur_func->allocate(ctx, arg, true);
			var->total_ref_count = count_references(node, var->name, true);
			out.args.push_back({ *arg->identifier, ctx.type(arg->data_type), var->to_reg(arg) });
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
			else if (node->rvalue->type == nt::identifier) rvalue = ctx.cur_func->get(node->rvalue);
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
			func* t = ctx.function(*node->callee);
			vector<var*> args;
			ast_node* arg = node->arguments;
			while (arg) {
				args.push_back(compile_expression(ctx, arg, nullptr));
				arg = arg->next;
			}
			ret = call(ctx, t, node, args);
			for (u32 i = 0;i < args.size();i++) ctx.cur_func->free(args[i]);
		} else {
			switch (node->op) {
				case op::invalid: { break; }
				case op::add: {
					var* result = dest ? dest : ctx.cur_func->allocate(ctx, lvalue->type);
					vmi possible[4] = { vmi::faddi, vmi::fadd, vmi::addi, vmi::add };
					arithmetic_op_maybe_fp(ctx, lvalue, rvalue, result, node, possible);
					ret = result;
					break;
				}
				case op::sub: {
					var* result = dest ? dest : ctx.cur_func->allocate(ctx, lvalue->type);
					vmi possible[4] = { vmi::fsubi, vmi::fsub, vmi::subi, vmi::sub };
					arithmetic_op_maybe_fp(ctx, lvalue, rvalue, result, node, possible);
					ret = result;
					break;
				}
				case op::mul: {
					var* result = dest ? dest : ctx.cur_func->allocate(ctx, lvalue->type);
					vmi possible[4] = { vmi::fmuli, vmi::fmul, vmi::muli, vmi::mul };
					arithmetic_op_maybe_fp(ctx, lvalue, rvalue, result, node, possible);
					ret = result;
					break;
				}
				case op::div: {
					var* result = dest ? dest : ctx.cur_func->allocate(ctx, lvalue->type);
					vmi possible[4] = { vmi::fdivi, vmi::fdiv, vmi::divi, vmi::div };
					arithmetic_op_maybe_fp(ctx, lvalue, rvalue, result, node, possible);
					ret = result;
					break;
				}
				case op::mod: {
					/*
					var* result = dest ? dest : ctx.cur_func->allocate(ctx, lvalue->type);
					vmi possible[2] = { vmi::modi, vmi::mod };
					arithmetic_op(ctx, lvalue, rvalue, result, node, possible);
					return result;
					*/
					var* result = dest ? dest : ctx.cur_func->allocate(ctx, lvalue->type);
					ret = result;
					break;
				}
				case op::shiftLeft: {
					var* result = dest ? dest : ctx.cur_func->allocate(ctx, lvalue->type);
					vmi possible[2] = { vmi::sli, vmi::sl };
					arithmetic_op(ctx, lvalue, rvalue, result, node, possible);
					ret = result;
					break;
				}
				case op::shiftRight: {
					var* result = dest ? dest : ctx.cur_func->allocate(ctx, lvalue->type);
					vmi possible[2] = { vmi::sri, vmi::sr };
					arithmetic_op(ctx, lvalue, rvalue, result, node, possible);
					ret = result;
					break;
				}
				case op::land: {
					var* result = dest ? dest : ctx.cur_func->allocate(ctx, lvalue->type);
					vmi possible[2] = { vmi::andi, vmi::and };
					arithmetic_op(ctx, lvalue, rvalue, result, node, possible);
					ret = result;
					break;
				}
				case op::lor: {
					var* result = dest ? dest : ctx.cur_func->allocate(ctx, lvalue->type);
					vmi possible[2] = { vmi::ori, vmi::or };
					arithmetic_op(ctx, lvalue, rvalue, result, node, possible);
					ret = result;
					break;
				}
				case op::band: {
					var* result = dest ? dest : ctx.cur_func->allocate(ctx, lvalue->type);
					vmi possible[2] = { vmi::bandi, vmi::band };
					arithmetic_op(ctx, lvalue, rvalue, result, node, possible);
					ret = result;
					break;
				}
				case op::bor: {
					var* result = dest ? dest : ctx.cur_func->allocate(ctx, lvalue->type);
					vmi possible[2] = { vmi::bori, vmi::bor };
					arithmetic_op(ctx, lvalue, rvalue, result, node, possible);
					ret = result;
					break;
				}
				case op::bxor: {
					var* result = dest ? dest : ctx.cur_func->allocate(ctx, lvalue->type);
					vmi possible[2] = { vmi::xori, vmi::xor };
					arithmetic_op(ctx, lvalue, rvalue, result, node, possible);
					ret = result;
					break;
				}
				case op::addEq: {
					vmi possible[4] = { vmi::faddi, vmi::fadd, vmi::addi, vmi::add };
					arithmetic_op_maybe_fp(ctx, lvalue, rvalue, lvalue, node, possible);
					ret = lvalue;
					break;
				}
				case op::subEq: {
					vmi possible[4] = { vmi::fsubi, vmi::fsub, vmi::subi, vmi::sub };
					arithmetic_op_maybe_fp(ctx, lvalue, rvalue, lvalue, node, possible);
					ret = lvalue;
					break;
				}
				case op::mulEq: {
					vmi possible[4] = { vmi::fmuli, vmi::fmul, vmi::muli, vmi::mul };
					arithmetic_op_maybe_fp(ctx, lvalue, rvalue, lvalue, node, possible);
					ret = lvalue;
					break;
				}
				case op::divEq: {
					vmi possible[4] = { vmi::fdivi, vmi::fdiv, vmi::divi, vmi::div };
					arithmetic_op_maybe_fp(ctx, lvalue, rvalue, lvalue, node, possible);
					ret = lvalue;
					break;
				}
				case op::modEq: {
					/*
					vmi possible[2] = { vmi::modi, vmi::mod };
					arithmetic_op(ctx, lvalue, rvalue, lvalue, node, possible);
					return result;
					*/
					var* result = dest ? dest : ctx.cur_func->allocate(ctx, lvalue->type);
					ret = result;
					break;
				}
				case op::shiftLeftEq: {
					vmi possible[2] = { vmi::sli, vmi::sl };
					arithmetic_op(ctx, lvalue, rvalue, lvalue, node, possible);
					ret = lvalue;
					break;
				}
				case op::shiftRightEq: {
					vmi possible[2] = { vmi::sri, vmi::sr };
					arithmetic_op(ctx, lvalue, rvalue, lvalue, node, possible);
					ret = lvalue;
					break;
				}
				case op::landEq: {
					vmi possible[2] = { vmi::andi, vmi::and };
					arithmetic_op(ctx, lvalue, rvalue, lvalue, node, possible);
					ret = lvalue;
					break;
				}
				case op::lorEq: {
					vmi possible[2] = { vmi::ori, vmi::or };
					arithmetic_op(ctx, lvalue, rvalue, lvalue, node, possible);
					ret = lvalue;
					break;
				}
				case op::bandEq: {
					vmi possible[2] = { vmi::bandi, vmi::band };
					arithmetic_op(ctx, lvalue, rvalue, lvalue, node, possible);
					ret = lvalue;
					break;
				}
				case op::borEq: {
					vmi possible[2] = { vmi::bori, vmi::bor };
					arithmetic_op(ctx, lvalue, rvalue, lvalue, node, possible);
					ret = lvalue;
					break;
				}
				case op::bxorEq: {
					vmi possible[2] = { vmi::xori, vmi::xor };
					arithmetic_op(ctx, lvalue, rvalue, lvalue, node, possible);
					ret = lvalue;
					break;
				}
				case op::preInc: {
					var* result = dest ? dest : ctx.cur_func->allocate(ctx, rvalue->type);
					var* tmp;
					if (rvalue->type->name == "decimal") tmp = ctx.cur_func->imm(ctx, 1.0f);
					else tmp = ctx.cur_func->imm(ctx, 1);

					rvalue->store_in(result->to_reg(node), node);
					vmi possible[4] = { vmi::faddi, vmi::fadd, vmi::addi, vmi:: add };
					arithmetic_op_maybe_fp(ctx, rvalue, tmp, rvalue, node, possible);

					ctx.cur_func->free(tmp);
					ret = result;
					break;
				}
				case op::preDec: {
					var* result = dest ? dest : ctx.cur_func->allocate(ctx, rvalue->type);
					var* tmp;
					if (rvalue->type->name == "decimal") tmp = ctx.cur_func->imm(ctx, 1.0f);
					else tmp = ctx.cur_func->imm(ctx, 1);

					rvalue->store_in(result->to_reg(node), node);
					vmi possible[4] = { vmi::fsubi, vmi::fsub, vmi::subi, vmi:: sub };
					arithmetic_op_maybe_fp(ctx, rvalue, tmp, rvalue, node, possible);

					ctx.cur_func->free(tmp);
					ret = result;
					break;
				}
				case op::postInc: {
					var* tmp;
					if (lvalue->type->name == "decimal") tmp = ctx.cur_func->imm(ctx, 1.0f);
					else tmp = ctx.cur_func->imm(ctx, 1);

					vmi possible[4] = { vmi::faddi, vmi::fadd, vmi::addi, vmi:: add };
					arithmetic_op_maybe_fp(ctx, lvalue, tmp, lvalue, node, possible);
					if (dest) lvalue->store_in(dest->to_reg(node), node);

					ctx.cur_func->free(tmp);
					ret = dest ? dest : lvalue;
					break;
				}
				case op::postDec: {
					var* tmp;
					if (lvalue->type->name == "decimal") tmp = ctx.cur_func->imm(ctx, 1.0f);
					else tmp = ctx.cur_func->imm(ctx, 1);

					vmi possible[4] = { vmi::fsubi, vmi::fsub, vmi::subi, vmi:: sub };
					arithmetic_op_maybe_fp(ctx, lvalue, tmp, lvalue, node, possible);
					if (dest) lvalue->store_in(dest->to_reg(node), node);

					ctx.cur_func->free(tmp);
					ret = dest ? dest : lvalue;
					break;
				}
				case op::less: {
					var* result = dest ? dest : ctx.cur_func->allocate(ctx, lvalue->type);
					ret = result;
					break;
				}
				case op::greater: {
					var* result = dest ? dest : ctx.cur_func->allocate(ctx, lvalue->type);
					ret = result;
					break;
				}
				case op::lessEq: {
					var* result = dest ? dest : ctx.cur_func->allocate(ctx, lvalue->type);
					ret = result;
					break;
				}
				case op::greaterEq: {
					var* result = dest ? dest : ctx.cur_func->allocate(ctx, lvalue->type);
					ret = result;
					break;
				}
				case op::notEq: {
					var* result = dest ? dest : ctx.cur_func->allocate(ctx, lvalue->type);
					ret = result;
					break;
				}
				case op::isEq: {
					var* result = dest ? dest : ctx.cur_func->allocate(ctx, lvalue->type);
					ret = result;
					break;
				}
				case op::eq: {
					vmi possible[4] = { vmi::faddi, vmi::fadd, vmi::addi, vmi::add };
					var* tmp = ctx.cur_func->imm(ctx, (integer)0);
					arithmetic_op_maybe_fp(ctx, rvalue, tmp, lvalue, node, possible);
					if (dest) lvalue->store_in(dest->to_reg(node), node);
					ret = dest ? dest : lvalue;
					break;
				}
				case op::not: {
					var* result = dest ? dest : ctx.cur_func->allocate(ctx, lvalue->type);
					ret = result;
					break;
				}
				case op::negate: {
					var* result = dest ? dest : ctx.cur_func->allocate(ctx, lvalue->type);
					ret = result;
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
					var* result = dest ? dest : ctx.cur_func->allocate(ctx, lvalue->type);
					ret = result;
					break;
				}
			}
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
		if (node->condition) {
			func::var* cond = compile_expression(ctx, node->condition, nullptr);
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
			encode(vmi::jmp).operand((integer)loop_cond),
			node
		);

		if (node->condition) {
			// update branch fail address
			instruction branch = (*ctx.out)[loop_cond];
			ctx.out->set(
				loop_cond,
				encode(decode_instruction(branch)).operand(decode_operand_1(branch)).operand((integer)ctx.out->size())
			);
		}

		ctx.cur_func->auto_free_consumed_vars = true;
		ctx.cur_func->pop_scope();
	}

	void compile_while_loop(compile_context& ctx, ast_node* node) {
		ctx.cur_func->auto_free_consumed_vars = false;
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
			encode(vmi::jmp).operand((integer)branchAddr),
			node
		);

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
			encode(vmi::bneqz).operand(cond->to_reg(node)).operand((integer)ctx.out->size() + 1),
			node
		);
		ctx.cur_func->free(cond);

		ctx.add(
			encode(vmi::jmp).operand((integer)startAddr),
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
				ctx.funcs.push_back(func());
				compile_function(ctx, node, ctx.funcs[ctx.funcs.size() - 1]);
				break;
			}
			case nt::class_declaration: { ctx.types.emplace_back(ctx, node); break; }
			case nt::format_declaration: { ctx.types.emplace_back(ctx, node); break; }
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
			default: {
				throw compile_exception("Invalid AST node type", node);
			}
		}
	}

	void init_context(compile_context& ctx) {
		data_type* t = nullptr;

		// define default types
		ctx.types.push_back(data_type("integer", true));
		t = &ctx.types[ctx.types.size() - 1];
		t->size = sizeof(integer);

		ctx.types.push_back(data_type("decimal", true));
		t = &ctx.types[ctx.types.size() - 1];
		t->size = sizeof(decimal);

		ctx.types.push_back(data_type("void", true));
		t = &ctx.types[ctx.types.size() - 1];
		t->size = 0;

		ctx.types.push_back(data_type("string", true));
		t = &ctx.types[ctx.types.size() - 1];
		t->size = sizeof(char*);

		ctx.types.push_back(data_type("function", true));
		t = &ctx.types[ctx.types.size() - 1];
		t->size = sizeof(address);

		// define types bound to vm_context
	}

	void compile_ast(vm_context* vctx, ast_node* tree, instruction_array* out) {
		if (!tree) {
			throw compile_exception("No AST to compile", "", "", 0, 0);
		}

		compile_context ctx;
		init_context(ctx);
		ctx.ctx = vctx;
		ctx.out = out;

		ast_node* n = tree->body;
		if (!n) throw compile_exception("AST has no body", tree);
		while (n) {
			compile_node(ctx, n);
			n = n->next;
		}
	}
};