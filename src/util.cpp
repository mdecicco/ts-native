#include <stdarg.h>
#include <util.h>
using namespace std;

namespace gjs {
    vector<string> split(const string& str, const string& delimiters) {
        vector<string> out;

        string cur;
        bool lastWasDelim = false;
        for(size_t i = 0;i < str.length();i++) {
            bool isDelim = false;
            for(unsigned char d = 0;d < delimiters.length() && !isDelim;d++) {
                isDelim = str[i] == delimiters[d];
            }

            if (isDelim) {
                if (!lastWasDelim) {
                    out.push_back(cur);
                    cur = "";
                    lastWasDelim = true;
                }
                continue;
            }

            cur += str[i];
            lastWasDelim = false;
        }

        if (cur.length() > 0) out.push_back(cur);
        return out;
    }

    vector<string> get_lines(const string& str, const string& delimiters) {
        vector<string> out;

        string cur;
        bool lastWasDelim = false;
        for(size_t i = 0;i < str.length();i++) {
            bool isDelim = false;
            for(unsigned char d = 0;d < delimiters.length() && !isDelim;d++) {
                isDelim = str[i] == delimiters[d];
            }

            if (isDelim) {
                out.push_back(cur);
                cur = "";
                continue;
            }

            cur += str[i];
        }

        if (cur.length() > 0) out.push_back(cur);
        return out;
    }

    string format(const char* fmt, ...) {
        va_list l;
        va_start(l, fmt);
        char out[1024] = { 0 };
        vsnprintf(out, 1024, fmt, l);
        va_end(l);
        return out;
    }

    u32 hash(const std::string& str) {
        u32 h = 0;
        for (u32 i = 0;i < str.length();i++) h = 37 * h + str[i];
        return h + (h >> 5);
    }
};