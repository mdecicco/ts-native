#include <value.h>

namespace gjs {
	std::string value::to_string() const {
		if (is_null()) return "null";

		switch(type()) {
			case value_type::integer: {
				char buf[32] = { 0 };
				snprintf(buf, 32, "%d", v.i);
				return buf;
			}
			case value_type::decimal: {
				char buf[32] = { 0 };
				snprintf(buf, 32, "%f", v.d);
				return buf;
			}
			case value_type::boolean: return v.b ? "true" : "false";
			case value_type::string: return v.s;
			default: break;
		}

		return "undefined";
	}
};