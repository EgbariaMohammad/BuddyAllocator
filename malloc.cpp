
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <sys/mman.h>

#include "malloc.hpp"

#define is ==
#define isnot !=
#define returned ==

int BuddyAllocator::findOrder(size_t size) // basically log function
{
    size_t number = size/MIN_BLOCK_SIZE;
    int order = -1;
    while (number)
    {
        number >>= 1;
        ++order;
    }
    return order;
}

size_t BuddyAllocator::sizeAfterSplitting(size_t size)
{
    return (size + META_DATA_SIZE)/2 - META_DATA_SIZE;
}


BuddyAllocator::BuddyAllocator() :
    blocksList(nullptr), mmapedList(nullptr), cookie(rand()), blocksNum(0), freeBlocksNum(0), freeBytesNum(0), totalAllocatedBytes(0)
{
    for (int i = 0; i < MAX_ORDER; i++)
        orderTable[i] = nullptr;

    size_t address = ( (intptr_t) sbrk(0) ) % ALIGNMENT;
    address = ALIGNMENT - address;
    if(sbrk(address) returned FAILURE) // align the heap
        throw BuddyAllocatorException();

    void* heapAddress = sbrk(INITIAL_BLOCKS_NUM*INITIAL_BLOCK_SIZE);
    if(heapAddress returned FAILURE)
        throw BuddyAllocatorException();

    MallocMetaData* iterator = blocksList = (MallocMetaData*) heapAddress;
    for(int i=0; i<INITIAL_BLOCKS_NUM; i++)
    {
		MallocMetaData* next = nullptr, *prev = nullptr;
		prev = (i == 0) ? nullptr : (MallocMetaData*) ((char*)iterator-INITIAL_BLOCK_SIZE) ;
		next = (i == INITIAL_BLOCKS_NUM-1) ? nullptr : (MallocMetaData*) ((char*)iterator+INITIAL_BLOCK_SIZE) ;
        int size = INITIAL_BLOCK_SIZE - META_DATA_SIZE;
        *iterator = MallocMetaData(size, cookie, prev, next);
        iterator = iterator->next;
    }
    orderTable[MAX_ORDER-1] = blocksList;
    blocksNum = freeBlocksNum = INITIAL_BLOCKS_NUM;
    totalAllocatedBytes = freeBytesNum = INITIAL_BLOCKS_NUM*INITIAL_BLOCK_SIZE - INITIAL_BLOCKS_NUM*META_DATA_SIZE;
}

/**
 * @function removeFromMmapedList
 * @abstract : removes a block that has been mmaped from MmapedList;
 * @param block : the block to remove;
*/
void BuddyAllocator::releaseMMapBlock(MallocMetaData* block)
{
    if(block == mmapedList) // head of the list
    {
        mmapedList = mmapedList->next;
        if(mmapedList)
            mmapedList->prev = nullptr;
    }
    else if(block->next == nullptr) // tail of the list (can't be head of the list)
    {
        block->prev->next = nullptr;
    }
    else {
        block->next->prev = block->prev;
        block->prev->next = block->next;
    }
    size_t size =  block->size;
    if(munmap(block, block->size + META_DATA_SIZE) != 0)
        throw BuddyAllocatorException();
    blocksNum--;
    totalAllocatedBytes-= size;
    return;
}


bool BuddyAllocator::isLegalToMerge(MallocMetaData* block, MallocMetaData* buddy)
{
    return not ((block->size+META_DATA_SIZE) == INITIAL_BLOCK_SIZE || buddy->isFree is false || block->size != buddy->size);
}

/**
 * @function removeBlockFromOrdersList
 * @abstract ;
 * @param block :
*/
void BuddyAllocator::removeBlockFromOrdersList(MallocMetaData* block)
{
    int order = findOrder((int) block->size + META_DATA_SIZE);
    if(block->prevFreeBlock is nullptr) // head of list
    {
        orderTable[order] = block->nextFreeBlock;
        if(orderTable[order] != nullptr)
            orderTable[order]->prevFreeBlock = nullptr;
    }
    else if(block->nextFreeBlock is nullptr) // tail of list (can't be head of list)
        block->prevFreeBlock->nextFreeBlock = nullptr;
    else
    {
        block->prevFreeBlock->nextFreeBlock = block->nextFreeBlock;
        block->nextFreeBlock->prevFreeBlock = block->prevFreeBlock;
    }
    block->nextFreeBlock = block->prevFreeBlock = nullptr;
}

