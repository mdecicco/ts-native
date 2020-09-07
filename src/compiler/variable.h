#pragma once
#include <string>
#include <vector>
#include <source_ref.h>

namespace gjs {
	class vm_type;
	class vm_function;

	namespace compile {
		struct context;

		bool has_valid_conversion(vm_type* from, vm_type* to);

		class var {
			public:
				var(const var& v);
				~var();

				inline std::string name() const { return m_name; }
				inline source_ref instantiation() const { return m_instantiation; }
				inline bool is_imm() const { return m_is_imm; }
				inline bool is_ptr() const { return m_is_ptr; }
				inline vm_type* type() const { return m_type; }
				inline u64 imm_u() const { return m_imm.u; }
				inline i64 imm_i() const { return m_imm.i; }
				inline f32 imm_f() const { return m_imm.f; }
				inline f64 imm_d() const { return m_imm.d; }
				u32 reg_id() const { return m_reg_id; }

				var operator + (const var& rhs) const;
				var operator - (const var& rhs) const;
				var operator * (const var& rhs) const;
				var operator / (const var& rhs) const;
				var operator % (const var& rhs) const;
				var operator << (const var& rhs) const;
				var operator >> (const var& rhs) const;
				var operator && (const var& rhs) const;
				var operator || (const var& rhs) const;
				var operator & (const var& rhs) const;
				var operator | (const var& rhs) const;
				var operator ^ (const var& rhs) const;
				var operator += (const var& rhs) const;
				var operator -= (const var& rhs) const;
				var operator *= (const var& rhs) const;
				var operator /= (const var& rhs) const;
				var operator %= (const var& rhs) const;
				var operator <<= (const var& rhs) const;
				var operator >>= (const var& rhs) const;
				var operator_landEq (const var& rhs) const; // &&=
				var operator_lorEq (const var& rhs) const; // ||=
				var operator &= (const var& rhs) const;
				var operator |= (const var& rhs) const;
				var operator ^= (const var& rhs) const;
				var operator ++ () const; // prefix
				var operator -- () const; // prefix
				var operator ++ (int) const; // postfix
				var operator -- (int) const; // postfix
				var operator < (const var& rhs) const;
				var operator > (const var& rhs) const;
				var operator <= (const var& rhs) const;
				var operator >= (const var& rhs) const;
				var operator != (const var& rhs) const;
				var operator == (const var& rhs) const;
				var operator ! () const;
				var operator - () const;
				var operator_eq (const var& v) const;
				var operator_idx (vm_type* ret_tp, const var& rhs) const;

				u64 size() const;
				bool has_prop(const std::string& name) const;
				var prop(const std::string& name) const;
				var prop_ptr(const std::string& name) const;
				vm_function* method(const std::string& name, vm_type* ret, const std::vector<vm_type*>& args) const;
				bool convertible_to(vm_type* tp) const;
				var convert(vm_type* tp) const;

				std::string to_string() const;

			protected:
				friend struct tac_instruction;
				friend struct context;

				var();
				var(context* ctx, u64 u);
				var(context* ctx, i64 i);
				var(context* ctx, f32 f);
				var(context* ctx, f64 d);
				var(context* ctx, const std::string& s);
				var(context* ctx, u32 reg_id, vm_type* type);

				context* m_ctx;
				std::string m_name;
				source_ref m_instantiation;
				vm_type* m_type;

				bool m_is_ptr;
				bool m_is_imm;
				u32 m_reg_id;

				union {
					u64 u;
					i64 i;
					f32 f;
					f64 d;
				} m_imm;
		};
	};
};