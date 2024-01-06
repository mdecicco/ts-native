#include <tsn/common/Config.h>

namespace tsn {
    Config::Config() {
        workspaceRoot = "./";
        supportDir = "./.tsn";
        scanForChanges = true;
        scanIntervalMS = 2500;
        debugLogging = false;
        disableOptimizations = false;
        disableExecution = false;
    }

    Config::~Config() {
    }
};