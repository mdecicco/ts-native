#pragma once
#include <gjs/common/types.h>
#include <gjs/common/script_module.h>
#include <gjs/util/robin_hood.h>
#include <string>
#include <list>

namespace gjs {
    class script_function;
    class script_type;
    class script_enum;

    namespace compile {
        class var;
        struct context;

        class symbol {
            public:
                enum symbol_type {
                    st_function,
                    st_type,
                    st_enum,
                    st_var,
                    st_modulevar
                };

                symbol(script_function* func);
                symbol(script_type* type);
                symbol(script_enum* enum_);
                symbol(const script_module::local_var* modulevar);
                symbol(var* var_);
                ~symbol();

                symbol_type sym_type() const { return m_stype; }
                script_function* get_func() const { return m_func; }
                script_type* get_type() const { return m_type; }
                script_enum* get_enum() const { return m_enum; }
                const script_module::local_var* get_modulevar() const { return m_modulevar; }
                var* get_var() const { return m_var; }

            protected:
                symbol_type m_stype;

                script_function* m_func;
                script_type* m_type;
                script_enum* m_enum;
                const script_module::local_var* m_modulevar;
                var* m_var;
        };

        class symbol_table;
        class symbol_list {
            public:
                ~symbol_list();

                symbol* add(script_function* func);
                symbol* add(script_type* type);
                symbol* add(script_enum* enum_);
                symbol* add(const script_module::local_var* modulevar);
                symbol* add(var* var_);
                void remove(script_function* func);
                void remove(script_type* type);
                void remove(script_enum* enum_);
                void remove(const script_module::local_var* modulevar);
                void remove(var* var_);
                
                std::list<symbol> symbols;
                std::list<symbol_table*> tables;
        };

        class symbol_table {
            public:
                symbol_table(context* ctx);
                ~symbol_table();

                void clear();

                symbol* set(const std::string& name, script_function* func);
                symbol* set(const std::string& name, script_type* type);
                symbol* set(const std::string& name, script_enum* enum_);
                symbol* set(const std::string& name, const script_module::local_var* modulevar);
                symbol* set(const std::string& name, var* var_);
                symbol_table* nest(const std::string& name);

                symbol_list* get(const std::string& name);

                symbol_table* get_module(const std::string& name);
                symbol_table* get_type(const std::string& name);
                symbol_table* get_enum(const std::string& name);
                script_function* get_func(const std::string& name, script_type* ret, const std::vector<script_type*>& args, bool log_errors = true, bool* was_ambiguous = nullptr);
                script_function* get_func_strict(const std::string& name, script_type* ret, const std::vector<script_type*>& args, bool log_errors = true);

                // if the table represents an imported module
                script_module* module;

                // if the table represents a type
                script_type* type;

                // if the table represents an enum
                script_enum* enum_;
            protected:
                robin_hood::unordered_map<std::string, symbol_list> m_symbols;
                context* m_ctx;
        };
    };
};
