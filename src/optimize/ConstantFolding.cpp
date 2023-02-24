#include <tsn/optimize/ConstantFolding.h>
#include <tsn/compiler/CodeHolder.h>
#include <tsn/optimize/OptimizationGroup.h>
#include <tsn/compiler/IR.h>
#include <tsn/compiler/Value.hpp>
#include <tsn/compiler/Logger.h>
#include <tsn/ffi/DataType.h>
#include <tsn/ffi/DataTypeRegistry.h>
#include <tsn/common/Context.h>
#include <tsn/common/Config.h>
#include <tsn/ffi/Function.h>
#include <tsn/pipeline/Pipeline.h>

namespace tsn {
    namespace optimize {
        using namespace compiler;
        using op = ir_instruction;

        template <typename T, typename S> T add(T a, S b) { return T(a + b); }
        template <typename T, typename S> T sub(T a, S b) { return T(a - b); }
        template <typename T, typename S> T mul(T a, S b) { return T(a * b); }
        template <typename T, typename S> T div(T a, S b) { return T(a / b); }
        template <typename T, typename S> T mod(T a, S b) {
            if constexpr (std::is_integral_v<T>) {
                if constexpr (std::is_integral_v<S>) return T(a % b);
                else return T(fmod(a, b));
            }
            else return T(fmod(a, b));
        }
        template <typename T, typename S> bool lt(T a, S b) { return a < b; }
        template <typename T, typename S> bool lte(T a, S b) { return a <= b; }
        template <typename T, typename S> bool gt(T a, S b) { return a > b; }
        template <typename T, typename S> bool gte(T a, S b) { return a >= b; }
        template <typename T, typename S> bool eq(T a, S b) { return a == b; }
        template <typename T, typename S> bool neq(T a, S b) { return a != b; }
        template <typename T> T neg(T a) { return -a; }
        template <typename T> bool not_(T a) { return !a; }
        template <typename T> T inv(T a) {
            if constexpr (std::is_integral_v<T>) return ~a;
            else { u64 t = ~(*(u64*)&a); return *(f64*)&t; }
        }
        template <typename T, typename S> T shl(T a, S b) {
            if constexpr (std::is_integral_v<T>) return a << u32(b);
            else { u64 t = (*(u64*)&a) << u32(b); return *(f64*)&t; }
        }
        template <typename T, typename S> T shr(T a, S b) {
            if constexpr (std::is_integral_v<T>) return a >> u32(b);
            else { u64 t = (*(u64*)&a) >> u32(b); return *(f64*)&t; }
        }
        template <typename T, typename S> bool land(T a, S b) { return a && b; }
        template <typename T, typename S> T band(T a, S b) {
            if constexpr (std::is_integral_v<T>) {
                if constexpr (std::is_integral_v<S>) return T(a & b);
                else return T(a & (*(u64*)&b));
            } else {
                if constexpr (std::is_integral_v<S>) return T((*(u64*)&a) & b);
                else return T((*(u64*)&a) & (*(u64*)&b));
            }
        }
        template <typename T, typename S> bool lor(T a, S b) { return a || b; }
        template <typename T, typename S> T bor(T a, S b) {
            if constexpr (std::is_integral_v<T>) {
                if constexpr (std::is_integral_v<S>) return T(a | b);
                else return T(a | (*(u64*)&b));
            } else {
                if constexpr (std::is_integral_v<S>) return T((*(u64*)&a) | b);
                else return T((*(u64*)&a) | (*(u64*)&b));
            }
        }
        template <typename T, typename S> T xor_(T a, S b) {
            if constexpr (std::is_integral_v<T>) {
                if constexpr (std::is_integral_v<S>) return a ^ b;
                else return a ^ (*(u64*)&b);
            } else {
                if constexpr (std::is_integral_v<S>) return T((*(u64*)&a) ^ b);
                else return T((*(u64*)&a) ^ (*(u64*)&b));
            }
        }

        ConstantFoldingStep::ConstantFoldingStep(Context* ctx) : IOptimizationStep(ctx) {
        }

        ConstantFoldingStep::~ConstantFoldingStep() {
        }

