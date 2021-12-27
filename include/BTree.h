//
// Created by fred on 31/07/2021.
//

#ifndef TESTDB_BTREE_H
#define TESTDB_BTREE_H
#include <array>
#include <algorithm>
#include <vector>
#include <limits>
#include <iostream>
#include <map>
#include <cassert>

template<typename T, size_t Order>
struct Node
{
public:
    using node_t = Node<T, Order>;
    size_t count = 0;
    std::array<T, Order - 1> list = {};
    std::array<node_t*, Order> children = {};

    Node()=default;
    Node(size_t count, std::array<T, Order - 1> items, std::array<node_t*, Order> children)
    : count(count),
      list(std::move(items)),
      children(std::move(children))
    {
    }

    bool operator==(const node_t &other) const
    {
        if(other.count != count)
            return false;
        if(!std::equal(other.list.begin(), other.list.begin() + count, list.begin()))
            return false;
        if(!std::equal(other.children.begin(), other.children.begin() + count + 1, children.begin()))
            return false;
        return true;
    }

    void erase(size_t position)
    {
        assert(position < count);
        for(size_t a = position; a < Order - 1; a++)
        {
            list[a] = list[a + 1];
        }
        for(size_t a = position; a < Order - 2; a++)
        {
            children[a + 1] = children[a + 2];
        }

        count--;
    }

    void insert(T val, node_t* &right_child, size_t insert_pos)
    {
        assert(insert_pos < Order);
        size_t index;
        for(index = count; index > insert_pos; index--)
        {
            list[index] = list[index - 1];
            children[index + 1] = children[index];
        }

        list[index] = std::move(val);
        children[index + 1] = right_child;
        count++;
    }


    bool is_full()
    {
        return count == Order - 1;
    }

    bool search(const T &val, size_t &location)
    {
        location = 0;
        while(location < count && val > list[location])
        {
            location++;
        }

        return location < count && val == list[location];
    }

    //Note: Overwrites last child
    void merge_right(node_t *right)
    {
        assert(right);
        for(size_t a = 0; a < right->count; a++)
        {
            list[count + a] = right->list[a];
        }
        for(size_t a = 0; a < right->count + 1; a++)
        {
            children[count + a] = right->children[a];
        }

        count += right->count;
    }

};

template<typename T, size_t Order>
class Tree
{
public:
    using node_t = Node<T, Order>;
    Tree()
    {
        root = nullptr;
    }

    bool search(const T &val)
    {
        node_t *node = root;
        while(root)
        {
            size_t location;
            if(node->search(val, location))
            {
                return true;
            }

            node = node->children[location];
        }

        return false;
    }

    template<typename ...Args>
    void insert(T val, const Args&... args)
    {
        insert(val);
        insert(args...);
    }

    void insert(T val)
    {
        bool is_taller = false;
        T median;

        node_t *right_child;
        insert_btree(root, std::move(val), median, right_child, is_taller);

        if(is_taller)
        {
            auto *temp_root = new node_t();
            temp_root->count = 1;
            temp_root->list[0] = median;
            temp_root->children[0] = root;
            temp_root->children[1] = right_child;
            root = temp_root;
            height++;
        }
    }

    void in_order()
    {
        std::map<size_t, std::vector<T>> levels;
        recurse(root, 0, levels);

        for(auto iter = levels.begin(); iter != levels.end(); iter++)
        {
            for(auto &l : iter->second)
            {
                std::cout << l << ", ";
            }
            std::cout << std::endl;
        }
    }

    bool erase(const T &val)
    {
        size_t position = 0;
        size_t depth = 1;
        bool doDelete = false;
        if(!erase(root, depth, val, doDelete, position))
        {
            return false;
        }

        //Tree is shrinking
#pragma clang diagnostic push
#pragma ide diagnostic ignored "ConstantConditionsOC"
#pragma ide diagnostic ignored "UnreachableCode"
        if(doDelete)
        {
           // rebalance_node(root, position, doDelete);
            if(doDelete)
            {
                root = root->children[0];
                height--;
            }
        }
#pragma clang diagnostic pop
        return true;
    }

private:
public:

