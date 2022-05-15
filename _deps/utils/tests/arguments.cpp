#include <catch2/catch_test_macros.hpp>
#include <filesystem>

using namespace utils;
TEST_CASE("Arguments", "[utils]") {
    auto p = std::filesystem::current_path() / "dummy";
    std::string path = p.string();

    const char* args[9] = {
        p.string().c_str(),
        "yo",
        "-test",
        "abc",
        "-test1",
        "123",
        "456",
        "-test2",
        "'yo what up'"
    };

    auto args = Arguments(9, args);
}
