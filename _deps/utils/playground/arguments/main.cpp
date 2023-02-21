#include <utils/Arguments.h>

using namespace utils;
int main(int argc, const char** argv) {
    const char* args[9] = {
        argv[0],
        "yo",
        "-test",
        "abc",
        "-test1",
        "123",
        "456",
        "-test2",
        "'yo what up'"
    };

    Arguments(9, args);

    return 0;
}
