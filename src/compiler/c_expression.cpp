#include <compiler/context.h>
#include <compiler/compile.h>
#include <compiler/function.h>
#include <parser/ast.h>
#include <common/errors.h>
#include <common/warnings.h>
#include <common/compile_log.h>
#include <bind/bind.h>
#include <common/script_type.h>
#include <common/module.h>
#include <builtin/script_buffer.h>

namespace gjs {
    using namespace parse;
    using exc = error::exception;
    using ec = error::ecode;
    using wc = warning::wcode;
    using nt = ast::node_type;
    using ot = ast::operation_type;
    using ct = ast::constant_type;

    namespace compile {
        bool is_assignment(ast* expr) {
            if (expr->type == nt::operation) {
                switch (expr->op) {
                    case ot::addEq:
                    case ot::subEq:
                    case ot::mulEq:
                    case ot::divEq:
                    case ot::modEq:
                    case ot::shiftLeftEq:
                    case ot::shiftRightEq:
                    case ot::landEq:
                    case ot::lorEq:
                    case ot::bandEq:
                    case ot::borEq:
                    case ot::bxorEq:
                    case ot::preInc:
                    case ot::preDec:
                    case ot::postInc:
                    case ot::postDec:
                    case ot::eq: return true;
                    default: return false;
                }
                return false;
            }

            return false;
        }

        var expression_inner(context& ctx, ast* n);

        var operation(context& ctx, ast* n) {
            switch (n->op) {
                case ot::invalid: {
                    break;
                }
                case ot::add:            return expression_inner(ctx, n->lvalue) + expression_inner(ctx, n->rvalue);
                case ot::sub:            return expression_inner(ctx, n->lvalue) - expression_inner(ctx, n->rvalue);
                case ot::mul:            return expression_inner(ctx, n->lvalue) * expression_inner(ctx, n->rvalue);
                case ot::div:            return expression_inner(ctx, n->lvalue) / expression_inner(ctx, n->rvalue);
                case ot::mod:            return expression_inner(ctx, n->lvalue) % expression_inner(ctx, n->rvalue);
                case ot::band:           return expression_inner(ctx, n->lvalue) & expression_inner(ctx, n->rvalue);
                case ot::bor:            return expression_inner(ctx, n->lvalue) | expression_inner(ctx, n->rvalue);
                case ot::bxor:           return expression_inner(ctx, n->lvalue) ^ expression_inner(ctx, n->rvalue);
                case ot::less:           return expression_inner(ctx, n->lvalue) < expression_inner(ctx, n->rvalue);
                case ot::greater:        return expression_inner(ctx, n->lvalue) > expression_inner(ctx, n->rvalue);
                case ot::shiftLeft:      return expression_inner(ctx, n->lvalue) << expression_inner(ctx, n->rvalue);
                case ot::shiftRight:     return expression_inner(ctx, n->lvalue) >> expression_inner(ctx, n->rvalue);
                case ot::land:           return expression_inner(ctx, n->lvalue) && expression_inner(ctx, n->rvalue);
                case ot::lor:            return expression_inner(ctx, n->lvalue) || expression_inner(ctx, n->rvalue);
                case ot::lessEq:         return expression_inner(ctx, n->lvalue) <= expression_inner(ctx, n->rvalue);
                case ot::greaterEq:      return expression_inner(ctx, n->lvalue) >= expression_inner(ctx, n->rvalue);
                case ot::notEq:          return expression_inner(ctx, n->lvalue) != expression_inner(ctx, n->rvalue);
                case ot::isEq:           return expression_inner(ctx, n->lvalue) == expression_inner(ctx, n->rvalue);
                case ot::addEq:          return expression_inner(ctx, n->lvalue) += expression_inner(ctx, n->rvalue);
                case ot::subEq:          return expression_inner(ctx, n->lvalue) -= expression_inner(ctx, n->rvalue);
                case ot::mulEq:          return expression_inner(ctx, n->lvalue) *= expression_inner(ctx, n->rvalue);
                case ot::divEq:          return expression_inner(ctx, n->lvalue) /= expression_inner(ctx, n->rvalue);
                case ot::modEq:          return expression_inner(ctx, n->lvalue) %= expression_inner(ctx, n->rvalue);
                case ot::bandEq:         return expression_inner(ctx, n->lvalue) &= expression_inner(ctx, n->rvalue);
                case ot::borEq:          return expression_inner(ctx, n->lvalue) |= expression_inner(ctx, n->rvalue);
                case ot::bxorEq:         return expression_inner(ctx, n->lvalue) ^= expression_inner(ctx, n->rvalue);
                case ot::shiftLeftEq:    return expression_inner(ctx, n->lvalue) <<= expression_inner(ctx, n->rvalue);
                case ot::shiftRightEq:   return expression_inner(ctx, n->lvalue) >>= expression_inner(ctx, n->rvalue);
                case ot::eq:             return expression_inner(ctx, n->lvalue).operator_eq(expression_inner(ctx, n->rvalue));
                case ot::lorEq:          return expression_inner(ctx, n->lvalue).operator_lorEq(expression_inner(ctx, n->rvalue));
                case ot::landEq:         return expression_inner(ctx, n->lvalue).operator_landEq(expression_inner(ctx, n->rvalue));
                case ot::index:          return expression_inner(ctx, n->lvalue)[expression_inner(ctx, n->rvalue)];
                case ot::not:            return !expression_inner(ctx, n->rvalue);
                case ot::negate:         return -expression_inner(ctx, n->rvalue);
                case ot::preInc:         return ++expression_inner(ctx, n->rvalue);
                case ot::preDec:         return --expression_inner(ctx, n->rvalue);
                case ot::postInc:        return expression_inner(ctx, n->lvalue)++;
                case ot::postDec:        return expression_inner(ctx, n->lvalue)--;
                case ot::member: {
                    // note: function_call compiler will handle the case when rvalue
                    // is an identifier that references a class method
                    if (n->lvalue->type == nt::type_identifier) {
                        ctx.push_node(n->lvalue);
                        script_type* tp = ctx.type(n->lvalue);
                        ctx.pop_node();
                        std::string pname = *n->rvalue;
                        ctx.push_node(n->rvalue);
                        var dummy = ctx.dummy_var(tp);
                        var ret = dummy.prop(pname);
                        ctx.pop_node();

                        // Checks if it has the property after because var::prop
                        // will log an error about the property not existing, which
                        // is desired. But if it doesn't exist, it is not desired
                        // to log a second error about a non-existent property not
                        // being static.
                        if (dummy.has_prop(pname) && !ret.flag(bind::pf_static)) {
                            ctx.log()->err(ec::c_class_property_not_static, n->rvalue->ref, tp->name.c_str(), pname.c_str());
                        }
                        return ret;
                    }
                    return expression_inner(ctx, n->lvalue).prop(*n->rvalue);
                    break;
                }
                case ot::newObj: {
                    ctx.push_node(n);
                    script_type* tp = ctx.type(n->data_type);
                    var obj = ctx.empty_var(tp);

                    std::vector<var> args;

                    ast* a = n->arguments;
                    while (a) {
                        var arg = expression_inner(ctx, a);
                        args.push_back(arg);
                        a = a->next;
                    }

                    construct_in_memory(ctx, obj, args);
                    ctx.pop_node();
                    return obj;
                    break;
                }
                case ot::stackObj: {
                    ctx.push_node(n->data_type);
                    var obj = ctx.empty_var(ctx.type(n->data_type));
                    std::vector<var> args;

                    ast* a = n->arguments;
                    while (a) {
                        var arg = expression_inner(ctx, a);
                        args.push_back(arg);
                        a = a->next;
                    }

                    construct_on_stack(ctx, obj, args);

                    ctx.pop_node();

                    obj.raise_stack_flag();
                    return obj;
                    break;
                }
            }

            ctx.push_node(n);
            var ret = ctx.error_var();
            ctx.pop_node();
            return ret;
        }

