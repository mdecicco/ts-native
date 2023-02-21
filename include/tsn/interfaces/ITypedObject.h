#pragma once

namespace tsn {
    namespace ffi {
        class DataType;
    };

    class ITypedObject {
        public:
            ITypedObject(ffi::DataType* type);
            virtual ~ITypedObject();

            ffi::DataType* getType() const;
        
        protected:
            ffi::DataType* m_type;
    };
};