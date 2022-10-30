//
// Created by fred on 29/05/2022.
//

#ifndef TESTDB_FILESYSTEMBACKING_H
#define TESTDB_FILESYSTEMBACKING_H

#include <fstream>
#include <string>

class FilesystemBacking
{
public:
    virtual ~FilesystemBacking()=default;
    [[nodiscard]] virtual bool open(const std::string &path, bool create) = 0;
    [[nodiscard]] virtual bool write(const char *buf, uint64_t buflen) = 0;
    [[nodiscard]] virtual bool read(char *buf, uint64_t buflen) = 0;
    [[nodiscard]] virtual uint64_t tellg() = 0;
    virtual void seekg(int64_t pos, std::ios_base::seekdir base) = 0;
    [[nodiscard]] virtual bool good() const = 0;
    [[nodiscard]] virtual bool is_open() const = 0;
    virtual void close() = 0;
};

class DiskBacking : public FilesystemBacking
{
public:
    [[nodiscard]] bool open(const std::string &path, bool create) override
    {
        assert(!file.is_open());
        if(create)
        {
            file.open(path, std::ios::out);
            file.close();
        }

        file.open(path, std::ios::in | std::ios::out | std::ios::binary);
        return file.is_open();
    }

    [[nodiscard]] bool write(const char *buf, uint64_t buflen) override
    {
        assert(file.is_open());
        assert(file.good());
        file.write(buf, buflen);
        assert(file.good());
        return file.good();
    }

    [[nodiscard]] bool read(char *buf, uint64_t buflen) override
    {
        assert(file.is_open());
        assert(file.good());
        file.read(buf, buflen);
        assert(file.good());
        return file.good();
    }

    [[nodiscard]] uint64_t tellg() override
    {
        assert(file.is_open());
        assert(file.good());
        return file.tellg();
    }

    void seekg(int64_t pos, std::ios_base::seekdir base) override
    {
        assert(file.is_open());
        file.seekg(pos, base);
        assert(file.good());
    }

    [[nodiscard]] bool good() const override
    {
        assert(file.is_open());
        return file.good();
    }

    [[nodiscard]] bool is_open() const override
    {
        return file.is_open();
    }

    void close() override
    {
        file.close();
    }

private:
    std::fstream file;
};

class MemoryBacking : public FilesystemBacking
{
public:

    [[nodiscard]] bool open(const std::string &, bool) override
    {
        return true;
    }

    [[nodiscard]] bool write(const char *buf, uint64_t buflen) override
    {
        if(cursor + buflen > buffer.size())
        {
            buffer.resize(cursor + buflen);
        }

        memcpy(buffer.data() + cursor, buf, buflen);
        cursor += buflen;
        return true;
    }

    [[nodiscard]] bool read(char *buf, uint64_t buflen) override
    {
        if(cursor + buflen > buffer.size())
        {
            return false;
        }

        memcpy(buf, buffer.data() + cursor, buflen);
        return true;
    }

    [[nodiscard]] uint64_t tellg() override
    {
        return cursor;
    }

    void seekg(int64_t pos, std::ios_base::seekdir base) override
    {
        switch(base)
        {
            case std::ios::beg:
                cursor = pos;
                break;
            case std::ios::end:
                cursor = buffer.size() - pos;
                break;
            case std::ios::cur:
                cursor += pos;
                break;
            default:
                abort();
        }

        if(cursor > buffer.size())
        {
            __debugbreak();
        }
        assert(cursor <= buffer.size());
    }

    [[nodiscard]] bool good() const override
    {
        return true;
    }

    [[nodiscard]] bool is_open() const override
    {
        return true;
    }

    void close() override
    {

    }

private:
    uint64_t cursor = 0;
    std::string buffer;
};

#endif //TESTDB_FILESYSTEMBACKING_H