    void rebalance_node(node_t *node, size_t position, bool &doDelete)
    {

        //Check each sibling node to see if either has more than the minimum
        if(position > 0 && node->children[position - 1]->count > Order / 2)
        {
            //Left sibling has enough. Move a key from left sibling into us, and one of our keys into right sibling
            node_t *left_child = node->children[position - 1];
            node_t *right_child = node->children[position];

            //Extract highest key from left child
            T left_val = left_child->list[left_child->count - 1];
            left_child->erase(left_child->count - 1);

            //Move our key into right child
            node_t *empty = nullptr;
            right_child->insert(node->list[position - 1], empty, 0);

            //Replace moved key with left
            node->list[position - 1] = left_val;

            //Now we're good
            doDelete = false;
        }
        else if(node->children[position + 1] && node->children[position + 1]->count > Order / 2)
        {
            //Right has enough
            node_t *left_child = node->children[position];
            node_t *right_child = node->children[position + 1];

            //Extract lowest key from right child
            T right_val = right_child->list[0];
            right_child->erase(0);

            //Move our key into left child
            node_t *empty = nullptr;
            left_child->insert(node->list[position], empty, right_child->count);

            //Replace moved key with right
            node->list[position] = right_val;

            doDelete = false;
        }
        else
        {
            //Else neither has enough, so we need to merge two siblings
            node_t *empty = nullptr;
            size_t median_pos;
            node_t *left, *right;
            if(node->children[position + 1])
            {
                //Merge with right sibling
                left = node->children[position];
                right = node->children[position + 1];
                median_pos = position;
            }
            else
            {
                //Merge with left sibling
                left = node->children[position - 1];
                right = node->children[position];
                median_pos = position - 1;
            }

            left->insert(node->list[median_pos], empty, left->count);
            left->merge_right(right);

            node->erase(median_pos);
            doDelete = node->count < Order / 2;
        }
    }

    bool erase(node_t *node, size_t depth, const T &val, bool &doDelete, size_t position)
    {
        if(!node)
        {
            return false;
        }

        bool found = node->search(val, position);
        if(!found)
        {
            // Keep searching
            bool erased = erase(node->children[position], ++depth, val, doDelete, position);

            // If the recursive erase succeeded, and we're in a re-balance state, re-balance the child
            if(doDelete)
            {
                rebalance_node(node, position, doDelete);
            }

            return erased;
        }

        // We've found it, found is true
        {
            // If we're not in a leaf, swap with child & erase from child instead
            if(depth != height)
            {
                node_t *leaf = node->children[position];
                std::swap(node->list[position], leaf->list[leaf->count - 1]);
                bool erased = erase(leaf, ++depth, val, doDelete, leaf->count - 1);
                if(doDelete)
                {
                    rebalance_node(node, position, doDelete);
                }
                return erased;
            }
        }

        //We've found the element & we're in a leaf node. Delete the item.
        node->erase(position);

        //If this node is still an acceptable size, we're done
        if(node->count >= Order / 2)
        {
            return true;
        }

        // Else we need to re-balance
        doDelete = true;
        return true;
    }

    void recurse(node_t *node, size_t level, std::map<size_t, std::vector<T>> &levels)
    {
        if(!node)
        {
            return;
        }

        recurse(node->children[0], level + 1, levels);
        for(size_t a = 0; a < node->count; a++)
        {
            levels[level].emplace_back(node->list[a]);
            recurse(node->children[a + 1], level + 1, levels);
        }
        levels[level].emplace_back();
    }

    void insert_btree(node_t *node, T val, T &median, node_t* &right_child, bool &is_taller)
    {
        if(!node)
        {
            //B-tree is empty or search ends at empty subtree
            median = val;
            right_child = nullptr;
            is_taller = true;
            return;
        }

        size_t location;
        bool found = node->search(val, location);
        if(found)
        {
            throw std::logic_error("Item already in tree!");
        }

        insert_btree(node->children[location], val, median, right_child, is_taller);
        if(is_taller)
        {
            if(node->is_full())
            {
                node_t *right;
                split_node(node, median, right_child, location, right, median);
                right_child = right;
            }
            else
            {
                node->insert(median, right_child, location);
                is_taller = false;
            }
        }

    }

    void split_node(node_t *node, T val, node_t *right_child, size_t insert_pos, node_t* &right_node, T &median)
    {
        right_node = new node_t();
        size_t mid = (Order - 1) / 2;

        // Insert into left side
        if(insert_pos <= mid)
        {
            //Move things on left side of mid into new node
            size_t index = 0;
            size_t i = mid;
            while(i < Order - 1)
            {
                right_node->list[index] = node->list[i];
                right_node->children[index + 1] = node->children[i + 1];
                index++;
                i++;
            }

            node->count = mid;
            node->insert(std::move(val), right_child, insert_pos);
            node->count--;
            median = node->list[node->count];

            right_node->count = index;
            right_node->children[0] = node->children[node->count + 1];
        }
        else //insert into right side
        {
            size_t index = 0;
            size_t i = mid + 1;
            while(i < Order - 1)
            {
                right_node->list[index] = node->list[i];
                right_node->children[index + 1] = node->children[i + 1];
                index++;
                i++;
            }

            node->count = mid;
            right_node->count = index;

            median = node->list[mid];
            right_node->insert(val, right_child, insert_pos - mid - 1);
            right_node->children[0] = node->children[node->count + 1];
        }
    }


    node_t *root;
    size_t height = 0;
};

#endif //TESTDB_BTREE_H
