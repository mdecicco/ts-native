#include <gjs/util/template_utils.hpp>
#include <gjs/common/script_type.h>
#include <gjs/common/types.h>

namespace gjs {
    func_signature::func_signature(const std::string& _name, script_type* ret, const std::vector<script_type*>& _arg_types)
        : name(_name), return_type(ret), arg_types(_arg_types) { }

    std::string func_signature::to_string() const {
        std::string sig = "";

        if (!return_type) sig += "<unbound>";
        else sig += return_type->name;

        sig += " " + name + "(";

        for (u8 i = 0;i < arg_types.size();i++) {
            if (i > 0) sig += ", ";
            if (!arg_types[i]) sig += "<unbound>";
            else sig += arg_types[i]->name;
        }

        sig += ")";
        return sig;
    }
};