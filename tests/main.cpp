#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "bind.hpp"

TEST_CASE("Binding utilities work", "[gjs::bind]") {
    bind_tests();
}