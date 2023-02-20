#include "tojson.h"

#include <tsn/common/Context.h>
#include <tsn/common/Module.h>
#include <tsn/ffi/Function.h>
#include <tsn/ffi/DataType.h>
#include <tsn/ffi/DataTypeRegistry.h>
#include <tsn/compiler/Logger.h>
#include <tsn/compiler/Compiler.h>
#include <tsn/compiler/OutputBuilder.h>
#include <tsn/compiler/FunctionDef.h>
#include <tsn/compiler/Parser.h>
#include <tsn/compiler/TemplateContext.h>
#include <tsn/utils/SourceLocation.h>
#include <tsn/utils/ModuleSource.h>
#include <tsn/io/Workspace.h>

#include <utils/String.h>
#include <utils/Array.hpp>

using namespace tsn;
using namespace compiler;
using namespace ffi;
using namespace utils;
using namespace nlohmann;

json toJson(const SourceLocation& src) {
    if (!src.isValid()) return json(nullptr);

    const SourceLocation& end = src.getEndLocation();
    u32 baseOffset = src.getOffset();
    u32 endOffset = end.getOffset();
    u32 wsc = 0;
    String ln = "";
    String indxStr = "";

    if (src.getSource()) {
        ln = src.getSource()->getLine(src.getLine()).clone();
        ln.replaceAll("\n", "");
        ln.replaceAll("\r", "");
        for (;wsc < ln.size();wsc++) {
            if (isspace(ln[wsc])) continue;
            break;
        }

        for (u32 i = 0;i < (src.getCol() - wsc);i++) indxStr += ' ';
        for (u32 i = 0;i < (endOffset - baseOffset) && indxStr.size() < (ln.size() - wsc);i++) {
            indxStr += '^';
        }
    }
    json rangeStart, rangeEnd;
    rangeStart["line"] = src.getLine();
    rangeStart["col"] = src.getCol();
    rangeStart["offset"] = baseOffset;
    rangeEnd["line"] = end.getLine();
    rangeEnd["col"] = end.getCol();
    rangeEnd["offset"] = endOffset;

    json range;
    range["start"] = rangeStart;
    range["end"] = rangeEnd;

    json out;
    out["range"] = range;
    out["line_txt"] = ln.c_str() + wsc;
    out["line_und"] = indxStr.c_str();

    if (src.getSource()) out["file"] = src.getSource()->getMeta()->path;
    else out["file"] = json(nullptr);

    return out;
}

json toJson(const log_message& log) {
    constexpr const char* logTps[] = { "debug", "info", "warning", "error" };
    json out;
    char prefix = 'U';
    if (log.code < IO_MESSAGES_START) prefix = 'G';
    else if (log.code > IO_MESSAGES_START && log.code < IO_MESSAGES_END) prefix = 'I';
    else if (log.code > PARSE_MESSAGES_START && log.code < PARSE_MESSAGES_END) prefix = 'P';
    else if (log.code > COMPILER_MESSAGES_START && log.code < COMPILER_MESSAGES_END) prefix = 'C';

    out["code"] = String::Format("%c%d", prefix, log.code);
    out["type"] = logTps[log.type];
    out["message"] = std::string(log.msg.c_str(), log.msg.size());
    out["src"] = toJson(log.src);
    // out["ast"] = toJson(log.node);

    return out;
}

