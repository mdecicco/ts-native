#include <gjs/common/io.h>
#ifdef _WIN32
    #include <direct.h>
    #define getcwd _getcwd
    #define chdir _chdir
#else
    #include <sys/stat.h>
    #include <unistd.h>
    void fopen_s(FILE** fpp, const char* file, const char* mode) {
        FILE* fp = fopen(file, mode);
        if (!fp) {
            *fpp = nullptr;
            return;
        }
        *fpp = fp;
    }
#endif

#include <filesystem>

namespace gjs {
    file_interface::file_interface(io_interface* io) : m_owner(io) { }

    dir_entry::dir_entry() : is_dir(false) { }


    void io_interface::set_cwd_from_args(i32 argc, const char** argv) {
        std::string p = std::filesystem::path(argv[0]).remove_filename().string();
        set_cwd(p);
    }


    basic_io_interface::basic_io_interface() {
    }

    basic_io_interface::~basic_io_interface() {
    }

    file_interface* basic_io_interface::open(const script_string& file, io_open_mode mode) {
        if (mode == io_open_mode::existing_only && !exists(file)) return nullptr;
        basic_file_interface* f = new basic_file_interface(this);
        if (!f->init(file, mode)) {
            delete f;
            return nullptr;
        }

        return f;
    }

    void basic_io_interface::close(file_interface* file) {
        delete file;
    }
    
    bool basic_io_interface::set_cwd(const script_string& dir) {
        if (!is_dir(dir)) return false;
        return chdir(dir.c_str()) == 0;
    }
    script_string basic_io_interface::get_cwd() {
        char cwd[256] = { 0 };
        // for some reason there is a compiler warning when the return value of getcwd is ignored...
        char* r = getcwd(cwd, 256);
        return script_string(cwd, strlen(cwd));
    }
    
    bool basic_io_interface::exists(const script_string& file) {
        struct stat s;
        return stat(file.c_str(), &s) == 0;
    }
    
    bool basic_io_interface::is_dir(const script_string& file) {
        struct stat s;
        if (stat(file.c_str(), &s) != 0) return false;
        return (s.st_mode & S_IFMT) == S_IFDIR;
    }



    basic_file_interface::basic_file_interface(io_interface* io) : file_interface(io), m_file(nullptr), m_cached_pos(0), m_cached_size(0) {
    }

    basic_file_interface::~basic_file_interface() {
        if (m_file) fclose(m_file);
    }

    bool basic_file_interface::set_position(u64 pos) {
        if (!m_file) return false;

        if (fseek(m_file, (long)pos, SEEK_SET) == 0) {
            m_cached_pos = pos;
            return true;
        }

        return false;
    }

    u64 basic_file_interface::get_position() {
        return m_cached_pos;
    }

    bool basic_file_interface::read(void* dest, u64 sz) {
        if (!m_file || (sz + m_cached_pos) > m_cached_size) return false;

        if (fread(dest, sz, 1, m_file) == 1) {
            m_cached_pos += sz;
            return true;
        }

        return false;
    }

    bool basic_file_interface::write(void* src, u64 sz) {
        if (!m_file) return false;
        
        if (fwrite(src, sz, 1, m_file) == 1) {
            m_cached_pos += sz;
            if (m_cached_pos > m_cached_size) m_cached_size = m_cached_pos;
            return true;
        }

        return false;
    }

    u64 basic_file_interface::size() {
        return m_cached_size;
    }

    bool basic_file_interface::init(const script_string& file, io_open_mode mode) {
        if (mode == io_open_mode::create_empty) fopen_s(&m_file, file.c_str(), "wb");
        else if (mode == io_open_mode::create_or_open) {
            if (m_owner->exists(file)) fopen_s(&m_file, file.c_str(), "rb+");
            else fopen_s(&m_file, file.c_str(), "wb");
        } else fopen_s(&m_file, file.c_str(), "rb+");

        if (m_file) {
            fseek(m_file, 0, SEEK_END);
            m_cached_size = ftell(m_file);
            fseek(m_file, 0, SEEK_SET);
            return true;
        }

        return false;
    }
};
