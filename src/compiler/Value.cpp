#include <gs/compiler/Value.h>
#include <gs/compiler/Compiler.h>
#include <gs/compiler/FunctionDef.h>
#include <gs/common/Context.h>
#include <gs/common/TypeRegistry.h>
#include <gs/interfaces/IDataTypeHolder.hpp>
#include <gs/common/Function.h>
#include <gs/common/DataType.h>

namespace gs {
    namespace compiler {
        Value::Value() : ITypedObject(nullptr) {
            m_func = nullptr;
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

        Value::Value(FunctionDef* o, ffi::DataType* tp) : ITypedObject(tp) {
            m_func = o;
            m_src = o->getCompiler()->getCurrentSrc();
            m_regId = 0;
            m_allocId = 0;
            m_flags.is_argument = 0;
            m_flags.is_const = 0;
            m_flags.is_pointer = 0;
            m_imm.u = 0;
        }

        Value::Value(FunctionDef* o, u64 imm) : ITypedObject(o->getContext()->getTypes()->getType<u64>()) {
            m_func = o;
            m_src = o->getCompiler()->getCurrentSrc();
            m_regId = 0;
            m_allocId = 0;
            m_flags.is_argument = 0;
            m_flags.is_const = 0;
            m_flags.is_pointer = 0;
            m_imm.u = imm;
        }

        Value::Value(FunctionDef* o, i64 imm) : ITypedObject(o->getContext()->getTypes()->getType<i64>()) {
            m_func = o;
            m_src = o->getCompiler()->getCurrentSrc();
            m_regId = 0;
            m_allocId = 0;
            m_flags.is_argument = 0;
            m_flags.is_const = 0;
            m_flags.is_pointer = 0;
            m_imm.i = imm;
        }

        Value::Value(FunctionDef* o, f64 imm) : ITypedObject(o->getContext()->getTypes()->getType<f64>()) {
            m_func = o;
            m_src = o->getCompiler()->getCurrentSrc();
            m_regId = 0;
            m_allocId = 0;
            m_flags.is_argument = 0;
            m_flags.is_const = 0;
            m_flags.is_pointer = 0;
            m_imm.f = imm;
        }

        Value::Value(FunctionDef* o, ffi::Function* imm) : ITypedObject(imm->getSignature()) {
            m_func = o;
            m_src = o->getCompiler()->getCurrentSrc();
            m_regId = 0;
            m_allocId = 0;
            m_flags.is_argument = 0;
            m_flags.is_const = 0;
            m_flags.is_pointer = 0;
            m_imm.fn = imm;
        }

        // Discards any info about the current value and adopts the info from another
        void Value::reset(const Value& o) {
            m_func = o.m_func;
            m_name = o.m_name;
            m_src = o.m_src;
            m_type = o.m_type;
            m_regId = o.m_regId;
            m_allocId = o.m_allocId;
            m_flags = o.m_flags;
            m_imm = o.m_imm;
        }

        Value Value::convertedTo(ffi::DataType* tp) const {
            // todo
            return *this;
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
            return m_regId == 0 && m_allocId == 0 && m_func;
        }

        bool Value::isStack() const {
            return m_allocId > 0;
        }

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
        ) {
            ffi::DataType* selfTp = self->getType();
            const auto& i = selfTp->getInfo();
            if (i.is_primitive) {
                Value out = fn->val(selfTp);

                ir_instruction inst = ir_noop;
                if (i.is_integral) {
                    if (i.is_unsigned) inst = _u;
                    else inst = _i;
                } else if (i.is_floating_point) {
                    if (i.size == sizeof(f64)) inst = _d;
                    else inst = _f;
                }

                fn->add(inst).op(out).op(*self).op(rhs.convertedTo(selfTp));
                return out;
            } else {
                const ffi::DataType* rtp = rhs.getType();
                utils::Array<ffi::Function*> matches = selfTp->findMethods(overrideName, nullptr, &rtp, 1, fm_skip_implicit_args);
                if (matches.size() == 1) {
                    return generateCall(fn->getCompiler(), matches[0], { rhs }, self);
                } else if (matches.size() > 1) {
                    u8 strictC = 0;
                    ffi::Function* func = nullptr;
                    for (u32 i = 0;i < matches.size();i++) {
                        const auto& args = matches[i]->getSignature()->getArguments();
                        for (u32 a = 0;a < args.size();a++) {
                            if (args[a].isImplicit()) continue;
                            if (args[a].dataType == rtp) {
                                if (args.size() > a + 1) {
                                    // Function has more arguments which are not explicitly required
                                    break;
                                }

                                strictC++;
                                func = matches[i];
                            }
                        }
                    }
                    
                    if (strictC == 1) {
                        return generateCall(fn->getCompiler(), func, { rhs }, self);
                    }

                    // todo: Error, ambiguous call
                } else {
                    // todo: Error, no matches
                }
            }

            return fn->getPoison();
        }

