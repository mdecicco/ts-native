#pragma once
#include <tsn/optimize/types.h>
#include <tsn/compiler/types.h>

#include <utils/robin_hood.h>

namespace tsn {
    namespace optimize {
        class CodeHolder;

        class LabelMap {
            public:
                LabelMap();
                LabelMap(CodeHolder* ch);

                address get(compiler::label_id label) const;

                void rebuild(CodeHolder* ch);

            protected:
                robin_hood::unordered_map<compiler::label_id, address> m_map;
        };
    };
};