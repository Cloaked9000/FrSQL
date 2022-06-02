//
// Created by fred on 29/05/2022.
//

#include <memory>
#include "filesystem/BasicFilesystem.h"
#include "filesystem/FilesystemBacking.h"

struct PAGE_HEADER
{
    uint64_t page_length = sizeof(PAGE_HEADER);
    uint64_t previous_page = 0;
    uint64_t current_page = 0;
    uint64_t next_page = 0;
} __attribute__((packed));

struct STREAM_HEADER
{
    char name[16]{};
    uint64_t page = 0;
    uint64_t size = 0;
    uint64_t id = 0;
} __attribute__((packed));


struct StreamHandle
{
    uint64_t cursor = sizeof(PAGE_HEADER);
    uint64_t stream_pos = 0;
    PAGE_HEADER currentPage{};
    STREAM_HEADER stream;
};

BasicFilesystem::BasicFilesystem(std::unique_ptr<FilesystemBacking> backing)
        : backing(std::move(backing))
{
    if(!load())
    {
        abort();
    }
}

BasicFilesystem::~BasicFilesystem()
{
    backing->seekg(0, std::ios::beg);
    backing->write(reinterpret_cast<const char *>(&fs_header), sizeof(fs_header));
}


bool BasicFilesystem::Format(const std::unique_ptr<FilesystemBacking> &backing)
{
    char page[PAGE_SIZE]{};
    FILE_HEADER header;
    memcpy(page, &header, sizeof(header));
    return backing->write(page, PAGE_SIZE);
}

bool BasicFilesystem::load()
{
    bool failed = false;
    failed |= !backing->read(reinterpret_cast<char*>(&fs_header), sizeof(fs_header));
    for(uint64_t a = 0; a < fs_header.stream_count; a++)
    {
        streams.emplace_back(read_stream_header(a));
    }
    return failed;
}

void BasicFilesystem::close(void *handle)
{
    auto ptr = reinterpret_cast<StreamHandle*>(handle);
    streams[ptr->stream.id] = ptr->stream;
    write_stream_header(ptr->stream.id, ptr->stream);
    delete ptr;
}

uint64_t BasicFilesystem::read(void *handle_, char *buf, uint64_t len)
{
    auto handle = reinterpret_cast<StreamHandle*>(handle_);
    uint64_t bytes_read = 0;
    while(len)
    {
        assert(handle->currentPage.page_length >= handle->cursor);
        uint64_t bytes_remaining = handle->currentPage.page_length - handle->cursor;
        if(!bytes_remaining) // need to move to next page, nothing left
        {
            if(!handle->currentPage.next_page)
            {
                break;
            }

            handle->currentPage = read_page_header(handle->currentPage.next_page);
            handle->cursor = sizeof(PAGE_HEADER);
            continue;
        }

        if(bytes_remaining > len)
        {
            bytes_remaining = len;
        }

        backing->seekg(handle->currentPage.current_page * PAGE_SIZE + handle->cursor, std::ios::beg);
        backing->read(buf, static_cast<std::streamsize>(bytes_remaining));
        bytes_read += bytes_remaining;
        buf += bytes_remaining;
        len -= bytes_remaining;
        handle->cursor += bytes_remaining;
    }

    return bytes_read;
}

void BasicFilesystem::write(void *handle_, char *buf, uint64_t len)
{
    auto handle = reinterpret_cast<StreamHandle*>(handle_);
    while(len)
    {
        backing->seekg(handle->currentPage.current_page * PAGE_SIZE + handle->cursor, std::ios::beg);
        uint64_t bytes_in_page = PAGE_SIZE - handle->cursor;
        uint64_t bytes_to_write = len > bytes_in_page ? bytes_in_page : len;
        backing->write(buf, bytes_to_write);
        buf += bytes_to_write;
        handle->cursor += bytes_to_write;
        len -= bytes_to_write;

        if(handle->cursor > handle->currentPage.page_length || !bytes_to_write)
        {
            handle->stream.size += bytes_to_write;
            handle->currentPage.page_length = handle->cursor;
            handle->stream_pos += bytes_to_write;
            if(handle->cursor == PAGE_SIZE)
            {
                handle->currentPage.next_page = alloc_page(handle->currentPage.current_page);
                write_page_header(handle->currentPage);
                handle->currentPage = read_page_header(handle->currentPage.next_page);
                handle->cursor = sizeof(PAGE_HEADER);
            }
            else
            {
                write_page_header(handle->currentPage);
            }
        }
    }

}