/**
 * @function releaseBlock
 * @abstract returns an allocated block to the system
 * @param block : the block we want to add
*/
void BuddyAllocator::releaseBlock(MallocMetaData* block)
{
    block->isFree = true;
    freeBlocksNum++;
    freeBytesNum += block->size;
    block = mergeBuddies(block);
    int order = findOrder((int) block->size + META_DATA_SIZE);
    insertBlockToOrdersTable(order, block);
}

/**
 * @function mergeBlocksHelper
 * @abstract : update the fields of the block we wanna merge into
 * @return the address ot the new merged block
*/
MallocMetaData* BuddyAllocator::mergeBuddiesHelper(MallocMetaData* block, MallocMetaData* buddy)
{
    if(block < buddy) // merge buddy into block;
    {
        removeBlockFromOrdersList(buddy);
        block->size += buddy->size + META_DATA_SIZE;
        block->next = buddy->next;
        if(block->next isnot nullptr)
            block->next->prev = block;
    }
    else // merge block into buddy
    {
        removeBlockFromOrdersList(buddy);
        buddy->size += block->size + META_DATA_SIZE;
        buddy->next = block->next;
        if(buddy->next isnot nullptr)
            buddy->next->prev = buddy;
        block = buddy;
    }
    return block;
}

/**
 * @function mergeBuddies
 * @abstract : merges two free blocks into one block recursivley until no possible merges left.
 * @param block : the first block
*/
MallocMetaData* BuddyAllocator::mergeBuddies(MallocMetaData* block)
{
    MallocMetaData* buddyBlock = block->getBuddy();
    while(isLegalToMerge(block, buddyBlock) returned true)
    {
        block = mergeBuddiesHelper(block, buddyBlock);
        blocksNum--;
        freeBlocksNum--;
        freeBytesNum += META_DATA_SIZE;
        totalAllocatedBytes += META_DATA_SIZE;
        buddyBlock = block->getBuddy();
    }
    return block;
}

/**
 * @function insertToOrdersTable
 * @abstract : inserts a given block to the OrdersTable in entry "order";
 * @param order : the entry in OrdersTable;
 * @param to_add : the block to be inserted;
*/
void BuddyAllocator::insertBlockToOrdersTable(int order, MallocMetaData* toAdd)
{
    MallocMetaData* it = nullptr, *prev = nullptr;
    if(orderTable[order] == nullptr)
    {
        orderTable[order] = toAdd;
        return;
    }

    MallocMetaData* head = orderTable[order];
    for(it = head; it != nullptr; prev = it, it = it->nextFreeBlock)
        if( (void*) it > (void*) toAdd)
            break;

    if(prev == nullptr)
    {
        toAdd->nextFreeBlock = head;
        toAdd->prevFreeBlock = nullptr;
        head->prevFreeBlock = toAdd;
        head = toAdd;
    }
    else
    {
        toAdd->nextFreeBlock = prev->nextFreeBlock;
        toAdd->prevFreeBlock = prev;
        prev->nextFreeBlock = toAdd;
    }
}

/**
 * @function splitFreeBlockInHalf
 * @abstract : splits a given a free block to two free block with the same size;
 * @param block : the block that needs be splitted.
 * @param entry : the suitable entry in OrdersTable for the block.
 * @return : the new order of the new blocks;
*/
int BuddyAllocator::splitBlock(MallocMetaData* block, int entry)
{
    orderTable[entry] = block->nextFreeBlock; // update the head of the list
    if(orderTable[entry] != nullptr)
        orderTable[entry]->prevFreeBlock = nullptr;
    size_t newSize = BuddyAllocator::sizeAfterSplitting(block->size);
    block->size = newSize;
    block->nextFreeBlock = block->prevFreeBlock = nullptr;

    // create a new block by adding the offset to the current pointer;
    MallocMetaData* newBlock = (MallocMetaData*)((char*) block + newSize + META_DATA_SIZE);
    *newBlock = MallocMetaData(newSize, cookie, block, block->next);
    newBlock->nextFreeBlock = newBlock->prevFreeBlock = nullptr;

    if(block->next != nullptr) // is not the tail of the list;
		block->next->prev = newBlock;
    block->next = newBlock;


    int order = BuddyAllocator::findOrder(newSize+META_DATA_SIZE);
    insertBlockToOrdersTable(order, newBlock);
    insertBlockToOrdersTable(order, block);

    freeBlocksNum++;
    blocksNum++;
    freeBytesNum -= META_DATA_SIZE;
    totalAllocatedBytes -= META_DATA_SIZE;
    return order;
}

