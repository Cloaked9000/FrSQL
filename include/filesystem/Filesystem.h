//
// Created by fred on 21/05/2022.
//

#ifndef TESTDB_FILESYSTEM_H
#define TESTDB_FILESYSTEM_H
#include <string>
#include <vector>

class FilesystemBacking;
class Filesystem
{
public:
    class Handle
    {
    public:
        Handle()
        : Handle(nullptr, nullptr)
        {

        }

        explicit Handle(Filesystem *fs, void *ref)
        : fs(fs), ref(ref)
        {

        }

        ~Handle()
        {
            close();
        }

        Handle(const Handle&)=delete;
        void operator=(const Handle&)=delete;

        Handle(Handle&& o) noexcept
        : fs(o.fs), ref(o.ref)
        {
            o.ref = nullptr;
        }

        Handle& operator=(Handle&& o) noexcept
        {
            close();
            ref = o.ref;
            fs = o.fs;
            o.ref = nullptr;
            return *this;
        }

        void close()
        {
            if(ref)
            {
                fs->close(ref);
                ref = nullptr;
            }
        }

        bool is_open()
        {
            return ref != nullptr;
        }

        uint64_t read(void *buf, uint64_t len)
        {
            return fs->read(ref, static_cast<char *>(buf), len);
        }

        void write(const char *buf, uint64_t len)
        {
            fs->write(ref, buf, len);
        }

        uint64_t seek(uint64_t position)
        {
            return fs->seek(ref, position);
        }

        uint64_t tell()
        {
            return fs->tell(ref);
        }


    private:
        Filesystem *fs;
        void *ref;
    };

    virtual ~Filesystem()=default;
    virtual void *open(const std::string &name, bool create) = 0;
    virtual void close(void *handle) = 0;
    virtual uint64_t read(void *handle, char *buf, uint64_t len) = 0;
    virtual void write(void *handle, const char *buf, uint64_t len) = 0;
    virtual uint64_t seek(void *handle, uint64_t position) = 0;
    virtual uint64_t tell(void *handle) = 0;
};


#endif //TESTDB_FILESYSTEM_H
