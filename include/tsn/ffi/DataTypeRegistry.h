#pragma once
#include <tsn/common/types.h>
#include <tsn/interfaces/IDataTypeHolder.h>

namespace tsn {
    namespace compiler {
        class Compiler;
    };

    namespace ffi {
        class DataType;
        class TemplateType;

        class DataTypeRegistry : public IDataTypeHolder {
            public:
                DataTypeRegistry(Context* ctx);
                ~DataTypeRegistry();

                DataType* getInt8() const;
                DataType* getUInt8() const;
                DataType* getInt16() const;
                DataType* getUInt16() const;
                DataType* getInt32() const;
                DataType* getUInt32() const;
                DataType* getInt64() const;
                DataType* getUInt64() const;
                DataType* getFloat32() const;
                DataType* getFloat64() const;
                DataType* getNull() const;
                DataType* getPoison() const;
                DataType* getVoid() const;
                DataType* getVoidPtr() const;
                DataType* getString() const;
                DataType* getBoolean() const;
                DataType* getCaptureData() const;
                DataType* getClosure() const;
                TemplateType* getArray() const;
                TemplateType* getPointer() const;

                void updateCachedTypes();
                u32 getNextAnonTypeIndex();
            
            protected:
                u32 m_anonTypeCount;

                // These types need to be looked up frequently, better to have them
                // cached.
                DataType* m_i8;
                DataType* m_u8;
                DataType* m_i16;
                DataType* m_u16;
                DataType* m_i32;
                DataType* m_u32;
                DataType* m_i64;
                DataType* m_u64;
                DataType* m_f32;
                DataType* m_f64;
                DataType* m_null;
                DataType* m_poison;
                DataType* m_void;
                DataType* m_voidPtr;
                DataType* m_string;
                DataType* m_boolean;
                DataType* m_captureData;
                DataType* m_closure;
                TemplateType* m_array;
                TemplateType* m_pointer;
        };
    };
};