void *BasicFilesystem::open(const std::string &name, bool should_create)
{
    auto iter = std::find_if(streams.begin(), streams.end(),  [&name](auto &a) {
        return name.compare(0, name.size(), a.name) == 0;
    });

    if(iter == streams.end())
    {
        return should_create ? create(name) : nullptr;
    }

    auto handle = new StreamHandle();
    handle->stream = *iter;
    handle->cursor = sizeof(PAGE_HEADER);
    handle->currentPage = read_page_header(iter->page);
    return handle;
}

PAGE_HEADER BasicFilesystem::read_page_header(uint64_t index)
{
    PAGE_HEADER ret;
    const auto orig = backing->tellg();
    backing->seekg(index * PAGE_SIZE, std::ios::beg);
    backing->read(reinterpret_cast<char *>(&ret), sizeof(PAGE_HEADER));
    backing->seekg(orig, std::ios::beg);
    return ret;
}

void BasicFilesystem::write_page_header(const PAGE_HEADER &header)
{
    const auto orig = backing->tellg();
    backing->seekg(header.current_page * PAGE_SIZE, std::ios::beg);
    backing->write(reinterpret_cast<const char *>(&header), sizeof(PAGE_HEADER));
    backing->seekg(orig, std::ios::beg);
}

uint64_t BasicFilesystem::alloc_page(uint64_t previous_page)
{
    const auto orig = backing->tellg();
    backing->seekg(0, std::ios::end);
    assert(backing->tellg() % PAGE_SIZE == 0);
    const uint64_t page = backing->tellg() / PAGE_SIZE;
    char empty[PAGE_SIZE]{};
    PAGE_HEADER header;
    header.current_page = page;
    header.previous_page = previous_page;
    memcpy(empty, &header, sizeof(header));
    backing->write(empty, PAGE_SIZE);
    backing->seekg(orig, std::ios::beg);
    return page;
}

void *BasicFilesystem::create(const std::string &name)
{
    assert(!name.empty());
    assert(name.size() <= sizeof(STREAM_HEADER::name));
    STREAM_HEADER stream;
    memcpy(stream.name, name.c_str(), name.size());
    stream.page = alloc_page(0);
    stream.id = fs_header.stream_count++;

    backing->seekg(sizeof(FILE_HEADER) + streams.size() * sizeof(STREAM_HEADER), std::ios::beg);
    backing->write(reinterpret_cast<const char *>(&stream), sizeof(STREAM_HEADER));

    PAGE_HEADER page;
    page.current_page = stream.page;
    write_page_header(page);

    streams.emplace_back(stream);
    return open(name, false);
}

void BasicFilesystem::seek(void *handle_, uint64_t position)
{
    auto handle = reinterpret_cast<StreamHandle*>(handle_);

    uint64_t desired_page = position / (PAGE_SIZE - sizeof(STREAM_HEADER));
    uint64_t current_page = handle->stream_pos / (PAGE_SIZE - sizeof(STREAM_HEADER));
    while(current_page < desired_page)
    {
        if(!handle->currentPage.next_page)
        {
            handle->cursor = PAGE_SIZE;
            handle->stream_pos = handle->stream.size;
            return;
        }

        handle->currentPage = read_page_header(handle->currentPage.next_page);
        current_page++;
    }

    while(current_page > desired_page)
    {
        handle->currentPage = read_page_header(handle->currentPage.previous_page);
        current_page--;
    }

    handle->cursor = sizeof(PAGE_HEADER) + (position % (PAGE_SIZE - sizeof(PAGE_HEADER)));
    handle->stream_pos = position > handle->stream.size ? handle->stream.size : position;
}

STREAM_HEADER BasicFilesystem::read_stream_header(uint64_t index)
{
    STREAM_HEADER ret;
    backing->read(reinterpret_cast<char *>(&streams.back()), sizeof(STREAM_HEADER));
    return ret;
}

void BasicFilesystem::write_stream_header(uint64_t index, const STREAM_HEADER &header)
{
    auto origin = backing->tellg();
    backing->seekg(sizeof(FILE_HEADER) + index * sizeof(STREAM_HEADER), std::ios::beg);
    backing->write(reinterpret_cast<const char *>(&header), sizeof(header));
    backing->seekg(origin, std::ios::beg);
}

uint64_t BasicFilesystem::tell(void *handle)
{
    return reinterpret_cast<StreamHandle*>(handle)->stream_pos;
}
