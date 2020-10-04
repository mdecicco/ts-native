#include <common/errors.h>
#include <builtin/builtin.h>
#include <stdarg.h>

namespace gjs {
    namespace error {
        static const char* error_fmts[] = {
            "No Error",
            "", // unspecified error
            "", // parse errors
            "Expected identifier",
            "Unexpected identifier '%s'",
            "Expected type identifier",
            "Expected '%c'",
            "Unexpected token '%s'",
            "Expected keyword '%s'",
            "Unexpected keyword '%s'",
            "Expected operator",
            "Expected expression",
            "Expected identifier, member expression, or index expression",
            "Expected identifier or member expression",
            "Expected class property or method",
            "Unexpected end of file while parsing %s",
            "", // end parse errors
            "", // compile errors
            "No source code to compile",
            "Encountered invalid AST node. This is not a user error",
            "Type '%s' has no property '%s'",
            "Type '%s' has no method '%s' that matches the argument list '%s' and return type '%s'",
            "No function '%s' matching argument list '%s' and return type '%s' was found",
            "Reference to method '%s' of type '%s' is ambiguous with the argument list '%s' and return type '%s'",
            "Reference to function '%s' is ambiguous with the argument list '%s' and return type '%s'",
            "Function '%s %s %s' was already declared",
            "Function '%s %s %s' already has a body",
            "Not all paths of function '%s' return a value",
            "Property '%s' of type '%s' is non-static. References to this property must be relative to a specific object",
            "'%s' does not name a type",
            "Type '%s' can not be constructed without parameters",
            "Identifier '%s' is undefined",
            "Cannot delete variable '%s' which was not constructed with 'new'",
            "Cannot delete object which was not constructed with 'new'",
            "Cannot delete variable '%s' which is not an object",
            "Cannot delete object which is not an object",
            "'return' statement is invalid in the global scope",
            "Cannot return a value from a function with no return type",
            "Cannot return a value from a constructor",
            "Cannot return a value from a destructor",
            "No valid conversion from type '%s' to '%s' was found",
            "Cannot assign value to read-only variable or property",
            "Cannot read value of write-only property",
            "The identifier '%s' is already in use",
            "The type '%s' cannot be instantiated without a subtype",
            "Unexpected 'subtype' while instantiating variable of type '%s', which is not a subtype class or format",
            "The special 'subtype' type can not be used outside the context of a subtype class",
            "Compilation finished with errors",
            "", // end compile errors
            "", // start runtime errors
            "Buffer offset %llu is out of range (size: %llu)",
            "Cannot read %llu bytes from buffer (%llu bytes remain from current position)",
            "Cannot write %llu bytes to buffer (%llu bytes remain from current position)",
            "" // end runtime errors
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

        runtime_exception::runtime_exception(ecode _code, ...) : code(_code) {
            // script_context* ctx = current_ctx();
            // todo: get source ref
            va_list l;
            va_start(l, _code);
            char out[1024] = { 0 };
            vsnprintf(out, 1024, error_fmts[(u16)code], l);
            va_end(l);
            message = out;
        }
    };
};