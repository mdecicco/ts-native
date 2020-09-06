#include <errors.h>
#include <stdarg.h>

namespace gjs {
	namespace error {
		static const char* error_fmts[] = {
			"No Error",
			"", // parse errors
			"Expected identifier",
			"Expected type identifier",
			"Expected '%c'",
			"Unexpected token '%s'",
			"Expected keyword '%s'",
			"Expected operator",
			"Expected expression",
			"Expected identifier, member expression, or index expression",
			"Expected identifier or member expression",
			"", // end parse errors
		};

		const char* format_str(ecode c) {
			return error_fmts[(u16)c];
		}

		exception::exception(ecode _code, source_ref at, ...) : code(_code), src(at) {
				va_list l;
				va_start(l, at);
				char out[1024] = { 0 };
				vsnprintf(out, 1024, error_fmts[(u16)code], l);
				va_end(l);
				message = out;
			}
	};
};