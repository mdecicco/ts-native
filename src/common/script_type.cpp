#include <gjs/common/script_function.h>
#include <gjs/common/script_type.h>
#include <gjs/common/script_context.h>
#include <gjs/common/script_module.h>
#include <gjs/common/function_signature.h>
#include <gjs/util/util.h>
#include <gjs/bind/bind.h>

namespace gjs {
    script_type::script_type() {
        m_id = 0;
        size = 0;
        is_pod = false;
        is_primitive = false;
        is_floating_point = false;
        is_unsigned = false;
        is_builtin = false;
        is_host = false;
        is_trivially_copyable = false;
        requires_subtype = false;
        owner = nullptr;
        destructor = nullptr;
        m_wrapped = nullptr;
        base_type = nullptr;
        sub_type = nullptr;
        signature = nullptr;
    }

    script_type::~script_type() {
        if (m_wrapped) delete m_wrapped;
        if (signature) delete signature;
    }

    bool script_type::has_any_method(const std::string& _name) const {
        for (u16 m = 0;m < methods.size();m++) {
            // match name
            script_function* func = nullptr;
            if (_name.find_first_of(' ') != std::string::npos) {
                // probably an operator
                std::vector<std::string> mparts = split(split(methods[m]->name, ":")[1], " \t\n\r");
                std::vector<std::string> sparts = split(_name, " \t\n\r");
                if (mparts.size() != sparts.size()) continue;
                bool matched = true;
                for (u32 i = 0;matched && i < mparts.size();i++) {
                    matched = mparts[i] == sparts[i];
                }
                if (matched) return true;
            }
            const std::string& bt_name = base_type ? base_type->name : name;
            if (methods[m]->name == bt_name + "::" + _name) return true;
        }

        return false;
    }

    bool script_type::has_unambiguous_method(const std::string& _name, script_type* ret, const std::vector<script_type*>& args) const {
        std::vector<script_function*> matches;

        for (u16 m = 0;m < methods.size();m++) {
            // match name
            script_function* func = nullptr;
            if (_name == "<cast>") {
                script_function* mt = methods[m];
                auto mparts = split(split(mt->name,":")[1], " \t\n\r");
                if (mparts.size() == 2 && mparts[0] == "operator" && mparts[1] == mt->type->signature->return_type->name && mt->type->signature->explicit_argc == 1) {
                    if (mt->type->signature->return_type->id() == ret->id()) {
                        return mt;
                    } else if (mt->type->signature->return_type->is_convertible_to(ret)) {
                        matches.push_back(mt);
                        continue;
                    }
                }
            } else if (_name.find_first_of(' ') != std::string::npos) {
                // probably an operator
                std::vector<std::string> mparts = split(split(methods[m]->name, ":")[1], " \t\n\r");
                std::vector<std::string> sparts = split(_name, " \t\n\r");
                if (mparts.size() != sparts.size()) continue;
                bool matched = true;
                for (u32 i = 0;matched && i < mparts.size();i++) {
                    matched = mparts[i] == sparts[i];
                }
                if (matched) func = methods[m];
            }
            const std::string& bt_name = base_type ? base_type->name : name;
            if (methods[m]->name == bt_name + "::" + _name) func = methods[m];

            if (!func) continue;

            if (_name == "constructor" && func->type->signature->explicit_argc == 1 && func->type->signature->explicit_arg(0).tp->id() == id()) {
                // don't match the copy constructor unless the requested arguments strictly match
                if (args.size() == 1 && args[0]->id() == id()) matches.push_back(func);
                continue;
            }

            // match return type
            if (ret && !ret->is_convertible_to(func->type->signature->return_type)) continue;
            bool ret_tp_strict = ret ? func->type->signature->return_type->id() == ret->id() : func->type->signature->return_type->size == 0;

            // match argument types
            if (func->type->signature->explicit_argc != args.size()) continue;

            // prefer strict type matches
            bool match = true;
            for (u8 i = 0;i < args.size();i++) {
                if (func->type->signature->explicit_arg(i).tp->name == "subtype") {
                    if (sub_type->id() != args[i]->id()) {
                        match = false;
                        break;
                    }
                } else {
                    if (func->type->signature->explicit_arg(i).tp->id() != args[i]->id()) {
                        match = false;
                        break;
                    }
                }
            }

            if (match && ret_tp_strict) return true;

            if (!match) {
                // check if the arguments are at least convertible
                match = true;
                for (u8 i = 0;i < args.size();i++) {
                    script_type* at = func->type->signature->explicit_arg(i).tp;
                    if (at->name == "subtype") at = sub_type;
                    if (!args[i]->is_convertible_to(at)) {
                        match = false;
                        break;
                    }
                }

                if (!match) continue;
            }

            matches.push_back(func);
        }

        if (matches.size() > 1) return false;
        if (matches.size() == 1) return true;
        return false;
    }

