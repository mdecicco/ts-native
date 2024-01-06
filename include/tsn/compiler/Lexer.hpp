#include <tsn/common/types.h>

namespace tsn {
    namespace compiler {
        template <u32 count>
        i32 stringMatchesOne(const char* strings[count], const char* str, const char** endPtr = nullptr) {
            if constexpr (count == 0) {
                if (endPtr) *endPtr = str;
                return -1;
            }

            if (!str) {
                if (endPtr) *endPtr = str;
                return -1;
            }

            bool skipStrings[count] = { false };
            u32 start = 0;
            u32 end = count;
            u32 i = 0;

            for (const char* ptr = str;*ptr != 0;*ptr++, i++) {
                char next = *(ptr + 1);
                bool tokenEnds = !isalnum(next) && next != '_';

                for (u32 c = start;c < end;c++) {
                    if (skipStrings[c]) continue;
                    
                    char ch = strings[c][i];
                    if (ch != *ptr) {
                        if (c == start) start++;
                        else if (c == end - 1) end--;
                        else skipStrings[c] = true;
                        continue;
                    }

                    if (*ptr == 0) {
                        // both strings terminated, is equal
                        if (endPtr) *endPtr = ptr;
                        return i32(c);
                    }

                    if (tokenEnds) {
                        // ptr points to the end of the relevant token
                        // don't care about anything beyond this

                        if (strings[c][i + 1] == 0) {
                            // string being compared ends here as well
                            // the token matches
                            if (endPtr) *endPtr = ptr + 1;
                            return i32(c);
                        }
                    }
                }

                if (tokenEnds) {
                    // none of the provided strings match
                    break;
                }

                if (start == end - 1) {
                    const char* cmpTo = strings[start];
                    while (!tokenEnds) {
                        char next = *(ptr + 1);
                        bool tokenEnds = !isalnum(next) && next != '_';

                        char ch = cmpTo[i];
                        if (ch != *ptr) {
                            if (endPtr) *endPtr = ptr;
                            return -1;
                        }

                        if (*ptr == 0) {
                            // both strings terminated, is equal
                            if (endPtr) *endPtr = ptr;
                            return i32(start);
                        }

                        if (tokenEnds) {
                            // ptr points to the end of the relevant token
                            // don't care about anything beyond this

                            if (cmpTo[i + 1] == 0) {
                                // string being compared ends here as well
                                // the token matches
                                if (endPtr) *endPtr = ptr + 1;
                                return i32(start);
                            }
                        }

                        ptr++;
                        i++;
                    }

                    break;
                }
            }

            if (endPtr) *endPtr = str;
            return -1;
        }
    };
};