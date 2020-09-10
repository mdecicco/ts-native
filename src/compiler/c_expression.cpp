#include <compiler/context.h>
#include <compiler/compile.h>
#include <compiler/function.h>
#include <parser/ast.h>
#include <common/errors.h>
#include <common/warnings.h>
#include <common/compile_log.h>
#include <bind/bind.h>
#include <vm/vm_type.h>

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
        }

        var operation(context& ctx, ast* n) {
            switch (n->op) {
                case ot::invalid: {
                    break;
                }
                case ot::add:            return expression(ctx, n->lvalue) + expression(ctx, n->rvalue);
                case ot::sub:            return expression(ctx, n->lvalue) - expression(ctx, n->rvalue);
                case ot::mul:            return expression(ctx, n->lvalue) * expression(ctx, n->rvalue);
                case ot::div:            return expression(ctx, n->lvalue) / expression(ctx, n->rvalue);
                case ot::mod:            return expression(ctx, n->lvalue) % expression(ctx, n->rvalue);
                case ot::band:           return expression(ctx, n->lvalue) & expression(ctx, n->rvalue);
                case ot::bor:            return expression(ctx, n->lvalue) | expression(ctx, n->rvalue);
                case ot::bxor:           return expression(ctx, n->lvalue) ^ expression(ctx, n->rvalue);
                case ot::less:           return expression(ctx, n->lvalue) < expression(ctx, n->rvalue);
                case ot::greater:        return expression(ctx, n->lvalue) > expression(ctx, n->rvalue);
                case ot::shiftLeft:      return expression(ctx, n->lvalue) << expression(ctx, n->rvalue);
                case ot::shiftRight:     return expression(ctx, n->lvalue) >> expression(ctx, n->rvalue);
                case ot::land:           return expression(ctx, n->lvalue) && expression(ctx, n->rvalue);
                case ot::lor:            return expression(ctx, n->lvalue) || expression(ctx, n->rvalue);
                case ot::lessEq:         return expression(ctx, n->lvalue) <= expression(ctx, n->rvalue);
                case ot::greaterEq:      return expression(ctx, n->lvalue) >= expression(ctx, n->rvalue);
                case ot::notEq:          return expression(ctx, n->lvalue) != expression(ctx, n->rvalue);
                case ot::isEq:           return expression(ctx, n->lvalue) == expression(ctx, n->rvalue);
                case ot::addEq:          return expression(ctx, n->lvalue) += expression(ctx, n->rvalue);
                case ot::subEq:          return expression(ctx, n->lvalue) -= expression(ctx, n->rvalue);
                case ot::mulEq:          return expression(ctx, n->lvalue) *= expression(ctx, n->rvalue);
                case ot::divEq:          return expression(ctx, n->lvalue) /= expression(ctx, n->rvalue);
                case ot::modEq:          return expression(ctx, n->lvalue) %= expression(ctx, n->rvalue);
                case ot::bandEq:         return expression(ctx, n->lvalue) &= expression(ctx, n->rvalue);
                case ot::borEq:          return expression(ctx, n->lvalue) |= expression(ctx, n->rvalue);
                case ot::bxorEq:         return expression(ctx, n->lvalue) ^= expression(ctx, n->rvalue);
                case ot::shiftLeftEq:    return expression(ctx, n->lvalue) <<= expression(ctx, n->rvalue);
                case ot::shiftRightEq:   return expression(ctx, n->lvalue) >>= expression(ctx, n->rvalue);
                case ot::eq:             return expression(ctx, n->lvalue).operator_eq(expression(ctx, n->rvalue));
                case ot::lorEq:          return expression(ctx, n->lvalue).operator_lorEq(expression(ctx, n->rvalue));
                case ot::landEq:         return expression(ctx, n->lvalue).operator_landEq(expression(ctx, n->rvalue));
                case ot::index:          return expression(ctx, n->lvalue)[expression(ctx, n->rvalue)];
                case ot::not:            return !expression(ctx, n->rvalue);
                case ot::negate:         return -expression(ctx, n->rvalue);
                case ot::preInc:         return ++expression(ctx, n->rvalue);
                case ot::preDec:         return --expression(ctx, n->rvalue);
                case ot::postInc:        return expression(ctx, n->lvalue)++;
                case ot::postDec:        return expression(ctx, n->lvalue)--;
                case ot::member: {
                    // note: function_call compiler will handle the case when rvalue
                    // is an identifier that references a class method
                    if (n->lvalue->type == nt::type_identifier) {
                        ctx.push_node(n->lvalue);
                        vm_type* tp = ctx.type(n->lvalue);
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
                    return expression(ctx, n->lvalue).prop(*n->rvalue);
                    break;
                }
                case ot::newObj: {
                    ctx.push_node(n);
                    vm_type* tp = ctx.type(n->data_type);
                    var obj = ctx.empty_var(tp);

                    std::vector<var> args;

                    ast* a = n->arguments;
                    while (a) {
                        var arg = expression(ctx, a);
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
                        var arg = expression(ctx, a);
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

        var expression(context& ctx, ast* n) {
            switch (n->type) {
                case nt::call: return function_call(ctx, n);
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
                            var v = ctx.imm(n->value.s);
                            ctx.pop_node();
                            return v;
                        }
                    }
                }
                case nt::conditional: {
                    ctx.push_node(n);
                    auto& b = ctx.add(operation::branch).operand(expression(ctx, n->condition));
                    var tmp = expression(ctx, n->lvalue);
                    var result = ctx.empty_var(tmp.type());
                    ctx.add(operation::eq).operand(result).operand(tmp);
                    auto& j = ctx.add(operation::jump);
                    b.operand(ctx.imm(ctx.code.size()));
                    ctx.add(operation::eq).operand(result).operand(expression(ctx, n->rvalue).convert(tmp.type()));
                    j.operand(ctx.imm(ctx.code.size()));
                    
                    ctx.pop_node();
                    return result;
                }
                case nt::operation: return operation(ctx, n);
                default: {
                }
            }

            return ctx.error_var();
        }
    };
};