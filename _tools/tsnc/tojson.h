#pragma once
#include <tsn/common/types.h>

#include <utils/json.hpp>

namespace tsn {
    class SourceLocation;
    struct module_data;

    namespace ffi {
        class DataType;
        class Function;
        struct type_base;
        struct type_property;
        struct function_argument;
    }

    namespace compiler {
        struct log_message;
        struct symbol_lifetime;
        class ParseNode;
        class FunctionDef;
    };
};

nlohmann::json toJson(const tsn::SourceLocation& src);
nlohmann::json toJson(const tsn::compiler::log_message& log);
nlohmann::json toJson(const tsn::compiler::ParseNode* n, bool isArrayElem = false);
nlohmann::json toJson(const tsn::compiler::symbol_lifetime& log);
nlohmann::json toJson(const tsn::ffi::DataType* type, bool brief);
nlohmann::json toJson(const tsn::ffi::Function* function, bool brief, tsn::compiler::FunctionDef* fd = nullptr);
nlohmann::json toJson(const tsn::ffi::type_base& b);
nlohmann::json toJson(const tsn::ffi::type_property& p);
nlohmann::json toJson(tsn::u32 index, const tsn::ffi::function_argument& a, tsn::compiler::FunctionDef* fd = nullptr);
nlohmann::json toJson(tsn::u32 slot, const tsn::module_data& d);