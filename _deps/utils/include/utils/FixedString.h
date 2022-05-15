#pragma once

namespace utils {
    // FixedString template struct thanks to 'user'
    // https://stackoverflow.com/a/59495681

    template <unsigned N>
    struct FixedString {
        char buf[N + 1]{};
        int length = N;

        constexpr FixedString(char const* string) {
            for (unsigned index = 0;index < N;++index) {
                buf[index] = string[index];
            }
        }
        constexpr operator char const*() const { return buf; }
    };

    template<unsigned N>
    FixedString(char const (&)[N]) -> FixedString<N - 1>;
};