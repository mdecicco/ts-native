#pragma once
#include <tsn/common/types.h>
#include <tsn/compiler/types.h>
#include <tsn/utils/SourceLocation.h>
#include <tsn/interfaces/ITypedObject.h>

#include <utils/Array.h>
#include <utils/String.h>

namespace tsn {
    class Module;
    class Context;
    
    namespace ffi {
        class DataType;
    };

    namespace compiler {
        class FunctionDef;
        enum ir_instruction : u32;
        struct member_expr_hints;

        class Value : public ITypedObject {
            public:
                struct flags {
                    // Whether or not the value is a pointer to the data type
                    // ...A real pointer, not a Pointer<T>
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

                    // If this value refers to a function_id (is_function will also be true)
                    unsigned is_function_id : 1;

                    // If this value refers to module data, module will be stored in m_imm.m, slot in m_slotId
                    unsigned is_module_data : 1;

                    // If this value is an immediate
                    unsigned is_immediate   : 1;
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
                ) const;
                Value getPropPtr(
                    const utils::String& name,
                    bool excludeInherited = false,
                    bool excludePrivate = false,
                    bool doError = true
                ) const;
                Value callMethod(const utils::String& name, ffi::DataType* retTp, const utils::Array<Value>& args) const;


                template <typename T>
                std::enable_if_t<is_imm_v<T>, T> getImm() const;
                template <typename T>
                void setImm(T val);

                vreg_id getRegId() const;
                alloc_id getStackAllocId() const;
                u32 getModuleDataSlotId() const;
                const utils::String& getName() const;
                const SourceLocation& getSource() const;
                const flags& getFlags() const;
                flags& getFlags();
                Value* getSrcPtr() const;
                Value* getSrcSelf() const;
                Value* getStackRef() const;
                
                void setType(ffi::DataType* to);
                void setRegId(vreg_id reg);
                void setStackAllocId(alloc_id id);
                void setStackRef(const Value& src);
                void setSrcPtr(const Value& src);

                bool isValid() const;
                bool isArg() const;

                // If the value referred to is actually floating point
                // data. For example, if flags.is_pointer == 1 then
                // this returns false
                bool isFloatingPoint() const;
                
                // Only one of these can be true
                bool isReg() const;
                bool isImm() const;
                bool isStack() const;

                // If any of these are true then isImm() is also true
                bool isModuleData() const;
                bool isType() const;
                bool isModule() const;
                bool isFunction() const;

                // if this is true, then isFunction() is also true
                bool isFunctionID() const;

                Value operator +  (const Value& rhs) const;
                Value operator += (const Value& rhs) const;
                Value operator -  (const Value& rhs) const;
                Value operator -= (const Value& rhs) const;
                Value operator *  (const Value& rhs) const;
                Value operator *= (const Value& rhs) const;
                Value operator /  (const Value& rhs) const;
                Value operator /= (const Value& rhs) const;
                Value operator %  (const Value& rhs) const;
                Value operator %= (const Value& rhs) const;
                Value operator ^  (const Value& rhs) const;
                Value operator ^= (const Value& rhs) const;
                Value operator &  (const Value& rhs) const;
                Value operator &= (const Value& rhs) const;
                Value operator |  (const Value& rhs) const;
                Value operator |= (const Value& rhs) const;
                Value operator << (const Value& rhs) const;
                Value operator <<=(const Value& rhs) const;
                Value operator >> (const Value& rhs) const;
                Value operator >>=(const Value& rhs) const;
                Value operator != (const Value& rhs) const;
                Value operator && (const Value& rhs) const;
                Value operator || (const Value& rhs) const; 
                Value operator =  (const Value& rhs) const;
                Value operator == (const Value& rhs) const;
                Value operator <  (const Value& rhs) const;
                Value operator <= (const Value& rhs) const;
                Value operator >  (const Value& rhs) const;
                Value operator >= (const Value& rhs) const;
                Value operator [] (const Value& rhs) const;
                Value operator () (const utils::Array<Value>& args, Value* self) const;
                Value operator -  () const;
                Value operator -- () const;
                Value operator -- (int) const;
                Value operator ++ () const;
                Value operator ++ (int) const;
                Value operator !  () const;
                Value operator ~  () const;
                Value operator *  () const;

                // a &&= b
                Value operator_logicalAndAssign(const Value& rhs) const;
                // a ||= b
                Value operator_logicalOrAssign(const Value& rhs) const;

                bool isEquivalentTo(const Value& v) const;

                utils::String toString(Context* ctx) const;
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
                Value(FunctionDef* o, Module* imm, u32 slotId);

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
                Value* m_srcPtr;
                Value* m_stackRef;
                FunctionDef* m_srcSetter;
                Value* m_srcSelf;

                utils::String m_name;
                SourceLocation m_src;
                vreg_id m_regId;
                alloc_id m_allocId;
                u32 m_slotId;
                flags m_flags;
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