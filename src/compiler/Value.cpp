#include <gs/compiler/Value.h>
#include <gs/compiler/Compiler.h>
#include <gs/common/Context.h>
#include <gs/common/TypeRegistry.h>
#include <gs/interfaces/IDataTypeHolder.hpp>
#include <gs/common/Function.h>
#include <gs/common/DataType.h>

namespace gs {
    namespace compiler {
        Value::Value() : ITypedObject(nullptr) {
            m_compiler = nullptr;
            m_regId = 0;
            m_allocId = 0;
            m_flags.is_argument = 0;
            m_flags.is_const = 0;
            m_flags.is_pointer = 0;
            m_imm.u = 0;
        }

        Value::Value(const Value& o) : ITypedObject(o.m_type) {
            reset(o);
        }

        Value::Value(Compiler* o, ffi::DataType* tp) : ITypedObject(tp) {
            m_compiler = o;
            m_src = o->getCurrentSrc();
            m_regId = 0;
            m_allocId = 0;
            m_flags.is_argument = 0;
            m_flags.is_const = 0;
            m_flags.is_pointer = 0;
            m_imm.u = 0;
        }

        Value::Value(Compiler* o, u64 imm) : ITypedObject(o->getContext()->getTypes()->getType<u64>()) {
            m_compiler = o;
            m_src = o->getCurrentSrc();
            m_regId = 0;
            m_allocId = 0;
            m_flags.is_argument = 0;
            m_flags.is_const = 0;
            m_flags.is_pointer = 0;
            m_imm.u = imm;
        }

        Value::Value(Compiler* o, i64 imm) : ITypedObject(o->getContext()->getTypes()->getType<i64>()) {
            m_compiler = o;
            m_src = o->getCurrentSrc();
            m_regId = 0;
            m_allocId = 0;
            m_flags.is_argument = 0;
            m_flags.is_const = 0;
            m_flags.is_pointer = 0;
            m_imm.i = imm;
        }

        Value::Value(Compiler* o, f64 imm) : ITypedObject(o->getContext()->getTypes()->getType<f64>()) {
            m_compiler = o;
            m_src = o->getCurrentSrc();
            m_regId = 0;
            m_allocId = 0;
            m_flags.is_argument = 0;
            m_flags.is_const = 0;
            m_flags.is_pointer = 0;
            m_imm.f = imm;
        }

        Value::Value(Compiler* o, ffi::Function* imm) : ITypedObject(imm->getSignature()) {
            m_compiler = o;
            m_src = o->getCurrentSrc();
            m_regId = 0;
            m_allocId = 0;
            m_flags.is_argument = 0;
            m_flags.is_const = 0;
            m_flags.is_pointer = 0;
            m_imm.fn = imm;
        }

        // Discards any info about the current value and adopts the info from another
        void Value::reset(const Value& o) {
            m_compiler = o.m_compiler;
            m_name = o.m_name;
            m_src = o.m_src;
            m_type = o.m_type;
            m_regId = o.m_regId;
            m_allocId = o.m_allocId;
            m_flags = o.m_flags;
            m_imm = o.m_imm;
        }

        vreg_id Value::getRegId() const {
            return m_regId;
        }

        alloc_id Value::getStackAllocId() const {
            return m_allocId;
        }

        const utils::String& Value::getName() const {
            return m_name;
        }

        const SourceLocation& Value::getSource() const {
            return m_src;
        }

        const Value::flags& Value::getFlags() const {
            return m_flags;
        }

        Value::flags& Value::getFlags() {
            return m_flags;
        }

        // Only one of these can be true
        bool Value::isReg() const {
            return m_regId > 0;
        }

        bool Value::isImm() const {
            return m_regId == 0 && m_allocId == 0 && m_compiler;
        }

        bool Value::isStack() const {
            return m_allocId > 0;
        }

        Value Value::operator +  (const Value& rhs) const {
            return Value();

        }

        Value Value::operator += (const Value& rhs) {
            return Value();

        }

        Value Value::operator -  (const Value& rhs) const {
            return Value();

        }

        Value Value::operator -= (const Value& rhs) {
            return Value();

        }

        Value Value::operator *  (const Value& rhs) const {
            return Value();

        }

        Value Value::operator *= (const Value& rhs) {
            return Value();

        }

        Value Value::operator /  (const Value& rhs) const {
            return Value();

        }

        Value Value::operator /= (const Value& rhs) {
            return Value();

        }

        Value Value::operator %  (const Value& rhs) const {
            return Value();

        }

        Value Value::operator %= (const Value& rhs) {
            return Value();

        }

        Value Value::operator ^  (const Value& rhs) const {
            return Value();

        }

        Value Value::operator ^= (const Value& rhs) {
            return Value();

        }

        Value Value::operator &  (const Value& rhs) const {
            return Value();

        }

        Value Value::operator &= (const Value& rhs) {
            return Value();

        }

        Value Value::operator |  (const Value& rhs) const {
            return Value();

        }

        Value Value::operator |= (const Value& rhs) {
            return Value();

        }

        Value Value::operator << (const Value& rhs) const {
            return Value();

        }

        Value Value::operator <<=(const Value& rhs) {
            return Value();

        }

        Value Value::operator >> (const Value& rhs) const {
            return Value();

        }

        Value Value::operator >>=(const Value& rhs) {
            return Value();

        }

        Value Value::operator != (const Value& rhs) const {
            return Value();

        }

        Value Value::operator && (const Value& rhs) const {
            return Value();

        }

        Value Value::operator || (const Value& rhs) const {
            return Value();

        }

        Value Value::operator =  (const Value& rhs) {
            return Value();

        }

        Value Value::operator == (const Value& rhs) const {
            return Value();

        }

        Value Value::operator <  (const Value& rhs) const {
            return Value();

        }

        Value Value::operator <= (const Value& rhs) const {
            return Value();

        }

        Value Value::operator >  (const Value& rhs) const {
            return Value();

        }

        Value Value::operator >= (const Value& rhs) const {
            return Value();

        }

        Value Value::operator [] (const Value& rhs) const {
            return Value();

        }

        Value Value::operator () (const utils::Array<Value>& args) const {
            return Value();

        }

        Value Value::operator -  () {
            return Value();

        }

        Value Value::operator -- () {
            return Value();

        }

        Value Value::operator -- (int) {
            return Value();

        }

        Value Value::operator ++ () {
            return Value();

        }

        Value Value::operator ++ (int) {
            return Value();

        }

        Value Value::operator !  () const {
            return Value();

        }

        Value Value::operator ~  () const {
            return Value();

        }

        Value Value::operator_logicalAndAssign(const Value& rhs) {
            return Value();

        }

        Value Value::operator_logicalOrAssign(const Value& rhs) {
            return Value();
        }
    };
};