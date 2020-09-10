#pragma once
#include <common/types.h>
#include <common/source_ref.h>
#include <exception>

namespace gjs {
    namespace error {
        enum class ecode {
            no_error = 0,
            unspecified_error,
            __parse_error_start,
            p_expected_identifier,
            p_unexpected_identifier,
            p_expected_type_identifier,
            p_expected_char,
            p_unexpected_token,
            p_expected_specific_keyword,
            p_unexpected_keyword,
            p_expected_operator,
            p_expected_expression,
            p_expected_assignable,
            p_expected_callable,
            p_expected_class_prop_or_meth,
            p_unexpected_eof,
            __parse_error_end,
            __compile_error_start,
            c_no_code,
            c_invalid_node,
            c_no_such_property,
            c_no_such_method,
            c_no_such_function,
            c_ambiguous_method,
            c_ambiguous_function,
            c_function_already_declared,
            c_function_already_defined,
            c_class_property_not_static,
            c_no_such_type,
            c_no_default_constructor,
            c_undefined_identifier,
            c_no_stack_delete_named_var,
            c_no_stack_delete,
            c_no_primitive_delete_named_var,
            c_no_primitive_delete,
            c_no_global_return,
            c_no_void_return_val,
            c_compile_finished_with_errors,
            __compile_error_end
        };

        const char* format_str(ecode code);

        class exception : public std::exception {
            public:
                exception(ecode code, source_ref at, ...);

                ecode code;
                source_ref src;
                std::string message;
        };
    };
};