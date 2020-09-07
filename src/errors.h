#pragma once
#include <types.h>
#include <source_ref.h>
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