        Value genUnaryOp(
            FunctionDef* fn,
            const Value* self,
            ir_instruction _i,
            ir_instruction _u,
            ir_instruction _f,
            ir_instruction _d,
            const char* overrideName,
            bool resultIsPreOp = false
        ) {
            ffi::DataType* selfTp = self->getType();
            const auto& i = selfTp->getInfo();
            if (i.is_primitive) {
                Value out = fn->val(selfTp);

                ir_instruction inst = ir_noop;
                if (i.is_integral) {
                    if (i.is_unsigned) inst = _u;
                    else inst = _i;
                } else if (i.is_floating_point) {
                    if (i.size == sizeof(f64)) inst = _d;
                    else inst = _f;
                }

                if (resultIsPreOp) {
                    fn->add(ir_set).op(out).op(*self);
                }

                fn->add(inst).op(out).op(*self);

                if (!resultIsPreOp) {
                    fn->add(ir_set).op(out).op(*self);
                }
                return out;
            } else {
                utils::Array<ffi::Function*> matches = selfTp->findMethods(overrideName, nullptr, nullptr, 0, fm_skip_implicit_args);
                if (matches.size() == 1) {
                    return generateCall(fn->getCompiler(), matches[0], { }, self);
                } else if (matches.size() > 1) {
                    // todo: Error, ambiguous call
                } else {
                    // todo: Error, no matches
                }
            }

            return fn->getPoison();
        }

        Value Value::operator +  (const Value& rhs) const {
            return genBinaryOp(m_func, this, rhs, ir_iadd, ir_uadd, ir_fadd, ir_dadd, "operator +");
        }

        Value Value::operator += (const Value& rhs) {
            return genBinaryOp(m_func, this, rhs, ir_iadd, ir_uadd, ir_fadd, ir_dadd, "operator +", true);
        }

        Value Value::operator -  (const Value& rhs) const {
            return genBinaryOp(m_func, this, rhs, ir_isub, ir_usub, ir_fsub, ir_dsub, "operator -");
        }

        Value Value::operator -= (const Value& rhs) {
            return genBinaryOp(m_func, this, rhs, ir_isub, ir_usub, ir_fsub, ir_dsub, "operator -", true);
        }

        Value Value::operator *  (const Value& rhs) const {
            return genBinaryOp(m_func, this, rhs, ir_imul, ir_umul, ir_fmul, ir_dmul, "operator *");
        }

        Value Value::operator *= (const Value& rhs) {
            return genBinaryOp(m_func, this, rhs, ir_imul, ir_umul, ir_fmul, ir_dmul, "operator *", true);
        }

        Value Value::operator /  (const Value& rhs) const {
            return genBinaryOp(m_func, this, rhs, ir_idiv, ir_udiv, ir_fdiv, ir_ddiv, "operator /");
        }

        Value Value::operator /= (const Value& rhs) {
            return genBinaryOp(m_func, this, rhs, ir_idiv, ir_udiv, ir_fdiv, ir_ddiv, "operator /", true);
        }

        Value Value::operator %  (const Value& rhs) const {
            return genBinaryOp(m_func, this, rhs, ir_imod, ir_umod, ir_fmod, ir_dmod, "operator %");
        }

