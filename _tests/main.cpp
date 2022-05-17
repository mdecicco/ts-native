#define CATCH_CONFIG_RUNNER
#include "catch.hpp"
#include <utils/String.h>
#include <utils/Allocator.hpp>

int main(int argc, const char *argv[]) {
    utils::String::Allocator::Create(16384, 1024);
    utils::Mem::Create();

    int result = Catch::Session().run(argc, argv);

    utils::Mem::Destroy();
    utils::String::Allocator::Destroy();
    return result;
}


#include "ffi/host2host_args.hpp"