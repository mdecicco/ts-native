#pragma once
#include <types.h>
#include <source_ref.h>
#include <exception>

namespace gjs {
    namespace error {
        enum class ecode {
            no_error = 0,
            __parse_error_start,
            p_expected_identifier,
            p_expected_type_identifier,
            p_expected_char,
            p_unexpected_token,
            p_expected_specific_keyword,
            p_expected_operator,
            p_expected_expression,
            p_expected_assignable,
            p_expected_callable,
            __parse_error_end
        };

        inline const char* format_str(ecode code);



        class exception : public std::exception {
            public:
                exception(ecode code, source_ref at, ...);

                ecode code;
                source_ref src;
                std::string message;
        };
    };
};