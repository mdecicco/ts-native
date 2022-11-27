#pragma once
#include <gs/common/types.h>
#include <gs/compiler/types.h>
#include <gs/utils/SourceLocation.h>
#include <gs/interfaces/ITypedObject.h>

#include <utils/Array.h>
#include <utils/String.h>

#include <xtr1common>

namespace gs {
    class Module;
    namespace ffi {
        class DataType;
    };

    namespace compiler {
        class FunctionDef;
        enum ir_instruction;
        struct member_expr_hints;

        class Value : public ITypedObject {
            public:
                struct flags {
                    // Whether or not the value is a pointer to the data type
                    // ...A real pointer, not a PointerType
                    unsigned is_pointer     : 1;
                    unsigned is_read_only   : 1;

                    // If it's an argument, the argument index will be stored in m_imm.u
                    unsigned is_argument    : 1;

                    // If this value refers to a data type
                    unsigned is_type        : 1;

                    // If this value refers to a module
                    unsigned is_module      : 1;

                    // If this value refers to a function
                    unsigned is_function    : 1;
                };
                Value();
                Value(const Value& o);
                ~Value();

                // Discards any info about the current value and adopts the info from another
                void reset(const Value& o);
                Value convertedTo(ffi::DataType* tp) const;
                Value getProp(
                    const utils::String& name,
                    bool excludeInherited = false,
                    bool excludePrivate = false,
                    bool excludeMethods = false,
                    bool doError = true,
                    member_expr_hints* hints = nullptr
                );


                template <typename T>
                std::enable_if_t<is_imm_v<T>, T> getImm() const;

                vreg_id getRegId() const;
                alloc_id getStackAllocId() const;
                const utils::String& getName() const;
                const SourceLocation& getSource() const;
                const flags& getFlags() const;
                flags& getFlags();
                Value* getSrcPtr() const;

                // Only one of these can be true
                bool isReg() const;
                bool isImm() const;
                bool isStack() const;

                Value operator +  (const Value& rhs) const;
                Value operator += (const Value& rhs);
                Value operator -  (const Value& rhs) const;
                Value operator -= (const Value& rhs);
                Value operator *  (const Value& rhs) const;
                Value operator *= (const Value& rhs);
                Value operator /  (const Value& rhs) const;
                Value operator /= (const Value& rhs);
                Value operator %  (const Value& rhs) const;
                Value operator %= (const Value& rhs);
                Value operator ^  (const Value& rhs) const;
                Value operator ^= (const Value& rhs);
                Value operator &  (const Value& rhs) const;
                Value operator &= (const Value& rhs);
                Value operator |  (const Value& rhs) const;
                Value operator |= (const Value& rhs);
                Value operator << (const Value& rhs) const;
                Value operator <<=(const Value& rhs);
                Value operator >> (const Value& rhs) const;
                Value operator >>=(const Value& rhs);
                Value operator != (const Value& rhs) const;
                Value operator && (const Value& rhs) const;
                Value operator || (const Value& rhs) const; 
                Value operator =  (const Value& rhs);
                Value operator == (const Value& rhs) const;
                Value operator <  (const Value& rhs) const;
                Value operator <= (const Value& rhs) const;
                Value operator >  (const Value& rhs) const;
                Value operator >= (const Value& rhs) const;
                Value operator [] (const Value& rhs) const;
                Value operator () (const utils::Array<Value>& args, Value* self) const;
                Value operator -  ();
                Value operator -- ();
                Value operator -- (int);
                Value operator ++ ();
                Value operator ++ (int);
                Value operator !  () const;
                Value operator ~  () const;
                Value operator *  () const;

                // a &&= b
                Value operator_logicalAndAssign(const Value& rhs);
                // a ||= b
                Value operator_logicalOrAssign(const Value& rhs);

                utils::String toString() const;


            protected:
                friend class Scope;
                friend class ScopeManager;
                friend class FunctionDef;

                Value(FunctionDef* o, ffi::DataType* tp);
                Value(FunctionDef* o, u64 imm);
                Value(FunctionDef* o, i64 imm);
                Value(FunctionDef* o, f64 imm);
                Value(FunctionDef* o, FunctionDef* imm);
                Value(FunctionDef* o, ffi::DataType* imm, bool unused);
                Value(FunctionDef* o, Module* imm);

                Value genBinaryOp(
                    FunctionDef* fn,
                    const Value* self,
                    const Value& rhs,
                    ir_instruction _i,
                    ir_instruction _u,
                    ir_instruction _f,
                    ir_instruction _d,
                    const char* overrideName,
                    bool assignmentOp = false
                ) const;
                Value genUnaryOp(
                    FunctionDef* fn,
                    const Value* self,
                    ir_instruction _i,
                    ir_instruction _u,
                    ir_instruction _f,
                    ir_instruction _d,
                    const char* overrideName,
                    bool resultIsPreOp = false,
                    bool assignmentOp = false,
                    bool noResultReg = false
                ) const;

                FunctionDef* m_func;
                utils::String m_name;
                SourceLocation m_src;
                vreg_id m_regId;
                alloc_id m_allocId;
                flags m_flags;
                Value* m_srcPtr;
                FunctionDef* m_srcSetter;
                Value* m_srcSelf;
                union {
                    u64 u;
                    i64 i;
                    f64 f;
                    FunctionDef* fn;
                    Module* mod;
                } m_imm;
        };
    };
};