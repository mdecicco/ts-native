#pragma once
namespace tsn {
    class ITransactional {
        public:
            virtual void begin() = 0;
            virtual void commit() = 0;
            virtual void revert() = 0;
    };
};