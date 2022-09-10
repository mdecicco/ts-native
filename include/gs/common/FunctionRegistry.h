#pragma once
#include <gs/common/types.h>
#include <gs/interfaces/IContextual.h>
#include <gs/interfaces/IFunctionHolder.h>

#include <utils/Array.h>
#include <utils/robin_hood.h>

namespace utils {
    class String;
};

namespace gs {
    namespace compiler {
        class Compiler;
    };

    namespace ffi {
        class Function;
        class DataType;
        class DataTypeRegistry;

        class FunctionRegistry : public IContextual, public IFunctionHolder {
            public:
                FunctionRegistry(Context* ctx);
                ~FunctionRegistry();

                void registerFunction(ffi::Function* fn);

                /**
                 * @brief Find functions that match the provided signature
                 * 
                 * @tparam Ret Signature return type
                 * @tparam Args Signature argument types
                 * 
                 * @param types Context's type registry
                 * @param name Function name
                 * @param flags Search options (See 'function_match_flags' documentation for more info)
                 * 
                 * @return Subset of held functions that match the provided signature
                 */
                template <typename Ret, typename...Args>
                utils::Array<ffi::Function*> findFunctions(
                    const utils::String& name,
                    function_match_flags flags
                );

                /**
                 * @brief Find functions that match the provided signature
                 * 
                 * @param name Function name
                 * @param retTp Signature return type
                 * @param argTps Signature argument types
                 * @param argCount Signature argument count
                 * @param flags Search options (See 'function_match_flags' documentation for more info)
                 * @return Subset of held functions that match the provided signature
                 */
                utils::Array<ffi::Function*> findFunctions(
                    const utils::String& name,
                    const ffi::DataType* retTp,
                    const ffi::DataType** argTps,
                    u8 argCount,
                    function_match_flags flags
                );
        };
    };
};