json toJson(const ParseNode* n, bool isArrayElem) {
    if (!n) return json(nullptr);

    if (n->next && !isArrayElem) {
        json out = json::array();
        
        while (n) {
            out.push_back(toJson(n, true));
            n = n->next;
        }

        return out;
    }

    constexpr const char* nts[] = {
        "empty",
        "error",
        "root",
        "eos",
        "break",
        "cast",
        "catch",
        "class",
        "continue",
        "delete",
        "export",
        "expression",
        "expression_sequence",
        "function_type",
        "function",
        "identifier",
        "if",
        "import_module",
        "import_symbol",
        "import",
        "literal",
        "loop",
        "object_decompositor",
        "object_literal_property",
        "parameter",
        "property",
        "return",
        "scoped_block",
        "sizeof",
        "switch_case",
        "switch",
        "this",
        "throw",
        "try",
        "type_modifier",
        "type_property",
        "type_specifier",
        "type",
        "variable"
    };
    constexpr const char* ops[] = {
        "undefined",
        "add",
        "addEq",
        "sub",
        "subEq",
        "mul",
        "mulEq",
        "div",
        "divEq",
        "mod",
        "modEq",
        "xor",
        "xorEq",
        "bitAnd",
        "bitAndEq",
        "bitOr",
        "bitOrEq",
        "bitInv",
        "shLeft",
        "shLeftEq",
        "shRight",
        "shRightEq",
        "not",
        "notEq",
        "logAnd",
        "logAndEq",
        "logOr",
        "logOrEq",
        "assign",
        "compare",
        "lessThan",
        "lessThanEq",
        "greaterThan",
        "greaterThanEq",
        "preInc",
        "postInc",
        "preDec",
        "postDec",
        "negate",
        "dereference",
        "index",
        "conditional",
        "member",
        "new",
        "placementNew",
        "call"
    };
    constexpr const char* lts[] = {
        "u8",
        "u16",
        "u32",
        "u64",
        "i8",
        "i16",
        "i32",
        "i64",
        "f32",
        "f64",
        "string",
        "template_string",
        "object",
        "array",
        "null",
        "true",
        "false"
    };

    json out;
    out["type"] = nts[n->tp];
    out["source"] = toJson(n->tok.src);

    if (n->tp == nt_literal) {
        out["value_type"] = lts[n->value_tp];
        switch (n->value_tp) {
            case lt_u8:
            case lt_u16:
            case lt_u32:
            case lt_u64: {
                out["value"] = n->value.u;
                break;
            }
            case lt_i8:
            case lt_i16:
            case lt_i32:
            case lt_i64: {
                out["value"] = n->value.i;
                break;
            }
            case lt_f32:
            case lt_f64: {
                out["value"] = n->value.f;
                break;
            }
            case lt_string:
            case lt_template_string: {
                String s = n->str();
                out["value"] = std::string(s.c_str(), s.size());
                break;
            }
            case lt_object:
            case lt_array: {
                out["value"] = toJson(n->body);
                break;
            }
            case lt_null: {
                out["value"] = json(nullptr);
                break;
            }
            case lt_true: {
                out["value"] = true;
                break;
            }
            case lt_false: {
                out["value"] = false;
                break;
            }
        }
        out["text"] = json(nullptr);
    } else {
        out["value_type"] = json(nullptr);
        out["value"] = json(nullptr);
        String s = n->str();
        out["text"] = std::string(s.c_str(), s.size());
    }

    json flags = json::array();
    if (n->flags.is_const) flags.push_back("is_const");
    if (n->flags.is_static) flags.push_back("is_static");
    if (n->flags.is_private) flags.push_back("is_private");
    if (n->flags.is_array) flags.push_back("is_array");
    if (n->flags.is_pointer) flags.push_back("is_pointer");
    if (n->flags.defer_cond) flags.push_back("defer_cond");
    out["flags"] = flags;

    if (n->data_type) out["data_type"] = toJson(n->data_type);
    if (n->lvalue) out["lvalue"] = toJson(n->lvalue);
    if (n->rvalue) out["rvalue"] = toJson(n->rvalue);
    if (n->cond) out["cond"] = toJson(n->cond);
    if (n->body) out["body"] = toJson(n->body);
    if (n->else_body) out["else_body"] = toJson(n->else_body);
    if (n->initializer) out["initializer"] = toJson(n->initializer);
    if (n->parameters) out["parameters"] = toJson(n->parameters);
    if (n->template_parameters) out["template_parameters"] = toJson(n->template_parameters);
    if (n->modifier) out["modifier"] = toJson(n->modifier);
    if (n->alias) out["alias"] = toJson(n->alias);
    if (n->inheritance) out["extends"] = toJson(n->inheritance);

    return out;
}

