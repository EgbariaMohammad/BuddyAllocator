
#pragma once

#include <iostream>
#include <unistd.h>
#include <cstring>
#include <sys/mman.h>

#define KILOBYTE 1024



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

    MallocMetaData* getBuddy() const {
        return (MallocMetaData*) ((intptr_t) this ^ (this->size+sizeof(MallocMetaData)));
    }
};

class BuddyAllocator {

    /** @brief The maximum order for the buddy system. */
    static const int MAX_ORDER = 11;

    /** @brief The initial number of blocks in the system. */
    static const int INITIAL_BLOCKS_NUM = 32;

    /** @brief The minimum size of a block in the system. */
    static const int MIN_BLOCK_SIZE = 128;

    /** @brief The limit for the sbrk system call. */
    static const int SBRK_LIMIT = MIN_BLOCK_SIZE*KILOBYTE;

    /** @brief The initial size of a block. */
    static const int INITIAL_BLOCK_SIZE = MIN_BLOCK_SIZE*KILOBYTE;

    /** @brief The alignment for the blocks. */
    static const int ALIGNMENT = INITIAL_BLOCKS_NUM*MIN_BLOCK_SIZE*KILOBYTE;

    /** @brief The size of the metadata for a block. */
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

    /**
     * @param size The size for which to find the order.
     * @return The order for the given size.
     */
    static int findOrder(size_t size);

    /**
     * @brief Calculates the size of the new block if it were to be splitted.
     * @param size The original size before splitting.
     * @return The size after splitting.
     */
    static size_t sizeAfterSplitting(size_t size);

    /**
     * @brief Checks if it is legal to merge two blocks.
     * @param block The first block to merge.
     * @param buddy The second block to merge.
     * @return True if it is legal to merge the blocks, false otherwise.
     */
    static bool isLegalToMerge(MallocMetaData* block, MallocMetaData* buddy);

    /**
     * @brief Inserts a block into the orders table.
     * @param order The order of the block.
     * @param toAdd The block to add.
     */
    void insertBlockToOrdersTable(int order, MallocMetaData* toAdd);

    /**
     * @brief Removes a block from the orders list.
     * @param block The block to remove.
     */
    void removeBlockFromOrdersList(MallocMetaData* block);

    /**
     * @brief Allocates a block using mmap.
     * @param size The size of the block to allocate.
     * @return A pointer to the allocated block.
     */
    void* allocateMMapBlock(size_t size);

    /**
     * @brief Releases a block using munmap.
     * @param block The block to release.
     */
    void releaseMMapBlock(MallocMetaData* block);

    /**
     * @brief Allocates a block of a given size.
     * @param size The size of the block to allocate.
     * @return A pointer to the allocated block.
     */
    void* allocateBlock(size_t size);

    /**
     * @brief Acquires a block of a given order.
     * @param order The order of the block to acquire.
     * @return A pointer to the acquired block.
     */
    MallocMetaData* acquireBlock(int order);

    /**
     * @brief Releases a block.
     * @param block The block to release.
     */
    void releaseBlock(MallocMetaData* block);

    /**
     * @brief Splits a block into two buddies.
     * @param block The block to split.
     * @param entry The entry of the block in the orders table.
     * @return The order of the split block.
     */
    int splitBlock(MallocMetaData* block, int entry);

    /**
     * @brief Merges two buddy blocks into one.
     * @param block The first block to merge.
     * @return A pointer to the merged block.
     */
    MallocMetaData* mergeBuddies(MallocMetaData* block);

    /**
     * @brief Helper method for merging two buddies.
     * @param block The first block to merge.
     * @param buddy The second block to merge.
     * @return A pointer to the merged block.
     */
    MallocMetaData* mergeBuddiesHelper(MallocMetaData* block, MallocMetaData* buddy);

public:
    /** @brief Returns the singleton instance of the BuddyAllocator. */
    static BuddyAllocator& getInstance() {
        static BuddyAllocator instance; // Instantiated on first use.
        return instance;
    }

    BuddyAllocator(const BuddyAllocator&) = delete; // Copy constructor
    BuddyAllocator& operator=(const BuddyAllocator&) = delete; // Assignment operator

    /**
     * @brief Allocates memory of a given size.
     * @param size The size of the memory to allocate.
     * @return A pointer to the allocated memory.
     */
    void* smalloc(size_t size);

    /**
     * @brief Frees a block of memory.
     * @param p A pointer to the memory block to free.
     */
    void sfree(void* p);

    /** @brief Returns the number of free blocks. */
    size_t num_free_blocks() const;

    /** @brief Returns the number of free bytes. */
    size_t num_free_bytes() const;

    /** @brief Returns the number of allocated blocks. */
    size_t num_allocated_blocks() const;

    /** @brief Returns the number of allocated bytes. */
    size_t num_allocated_bytes() const;

    /** @brief Returns the number of bytes used for metadata. */
    size_t num_meta_data_bytes() const;

    /** @brief Returns the size of the metadata. */
    size_t size_meta_data() const;

    /** @brief An exception class for the BuddyAllocator. */
    class BuddyAllocatorException : public std::exception {};

    // void* scalloc(size_t num, size_t size);
    // void* srealloc(void* oldp, size_t size);
};
