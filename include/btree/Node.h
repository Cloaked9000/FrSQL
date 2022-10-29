//
// Created by fred on 05/06/2022.
//

#ifndef TESTDB_NODE_H
#define TESTDB_NODE_H

#include <cassert>
#include <cstdint>
#include <algorithm>
#include <string>

static constexpr auto NodeOrder = 5;

class NodeStore;
struct Node;
class NodePtr
{
public:
    NodePtr()
    : NodePtr(nullptr, nullptr)
    {

    }

    NodePtr(NodeStore *store, Node *node)
    : store(store), node(node)
    {

    }

    NodePtr(const NodePtr&)=delete;
    void operator=(const NodePtr&)=delete;
    NodePtr(NodePtr&&o) noexcept
            : store(o.store),
              node(o.node)
    {
        o.store = nullptr;
    }

    NodePtr &operator=(NodePtr&& o) noexcept
    {
        release();
        store = o.store;
        node = o.node;
        o.store = nullptr;
        return *this;
    }

    ~NodePtr()
    {
        release();
    }

    void release();

    Node *operator->()
    {
        return node;
    }

    bool valid()
    {
        return node != nullptr;
    }

private:
    NodeStore *store;
    Node *node;
};

struct Node
{
    uint64_t id = 0;
    uint64_t list[NodeOrder - 1]{};
    uint64_t values[NodeOrder - 1]{};
    uint64_t children[NodeOrder]{};
    uint64_t count = 0;

    bool operator==(const Node &other) const
    {
        if(other.count != count)
            return false;
        if(!std::equal(std::begin(other.list), std::begin(other.list) + count, std::begin(list)))
            return false;
        if(!std::equal(std::begin(other.children), std::begin(other.children) + count + 1, std::begin(children)))
            return false;
        return true;
    }

    void erase(size_t position)
    {
        assert(position < count);
        for(size_t a = position; a < NodeOrder - 2; a++)
        {
            list[a] = list[a + 1];
            values[a] = values[a + 1];
        }
        for(size_t a = position; a < NodeOrder - 2; a++)
        {
            children[a + 1] = children[a + 2];
        }

        count--;
    }

    void insert(uint64_t val, uint64_t val2, uint64_t right_child, size_t insert_pos)
    {
        assert(insert_pos < NodeOrder);
        size_t index;
        for(index = count; index > insert_pos; index--)
        {
            list[index] = list[index - 1];
            values[index] = values[index - 1];
            children[index + 1] = children[index];
        }

        list[index] = val;
        values[index] = val2;
        children[index + 1] = right_child;
        count++;
    }


    [[nodiscard]] bool is_full() const
    {
        return count == NodeOrder - 1;
    }

    bool search(uint64_t val, size_t &location)
    {
        location = 0;
        while(location < count && val > list[location])
        {
            location++;
        }

        return location < count && val == list[location];
    }

    //Note: Overwrites last child
    void merge_right(NodePtr &right)
    {
        assert(right->count);
        for(size_t a = 0; a < right->count; a++)
        {
            list[count + a] = right->list[a];
            values[count + a] = right->values[a];
        }
        for(size_t a = 0; a < right->count + 1; a++)
        {
            children[count + a] = right->children[a];
        }

        count += right->count;
    }
};



#endif //TESTDB_NODE_H