json toJson(const symbol_lifetime& s) {
    const char* type = "value";

    String detail = "";
    if (s.sym->isFunction()) {
        type = "function";
        if (s.sym->isImm()) {
            FunctionDef* fn = s.sym->getImm<FunctionDef*>();
            if (fn->getOutput()) detail = fn->getOutput()->getFullyQualifiedName();
            else detail = fn->getName();
        } else {
            detail = s.sym->getType()->getFullyQualifiedName();
        }
    } else if (s.sym->getFlags().is_module) {
        type = "module";
        detail = s.sym->getImm<Module*>()->getName();
    } else if (s.sym->getFlags().is_type) {
        type = "type";
        detail = s.sym->getType()->getFullyQualifiedName();
    } else {
        detail = s.sym->getType()->getFullyQualifiedName();
    }

    json out;
    out["name"] = s.name.c_str();
    out["type"] = type;
    out["detail"] = detail.c_str();

    json range, rangeStart, rangeEnd;
    range = json(nullptr);
    rangeStart = json(nullptr);
    rangeEnd = json(nullptr);
    
    if (s.begin.isValid()) {
        rangeStart["line"] = s.begin.getLine();
        rangeStart["col"] = s.begin.getCol();
        rangeStart["offset"] = s.begin.getOffset();
    }

    if (s.end.isValid()) {
        rangeEnd["line"] = s.end.getLine();
        rangeEnd["col"] = s.end.getCol();
        rangeEnd["offset"] = s.end.getOffset();
    }

    if (s.begin.isValid() || s.end.isValid()) {
        range = json();
        range["start"] = rangeStart;
        range["end"] = rangeEnd;
    }

    out["range"] = range;

    return out;
}

json toJson(const DataType* t, bool brief) {
    if (!t) return json(nullptr);
    constexpr const char* access[] = { "public", "private", "trusted" };

    const auto& info = t->getInfo();

    json out;
    out["id"] = t->getId();
    out["name"] = t->getName().c_str();
    out["fully_qualified_name"] = t->getFullyQualifiedName().c_str();

    if (brief) return out;

    out["size"] = info.size;
    out["host_hash"] = info.host_hash;
    out["access"] = access[t->getAccessModifier()];
    out["destructor"] = toJson(t->getDestructor(), true);

    json flags = json::array();
    if (info.is_pod) flags.push_back("is_pod");
    if (info.is_trivially_constructible) flags.push_back("is_trivially_constructible");
    if (info.is_trivially_copyable) flags.push_back("is_trivially_copyable");
    if (info.is_trivially_destructible) flags.push_back("is_trivially_destructible");
    if (info.is_primitive) flags.push_back("is_primitive");
    if (info.is_floating_point) flags.push_back("is_floating_point");
    if (info.is_integral) flags.push_back("is_integral");
    if (info.is_unsigned) flags.push_back("is_unsigned");
    if (info.is_function) flags.push_back("is_function");
    if (info.is_template) flags.push_back("is_template");
    if (info.is_alias) flags.push_back("is_alias");
    if (info.is_host) flags.push_back("is_host");
    if (info.is_anonymous) flags.push_back("is_anonymous");

    if (info.is_alias) out["alias_of"] = toJson(((AliasType*)t)->getRefType(), true);
    else out["alias_of"] = json(nullptr);

    out["inherits"] = json::array();
    const auto& bases = t->getBases();
    for (u32 i = 0;i < bases.size();i++) {
        out["inherits"].push_back(toJson(bases[i]));
    }

    out["properties"] = json::array();
    const auto& props = t->getProperties();
    for (u32 i = 0;i < props.size();i++) {
        out["properties"].push_back(toJson(props[i]));
    }

    out["methods"] = json::array();
    const auto& methods = t->getMethods();
    for (u32 i = 0;i < props.size();i++) {
        out["methods"].push_back(toJson(methods[i], true));
    }

    if (info.is_template) {
        TemplateContext* tctx = ((TemplateType*)t)->getTemplateData();
        out["ast"] = toJson(tctx->getAST());
    }
    else out["ast"] = json(nullptr);

    return out;
}

