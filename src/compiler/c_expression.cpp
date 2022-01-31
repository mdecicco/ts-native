#include <gjs/compiler/context.h>
#include <gjs/compiler/compile.h>
#include <gjs/compiler/function.h>
#include <gjs/parser/ast.h>
#include <gjs/common/errors.h>
#include <gjs/common/warnings.h>
#include <gjs/common/compile_log.h>
#include <gjs/bind/bind.h>
#include <gjs/common/script_type.h>
#include <gjs/common/script_module.h>
#include <gjs/common/script_enum.h>
#include <gjs/builtin/script_buffer.h>

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

        var eval_member_op(context& ctx, ast* n) {
            // [type].[static var]
            // [module].[type].[static var]
            // [module].[global var]
            // [module].[enum].[name]
            // [enum].[name]
            // [expression | var].[property]

            if (n->lvalue->type == nt::type_identifier) {
                // [type].[static var]
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
                if (dummy.has_prop(pname) && !ret.flag(property_flags::pf_static)) {
                    ctx.log()->err(ec::c_class_property_not_static, n->rvalue->ref, tp->name.c_str(), pname.c_str());
                }
                return ret;
            }

            if (n->lvalue->type == nt::identifier) {
                std::string name = *n->lvalue;

                symbol_table* mod = ctx.symbols.get_module(name);
                if (mod) {
                    std::string vname = *n->rvalue;
                    symbol_list* syms = mod->get(vname);
                    if (syms) {
                        for (symbol& sym : syms->symbols) {
                            if (sym.sym_type() == symbol::symbol_type::st_modulevar) {
                                // [module].[global var]
                                const script_module::local_var* v = sym.get_modulevar();
                                var ret = ctx.empty_var(v->type);
                                ret.set_code_ref(v->ref);
                                ctx.push_node(n);
                                if (v->type->is_primitive) {
                                    var ptr = ctx.empty_var(ctx.type("data"));
                                    ctx.add(operation::module_data).operand(ptr).operand(ctx.imm((u64)mod->module->id())).operand(ctx.imm(v->offset));
                                    ret.set_mem_ptr(ptr);
                                    ctx.add(operation::load).operand(ret).operand(ptr);
                                } else {
                                    ctx.add(operation::module_data).operand(ret).operand(ctx.imm((u64)mod->module->id())).operand(ctx.imm(v->offset));
                                }
                                ctx.pop_node();

                                return ret;
                            }
                        }
                    }
                }

                symbol_table* _enum = ctx.symbols.get_enum(name);
                if (_enum) {
                    std::string vname = *n->rvalue;
                    // there should only be one of these, but I'll do it this way for semantics
                    symbol_list* syms = _enum->get(vname);
                    if (syms) {
                        for (symbol& sym : syms->symbols) {
                            if (sym.sym_type() == symbol::symbol_type::st_var) {
                                // [enum].[name]
                                return *sym.get_var();
                            }
                        }
                    }
                }

                symbol_list* any = ctx.symbols.get(name);
                if (!any || any->symbols.size() == 0) {
                    if (_enum) {
                        ctx.log()->err(ec::c_no_such_enum_value, n->lvalue->ref, name.c_str(), std::string(*n->rvalue).c_str());
                    } else if (mod) {
                        symbol_list* syms = mod->get(*n->rvalue);
                        if (syms) {
                            symbol& last = syms->symbols.back();
                            switch (last.sym_type()) {
                                case symbol::symbol_type::st_enum: {
                                    ctx.log()->err(ec::c_enum_is_not_value, n->lvalue->ref);
                                    break;
                                }
                                case symbol::symbol_type::st_function: {
                                    ctx.log()->err(ec::c_func_is_not_value, n->lvalue->ref);
                                    break;
                                }
                                case symbol::symbol_type::st_type: {
                                    ctx.log()->err(ec::c_type_is_not_value, n->lvalue->ref);
                                    break;
                                }
                                default: { break; }
                            }
                        }
                        else ctx.log()->err(ec::c_symbol_not_found_in_module, n->lvalue->ref, std::string(*n->rvalue).c_str(), name.c_str());
                    }
                    return ctx.error_var();
                }
            }

            if (n->lvalue->type == nt::operation && n->lvalue->op == ot::member) {
                if (n->lvalue->lvalue->type == nt::identifier) {
                    std::string llname = *n->lvalue->lvalue;
                    std::string lrname = *n->lvalue->rvalue;
                    std::string vname = *n->rvalue;

                    symbol_table* mod = ctx.symbols.get_module(llname);
                    if (mod) {
                        symbol_table* tbl = mod->get_type(lrname);
                        if (tbl) {
                            script_type* tp = tbl->type;
                            ctx.push_node(n->rvalue);
                            var dummy = ctx.dummy_var(tp);
                            if (dummy.has_prop(vname)) {
                                // [module].[type].[static var]
                                var ret = dummy.prop(vname);
                                ctx.pop_node();
                                return ret;
                            }
                            ctx.pop_node();
                        }

                        tbl = mod->get_enum(lrname);
                        if (tbl) {
                            symbol_list* syms = tbl->get(vname);
                            for (symbol& sym : syms->symbols) {
                                if (sym.sym_type() == symbol::symbol_type::st_var) {
                                    // [module].[enum].[name]
                                    return *sym.get_var();
                                }
                            }
                        }
                    }
                }
            }

            // [expression | var].[property]
            ctx.push_node(n->rvalue);
            var expr = expression_inner(ctx, n->lvalue);
            var ret = expr.prop(*n->rvalue);
            ctx.pop_node();
            return ret;
        }

        var operation(context& ctx, ast* n) {
            switch (n->op) {
                case ot::invalid:        break;
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
                case ot::_not:           return !expression_inner(ctx, n->rvalue);
                case ot::negate:         return -expression_inner(ctx, n->rvalue);
                case ot::preInc:         return ++expression_inner(ctx, n->rvalue);
                case ot::preDec:         return --expression_inner(ctx, n->rvalue);
                case ot::postInc:        return expression_inner(ctx, n->lvalue)++;
                case ot::postDec:        return expression_inner(ctx, n->lvalue)--;
                case ot::member:         return eval_member_op(ctx, n);
                case ot::newObj: {
                    ctx.push_node(n);
                    script_type* tp = ctx.type(n->data_type);
                    var obj = ctx.empty_var(tp);
                    if (n->body) {
                        // new format
                        construct_in_memory(ctx, obj, {});

                        ast* field = n->body->body;
                        var ptr = ctx.empty_var(ctx.type("u64"));
                        while (field) {
                            auto info = tp->prop(*field->identifier);
                            if (info) {
                                ctx.add(operation::uadd).operand(ptr).operand(obj).operand(ctx.imm(info->offset));
                                var val = expression_inner(ctx, field);
                                if (info->type->is_primitive) {
                                    var vc = val.convert(info->type, true);
                                    ctx.add(operation::store).operand(ptr).operand(vc);
                                }
                                else construct_in_place(ctx, ctx.force_cast_var(ptr, info->type), { val });
                            } else {
                                ctx.log()->err(ec::c_no_such_property, field->ref, tp->name.c_str(), std::string(*field->identifier).c_str());
                            }

                            field = field->next;
                        }
                    } else {
                        // new class
                        std::vector<var> args;

                        ast* a = n->arguments;
                        while (a) {
                            var arg = expression_inner(ctx, a);
                            args.push_back(arg);
                            a = a->next;
                        }

                        construct_in_memory(ctx, obj, args);
                    }

                    ctx.pop_node();
                    return obj;
                    break;
                }
                case ot::stackObj: {
                    ctx.push_node(n->data_type);
                    script_type* tp = ctx.type(n->data_type);
                    var obj = ctx.empty_var(tp);

                    if (n->body) {
                        // stack format
                        obj.raise_stack_flag();
                        construct_on_stack(ctx, obj, {});

                        ast* field = n->body->body;
                        var ptr = ctx.empty_var(ctx.type("u64"));
                        while (field) {
                            auto info = tp->prop(*field->identifier);
                            if (info) {
                                ctx.add(operation::uadd).operand(ptr).operand(obj).operand(ctx.imm(info->offset));
                                var val = expression_inner(ctx, field->initializer);
                                if (info->type->is_primitive) {
                                    var vc = val.convert(info->type, true);
                                    ctx.add(operation::store).operand(ptr).operand(vc);
                                }
                                else construct_in_place(ctx, ctx.force_cast_var(ptr, info->type), { val });
                            } else {
                                ctx.log()->err(ec::c_no_such_property, field->ref, tp->name.c_str(), std::string(*field->identifier).c_str());
                            }

                            field = field->next;
                        }
                    } else {
                        // stack class
                        std::vector<var> args;

                        ast* a = n->arguments;
                        while (a) {
                            var arg = expression_inner(ctx, a);
                            args.push_back(arg);
                            a = a->next;
                        }

                        obj.raise_stack_flag();
                        construct_on_stack(ctx, obj, args);
                    }

                    ctx.pop_node();

                    return obj;
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
                            u64 len = strlen(n->value.s);
                            ctx.out.mod->data()->write(n->value.s, len);

                            var d = ctx.empty_var(ctx.type("data"));
                            ctx.add(operation::module_data).operand(d).operand(ctx.imm((u64)ctx.out.mod->id())).operand(ctx.imm(off));

                            var v = ctx.empty_var(ctx.type("string"));
                            v.raise_stack_flag();
                            construct_on_stack(ctx, v, { d, ctx.imm((u64)len) });
                            ctx.pop_node();
                            return v;
                        }
                    }
                }
                case nt::conditional: {
                    ctx.push_node(n);
                    var cond = expression_inner(ctx, n->condition);
                    //auto& meta = ctx.add(operation::meta_if_branch);
                    auto& b = ctx.add(operation::branch).operand(cond);

                    // truth body begin
                    var tmp0 = expression_inner(ctx, n->lvalue);
                    var result = ctx.empty_var(tmp0.type());
                    result.operator_eq(tmp0);
                    //meta.label(ctx.label());
                    tac_wrapper j = ctx.add(operation::jump);
                    // truth body end

                    b.label(ctx.label());

                    // false body begin
                    var tmp1 = expression_inner(ctx, n->rvalue);
                    result.operator_eq(tmp1);
                    //meta.label(ctx.label());
                    // false body end

                    // join address
                    j.label(ctx.label());
                    //meta.label(ctx.label());
                    
                    ctx.pop_node();
                    return result;
                }
                case nt::operation: {
                    ctx.push_node(n);
                    var result = operation(ctx, n);
                    ctx.pop_node();
                    return result;
                }
                case nt::lambda_expression: {
                    return lambda_expression(ctx, n);
                }
                default: {
                }
            }

            return ctx.error_var();
        }

        var expression(context& ctx, ast* n) {
            ctx.push_node(n);
            ctx.push_block();
            var ret = expression_inner(ctx, n);
            ctx.pop_block(ret);
            ctx.pop_node();

            return ret;
        }
    };
};
