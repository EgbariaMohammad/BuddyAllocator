
#pragma once

#include <iostream>
#include <unistd.h>
#include <cstring>
#include <sys/mman.h>

#define LIMIT 1e8
#define KILOBYTE 1024
#define FAILURE ((void*)(-1))


/**
 * @struct MallocMetaData
 * @abstract type that holds the MetaData of the allocated block in our program.
 * @field cookie : random value that insures the corectness of the MetaData;
 * @field size : the size of the allocated block - without the MetaData block;
 * @field isFree : is the block free or is it allocated.
 * @field next : pointer to the next block in increasing addresses order.
 * @field prev : pointer to the prev block in increasing addresses order.
 * @field nextFree : pointer to the next free block in OrdersTable.
 * @field prevFree : pointer to the prev free block in OrdersTable.
 */
struct MallocMetaData
{
    int cookie;
    size_t size;
    bool isFree;
    MallocMetaData *next;
    MallocMetaData *prev;
    MallocMetaData *nextFreeBlock;
    MallocMetaData *prevFreeBlock;

    MallocMetaData(size_t size, int cookie, MallocMetaData *prev = nullptr, MallocMetaData *next = nullptr)
    : cookie(cookie), size(size), isFree(true), next(next), prev(prev), nextFreeBlock(next), prevFreeBlock(prev) {}
    void updateSize(size_t new_size) { size = new_size; }

    MallocMetaData* getBuddy() const {
        return (MallocMetaData*) ((intptr_t) this ^ (this->size+sizeof(MallocMetaData)));
    }
};

class BuddyAllocator {

    static const int MAX_ORDER = 11;
    static const int INITIAL_BLOCKS_NUM = 32;
    static const int MIN_BLOCK_SIZE = 128;

    static const int SBRK_LIMIT = MIN_BLOCK_SIZE*KILOBYTE;
    static const int INITIAL_BLOCK_SIZE = MIN_BLOCK_SIZE*KILOBYTE;
    static const int ALIGNMENT = INITIAL_BLOCKS_NUM*MIN_BLOCK_SIZE*KILOBYTE;

    static const int META_DATA_SIZE = sizeof(MallocMetaData);

    MallocMetaData* blocksList;
    MallocMetaData* mmapedList;
    MallocMetaData* orderTable[MAX_ORDER];
    const int cookie;
    int blocksNum;
    int freeBlocksNum;
    int freeBytesNum;
    int totalAllocatedBytes;

    BuddyAllocator();

    static int findOrder(size_t size);
    static size_t sizeAfterSplitting(size_t size);
    static bool isLegalToMerge(MallocMetaData* block, MallocMetaData* buddy);

    void insertBlockToOrdersTable(int order, MallocMetaData* toAdd);
    void removeBlockFromOrdersList(MallocMetaData* block);

    void* allocateMMapBlock(size_t size);
    void releaseMMapBlock(MallocMetaData* block);

    void* allocateBlock(size_t size);
    MallocMetaData* acquireBlock(int order);
    void releaseBlock(MallocMetaData* block);

    int splitBlock(MallocMetaData* block, int entry);
    MallocMetaData* mergeBuddies(MallocMetaData* block);
    MallocMetaData* mergeBuddiesHelper(MallocMetaData* block, MallocMetaData* buddy);


public:
    static BuddyAllocator& getInstance() {
        static BuddyAllocator instance; // Instantiated on first use.
        return instance;
    }

    BuddyAllocator(const BuddyAllocator&) = delete; // Copy constructor
    BuddyAllocator& operator=(const BuddyAllocator&) = delete; // Assignment operator

    void* smalloc(size_t size);
    // void* scalloc(size_t num, size_t size);
    void sfree(void* p);
    // void* srealloc(void* oldp, size_t size);
    size_t num_free_blocks() const;
    size_t num_free_bytes() const;
    size_t num_allocated_blocks() const;
    size_t num_allocated_bytes() const;
    size_t num_meta_data_bytes() const;
    size_t size_meta_data() const;

    class BuddyAllocatorException : public std::exception {};
};