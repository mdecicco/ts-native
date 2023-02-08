#pragma once
#include <tsn/common/types.h>

namespace utils {
    class String;
    
    template <typename T>
    class Array;
};

namespace tsn {
    namespace ffi {
        class Function;
        class Method;
        class DataType;
        class DataTypeRegistry;
    };

    /**
     * @brief Find functions that match the provided signature
     * 
     * @tparam Ret Signature return type
     * @tparam Args Signature argument types
     * 
     * @param types Context's type registry
     * @param name Function name
     * @param funcs Set of functions to search
     * @param flags Search options (See '_function_match_flags' documentation for more info)
     * 
     * @return Subset of 'funcs' that match the provided signature
     */
    template <typename Ret, typename...Args>
    utils::Array<ffi::Function*> function_match(
        ffi::DataTypeRegistry* types,
        const utils::String& name,
        const utils::Array<ffi::Function*>& funcs,
        function_match_flags flags
    );

    /**
     * @brief Find class methods that match the provided signature
     * 
     * @tparam Ret Signature return type
     * @tparam Args Signature argument types
     * 
     * @param types Context's type registry
     * @param name Method name
     * @param methods Set of methods to search
     * @param flags Search options (See '_function_match_flags' documentation for more info)
     * 
     * @return Subset of 'methods' that match the provided signature
     */
    template <typename Ret, typename...Args>
    utils::Array<ffi::Method*> function_match(
        ffi::DataTypeRegistry* types,
        const utils::String& name,
        const utils::Array<ffi::Method*>& methods,
        function_match_flags flags
    );

    /**
     * @brief Find functions that match the provided signature
     * 
     * @param name Function name
     * @param retTp Signature return type
     * @param argTps Signature argument types
     * @param argCount Signature argument count
     * @param funcs Set of functions to search
     * @param flags Search options (See '_function_match_flags' documentation for more info)
     * @return Subset of 'funcs' that match the provided signature
     */
    utils::Array<ffi::Function*> function_match(
        const utils::String& name,
        const ffi::DataType* retTp,
        const ffi::DataType** argTps,
        u8 argCount,
        const utils::Array<ffi::Function*>& funcs,
        function_match_flags flags
    );

    /**
     * @brief Find methods that match the provided signature
     * 
     * @param name Method name
     * @param retTp Signature return type
     * @param argTps Signature argument types
     * @param argCount Signature argument count
     * @param methods Set of methods to search
     * @param flags Search options (See '_function_match_flags' documentation for more info)
     * @return Subset of 'methods' that match the provided signature
     */
    utils::Array<ffi::Method*> function_match(
        const utils::String& name,
        const ffi::DataType* retTp,
        const ffi::DataType** argTps,
        u8 argCount,
        const utils::Array<ffi::Method*>& methods,
        function_match_flags flags
    );
};