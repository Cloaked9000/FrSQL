//
// Created by fred on 29/05/2022.
//

#include "filesystem/BasicFilesystem.h"
#include "TestUtils.h"
#include "filesystem/FilesystemBacking.h"
#define DEF_FS std::string data; \
std::unique_ptr<FilesystemBacking> backing(new MemoryBacking(&data)); \
BasicFilesystem::Format(backing);                                 \
BasicFilesystem fs(std::move(backing));

std::string gen_random(size_t len, size_t div = 10)
{
    std::string source(len, '\0');
    for(size_t a = 0; a < source.size(); a++)
    {
        source[a] = static_cast<char>(a % div);
    }
    return source;
}

TEST(FilesystemTest, test_create_new_file)
{
    DEF_FS
    auto handle = fs.open("file1", true);
    ASSERT_NE(nullptr, handle);
    fs.close(handle);
}

TEST(FilesystemTest, test_open_invalid_file)
{
    DEF_FS
    auto handle = fs.open("file1", false);
    ASSERT_EQ(nullptr, handle);
}

TEST(FilesystemTest, test_open_valid_file)
{
    DEF_FS
    auto handle = fs.open("file1", true);
    fs.close(handle);
    handle = fs.open("file1", false);
    ASSERT_NE(nullptr, handle);
    fs.close(handle);
}

TEST(FilesystemTest, test_single_page_read_write)
{
    DEF_FS
    auto handle = fs.open("file1", true);
    fs.write(handle, (char*)"hello", 5);

    std::string buf(5, '\0');
    fs.seek(handle, 0);
    fs.read(handle, buf.data(), buf.size());
    ASSERT_EQ(buf, "hello");
    fs.close(handle);
}

TEST(FilesystemTest, test_large_multi_page_read_write)
{
    DEF_FS
    auto source = gen_random(PAGE_SIZE * 5);
    auto handle = fs.open("file1", true);
    fs.write(handle, source.data(), source.size());

    std::string sink(source.size(), '\0');
    fs.seek(handle, 0);
    fs.read(handle, sink.data(), sink.size());
    ASSERT_EQ(sink, source);
    fs.close(handle);
}

TEST(FilesystemTest, test_tiny_multi_page_read_write)
{
    DEF_FS
    auto source = gen_random(PAGE_SIZE * 5);

    auto handle = fs.open("file1", true);
    std::string cpy = source;
    uint64_t amount = 1;
    while(!cpy.empty())
    {
        fs.write(handle, cpy.data(), amount);
        cpy.erase(0, amount);
        amount *= 3;
    }

    std::string sink(source.size(), '\0');
    fs.seek(handle, 0);

    amount = 1;
    uint64_t offset = 0;
    while(fs.read(handle, &sink[offset], amount))
    {
        offset += amount;
        amount *= 3;
        if(offset + amount > sink.size())
        {
            amount = sink.size() - offset;
        }
    }

    ASSERT_EQ(sink, source);
    fs.close(handle);
}

TEST(FilesystemTest, test_read_too_much)
{
    DEF_FS
    auto handle = fs.open("file1", true);
    fs.write(handle, (char*)"hello", 5);

    std::string out(5, '\0');
    ASSERT_EQ(5, fs.read(handle, out.data(), 200));
    fs.close(handle);
}

TEST(FilesystemTest, test_single_page_seek)
{
    DEF_FS
    auto handle = fs.open("file1", true);
    fs.write(handle, (char*)"hello", 5);

    std::string sink(5, '\0');
    fs.seek(handle, 0);
    fs.read(handle, sink.data(), 1);
    ASSERT_EQ(sink[0], 'h');

    fs.seek(handle, 4);
    fs.read(handle, sink.data(), 1);
    ASSERT_EQ(sink[0], 'o');
    fs.close(handle);
}

TEST(FilesystemTest, test_multi_page_seek)
{
    DEF_FS
    char buf;
    auto source = gen_random(PAGE_SIZE * 5);
    auto handle = fs.open("file1", true);
    fs.write(handle, source.data(), source.size());

    fs.seek(handle, 0); // seek backwards 5 pages
    fs.read(handle, &buf, 1);
    ASSERT_EQ(buf, source[0]);

    fs.seek(handle, PAGE_SIZE * 2); // seek forwards a couple pages
    fs.read(handle, &buf, 1);
    ASSERT_EQ(buf, source[PAGE_SIZE]);
    fs.close(handle);
}

TEST(FilesystemTest, test_seek_past_end_single_page)
{
    DEF_FS
    auto handle = fs.open("file1", true);
    fs.write(handle, (char*)"hello", 5);
    fs.seek(handle, 1);
    fs.seek(handle, 10);
    ASSERT_EQ(fs.tell(handle), 5);
    fs.close(handle);
}

TEST(FilesystemTest, test_seek_past_end_multi_page)
{
    DEF_FS
    auto source = gen_random(PAGE_SIZE * 5);
    auto handle = fs.open("file1", true);
    fs.write(handle, source.data(), source.size());
    fs.seek(handle, 0);
    ASSERT_EQ(fs.tell(handle), 0);
    fs.seek(handle, 100000);
    ASSERT_EQ(fs.tell(handle), source.size());
    fs.close(handle);
}

TEST(FilesystemTest, test_interspersed_file_read_write)
{
    DEF_FS
    auto source1 = gen_random(PAGE_SIZE * 5, 10);
    auto source2 = gen_random(PAGE_SIZE * 5, 7);

    auto handle = fs.open("file1", true);
    auto handle2 = fs.open("file2", true);

    for(size_t a = 0; a < source1.size(); a+= PAGE_SIZE)
    {
        fs.write(handle, &source1[a], PAGE_SIZE);
        fs.write(handle2, &source2[a], PAGE_SIZE);
    }

    fs.seek(handle, 0);
    fs.seek(handle2, 0);

    std::string sink1(source1.size(), '\0');
    std::string sink2(source2.size(), '\0');

    fs.read(handle, sink1.data(), source1.size());
    fs.read(handle2, sink2.data(), source2.size());

    ASSERT_EQ(sink1, source1);
    ASSERT_EQ(sink2, source2);

    fs.close(handle);
    fs.close(handle2);
}