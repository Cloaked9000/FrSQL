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
    virtual ~Filesystem()=default;
    virtual void *open(const std::string &name, bool create) = 0;
    virtual void close(void *handle) = 0;
    virtual uint64_t read(void *handle, char *buf, uint64_t len) = 0;
    virtual void write(void *handle, char *buf, uint64_t len) = 0;
    virtual void seek(void *handle, uint64_t position) = 0;
    virtual uint64_t tell(void *handle) = 0;
};


#endif //TESTDB_FILESYSTEM_H
