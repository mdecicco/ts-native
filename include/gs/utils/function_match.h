#pragma once
#include <gs/common/types.h>

namespace utils {
    class String;
    
    template <typename T>
    class Array;
};

namespace gs {
    namespace ffi {
        class Function;
        class DataType;
        class DataTypeRegistry;
    };

    /**
     * @brief Function match search options
     */
    typedef u8 function_match_flags;
    enum _function_match_flags {
        /**
         * @brief Ignore the implicit arguments of functions being compared to the provided signature
         */
        fm_skip_implicit_args = 0b00000001,
        
        /**
         * @brief Exclude functions with signatures that don't strictly match the provided return type.
         *        Without this flag, return types that are convertible to the provided return type with
         *        only one degree of separation will be considered matching.
         */
        fm_strict_return      = 0b00000010,
        
        /**
         * @brief Exclude functions with signatures that don't strictly match the provided argument types.
         *        Without this flag, argument types that are convertible to the provided argument types
         *        with only one degree of separation will be considered matching.
         */
        fm_strict_args        = 0b00000100,
        
        /**
         * @brief Same as fm_strict_return | fm_strict_args
         */
        fm_strict             = 0b00000110
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
     * @param flags Search options (See 'function_match_flags' documentation for more info)
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
     * @brief Find functions that match the provided signature
     * 
     * @param name Function name
     * @param retTp Signature return type
     * @param argTps Signature argument types
     * @param argCount Signature argument count
     * @param funcs Set of functions to search
     * @param flags Search options (See 'function_match_flags' documentation for more info)
     * @return Subset of 'funcs' that match the provided signature
     */
    utils::Array<ffi::Function*> function_match(
        const utils::String& name,
        ffi::DataType* retTp,
        ffi::DataType** argTps,
        u8 argCount,
        const utils::Array<ffi::Function*>& funcs,
        function_match_flags flags
    );
};