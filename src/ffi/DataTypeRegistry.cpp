#include <tsn/ffi/DataTypeRegistry.h>
#include <tsn/interfaces/IDataTypeHolder.hpp>
#include <tsn/ffi/DataType.h>
#include <tsn/ffi/Closure.h>
#include <tsn/common/Context.h>
#include <tsn/common/Module.h>
#include <utils/Array.hpp>

namespace tsn {
    namespace ffi {
        DataTypeRegistry::DataTypeRegistry(Context* ctx) : IContextual(ctx) {
            m_anonTypeCount = 0;
            m_i8 = nullptr;
            m_u8 = nullptr;
            m_i16 = nullptr;
            m_u16 = nullptr;
            m_i32 = nullptr;
            m_u32 = nullptr;
            m_i64 = nullptr;
            m_u64 = nullptr;
            m_f32 = nullptr;
            m_f64 = nullptr;
            m_null = nullptr;
            m_poison = nullptr;
            m_void = nullptr;
            m_voidPtr = nullptr;
            m_string = nullptr;
            m_boolean = nullptr;
            m_closure = nullptr;
            m_closureRef = nullptr;
            m_array = nullptr;
            m_pointer = nullptr;
        }
        DataTypeRegistry::~DataTypeRegistry() { }

        DataType* DataTypeRegistry::getInt8() const { return m_i8; }
        DataType* DataTypeRegistry::getUInt8() const { return m_u8; }
        DataType* DataTypeRegistry::getInt16() const { return m_i16; }
        DataType* DataTypeRegistry::getUInt16() const { return m_u16; }
        DataType* DataTypeRegistry::getInt32() const { return m_i32; }
        DataType* DataTypeRegistry::getUInt32() const { return m_u32; }
        DataType* DataTypeRegistry::getInt64() const { return m_i64; }
        DataType* DataTypeRegistry::getUInt64() const { return m_u64; }
        DataType* DataTypeRegistry::getFloat32() const { return m_f32; }
        DataType* DataTypeRegistry::getFloat64() const { return m_f64; }
        DataType* DataTypeRegistry::getNull() const { return m_null; }
        DataType* DataTypeRegistry::getPoison() const { return m_poison; }
        DataType* DataTypeRegistry::getVoid() const { return m_void; }
        DataType* DataTypeRegistry::getVoidPtr() const { return m_voidPtr; }
        DataType* DataTypeRegistry::getString() const { return m_string; }
        DataType* DataTypeRegistry::getBoolean() const { return m_boolean; }
        DataType* DataTypeRegistry::getClosure() const { return m_closure; }
        DataType* DataTypeRegistry::getClosureRef() const { return m_closureRef; }
        TemplateType* DataTypeRegistry::getArray() const { return m_array; }
        TemplateType* DataTypeRegistry::getPointer() const { return m_pointer; }

        void DataTypeRegistry::updateCachedTypes() {
            m_i8 = getType<i8>();
            m_u8 = getType<u8>();
            m_i16 = getType<i16>();
            m_u16 = getType<u16>();
            m_i32 = getType<i32>();
            m_u32 = getType<u32>();
            m_i64 = getType<i64>();
            m_u64 = getType<u64>();
            m_f32 = getType<f32>();
            m_f64 = getType<f64>();
            m_null = getType<null_t>();
            m_poison = getType<poison_t>();
            m_void = getType<void>();
            m_voidPtr = getType<void*>();
            m_string = getType<utils::String>();
            m_boolean = getType<bool>();
            m_closure = getType<Closure>();
            m_closureRef = getType<ClosureRef>();

            Module* ma = m_ctx->getModule("trusted/array");
            m_array = (TemplateType*)(ma ? ma->allTypes().find([](const DataType* t) { return t->getName() == "Array"; }) : nullptr);

            Module* mp = m_ctx->getModule("trusted/pointer");
            m_pointer = (TemplateType*)(mp ? mp->allTypes().find([](const DataType* t) { return t->getName() == "Pointer"; }) : nullptr);
        }

        u32 DataTypeRegistry::getNextAnonTypeIndex() {
            return m_anonTypeCount++;
        }
    };
};