        var expression_inner(context& ctx, ast* n) {
            switch (n->type) {
                case nt::call: {
                    ctx.push_node(n);
                    var ret = function_call(ctx, n);
                    ctx.pop_node();
                    return ret;
                }
                case nt::identifier: {
                    ctx.push_node(n);
                    var v = ctx.get_var(*n);
                    ctx.pop_node();
                    return v;
                }
                case nt::constant: {
                    ctx.push_node(n);
                    switch (n->c_type) {
                        case ct::integer: {
                            var v = ctx.imm(n->value.i);
                            ctx.pop_node();
                            return v;
                        }
                        case ct::f32: {
                            var v = ctx.imm(n->value.f_32);
                            ctx.pop_node();
                            return v;
                        }
                        case ct::f64: {
                            var v = ctx.imm(n->value.f_64);
                            ctx.pop_node();
                            return v;
                        }
                        case ct::string: {
                            u64 off = ctx.out.mod->data()->position();
                            u32 len = strlen(n->value.s);
                            ctx.out.mod->data()->write(n->value.s, len);

                            var d = ctx.empty_var(ctx.type("data"));
                            ctx.add(operation::module_data).operand(d).operand(ctx.imm((u64)ctx.out.mod->id())).operand(ctx.imm(off));

                            var v = ctx.empty_var(ctx.type("string"));
                            construct_on_stack(ctx, v, { d, ctx.imm((u64)len) });
                            v.raise_stack_flag();
                            ctx.pop_node();
                            return v;
                        }
                    }
                }
                case nt::conditional: {
                    ctx.push_node(n);
                    auto& b = ctx.add(operation::branch).operand(expression_inner(ctx, n->condition));
                    var tmp = expression_inner(ctx, n->lvalue);
                    var result = ctx.empty_var(tmp.type());
                    result.operator_eq(tmp);
                    auto& j = ctx.add(operation::jump);
                    b.operand(ctx.imm(ctx.code_sz()));
                    result.operator_eq(expression_inner(ctx, n->rvalue));
                    j.operand(ctx.imm(ctx.code_sz()));
                    
                    ctx.pop_node();
                    return result;
                }
                case nt::operation: {
                    ctx.push_node(n);
                    var result = operation(ctx, n);
                    ctx.pop_node();
                    return result;
                }
                default: {
                }
            }

            return ctx.error_var();
        }

        var expression(context& ctx, ast* n) {
            ctx.push_block();
            var ret = expression_inner(ctx, n);
            ctx.pop_block(ret);

            return ret;
        }
    };
};