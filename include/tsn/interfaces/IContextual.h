#pragma once

namespace tsn {
    class Context;

    class IContextual {
        public:
            IContextual(Context* ctx);
            virtual ~IContextual();

            Context* getContext() const;

        protected:
            Context* m_ctx;
    };
};