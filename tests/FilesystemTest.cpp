//
// Created by fred on 29/05/2022.
//

#include "filesystem/BasicFilesystem.h"
#include "TestUtils.h"
#include "filesystem/FilesystemBacking.h"
#define DEF_FS \
std::unique_ptr<FilesystemBacking> backing(new MemoryBacking()); \
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
    ASSERT_TRUE(handle.is_open());
}

TEST(FilesystemTest, test_open_invalid_file)
{
    DEF_FS
    auto handle = fs.open("file1", false);
    ASSERT_FALSE(handle.is_open());
}

TEST(FilesystemTest, test_open_valid_file)
{
    DEF_FS
    {
        auto handle = fs.open("file1", true);
        ASSERT_TRUE(handle.is_open());
    }
    {
        auto handle = fs.open("file1", false);
        ASSERT_TRUE(handle.is_open());
    }
}

TEST(FilesystemTest, test_single_page_read_write)
{
    DEF_FS
    auto handle = fs.open("file1", true);
    handle.write("hello", 5);

    std::string buf(5, '\0');
    handle.seek(0);
    handle.read(buf.data(), buf.size());
    ASSERT_EQ(buf, "hello");
}

TEST(FilesystemTest, test_large_multi_page_read_write)
{
    DEF_FS
    auto source = gen_random(PAGE_SIZE * 5);
    auto handle = fs.open("file1", true);
    handle.write(source.data(), source.size());

    std::string sink(source.size(), '\0');
    handle.seek(0);
    handle.read(sink.data(), sink.size());
    ASSERT_EQ(sink, source);
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
        handle.write(cpy.data(), amount);
        cpy.erase(0, amount);
        amount *= 3;
    }

    std::string sink(source.size(), '\0');
    handle.seek(0);

    amount = 1;
    uint64_t offset = 0;
    while(handle.read(&sink[offset], amount))
    {
        offset += amount;
        amount *= 3;
        if(offset + amount > sink.size())
        {
            amount = sink.size() - offset;
        }
    }

    ASSERT_EQ(sink, source);
}

TEST(FilesystemTest, test_read_too_much)
{
    DEF_FS
    auto handle = fs.open("file1", true);
    handle.write("hello", 5);
    handle.seek(0);

    std::string out(5, '\0');
    ASSERT_EQ(5, handle.read(out.data(), 200));
}

TEST(FilesystemTest, test_single_page_seek)
{
    DEF_FS
    auto handle = fs.open("file1", true);
    handle.write("hello", 5);

    std::string sink(5, '\0');
    handle.seek(0);
    handle.read(sink.data(), 1);
    ASSERT_EQ(sink[0], 'h');

    handle.seek(4);
    handle.read(sink.data(), 1);
    ASSERT_EQ(sink[0], 'o');
}

TEST(FilesystemTest, test_multi_page_seek)
{
    DEF_FS
    char buf;
    auto source = gen_random(PAGE_SIZE * 5);
    auto handle = fs.open("file1", true);
    handle.write(source.data(), source.size());

    handle.seek(0); // seek backwards 5 pages
    handle.read(&buf, 1);
    ASSERT_EQ(buf, source[0]);

    handle.seek(PAGE_SIZE * 2); // seek forwards a couple pages
    handle.read(&buf, 1);
    ASSERT_EQ(buf, source[PAGE_SIZE * 2]);
}

TEST(FilesystemTest, test_seek_past_end_single_page)
{
    DEF_FS
    auto handle = fs.open("file1", true);
    handle.write("hello", 5);
    handle.seek(1);
    handle.seek(10);
    ASSERT_EQ(handle.tell(), 5);
}

TEST(FilesystemTest, test_seek_past_end_multi_page)
{
    DEF_FS
    auto source = gen_random(PAGE_SIZE * 5);
    auto handle = fs.open("file1", true);
    handle.write(source.data(), source.size());
    handle.seek(0);
    ASSERT_EQ(handle.tell(), 0);
    handle.seek(100000);
    ASSERT_EQ(handle.tell(), source.size());
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
        handle.write(&source1[a], PAGE_SIZE);
        handle2.write(&source2[a], PAGE_SIZE);
    }

    handle.seek(0);
    handle2.seek(0);

    std::string sink1(source1.size(), '\0');
    std::string sink2(source2.size(), '\0');

    handle.read(sink1.data(), source1.size());
    handle2.read(sink2.data(), source2.size());

    ASSERT_EQ(sink1, source1);
    ASSERT_EQ(sink2, source2);
}