        Value Value::operator %= (const Value& rhs) {
            return genBinaryOp(m_func, this, rhs, ir_imod, ir_umod, ir_fmod, ir_dmod, "operator %", true);
        }

        Value Value::operator ^  (const Value& rhs) const {
            return genBinaryOp(m_func, this, rhs, ir_xor, ir_xor, ir_noop, ir_noop, "operator ^");
        }

        Value Value::operator ^= (const Value& rhs) {
            return genBinaryOp(m_func, this, rhs, ir_xor, ir_xor, ir_noop, ir_noop, "operator ^", true);
        }

        Value Value::operator &  (const Value& rhs) const {
            return genBinaryOp(m_func, this, rhs, ir_band, ir_band, ir_noop, ir_noop, "operator &");
        }

        Value Value::operator &= (const Value& rhs) {
            return genBinaryOp(m_func, this, rhs, ir_band, ir_band, ir_noop, ir_noop, "operator &", true);
        }

        Value Value::operator |  (const Value& rhs) const {
            return genBinaryOp(m_func, this, rhs, ir_bor, ir_bor, ir_noop, ir_noop, "operator |");
        }

        Value Value::operator |= (const Value& rhs) {
            return genBinaryOp(m_func, this, rhs, ir_bor, ir_bor, ir_noop, ir_noop, "operator |", true);
        }

        Value Value::operator << (const Value& rhs) const {
            return genBinaryOp(m_func, this, rhs, ir_shl, ir_shl, ir_noop, ir_noop, "operator <<");
        }

        Value Value::operator <<=(const Value& rhs) {
            return genBinaryOp(m_func, this, rhs, ir_shl, ir_shl, ir_noop, ir_noop, "operator <<", true);
        }

        Value Value::operator >> (const Value& rhs) const {
            return genBinaryOp(m_func, this, rhs, ir_shr, ir_shr, ir_noop, ir_noop, "operator >>");
        }

        Value Value::operator >>=(const Value& rhs) {
            return genBinaryOp(m_func, this, rhs, ir_shr, ir_shr, ir_noop, ir_noop, "operator >>", true);
        }

        Value Value::operator != (const Value& rhs) const {
            return genBinaryOp(m_func, this, rhs, ir_ineq, ir_uneq, ir_fneq, ir_dneq, "operator !=");
        }

        Value Value::operator && (const Value& rhs) const {
            return genBinaryOp(m_func, this, rhs, ir_land, ir_land, ir_land, ir_land, "operator &&");
        }

        Value Value::operator || (const Value& rhs) const {
            return genBinaryOp(m_func, this, rhs, ir_lor, ir_lor, ir_lor, ir_lor, "operator ||");
        }

        Value Value::operator =  (const Value& rhs) {
            return genBinaryOp(m_func, this, rhs, ir_set, ir_set, ir_set, ir_set, "operator =");
        }

        Value Value::operator == (const Value& rhs) const {
            return genBinaryOp(m_func, this, rhs, ir_ieq, ir_ueq, ir_feq, ir_deq, "operator ==");
        }

        Value Value::operator <  (const Value& rhs) const {
            return genBinaryOp(m_func, this, rhs, ir_ilt, ir_ult, ir_flt, ir_dlt, "operator <");
        }

        Value Value::operator <= (const Value& rhs) const {
            return genBinaryOp(m_func, this, rhs, ir_ilte, ir_ulte, ir_flte, ir_dlte, "operator <=");
        }

        Value Value::operator >  (const Value& rhs) const {
            return genBinaryOp(m_func, this, rhs, ir_igt, ir_ugt, ir_fgt, ir_dgt, "operator >");
        }

        Value Value::operator >= (const Value& rhs) const {
            return genBinaryOp(m_func, this, rhs, ir_igte, ir_ugte, ir_fgte, ir_dgte, "operator >=");
        }

