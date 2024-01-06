#pragma once
#include <tsn/common/types.h>
#include <tsn/utils/SourceLocation.h>
#include <tsn/interfaces/ITransactional.h>

#include <utils/String.h>
#include <utils/Array.h>

namespace tsn {
    namespace compiler {
        class ParseNode;

        enum log_message_code {
            lm_generic,
            
            IO_MESSAGES_START,
            iom_failed_to_open_file,
            iom_failed_to_cache_script,
            IO_MESSAGES_END,

            PARSE_MESSAGES_START,
            pm_none,
            pm_expected_eof,
            pm_expected_eos,
            pm_expected_open_brace,
            pm_expected_open_parenth,
            pm_expected_closing_brace,
            pm_expected_closing_parenth,
            pm_expected_closing_bracket,
            pm_expected_parameter,
            pm_expected_colon,
            pm_expected_expr,
            pm_expected_variable_decl,
            pm_expected_type_specifier,
            pm_expected_expgroup,
            pm_expected_statement,
            pm_expected_while,
            pm_expected_function_body,
            pm_expected_identifier,
            pm_expected_type_name,
            pm_expected_object_property,
            pm_expected_catch,
            pm_expected_catch_param,
            pm_expected_catch_param_type,
            pm_expected_import_alias,
            pm_expected_import_list,
            pm_expected_import_name,
            pm_expected_export_decl,
            pm_expected_symbol,
            pm_expected_parameter_list,
            pm_expected_single_template_arg,
            pm_expected_operator_override_target,
            pm_expected_return_type,
            pm_unexpected_type_specifier,
            pm_malformed_class_element,
            pm_empty_class,
            pm_reserved_word,
            PARSE_MESSAGES_END,
            
            COMPILER_MESSAGES_START,
            cm_info_could_be,
            cm_info_arg_conversion,
            cm_err_ambiguous_constructor,
            cm_err_no_matching_constructor,
            cm_err_private_constructor,
            cm_err_identifier_does_not_refer_to_type,
            cm_err_identifier_not_found,
            cm_err_identifier_ambiguous,
            cm_err_template_type_expected,
            cm_err_too_few_template_args,
            cm_err_too_many_template_args,
            cm_err_symbol_already_exists,
            cm_err_dtor_already_exists,
            cm_err_expected_type_specifier,
            cm_err_expected_method_argument_type,
            cm_err_expected_function_argument_type,
            cm_err_export_not_in_root_scope,
            cm_err_export_invalid,
            cm_err_import_not_in_root_scope,
            cm_err_import_module_not_found,
            cm_err_import_type_spec_invalid,
            cm_err_property_not_found,
            cm_err_property_is_private,
            cm_err_property_not_static,
            cm_err_property_is_static,
            cm_err_property_no_read_access,
            cm_err_property_or_method_ambiguous,
            cm_err_property_already_defined,
            cm_err_property_getter_already_defined,
            cm_err_property_setter_already_defined,
            cm_err_no_pointer_to_getter,
            cm_err_getter_setter_static_mismatch,
            cm_err_value_not_callable_with_args,
            cm_err_method_not_found,
            cm_err_method_ambiguous,
            cm_err_method_is_static,
            cm_err_method_not_static,
            cm_err_export_ambiguous,
            cm_err_export_not_found,
            cm_err_malformed_member_expression,
            cm_err_this_outside_class,
            cm_err_function_must_return_a_value,
            cm_err_function_ambiguous,
            cm_err_function_property_access,
            cm_err_function_argument_count_mismatch,
            cm_err_function_signature_not_determined,
            cm_err_could_not_deduce_type,
            cm_err_continue_scope,
            cm_err_break_scope,
            cm_err_class_scope,
            cm_err_function_scope,
            cm_err_type_scope,
            cm_err_type_not_convertible,
            cm_err_value_not_writable,
            cm_err_type_used_as_value,
            cm_err_typeinfo_used_as_value,
            cm_err_module_used_as_value,
            cm_err_module_data_used_as_value,
            cm_err_internal,
            cm_err_not_trusted,
            cm_err_duplicate_name,
            COMPILER_MESSAGES_END
        };

        enum log_type {
            lt_debug,
            lt_info,
            lt_warn,
            lt_error
        };

        struct log_message {
            log_type type;
            log_message_code code;
            utils::String msg;
            SourceLocation src;
            ParseNode* node;
        };

        class Logger : public ITransactional {
            public:
                Logger();
                ~Logger();

                virtual void begin();
                virtual void commit();
                virtual void revert();

                log_message& submit(log_type type, log_message_code code, const utils::String& msg, const SourceLocation& src, ParseNode* ast);
                log_message& submit(log_type type, log_message_code code, const utils::String& msg);

                bool hasErrors() const;
                const utils::Array<log_message>& getMessages();
            
            protected:
                utils::Array<u32> m_logCountStack;
                utils::Array<log_message> m_messages;
        };
    };
};