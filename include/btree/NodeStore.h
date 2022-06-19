//
// Created by fred on 05/06/2022.
//

#ifndef TESTDB_NODESTORE_H
#define TESTDB_NODESTORE_H


#include <cstdint>
#include <unordered_map>
#include "filesystem/Filesystem.h"
#include "Node.h"

struct NodeStoreHeader
{
    uint64_t root = 0;
    uint64_t height = 0;
    uint64_t node_count = 1;
};

class NodeStore
{
public:
    ~NodeStore()
    {
        close();
    }

    bool create(Filesystem::Handle file_)
    {
        file = std::move(file_);
        file.seek(0);
        file.write(reinterpret_cast<char *>(&header), sizeof(header));
        return true;
    }

    bool open(Filesystem::Handle file_)
    {
        file = std::move(file_);
        file.seek(0);
        file.read(reinterpret_cast<char *>(&header), sizeof(header));
        return true;
    }

    void close()
    {
        if(file.is_open())
        {
            file.seek(0);
            file.write(reinterpret_cast<char *>(&header), sizeof(header));
        }
    }

    NodePtr alloc()
    {
        NodePtr ptr{this, &cache_node(alloc_node())};
        return ptr;
    }

    void free(Node *node)
    {
        write_node(node);
        uncache_node(node);
    }

    NodePtr load_root()
    {
        return load(header.root);
    }

    NodePtr load(uint64_t id)
    {
        if(!id)
        {
            return {nullptr, nullptr};
        }

        // Check in-memory working set first
        auto const iter = nodes.find(id);
        if(iter != nodes.end())
        {
            return {this, &iter->second};
        }

        // Couldn't find it, fallback to disk
        Node ret;
        if(id < header.node_count && read_node(id, &ret))
        {
            return {this, &cache_node(ret)};
        }

        return {nullptr, nullptr};
    }

    void set_root(NodePtr &node)
    {
        header.root = node->id;
    }

    uint64_t &height()
    {
        return header.height;
    }

private:
    Node alloc_node()
    {
        Node node;
        node.id = header.node_count++;
        assert(write_node(&node));
        return node;
    }

    bool read_node(uint64_t id, Node *in)
    {
        file.seek(sizeof(NodeStoreHeader) + ((id - 1) * sizeof(Node)));
        file.read(reinterpret_cast<char *>(in), sizeof(Node));
        return true;
    }

    bool write_node(Node *out)
    {
        file.seek(sizeof(NodeStoreHeader) + ((out->id - 1) * sizeof(Node)));
        file.write(reinterpret_cast<char *>(out), sizeof(Node));
        return true;
    }

    Node &cache_node(Node node)
    {
        return nodes.emplace(node.id, node).first->second;
    }

    void uncache_node(Node *node)
    {
        nodes.erase(node->id);
    }

    NodeStoreHeader header;
    Filesystem::Handle file;
    std::unordered_map<uint64_t, Node> nodes;
};

#endif //TESTDB_NODESTORE_H