void* BuddyAllocator::allocateMMapBlock(size_t size)
{
    void* newBlockAddress = nullptr;
    newBlockAddress = mmap(NULL, size+META_DATA_SIZE, PROT_READ|PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if(newBlockAddress == FAILURE)
        throw BuddyAllocator::BuddyAllocatorException();

    blocksNum++;
    totalAllocatedBytes += size;

    if(mmapedList == nullptr)
    {
        mmapedList = (MallocMetaData*) newBlockAddress;
        *mmapedList =  MallocMetaData(size, cookie);
        mmapedList->isFree = false;
        return (void*) (mmapedList+1);
    }

    MallocMetaData* iterator = mmapedList;
    while(iterator->next != nullptr)
        iterator = iterator->next;

    MallocMetaData* block = (MallocMetaData*) newBlockAddress;
    *block = MallocMetaData(size, cookie, iterator);
    block->isFree = false;
    iterator->next = block;
    return (void*) (block+1);
}

/**
 * @function acquireBlock
 * @abstract removes the head of the List in OrdersTable[order]
 * @param order : the entry in OrdersTable
 * @return : pointer to the head removed;
*/
MallocMetaData* BuddyAllocator::acquireBlock(int order)
{
    // // pop the head of the list
    // MallocMetaData* toPop = orderTable[order];
    // orderTable[order] = orderTable[order]->nextFreeBlock;
    // toPop->nextFreeBlock = toPop->prevFreeBlock = nullptr;
    // toPop->isFree = false;

    // // update the state of the Allocator;
    // MallocMetaData* newHead = orderTable[order];
    // if(newHead != nullptr)
    //     newHead->prevFreeBlock = nullptr;
    MallocMetaData* toPop = orderTable[order];
    removeBlockFromOrdersList(orderTable[order]);
    freeBlocksNum--;
    freeBytesNum -= toPop->size;
    return toPop;
}

void* BuddyAllocator::allocateBlock(size_t size)
{
    int order = BuddyAllocator::findOrder((int)(size+META_DATA_SIZE));
    for(; order<MAX_ORDER; order++) // find the minimal block that need to split
        if(orderTable[order] != nullptr)
            break;

    if(order == MAX_ORDER)
        return FAILURE;

    MallocMetaData* head = orderTable[order];
    size_t newSize = BuddyAllocator::sizeAfterSplitting(head->size);
    while(order>0 && newSize >= size)
    {
        order = splitBlock(head, order);
        head = orderTable[order];
        newSize = BuddyAllocator::sizeAfterSplitting(head->size);
    }
    return (void*) (acquireBlock(order)+1);
}

/**
 * @function smalloc
 * @abstract allocates the smallest block possible with the required size (best-fit);
 *  if a pre-allocated block is reused and is "large enough", smalloc will cut the block into two blocks,
 *  one will serve the current allocation, and the other will remain unused for later.
 *  this process is done iteratively until the block cannot be cut into two.
 * @param size : the size of the desired block
 * @return a pointer to the first byte in the allocated block upon success.
 * @return NULL upon failure.
*/
void* BuddyAllocator::smalloc(size_t size)
{
    // BuddyAllocator& instance = BuddyAllocator::getInstance();
    if(size <= 0 or size > LIMIT)
        return NULL;

    if(size >= SBRK_LIMIT)
        return allocateMMapBlock(size);

    void* newBlock = allocateBlock(size);
    return newBlock == FAILURE ? NULL : newBlock;
}

/**
 * @function sfree
 * @abstract frees the allocated memory. if the buddy of the free'd block is also free, we merge them into one free block.
 *  this is done recursivly until we can't merge blovks anymore.
 * @param p : pointer to the previously allocated block.
*/
void BuddyAllocator::sfree(void* p)
{
    if(p is nullptr)
        return;

    // searchBrokenCookies(MmapedList);
    // searchBrokenCookies(blocksList);

    MallocMetaData* blockMetaData = (MallocMetaData*)p - 1;
    if(blockMetaData->cookie != cookie)
        exit(0xdeadbeef);
    else if(blockMetaData->isFree == true)
        return;

    if(blockMetaData->size >= SBRK_LIMIT)
        releaseMMapBlock(blockMetaData);
    else
        releaseBlock(blockMetaData);
}

size_t BuddyAllocator::num_free_blocks() const { return freeBlocksNum; }
size_t BuddyAllocator::num_free_bytes() const { return freeBytesNum; }
size_t BuddyAllocator::num_allocated_blocks() const { return blocksNum; }
size_t BuddyAllocator::num_allocated_bytes() const { return totalAllocatedBytes;}
size_t BuddyAllocator::num_meta_data_bytes() const { return sizeof(MallocMetaData)*(blocksNum); }
size_t BuddyAllocator::size_meta_data() const { return sizeof(MallocMetaData); }