        bool ConstantFoldingStep::execute(compiler::CodeHolder* ch, Pipeline* pipeline) {
            Logger* log = pipeline->getLogger();
            bool doDebug = m_ctx->getConfig()->debugLogging;
            bool didChange = false;

            if (doDebug) {
                log->submit(
                    lt_debug,
                    lm_generic,
                    utils::String::Format("ConstantFoldingStep: Analyzing %s", ch->owner->getFullyQualifiedName().c_str())
                );
            }

            for (address c = 0;c < ch->code.size();c++) {
                Instruction& i = ch->code[c];
                const auto& info = instruction_info(i.op);
                bool foundMatch = false;

                Value result;
                result.getFlags().is_immediate = true;

                if (i.oCnt == 3 && info.assigns_operand_index == 0) {
                    Value& a = i.operands[1];
                    Value& b = i.operands[2];
                    if (!a.isImm() || !b.isImm()) continue;

                    const auto& ai = a.getType()->getInfo();
                    const auto& bi = b.getType()->getInfo();
                    result.setType(a.getType());

                    switch (i.op) {
                        case ir_iadd:
                        case ir_uadd:
                        case ir_fadd:
                        case ir_dadd: {
                            if (ai.is_floating_point) {
                                if (bi.is_floating_point) result.setImm(add(a.getImm<f64>(), b.getImm<f64>()));
                                else if (bi.is_unsigned) result.setImm(add(a.getImm<f64>(), b.getImm<u64>()));
                                else result.setImm(add(a.getImm<f64>(), b.getImm<i64>()));
                            } else if (ai.is_unsigned) {
                                if (bi.is_floating_point) result.setImm(add(a.getImm<u64>(), b.getImm<f64>()));
                                else if (bi.is_unsigned) result.setImm(add(a.getImm<u64>(), b.getImm<u64>()));
                                else result.setImm(add(a.getImm<u64>(), b.getImm<i64>()));
                            } else {
                                if (bi.is_floating_point) result.setImm(add(a.getImm<i64>(), b.getImm<f64>()));
                                else if (bi.is_unsigned) result.setImm(add(a.getImm<i64>(), b.getImm<u64>()));
                                else result.setImm(add(a.getImm<i64>(), b.getImm<i64>()));
                            }
                            break;
                        }
                        case ir_isub:
                        case ir_usub:
                        case ir_fsub:
                        case ir_dsub: {
                            if (ai.is_floating_point) {
                                if (bi.is_floating_point) result.setImm(sub(a.getImm<f64>(), b.getImm<f64>()));
                                else if (bi.is_unsigned) result.setImm(sub(a.getImm<f64>(), b.getImm<u64>()));
                                else result.setImm(sub(a.getImm<f64>(), b.getImm<i64>()));
                            } else if (ai.is_unsigned) {
                                if (bi.is_floating_point) result.setImm(sub(a.getImm<u64>(), b.getImm<f64>()));
                                else if (bi.is_unsigned) result.setImm(sub(a.getImm<u64>(), b.getImm<u64>()));
                                else result.setImm(sub(a.getImm<u64>(), b.getImm<i64>()));
                            } else {
                                if (bi.is_floating_point) result.setImm(sub(a.getImm<i64>(), b.getImm<f64>()));
                                else if (bi.is_unsigned) result.setImm(sub(a.getImm<i64>(), b.getImm<u64>()));
                                else result.setImm(sub(a.getImm<i64>(), b.getImm<i64>()));
                            }
                            break;
                        }
                        case ir_imul:
                        case ir_umul:
                        case ir_fmul:
                        case ir_dmul: {
                            if (ai.is_floating_point) {
                                if (bi.is_floating_point) result.setImm(mul(a.getImm<f64>(), b.getImm<f64>()));
                                else if (bi.is_unsigned) result.setImm(mul(a.getImm<f64>(), b.getImm<u64>()));
                                else result.setImm(mul(a.getImm<f64>(), b.getImm<i64>()));
                            } else if (ai.is_unsigned) {
                                if (bi.is_floating_point) result.setImm(mul(a.getImm<u64>(), b.getImm<f64>()));
                                else if (bi.is_unsigned) result.setImm(mul(a.getImm<u64>(), b.getImm<u64>()));
                                else result.setImm(mul(a.getImm<u64>(), b.getImm<i64>()));
                            } else {
                                if (bi.is_floating_point) result.setImm(mul(a.getImm<i64>(), b.getImm<f64>()));
                                else if (bi.is_unsigned) result.setImm(mul(a.getImm<i64>(), b.getImm<u64>()));
                                else result.setImm(mul(a.getImm<i64>(), b.getImm<i64>()));
                            }
                            break;
                        }
                        case ir_idiv:
                        case ir_udiv:
                        case ir_fdiv:
                        case ir_ddiv: {
                            if (ai.is_floating_point) {
                                if (bi.is_floating_point) result.setImm(div(a.getImm<f64>(), b.getImm<f64>()));
                                else if (bi.is_unsigned) result.setImm(div(a.getImm<f64>(), b.getImm<u64>()));
                                else result.setImm(div(a.getImm<f64>(), b.getImm<i64>()));
                            } else if (ai.is_unsigned) {
                                if (bi.is_floating_point) result.setImm(div(a.getImm<u64>(), b.getImm<f64>()));
                                else if (bi.is_unsigned) result.setImm(div(a.getImm<u64>(), b.getImm<u64>()));
                                else result.setImm(div(a.getImm<u64>(), b.getImm<i64>()));
                            } else {
                                if (bi.is_floating_point) result.setImm(div(a.getImm<i64>(), b.getImm<f64>()));
                                else if (bi.is_unsigned) result.setImm(div(a.getImm<i64>(), b.getImm<u64>()));
                                else result.setImm(div(a.getImm<i64>(), b.getImm<i64>()));
                            }
                            break;
                        }
                        case ir_imod:
                        case ir_umod:
                        case ir_fmod:
                        case ir_dmod: {
                            if (ai.is_floating_point) {
                                if (bi.is_floating_point) result.setImm(mod(a.getImm<f64>(), b.getImm<f64>()));
                                else if (bi.is_unsigned) result.setImm(mod(a.getImm<f64>(), b.getImm<u64>()));
                                else result.setImm(mod(a.getImm<f64>(), b.getImm<i64>()));
                            } else if (ai.is_unsigned) {
                                if (bi.is_floating_point) result.setImm(mod(a.getImm<u64>(), b.getImm<f64>()));
                                else if (bi.is_unsigned) result.setImm(mod(a.getImm<u64>(), b.getImm<u64>()));
                                else result.setImm(mod(a.getImm<u64>(), b.getImm<i64>()));
                            } else {
                                if (bi.is_floating_point) result.setImm(mod(a.getImm<i64>(), b.getImm<f64>()));
                                else if (bi.is_unsigned) result.setImm(mod(a.getImm<i64>(), b.getImm<u64>()));
                                else result.setImm(mod(a.getImm<i64>(), b.getImm<i64>()));
                            }
                            break;
                        }
                        case ir_ilt:
                        case ir_ult:
                        case ir_flt:
                        case ir_dlt: {
                            if (ai.is_floating_point) {
                                if (bi.is_floating_point) result.setImm(lt(a.getImm<f64>(), b.getImm<f64>()));
                                else if (bi.is_unsigned) result.setImm(lt(a.getImm<f64>(), b.getImm<u64>()));
                                else result.setImm(lt(a.getImm<f64>(), b.getImm<i64>()));
                            } else if (ai.is_unsigned) {
                                if (bi.is_floating_point) result.setImm(lt(a.getImm<u64>(), b.getImm<f64>()));
                                else if (bi.is_unsigned) result.setImm(lt(a.getImm<u64>(), b.getImm<u64>()));
                                else result.setImm(lt(a.getImm<u64>(), b.getImm<i64>()));
                            } else {
                                if (bi.is_floating_point) result.setImm(lt(a.getImm<i64>(), b.getImm<f64>()));
                                else if (bi.is_unsigned) result.setImm(lt(a.getImm<i64>(), b.getImm<u64>()));
                                else result.setImm(lt(a.getImm<i64>(), b.getImm<i64>()));
                            }
                            result.setType(m_ctx->getTypes()->getBoolean());
                            break;
                        }
                        case ir_ilte:
                        case ir_ulte:
                        case ir_flte:
                        case ir_dlte: {
                            if (ai.is_floating_point) {
                                if (bi.is_floating_point) result.setImm(lte(a.getImm<f64>(), b.getImm<f64>()));
                                else if (bi.is_unsigned) result.setImm(lte(a.getImm<f64>(), b.getImm<u64>()));
                                else result.setImm(lte(a.getImm<f64>(), b.getImm<i64>()));
                            } else if (ai.is_unsigned) {
                                if (bi.is_floating_point) result.setImm(lte(a.getImm<u64>(), b.getImm<f64>()));
                                else if (bi.is_unsigned) result.setImm(lte(a.getImm<u64>(), b.getImm<u64>()));
                                else result.setImm(lte(a.getImm<u64>(), b.getImm<i64>()));
                            } else {
                                if (bi.is_floating_point) result.setImm(lte(a.getImm<i64>(), b.getImm<f64>()));
                                else if (bi.is_unsigned) result.setImm(lte(a.getImm<i64>(), b.getImm<u64>()));
                                else result.setImm(lte(a.getImm<i64>(), b.getImm<i64>()));
                            }
                            result.setType(m_ctx->getTypes()->getBoolean());
                            break;
                        }
                        case ir_igt:
                        case ir_ugt:
                        case ir_fgt:
                        case ir_dgt: {
                            if (ai.is_floating_point) {
                                if (bi.is_floating_point) result.setImm(gt(a.getImm<f64>(), b.getImm<f64>()));
                                else if (bi.is_unsigned) result.setImm(gt(a.getImm<f64>(), b.getImm<u64>()));
                                else result.setImm(gt(a.getImm<f64>(), b.getImm<i64>()));
                            } else if (ai.is_unsigned) {
                                if (bi.is_floating_point) result.setImm(gt(a.getImm<u64>(), b.getImm<f64>()));
                                else if (bi.is_unsigned) result.setImm(gt(a.getImm<u64>(), b.getImm<u64>()));
                                else result.setImm(gt(a.getImm<u64>(), b.getImm<i64>()));
                            } else {
                                if (bi.is_floating_point) result.setImm(gt(a.getImm<i64>(), b.getImm<f64>()));
                                else if (bi.is_unsigned) result.setImm(gt(a.getImm<i64>(), b.getImm<u64>()));
                                else result.setImm(gt(a.getImm<i64>(), b.getImm<i64>()));
                            }
                            result.setType(m_ctx->getTypes()->getBoolean());
                            break;
                        }
                        case ir_igte:
                        case ir_ugte:
                        case ir_fgte:
                        case ir_dgte: {
                            if (ai.is_floating_point) {
                                if (bi.is_floating_point) result.setImm(gte(a.getImm<f64>(), b.getImm<f64>()));
                                else if (bi.is_unsigned) result.setImm(gte(a.getImm<f64>(), b.getImm<u64>()));
                                else result.setImm(gte(a.getImm<f64>(), b.getImm<i64>()));
                            } else if (ai.is_unsigned) {
                                if (bi.is_floating_point) result.setImm(gte(a.getImm<u64>(), b.getImm<f64>()));
                                else if (bi.is_unsigned) result.setImm(gte(a.getImm<u64>(), b.getImm<u64>()));
                                else result.setImm(gte(a.getImm<u64>(), b.getImm<i64>()));
                            } else {
                                if (bi.is_floating_point) result.setImm(gte(a.getImm<i64>(), b.getImm<f64>()));
                                else if (bi.is_unsigned) result.setImm(gte(a.getImm<i64>(), b.getImm<u64>()));
                                else result.setImm(gte(a.getImm<i64>(), b.getImm<i64>()));
                            }
                            result.setType(m_ctx->getTypes()->getBoolean());
                            break;
                        }
                        case ir_ieq:
                        case ir_ueq:
                        case ir_feq:
                        case ir_deq: {
                            if (ai.is_floating_point) {
                                if (bi.is_floating_point) result.setImm(eq(a.getImm<f64>(), b.getImm<f64>()));
                                else if (bi.is_unsigned) result.setImm(eq(a.getImm<f64>(), b.getImm<u64>()));
                                else result.setImm(eq(a.getImm<f64>(), b.getImm<i64>()));
                            } else if (ai.is_unsigned) {
                                if (bi.is_floating_point) result.setImm(eq(a.getImm<u64>(), b.getImm<f64>()));
                                else if (bi.is_unsigned) result.setImm(eq(a.getImm<u64>(), b.getImm<u64>()));
                                else result.setImm(eq(a.getImm<u64>(), b.getImm<i64>()));
                            } else {
                                if (bi.is_floating_point) result.setImm(eq(a.getImm<i64>(), b.getImm<f64>()));
                                else if (bi.is_unsigned) result.setImm(eq(a.getImm<i64>(), b.getImm<u64>()));
                                else result.setImm(eq(a.getImm<i64>(), b.getImm<i64>()));
                            }
                            result.setType(m_ctx->getTypes()->getBoolean());
                            break;
                        }
                        case ir_ineq:
                        case ir_uneq:
                        case ir_fneq:
                        case ir_dneq: {
                            if (ai.is_floating_point) {
                                if (bi.is_floating_point) result.setImm(neq(a.getImm<f64>(), b.getImm<f64>()));
                                else if (bi.is_unsigned) result.setImm(neq(a.getImm<f64>(), b.getImm<u64>()));
                                else result.setImm(neq(a.getImm<f64>(), b.getImm<i64>()));
                            } else if (ai.is_unsigned) {
                                if (bi.is_floating_point) result.setImm(neq(a.getImm<u64>(), b.getImm<f64>()));
                                else if (bi.is_unsigned) result.setImm(neq(a.getImm<u64>(), b.getImm<u64>()));
                                else result.setImm(neq(a.getImm<u64>(), b.getImm<i64>()));
                            } else {
                                if (bi.is_floating_point) result.setImm(neq(a.getImm<i64>(), b.getImm<f64>()));
                                else if (bi.is_unsigned) result.setImm(neq(a.getImm<i64>(), b.getImm<u64>()));
                                else result.setImm(neq(a.getImm<i64>(), b.getImm<i64>()));
                            }
                            result.setType(m_ctx->getTypes()->getBoolean());
                            break;
                        }
                        case ir_shl: {
                            if (ai.is_floating_point) {
                                if (bi.is_floating_point) result.setImm(shl(a.getImm<f64>(), b.getImm<f64>()));
                                else if (bi.is_unsigned) result.setImm(shl(a.getImm<f64>(), b.getImm<u64>()));
                                else result.setImm(shl(a.getImm<f64>(), b.getImm<i64>()));
                            } else if (ai.is_unsigned) {
                                if (bi.is_floating_point) result.setImm(shl(a.getImm<u64>(), b.getImm<f64>()));
                                else if (bi.is_unsigned) result.setImm(shl(a.getImm<u64>(), b.getImm<u64>()));
                                else result.setImm(shl(a.getImm<u64>(), b.getImm<i64>()));
                            } else {
                                if (bi.is_floating_point) result.setImm(shl(a.getImm<i64>(), b.getImm<f64>()));
                                else if (bi.is_unsigned) result.setImm(shl(a.getImm<i64>(), b.getImm<u64>()));
                                else result.setImm(shl(a.getImm<i64>(), b.getImm<i64>()));
                            }
                            break;
                        }
                        case ir_shr: {
                            if (ai.is_floating_point) {
                                if (bi.is_floating_point) result.setImm(shr(a.getImm<f64>(), b.getImm<f64>()));
                                else if (bi.is_unsigned) result.setImm(shr(a.getImm<f64>(), b.getImm<u64>()));
                                else result.setImm(shr(a.getImm<f64>(), b.getImm<i64>()));
                            } else if (ai.is_unsigned) {
                                if (bi.is_floating_point) result.setImm(shr(a.getImm<u64>(), b.getImm<f64>()));
                                else if (bi.is_unsigned) result.setImm(shr(a.getImm<u64>(), b.getImm<u64>()));
                                else result.setImm(shr(a.getImm<u64>(), b.getImm<i64>()));
                            } else {
                                if (bi.is_floating_point) result.setImm(shr(a.getImm<i64>(), b.getImm<f64>()));
                                else if (bi.is_unsigned) result.setImm(shr(a.getImm<i64>(), b.getImm<u64>()));
                                else result.setImm(shr(a.getImm<i64>(), b.getImm<i64>()));
                            }
                            break;
                        }
                        case ir_land: {
                            if (ai.is_floating_point) {
                                if (bi.is_floating_point) result.setImm(land(a.getImm<f64>(), b.getImm<f64>()));
                                else if (bi.is_unsigned) result.setImm(land(a.getImm<f64>(), b.getImm<u64>()));
                                else result.setImm(land(a.getImm<f64>(), b.getImm<i64>()));
                            } else if (ai.is_unsigned) {
                                if (bi.is_floating_point) result.setImm(land(a.getImm<u64>(), b.getImm<f64>()));
                                else if (bi.is_unsigned) result.setImm(land(a.getImm<u64>(), b.getImm<u64>()));
                                else result.setImm(land(a.getImm<u64>(), b.getImm<i64>()));
                            } else {
                                if (bi.is_floating_point) result.setImm(land(a.getImm<i64>(), b.getImm<f64>()));
                                else if (bi.is_unsigned) result.setImm(land(a.getImm<i64>(), b.getImm<u64>()));
                                else result.setImm(land(a.getImm<i64>(), b.getImm<i64>()));
                            }
                            result.setType(m_ctx->getTypes()->getBoolean());
                            break;
                        }
                        case ir_band: {
                            if (ai.is_floating_point) {
                                if (bi.is_floating_point) result.setImm(band(a.getImm<f64>(), b.getImm<f64>()));
                                else if (bi.is_unsigned) result.setImm(band(a.getImm<f64>(), b.getImm<u64>()));
                                else result.setImm(band(a.getImm<f64>(), b.getImm<i64>()));
                            } else if (ai.is_unsigned) {
                                if (bi.is_floating_point) result.setImm(band(a.getImm<u64>(), b.getImm<f64>()));
                                else if (bi.is_unsigned) result.setImm(band(a.getImm<u64>(), b.getImm<u64>()));
                                else result.setImm(band(a.getImm<u64>(), b.getImm<i64>()));
                            } else {
                                if (bi.is_floating_point) result.setImm(band(a.getImm<i64>(), b.getImm<f64>()));
                                else if (bi.is_unsigned) result.setImm(band(a.getImm<i64>(), b.getImm<u64>()));
                                else result.setImm(band(a.getImm<i64>(), b.getImm<i64>()));
                            }
                            break;
                        }
                        case ir_lor: {
                            if (ai.is_floating_point) {
                                if (bi.is_floating_point) result.setImm(lor(a.getImm<f64>(), b.getImm<f64>()));
                                else if (bi.is_unsigned) result.setImm(lor(a.getImm<f64>(), b.getImm<u64>()));
                                else result.setImm(lor(a.getImm<f64>(), b.getImm<i64>()));
                            } else if (ai.is_unsigned) {
                                if (bi.is_floating_point) result.setImm(lor(a.getImm<u64>(), b.getImm<f64>()));
                                else if (bi.is_unsigned) result.setImm(lor(a.getImm<u64>(), b.getImm<u64>()));
                                else result.setImm(lor(a.getImm<u64>(), b.getImm<i64>()));
                            } else {
                                if (bi.is_floating_point) result.setImm(lor(a.getImm<i64>(), b.getImm<f64>()));
                                else if (bi.is_unsigned) result.setImm(lor(a.getImm<i64>(), b.getImm<u64>()));
                                else result.setImm(lor(a.getImm<i64>(), b.getImm<i64>()));
                            }
                            result.setType(m_ctx->getTypes()->getBoolean());
                            break;
                        }
                        case ir_bor: {
                            if (ai.is_floating_point) {
                                if (bi.is_floating_point) result.setImm(bor(a.getImm<f64>(), b.getImm<f64>()));
                                else if (bi.is_unsigned) result.setImm(bor(a.getImm<f64>(), b.getImm<u64>()));
                                else result.setImm(bor(a.getImm<f64>(), b.getImm<i64>()));
                            } else if (ai.is_unsigned) {
                                if (bi.is_floating_point) result.setImm(bor(a.getImm<u64>(), b.getImm<f64>()));
                                else if (bi.is_unsigned) result.setImm(bor(a.getImm<u64>(), b.getImm<u64>()));
                                else result.setImm(bor(a.getImm<u64>(), b.getImm<i64>()));
                            } else {
                                if (bi.is_floating_point) result.setImm(bor(a.getImm<i64>(), b.getImm<f64>()));
                                else if (bi.is_unsigned) result.setImm(bor(a.getImm<i64>(), b.getImm<u64>()));
                                else result.setImm(bor(a.getImm<i64>(), b.getImm<i64>()));
                            }
                            break;
                        }
                        case ir_xor: {
                            if (ai.is_floating_point) {
                                if (bi.is_floating_point) result.setImm(xor_(a.getImm<f64>(), b.getImm<f64>()));
                                else if (bi.is_unsigned) result.setImm(xor_(a.getImm<f64>(), b.getImm<u64>()));
                                else result.setImm(xor_(a.getImm<f64>(), b.getImm<i64>()));
                            } else if (ai.is_unsigned) {
                                if (bi.is_floating_point) result.setImm(xor_(a.getImm<u64>(), b.getImm<f64>()));
                                else if (bi.is_unsigned) result.setImm(xor_(a.getImm<u64>(), b.getImm<u64>()));
                                else result.setImm(xor_(a.getImm<u64>(), b.getImm<i64>()));
                            } else {
                                if (bi.is_floating_point) result.setImm(xor_(a.getImm<i64>(), b.getImm<f64>()));
                                else if (bi.is_unsigned) result.setImm(xor_(a.getImm<i64>(), b.getImm<u64>()));
                                else result.setImm(xor_(a.getImm<i64>(), b.getImm<i64>()));
                            }
                            break;
                        }
                        default: continue;
                    }
                    
                    foundMatch = true;
                }

                if (i.oCnt == 2 && info.assigns_operand_index == 0) {
                    Value& v = i.operands[1];
                    if (!v.isImm()) continue;

                    const auto& ti = v.getType()->getInfo();
                    result.setType(v.getType());

                    switch (i.op) {
                        case ir_ineg:
                        case ir_fneg:
                        case ir_dneg: {
                            if (ti.is_floating_point) result.setImm(neg(v.getImm<f64>()));
                            else if (ti.is_unsigned) result.setImm(v.getImm<u64>());
                            else result.setImm(neg(v.getImm<i64>()));
                            break;
                        }
                        case ir_not: {
                            if (ti.is_floating_point) result.setImm(not_(v.getImm<f64>()));
                            else if (ti.is_unsigned) result.setImm(not_(v.getImm<u64>()));
                            else result.setImm(not_(v.getImm<i64>()));
                            result.setType(m_ctx->getTypes()->getBoolean());
                            break;
                        }
                        case ir_inv: {
                            if (ti.is_floating_point) result.setImm(inv(v.getImm<f64>()));
                            else if (ti.is_unsigned) result.setImm(inv(v.getImm<u64>()));
                            else result.setImm(inv(v.getImm<i64>()));
                            break;
                        }
                        default: continue;
                    }

                    foundMatch = true;
                }

                if (!foundMatch) continue;

                if (doDebug) {
                    log->submit(
                        lt_debug,
                        lm_generic,
                        utils::String::Format("[%lu] %s <- constant-only operation", i.toString(m_ctx))
                    );
                }

                i.op = ir_assign;
                i.operands[1].reset(result);
                i.operands[2].reset(Value());
                i.oCnt = 2;

                if (doDebug) {
                    log->submit(
                        lt_debug,
                        lm_generic,
                        utils::String::Format("^ [%lu] %s (updated)", i.toString(m_ctx))
                    );
                }
            }

            if (didChange) getGroup()->setShouldRepeat(true);
            return false;
        }
    };
};