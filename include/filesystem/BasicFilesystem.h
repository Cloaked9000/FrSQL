//
// Created by fred on 29/05/2022.
//

#ifndef TESTDB_BASICFILESYSTEM_H
#define TESTDB_BASICFILESYSTEM_H

#include <cassert>
#include <fstream>
#include <memory>
#include "filesystem/Filesystem.h"
#include "FilesystemBacking.h"

struct FILE_HEADER
{
    uint64_t stream_count = 0;
} __attribute__((packed));

#define PAGE_SIZE 0x1000
struct PAGE_HEADER;
struct STREAM_HEADER;
class BasicFilesystem : public Filesystem
{
public:
    explicit BasicFilesystem(std::unique_ptr<FilesystemBacking> backing);
    ~BasicFilesystem() override;
    BasicFilesystem(BasicFilesystem&&)=delete;
    BasicFilesystem(const Filesystem&)=delete;
    void operator=(BasicFilesystem&&)=delete;
    void operator=(const BasicFilesystem&&)=delete;

    static bool Format(const std::unique_ptr<FilesystemBacking> &backing);
    void *open(const std::string &name, bool create) override;
    void close(void *handle) override;
    uint64_t read(void *handle_, char *buf, uint64_t len) override;
    void write(void *handle, char *buf, uint64_t len) override;
    void seek(void *handle, uint64_t position) override;
    uint64_t tell(void *handle) override;

private:
    bool load();
    void *create(const std::string &name);

    PAGE_HEADER read_page_header(uint64_t index);
    STREAM_HEADER read_stream_header(uint64_t index);
    void write_stream_header(uint64_t index, const STREAM_HEADER &header);
    void write_page_header(const PAGE_HEADER &header);
    uint64_t alloc_page(uint64_t previous_page);

    std::vector<STREAM_HEADER> streams;
    std::unique_ptr<FilesystemBacking> backing;
    FILE_HEADER fs_header;
};


#endif //TESTDB_BASICFILESYSTEM_H
