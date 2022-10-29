//
// Created by fred on 29/10/2022.
//

#ifndef TESTDB_FILEHANDLESERIALIZERADAPTER_H
#define TESTDB_FILEHANDLESERIALIZERADAPTER_H

#include "Serializer.h"
#include "filesystem/Filesystem.h"

class FilehandleSerializerAdapter : public serializer::Medium
{
public:
    explicit FilehandleSerializerAdapter(Filesystem::Handle &file)
    : file(file){}

    [[nodiscard]] bool read(char *ptr, size_t len) override
    {
        return file.read(ptr, len) == len;
    }
    [[nodiscard]] bool write(const char  *ptr, size_t len) override
    {
        file.write(ptr, len);
        return true;
    }

private:
    Filesystem::Handle &file;
};


#endif //TESTDB_FILEHANDLESERIALIZERADAPTER_H
