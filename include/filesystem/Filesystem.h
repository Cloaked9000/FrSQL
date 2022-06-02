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

        }

        Handle& operator=(Handle&& o) noexcept
        {
            close();
            ref = o.ref;
            fs = o.fs;
            o.ref = nullptr;
        }

        void close()
        {
            if(ref)
            {
                fs->close(ref);
                ref = nullptr;
            }
        }

        uint64_t read(char *buf, uint64_t len)
        {
            return fs->read(ref, buf, len);
        }

        void write(char *buf, uint64_t len)
        {
            fs->write(ref, buf, len);
        }

        void seek(uint64_t position)
        {
            fs->seek(fs, position);
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
    virtual void write(void *handle, char *buf, uint64_t len) = 0;
    virtual void seek(void *handle, uint64_t position) = 0;
    virtual uint64_t tell(void *handle) = 0;
};


#endif //TESTDB_FILESYSTEM_H