json toJson(const Function* f, bool brief, FunctionDef* fd) {
    constexpr const char* access[] = { "public", "private", "trusted" };
    if (!f) return json(nullptr);

    json out;

    out["id"] = f->getId();
    out["name"] = f->getName().c_str();
    out["display_name"] = f->getDisplayName().c_str();
    out["fully_qualified_name"] = f->getFullyQualifiedName().c_str();
    if (brief) return out;

    out["access"] = access[f->getAccessModifier()];
    out["is_method"] = f->isMethod();
    out["is_template"] = f->isTemplate();

    if (f->isTemplate()) {
        out["signature"] = json(nullptr);
        out["is_thiscall"] = json(nullptr);
        out["args"] = json(nullptr);
        out["code"] = json(nullptr);

        if (f->isMethod()) {
            TemplateContext* tctx = ((TemplateMethod*)f)->getTemplateData();
            out["ast"] = toJson(tctx->getAST());
        } else {
            TemplateContext* tctx = ((TemplateFunction*)f)->getTemplateData();
            out["ast"] = toJson(tctx->getAST());
        }
    } else {
        out["signature"] = toJson(f->getSignature(), true);
        out["is_thiscall"] = f->isThisCall();
        out["args"] = json::array();
        const auto& args = f->getSignature()->getArguments();
        for (u32 i = 0;i < args.size();i++) out["args"].push_back(toJson(i, args[i], fd));

        out["code"] = json::array();
        if (fd) {
            const auto& code = fd->getCode();
            for (u32 i = 0;i < code.size();i++) {
                out["code"].push_back(code[i].toString(fd->getContext()).c_str());
            }
        }

        out["ast"] = json(nullptr);
    }

    return out;
}

json toJson(const type_base& b) {
    constexpr const char* access[] = { "public", "private", "trusted" };
    json out;
    
    out["type"] = toJson(b.type, true);
    out["access"] = access[b.access];
    out["offset"] = b.offset;

    return out;
}

json toJson(const type_property& p) {
    constexpr const char* access[] = { "public", "private", "trusted" };
    json out;

    out["name"] = p.name.c_str();
    out["type"] = toJson(p.type, true);
    out["offset"] = p.offset;
    out["access"] = access[p.access];
    out["getter"] = toJson(p.getter, true);
    out["setter"] = toJson(p.setter, true);
    out["flags"] = json::array();

    if (p.flags.is_static) out["flags"].push_back("is_static");
    if (p.flags.is_pointer) out["flags"].push_back("is_pointer");
    if (p.flags.can_read) out["flags"].push_back("can_read");
    if (p.flags.can_write) out["flags"].push_back("can_write");

    return out;
}

json toJson(u32 index, const function_argument& a, FunctionDef* fd) {
    constexpr const char* argTpStr[] = {
        "func_ptr",
        "ret_ptr",
        "ectx_ptr",
        "this_ptr",
        "value",
        "pointer"
    };

    json out;

    out["arg_type"] = argTpStr[u32(a.argType)];
    out["is_implicit"] = a.isImplicit();
    out["data_type"] = toJson(a.dataType, true);

    if (fd) {
        json loc;
        if (a.argType == arg_type::context_ptr) {
            loc = fd->getECtx().toString().c_str();
        } else if (a.argType == arg_type::func_ptr) {
            loc = fd->getFPtr().toString().c_str();
        } else if (a.argType == arg_type::ret_ptr) {
            if (fd->getReturnType()->isEqualTo(fd->getContext()->getTypes()->getType<void>())) loc = json(nullptr);
            else loc = fd->getRetPtr().toString().c_str();
        } else if (a.argType == arg_type::this_ptr) {
            if (fd->getThisType()->isEqualTo(fd->getContext()->getTypes()->getType<void>())) loc = json(nullptr);
            else loc = fd->getThis().toString().c_str();
        } else {
            loc = fd->getArg(index - fd->getImplicitArgCount()).toString().c_str();
        }
        out["location"] = loc;
    } else out["location"] = json(nullptr);

    return out;
}

json toJson(u32 slot, const module_data& d) {
    constexpr const char* access[] = { "public", "private", "trusted" };
    json out;

    out["slot"] = slot;
    out["name"] = d.name.c_str();
    out["access"] = access[d.access];
    out["type"] = toJson(d.type, true);

    return out;
}