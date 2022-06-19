//
// Created by fred on 05/06/2022.
//

#include "btree/Node.h"
#include "btree/NodeStore.h"

void NodePtr::release()
{
    if(store)
    {
        store->free(node);
    }
}
