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
            p_expected_import_path,
            p_malformed_import,
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
            c_missing_return_value,
            c_class_property_not_static,
            c_no_such_type,
            c_no_default_constructor,
            c_undefined_identifier,
            c_no_stack_delete_named_var,
            c_no_stack_delete,
            c_no_primitive_delete_named_var,
            c_no_primitive_delete,
            c_no_void_return_val,
            c_no_ctor_return_val,
            c_no_dtor_return_val,
            c_no_valid_conversion,
            c_no_assign_read_only,
            c_no_read_write_only,
            c_identifier_in_use,
            c_instantiation_requires_subtype,
            c_unexpected_instantiation_subtype,
            c_invalid_subtype_use,
            c_symbol_not_found_in_module,
            c_compile_finished_with_errors,
            __compile_error_end,
            __runtime_error_start,
            r_buffer_offset_out_of_range,
            r_buffer_read_out_of_range,
            r_buffer_overflow,
            __runtime_error_end
        };

        const char* format_str(ecode code);

        class exception : public std::exception {
            public:
                exception(ecode code, source_ref at, ...);

                ecode code;
                source_ref src;
                std::string message;
        };


        class script_context;
        class runtime_exception : public std::exception {
            public:
                runtime_exception(ecode code, ...);

                ecode code;
                source_ref src;
                std::string message;
        };
    };
};