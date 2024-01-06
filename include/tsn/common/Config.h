#pragma once
#include <tsn/common/types.h>
#include <string>

namespace tsn {
    class Config {
        public:
            Config();
            ~Config();

            std::string workspaceRoot;
            std::string supportDir;
            bool scanForChanges;
            u32 scanIntervalMS;
            bool debugLogging;
            bool disableOptimizations;
            bool disableExecution;
    };
};