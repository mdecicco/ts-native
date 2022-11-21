#pragma once
#include <gs/common/types.h>
#include <gs/compiler/types.h>
#include <gs/utils/SourceLocation.h>
#include <gs/interfaces/ITypedObject.h>

#include <utils/Array.h>
#include <utils/String.h>

#include <xtr1common>

namespace gs {
    namespace ffi {
        class DataType;
        class Function;
    };

    namespace compiler {
        class FunctionDef;
        enum ir_instruction;

        class Value : public ITypedObject {
            public:
                struct flags {
                    // Whether or not the value is a pointer to the data type
                    // ...A real pointer, not a PointerType
                    unsigned is_pointer  : 1;
                    unsigned is_const    : 1;

                    // If it's an argument, the argument index will be stored in m_imm.u
                    unsigned is_argument : 1;

                    // If it's stored in a module data slot, the slot index will be stored
                    // in m_imm.u
                    unsigned is_module_data : 1;
                };
                Value();
                Value(const Value& o);

                // Discards any info about the current value and adopts the info from another
                void reset(const Value& o);
                Value convertedTo(ffi::DataType* tp) const;
                Value getProp(const utils::String& name, bool excludeInherited = false, bool excludePrivate = false, bool doError = true);

                template <typename T>
                std::enable_if_t<is_imm_v<T>, T> getImm() const;

                vreg_id getRegId() const;
                alloc_id getStackAllocId() const;
                const utils::String& getName() const;
                const SourceLocation& getSource() const;
                const flags& getFlags() const;
                flags& getFlags();

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
                Value operator () (const utils::Array<Value>& args) const;
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


            protected:
                friend class ScopeManager;
                friend class FunctionDef;

                Value(FunctionDef* o, ffi::DataType* tp);
                Value(FunctionDef* o, u64 imm);
                Value(FunctionDef* o, i64 imm);
                Value(FunctionDef* o, f64 imm);
                Value(FunctionDef* o, ffi::Function* imm);

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
                    bool assignmentOp = false
                ) const;

                FunctionDef* m_func;
                utils::String m_name;
                SourceLocation m_src;
                vreg_id m_regId;
                alloc_id m_allocId;
                flags m_flags;
                union {
                    u64 u;
                    i64 i;
                    f64 f;
                    ffi::Function* fn;
                } m_imm;
        };
    };
};