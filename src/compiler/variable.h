#pragma once
#include <string>
#include <vector>
#include <memory>
#include <common/source_ref.h>

namespace gjs {
    class script_type;
    class script_function;
    class register_allocator;
    struct compilation_output;

    namespace bind {
        enum property_flags;
    };

    namespace compile {
        struct context;

        bool has_valid_conversion(context& ctx, script_type* from, script_type* to);

        class var {
            public:
                var(const var& v);
                ~var();

                inline std::string name() const { return m_name; }
                inline source_ref instantiation() const { return m_instantiation; }
                inline void set_code_ref(const source_ref& ref) { m_instantiation = ref; }
                inline bool is_imm() const { return m_is_imm; }
                inline script_type* type() const { return m_type; }
                inline u64 imm_u() const { return m_imm.u; }
                inline i64 imm_i() const { return m_imm.i; }
                inline f32 imm_f() const { return m_imm.f; }
                inline f64 imm_d() const { return m_imm.d; }
                inline u32 reg_id() const { return m_reg_id; }
                inline bool valid() const { return m_ctx != nullptr; }
                inline bool flag(bind::property_flags f) const { return m_flags & f; }
                inline void raise_flag(bind::property_flags f) { m_flags |= f; }
                inline bool is_stack_obj() const { return m_stack_id != 0; }
                inline u64 stack_id() const { return m_stack_id; }
                inline bool is_arg() const { return m_arg_idx != u8(-1); }
                inline u8 arg_idx() const { return m_arg_idx; }
                inline void set_arg_idx(u8 idx) { m_arg_idx = idx; }

                // used by code generation phase when generating load instructions
                // for 'spilled' variables
                inline bool is_spilled() const { return m_stack_loc != u64(-1); }
                inline u64 stack_off() const { return m_stack_loc; }

                u64 size() const;
                bool has_prop(const std::string& name) const;
                var prop(const std::string& name) const;
                var prop_ptr(const std::string& name) const;
                bool has_any_method(const std::string& name) const;
                bool has_unambiguous_method(const std::string& name, script_type* ret, const std::vector<script_type*>& args) const;
                script_function* method(const std::string& name, script_type* ret, const std::vector<script_type*>& args) const;
                bool convertible_to(script_type* tp) const;
                var convert(script_type* tp) const;
                void set_mem_ptr(const var& v);
                void raise_stack_flag();
                void adopt_stack_flag(var& from);

                // type used for first argument when calling methods
                script_type* call_this_tp() const;

                std::string to_string() const;

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
                var operator [] (const var& rhs) const;
                var operator_eq (const var& v) const;

            protected:
                friend struct tac_instruction;
                friend struct context;
                friend class register_allocator;
                friend struct compilation_output;

                var();
                var(context* ctx, u64 u);
                var(context* ctx, i64 i);
                var(context* ctx, f32 f);
                var(context* ctx, f64 d);
                var(context* ctx, const std::string& s);
                var(context* ctx, u32 reg_id, script_type* type);

                context* m_ctx;
                std::string m_name;
                source_ref m_instantiation;
                script_type* m_type;
                u8 m_flags;
                u8 m_arg_idx;

                bool m_is_imm;
                u32 m_reg_id;
                
                // Used in the register allocation phase when no registers are available
                u64 m_stack_loc;

                // If the object was allocated in the stack, this will be set to some unique
                // value. If it's non-zero, this var represents a pointer to a stack object.
                u64 m_stack_id;

                struct {
                    bool valid;
                    u32 reg;
                } m_mem_ptr;

                // this is set when this var is a reference to an object property that has a setter
                struct {
                    std::shared_ptr<var> this_obj; // dynamically allocated copy of the original var
                    script_function* func = nullptr;
                } m_setter;

                union {
                    u64 u;
                    i64 i;
                    f32 f;
                    f64 d;
                } m_imm;
        };
    };
};