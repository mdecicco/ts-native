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

    script_function* script_type::method(const std::string& _name, script_type* ret, const std::vector<script_type*>& arg_types) {
        return function_search(name + "::" + _name, methods, ret, arg_types);
    }

    script_type::property* script_type::prop(const std::string& _name) {
        for (u16 i = 0;i < properties.size();i++) {
            if (properties[i].name == _name) return &properties[i];
        }

        return nullptr;
    }
};