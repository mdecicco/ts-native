#pragma once
#include <gjs/optimize/optimize.h>
#include <gjs/util/robin_hood.h>
#include <vector>

namespace gjs {
    struct compilation_output;

    namespace compile {
        class var;
    };

    namespace optimize {
        struct reg_lifetime {
            u32 reg_id;
            u64 begin;
            u64 end;
            u16 usage_count;
            bool is_fp;

            bool is_concurrent(const reg_lifetime& o) const;
        };

        struct liveness {
            std::vector<reg_lifetime> lifetimes;
            robin_hood::unordered_map<u32, std::vector<u32>> reg_lifetime_map;

            void build(compilation_output& in, u16 fidx, label_map& lmap);
            std::vector<reg_lifetime*> ranges_of(const compile::var& v);
            std::vector<reg_lifetime*> ranges_of(u32 reg_id);
            bool is_live(const compile::var& v, address at);
            bool is_live(u32 reg_id, address at);
            reg_lifetime* get_live_range(const compile::var& v, address at);
            reg_lifetime* get_live_range(u32 reg_id, address at);
        };
    };
};