        Value Value::operator [] (const Value& rhs) const {
            // todo: If script is trusted then allow offsetting from
            //       pointers. Otherwise error if that behavior is
            //       attempted.

            const ffi::DataType* rtp = rhs.getType();
            utils::Array<ffi::Function*> matches = m_type->findMethods("operator []", nullptr, &rtp, 1, fm_skip_implicit_args);
            if (matches.size() == 1) {
                return generateCall(m_func->getCompiler(), matches[0], { rhs }, this);
            } else if (matches.size() > 1) {
                u8 strictC = 0;
                ffi::Function* func = nullptr;
                for (u32 i = 0;i < matches.size();i++) {
                    const auto& args = matches[i]->getSignature()->getArguments();
                    for (u32 a = 0;a < args.size();a++) {
                        if (args[a].isImplicit()) continue;
                        if (args[a].dataType == rtp) {
                            if (args.size() > a + 1) {
                                // Function has more arguments which are not explicitly required
                                break;
                            }

                            strictC++;
                            func = matches[i];
                        }
                    }
                }
                
                if (strictC == 1) {
                    return generateCall(m_func->getCompiler(), func, { rhs }, this);
                }

                // todo: Error, ambiguous call
            } else {
                // todo: Error, no matches
            }

            return m_func->getPoison();
        }

        Value Value::operator () (const utils::Array<Value>& args) const {
            utils::Array<ffi::DataType*> argTps = args.map([](const Value& v) { return v.m_type; });
            utils::Array<ffi::Function*> matches = m_type->findMethods("operator ()", nullptr, const_cast<const ffi::DataType**>(argTps.data()), argTps.size(), fm_skip_implicit_args);
            if (matches.size() == 1) {
                return generateCall(m_func->getCompiler(), matches[0], args, this);
            } else if (matches.size() > 1) {
                u8 strictC = 0;
                ffi::Function* func = nullptr;
                for (u32 i = 0;i < matches.size();i++) {
                    const auto& margs = matches[i]->getSignature()->getArguments();
                    u32 nonImplicitCount = 0;
                    for (u32 a = 0;a < margs.size();a++) {
                        if (margs[a].isImplicit() || margs[a].dataType != argTps[nonImplicitCount]) continue;
                        nonImplicitCount++;
                        if (nonImplicitCount == args.size() && margs.size() == a + 1) {
                            strictC++;
                            func = matches[i];
                        }
                    }
                }
                
                if (strictC == 1) {
                    return generateCall(m_func->getCompiler(), func, args, this);
                }

                // todo: Error, ambiguous call
            } else {
                // todo: Error, no matches
            }

            return m_func->getPoison();
        }

        Value Value::operator -  () {
            return genUnaryOp(m_func, this, ir_ineg, ir_noop, ir_fneg, ir_dneg, "operator -");
        }

        Value Value::operator -- () {
            return genUnaryOp(m_func, this, ir_idec, ir_udec, ir_fdec, ir_ddec, "operator --");
        }

        Value Value::operator -- (int) {
            return genUnaryOp(m_func, this, ir_idec, ir_udec, ir_fdec, ir_ddec, "operator --", true);
        }

        Value Value::operator ++ () {
            return genUnaryOp(m_func, this, ir_iinc, ir_uinc, ir_finc, ir_dinc, "operator ++");
        }

        Value Value::operator ++ (int) {
            return genUnaryOp(m_func, this, ir_iinc, ir_uinc, ir_finc, ir_dinc, "operator ++", true);
        }

        Value Value::operator !  () const {
            return genUnaryOp(m_func, this, ir_not, ir_not, ir_not, ir_not, "operator !");
        }

        Value Value::operator ~  () const {
            return genUnaryOp(m_func, this, ir_inv, ir_inv, ir_noop, ir_noop, "operator ~");
        }

        Value Value::operator_logicalAndAssign(const Value& rhs) {
            return genBinaryOp(m_func, this, rhs, ir_land, ir_land, ir_land, ir_land, "operator &&", true);
        }

        Value Value::operator_logicalOrAssign(const Value& rhs) {
            return genBinaryOp(m_func, this, rhs, ir_lor, ir_lor, ir_lor, ir_lor, "operator ||", true);
        }
    };
};