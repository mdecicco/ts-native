#pragma once
#include <tsn/common/types.h>
#include <tsn/compiler/types.h>
#include <tsn/optimize/types.h>

#include <utils/Array.h>
#include <utils/robin_hood.h>

namespace tsn {
    namespace compiler {
        class Value;
        class CodeHolder;
    };

    namespace optimize {
        
        struct reg_lifetime {
            compiler::vreg_id reg_id;
            address begin;
            address end;
            u16 usage_count;
            bool is_fp;

            bool is_concurrent(const reg_lifetime& o) const;
        };

        class LivenessData {
            public:
                LivenessData();
                LivenessData(compiler::CodeHolder* ch);
                void rebuild(compiler::CodeHolder* ch);

                utils::Array<reg_lifetime*> rangesOf(const compiler::Value& v);
                utils::Array<reg_lifetime*> rangesOf(u32 reg_id);
                bool isLive(const compiler::Value& v, address at);
                bool isLive(u32 reg_id, address at);
                reg_lifetime* getLiveRange(const compiler::Value& v, address at);
                reg_lifetime* getLiveRange(u32 reg_id, address at);

                utils::Array<reg_lifetime> lifetimes;
                robin_hood::unordered_map<u32, utils::Array<u32>> regLifetimeMap;
        };
    };
};