    bool script_type::is_convertible_to(script_type* to) const {
        // same type
        if (m_id == to->id()) return true;
        if (is_host && base_type && base_type->id() == to->id()) return true;

        // pointers of specific type are convertible to void*
        // void* is convertible to pointers of specific type
        script_type* ptrTp = type_of<void*>(owner->context());
        if (!is_primitive && to == ptrTp) return true;
        if (this == ptrTp && !to->is_primitive) return true;

        // nothing is convertible to void and void is convertible to nothing
        if (size == 0 || to->size == 0) return false;

        // is explicitly castable to desired type
        if (!is_primitive) {
            bool hasUnambiguousCast = has_unambiguous_method("<cast>", to, { const_cast<script_type*>(this) });
            if (to->is_primitive) {
                return hasUnambiguousCast;
            } else if (hasUnambiguousCast) return true;
        }

        // desired type can be constructed from this type
        if (!to->is_primitive) {
            return to->has_unambiguous_method("constructor", nullptr, { const_cast<script_type*>(this) });
        }

        // conversion between non-void primitive types is always possible
        return is_primitive;
    }

    script_function* script_type::method(const std::string& _name, script_type* ret, const std::vector<script_type*>& args, bool* was_ambiguous) {
        std::vector<script_function*> matches;

        for (u16 m = 0;m < methods.size();m++) {
            // match name
            script_function* func = nullptr;
            if (_name == "<cast>") {
                script_function* mt = methods[m];
                auto mparts = split(split(mt->name,":")[1], " \t\n\r");
                if (mparts.size() == 2 && mparts[0] == "operator" && mparts[1] == mt->type->signature->return_type->name && mt->type->signature->args.size() == 1) {
                    if (mt->type->signature->return_type->name == "subtype") {
                        if (sub_type->id() == ret->id()) {
                            return mt;
                        } else if (sub_type->is_convertible_to(ret)) {
                            matches.push_back(mt);
                            continue;
                        }
                    } else {
                        if (mt->type->signature->return_type->id() == ret->id()) {
                            return mt;
                        } else if (mt->type->signature->return_type->is_convertible_to(ret)) {
                            matches.push_back(mt);
                            continue;
                        }
                    }
                }
            } else if (_name.find_first_of(' ') != std::string::npos) {
                // probably an operator
                std::vector<std::string> mparts = split(split(methods[m]->name, ":")[1], " \t\n\r");
                std::vector<std::string> sparts = split(_name, " \t\n\r");
                if (mparts.size() != sparts.size()) continue;
                bool matched = true;
                for (u32 i = 0;matched && i < mparts.size();i++) {
                    matched = mparts[i] == sparts[i];
                }
                if (matched) func = methods[m];
            }

            const std::string& bt_name = base_type ? base_type->name : name;
            if (methods[m]->name == bt_name + "::" + _name) func = methods[m];

            if (!func) continue;

            // match return type
            if (ret) {
                if (ret->base_type && !func->type->signature->return_type->is_convertible_to(ret->base_type)) continue;
                else if (!ret->base_type && !func->type->signature->return_type->is_convertible_to(ret)) continue;
            }

            bool ret_tp_strict = true;
            if (ret) {
                ret_tp_strict = false;
                if (ret->base_type && func->type->signature->return_type->id() == ret->base_type->id()) ret_tp_strict = true;
                else if (!ret->base_type && func->type->signature->return_type->id() == ret->id()) ret_tp_strict = true;
            }

            // match argument types
            if (func->type->signature->explicit_argc != args.size()) continue;

            // prefer strict type matches
            bool match = true;
            for (u8 i = 0;i < args.size();i++) {
                if (func->type->signature->explicit_arg(i).tp->name == "subtype") {
                    if (sub_type->id() != args[i]->id()) {
                        match = false;
                        break;
                    }
                } else {
                    if (func->type->signature->explicit_arg(i).tp->id() != args[i]->id()) {
                        match = false;
                        break;
                    }
                }
            }

            if (match && ret_tp_strict) return func;

            if (!match) {
                // check if the arguments are at least convertible
                match = true;
                bool args_strict = true;
                for (u8 i = 0;i < args.size();i++) {
                    script_type* at = func->type->signature->explicit_arg(i).tp;
                    if (at->name == "subtype") at = sub_type;
                    if (at->id() != args[i]->id()) args_strict = false;
                    if (!args[i]->is_convertible_to(at)) {
                        match = false;
                        break;
                    }
                }

                if (ret_tp_strict && args_strict) return func;
                if (!match) continue;
            }

            matches.push_back(func);
        }

        if (matches.size() > 1) {
            if (was_ambiguous) *was_ambiguous = true;
            return nullptr;
        }

        if (matches.size() == 1) {
            return matches[0];
        }

        return nullptr;
    }

    script_type::property* script_type::prop(const std::string& _name) {
        for (u16 i = 0;i < properties.size();i++) {
            if (properties[i].name == _name) return &properties[i];
        }

        return nullptr;
    }
};