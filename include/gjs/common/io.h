#pragma once
#include <gjs/common/types.h>
#include <gjs/builtin/script_string.h>

namespace gjs {
    class io_interface;
    enum class io_open_mode {
        // only opens the file if it exists
        existing_only,
        // creates the file if it doesn't exist, otherwise opens it normally
        create_or_open,
        // creates the file if it doesn't exist, opens it and empties it if it does
        create_empty
    };

    class file_interface {
        public:
            file_interface(io_interface* io);
            virtual ~file_interface() { }

            inline io_interface* owner() const { return m_owner; }
            const script_string& filename() const { return m_filename; }

            virtual bool set_position(u64 pos) = 0;
            virtual u64 get_position() = 0;
            virtual bool read(void* dest, u64 sz) = 0;
            virtual bool write(void* src, u64 sz) = 0;
            virtual u64 size() = 0;

        protected:
            friend class io_interface;
            virtual bool init(const script_string& file, io_open_mode mode) = 0;

            io_interface* m_owner;
            script_string m_filename;
    };

    // todo: more script_array work...
    struct dir_entry {
        dir_entry();
        ~dir_entry() { }

        script_string label;
        bool is_dir;
    };

    class io_interface {
        public:
            io_interface() { }
            virtual ~io_interface() { }

            virtual file_interface* open(const script_string& file, io_open_mode mode) = 0;
            virtual void close(file_interface* file) = 0;
            virtual bool set_cwd(const script_string& dir) = 0;
            virtual script_string get_cwd() = 0;
            virtual bool exists(const script_string& file) = 0;
            virtual bool is_dir(const script_string& file) = 0;

            void set_cwd_from_args(i32 argc, const char* argv[]);
            
            // virtual script_array directory(const script_string& dir) = 0;
    };

    class basic_io_interface : public io_interface {
        public:
            basic_io_interface();
            virtual ~basic_io_interface();

            virtual file_interface* open(const script_string& file, io_open_mode mode);
            virtual void close(file_interface* file);
            virtual bool set_cwd(const script_string& dir);
            virtual script_string get_cwd();
            virtual bool exists(const script_string& file);
            virtual bool is_dir(const script_string& file);
    };

    class basic_file_interface : public file_interface {
        public:
            basic_file_interface(io_interface* io);
            virtual ~basic_file_interface();

            virtual bool set_position(u64 pos);
            virtual u64 get_position();
            virtual bool read(void* dest, u64 sz);
            virtual bool write(void* src, u64 sz);
            virtual u64 size();

        protected:
            friend class basic_io_interface;
            virtual bool init(const script_string& file, io_open_mode mode);

            u64 m_cached_size;
            u64 m_cached_pos;
            FILE* m_file;
    };
};

