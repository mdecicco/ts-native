#include <utils/Arguments.h>
#include <utils/String.h>
#include <utils/Array.hpp>

namespace utils {
    Arguments::Arguments() : m_argc(0), m_argv(nullptr) {
    }

    Arguments::Arguments(u32 argc, const char** argv) {
        m_argc = argc;
        m_argv = argv;

        if (argc == 0) return;
        m_path = argv[0];
        //Engine::Get()->log(LogCode::program_path, argv[0]);
        String argStr = "";
        for (u32 i = 1;i < argc;i++) {
            if (i > 1) argStr += ' ';
            argStr += argv[i];
        }

        Array<String> comps = argStr.split("-", "''\"\"``").filter([](const String& s) { return s.size() > 0; });
        Array<String> arg(2);

        for (u32 i = 0;i < comps.size();i++) {
            arg = comps[i].split(" ", nullptr, 1);
            
            if (arg.size() == 2) {
                String val = arg[1].trim();
                m_args[arg[0]] = val;
                //Engine::Get()->log(LogCode::program_argument, arg[0].c_str(), val.c_str());
            }
            else {
                m_args[arg[0]] = "";
                //Engine::Get()->log(LogCode::program_argument, arg[0].c_str(), "");
            }
        }
    }

    Arguments::~Arguments() {
    }

    void Arguments::set(const String& name) {
        m_args[name] = "";
    }

    void Arguments::set(const String& name, const String& value) {
        m_args[name] = value;
    }

    void Arguments::remove(const String& name) {
        m_args.erase(name);
    }

    bool Arguments::exists(const String& name) const {
        return m_args.count(name) > 0;
    }

    const String& Arguments::getArg(const String& name) const {
        static String nullArg;
        auto it = m_args.find(name);
        if (it == m_args.end()) return nullArg;
        return it->second;
    }

    const String& Arguments::getPath() const {
        return m_path;
    }

    u32 Arguments::getRawArgCount() const {
        return m_argc;
    }

    const char** Arguments::getRawArgs() const {
        return m_argv;
    }
};