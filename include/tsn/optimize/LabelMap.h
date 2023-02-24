#pragma once
#include <tsn/optimize/types.h>
#include <tsn/compiler/types.h>

#include <utils/robin_hood.h>

namespace tsn {
    namespace compiler {
        class CodeHolder;
    };

    namespace optimize {
        class LabelMap {
            public:
                LabelMap();
                LabelMap(compiler::CodeHolder* ch);

                address get(compiler::label_id label) const;

                void rebuild(compiler::CodeHolder* ch);

            protected:
                robin_hood::unordered_map<compiler::label_id, address> m_map;
        };
    };
};