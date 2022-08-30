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
        class Compiler;

        class Value : public ITypedObject {
            public:
                struct flags {
                    // Whether or not the value is a pointer to the data type
                    // ...A real pointer, not a PointerType
                    unsigned is_pointer  : 1;
                    unsigned is_const    : 1;

                    // If it's an argument, the argument index will be stored in m_imm.u
                    unsigned is_argument : 1;
                };
                Value();
                Value(const Value& o);

                // Discards any info about the current value and adopts the info from another
                void reset(const Value& o);

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

                // a &&= b
                Value operator_logicalAndAssign(const Value& rhs);
                // a ||= b
                Value operator_logicalOrAssign(const Value& rhs);


            protected:
                friend class ScopeManager;
                friend class FunctionDef;

                Value(Compiler* o, ffi::DataType* tp);
                Value(Compiler* o, u64 imm);
                Value(Compiler* o, i64 imm);
                Value(Compiler* o, f64 imm);
                Value(Compiler* o, ffi::Function* imm);

                Compiler* m_compiler;
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