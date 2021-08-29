#pragma once
#include <gjs/common/types.h>
#include <gjs/common/source_ref.h>
#include <exception>

namespace gjs {
    class vm_backend;

    namespace error {
        enum class ecode {
            no_error = 0,
            unspecified_error,

            __bind_error_start,
            b_prop_already_bound,
            b_prop_type_unbound,
            b_method_class_unbound,
            b_function_return_type_unbound,
            b_method_return_type_unbound,
            b_method_arg_struct_pass_by_value,
            b_method_arg_type_unbound,
            b_arg_struct_pass_by_value,
            b_arg_type_unbound,
            b_cannot_get_signature_unbound_return,
            b_cannot_get_signature_unbound_arg,
            b_function_already_bound,
            b_module_already_created,
            b_module_hash_collision,
            b_enum_value_exists,
            b_function_already_added_to_module,
            b_type_already_bound,
            b_type_hash_collision,
            b_type_not_found_cannot_finalize,
            __bind_error_end,

            __lexer_error_start,
            l_unknown_token,
            __lexer_error_end,

            __parse_error_start,
            p_expected_identifier,
            p_unexpected_identifier,
            p_expected_type_identifier,
            p_expected_char,
            p_expected_x,
            p_unexpected_token,
            p_expected_specific_keyword,
            p_expected_keyword,
            p_unexpected_keyword,
            p_expected_operator,
            p_expected_specific_operator,
            p_expected_expression,
            p_expected_assignable,
            p_expected_callable,
            p_expected_class_prop_or_meth,
            p_unexpected_eof,
            p_expected_import_path,
            p_malformed_import,
            p_failed_to_resolve_module,
            p_cyclic_imports,
            p_malformed_numerical_constant,
            p_string_should_not_be_empty,
            p_expected_numerical_constant,
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
            c_invalid_enum_value_expr,
            c_duplicate_enum_value_name,
            c_no_such_enum_value,
            c_no_assign_literal,
            c_type_is_not_value,
            c_enum_is_not_value,
            c_func_is_not_value,
            c_prop_already_initialized,
            c_prop_has_no_default_ctor,
            c_compile_finished_with_errors,
            c_invalid_binary_operator,
            c_object_not_callable,
            c_call_signature_mismatch,
            __compile_error_end,

            __runtime_error_start,
            r_buffer_offset_out_of_range,
            r_buffer_read_out_of_range,
            r_buffer_overflow,
            r_invalid_object_constructor,
            r_invalid_object_property,
            r_invalid_object_method,
            r_cannot_read_object_property,
            r_cannot_write_object_property,
            r_cannot_ref_restricted_object_property,
            r_cannot_ref_object_accessor_property,
            r_unbound_type,
            r_invalid_enum_value,
            r_cannot_get_null_pointer,
            r_pointer_assign_type_mismatch,
            r_call_null_obj_method,
            r_fp_bind_self_thiscall_only,
            r_file_not_found,
            r_failed_to_open_file,
            r_callback_missing_data,
            __runtime_error_end,

            __vm_error_start,
            vm_stack_overflow,
            vm_invalid_module_id,
            vm_invalid_function_id,
            vm_invalid_callback,
            vm_invalid_instruction,
            vm_no_subtype_provided_for_call,
            vm_cannot_pass_struct_by_value,
            __vm_error_end
        };

        const char* format_str(ecode code);

        class exception : public std::exception {
            public:
                exception(ecode code, source_ref at, ...);
                exception();
                
                virtual const char* what() const;

                ecode code;
                source_ref src;
                std::string message;
        };

        class bind_exception : public exception {
            public:
                bind_exception(ecode code, ...);
        };

        class runtime_exception : public exception {
            public:
                runtime_exception(ecode code, ...);
        };

        class vm_exception : public exception {
            public:
                vm_exception(ecode code, ...);

                bool raised_from_script;
        };
    };
};