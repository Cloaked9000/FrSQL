//
// Created by fred on 19/03/2021.
//

#include "TestUtils.h"

TEST(StackTest, test_push_pop)
{
    Stack<int> stack;
    ASSERT_TRUE(stack.empty());
    ASSERT_EQ(stack.size(), 0);
    stack.push(10);
    ASSERT_EQ(stack.size(), 1);
    ASSERT_FALSE(stack.empty());
    stack.push(20);
    ASSERT_EQ(stack.pop(), 20);
    ASSERT_EQ(stack.size(), 1);
    ASSERT_EQ(stack.pop(), 10);
    ASSERT_TRUE(stack.empty());
}

TEST(StackTest, test_pop_multi)
{
    Stack<int> stack;
    stack.push(10).push(20).push(30);
    stack.pop(2);
    ASSERT_EQ(stack.pop(), 10);
}

TEST(StackTest, test_clear)
{
    Stack<int> stack;
    stack.push(5).push(10);
    stack.clear();
    ASSERT_TRUE(stack.empty());
}