#include <tsn/common/Module.h>
#include <tsn/common/Context.h>
#include <tsn/common/types.hpp>
#include <tsn/ffi/DataTypeRegistry.h>
#include <tsn/bind/bind.h>

namespace tsn {
    template <typename T>
    u32 Module::addData(const utils::String& name, const T& value, access_modifier access) {
        ffi::DataType* tp = getContext()->getTypes()->getType<T>();
        if (!tp) {
            throw ffi::BindException(utils::String::Format(
                "Attempted to add data to module of type '%s' which has not been bound yet",
                type_name<T>()
            ));
        }

        u32 slot = addData(name, tp, access);
        memcpy(m_data[slot].ptr, &value, sizeof(T));

        return slot;
    }
};