# Buddy Memory Allocator

## Description 

A Buddy Allocator is a type of memory management scheme that works by dividing memory into partitions to satisfy memory allocation requests. It starts with a single block of available memory. When a request is made, it splits this block into two "buddies" of equal size. This process continues until a block of the smallest possible size that satisfies the request is created.
The Buddy Allocator is efficient in terms of both time and space. It can allocate and deallocate memory in constant time (O(1)) by manipulating pointers in the free lists. It also reduces external fragmentation by merging free blocks back together.
However, the Buddy Allocator can have higher space overhead due to rounding up allocation sizes to the nearest power of two. It's best suited for scenarios where there are a large number of allocations and deallocations, and predictable performance is required, such as in real-time systems.

## Table of Contents
1. [Features](#features)
2. [Projects and Use Cases](#projects-and-use-cases)
3. [Example Usages](#example-usages)
4. [Performance Considerations](#performance-considerations)
5. [Pros and Cons](#pros-and-cons)
6. [Conclusion](#conclusion)

## Features
- **Best Fit:** The allocator shall allocate the smallest block possible that fits the requested memory, that way the internal fragmentation caused by this allocation will be as small as possible
- **Fixed Overhead:** Each block's metadata is stored directly within the block, ensuring a consistent overhead.
- **Saftey:** If a buffer overflow happens in the heap memory area (either on accident or on purpose), and this overflow overwrites the metadata bytes of some allocation with arbitrary junk (or worse), the allocator shall detect that and kill the process.
- **Fixed Allocation Time:** The allocator shall allocate and deallocate memory in constant time (O(1))

## Projects and Use Cases
1. **Real-time Systems**
2. **Memory-Constrained Embedded Systems**
3. **Database Systems**
4. **Networking**
5. **Operating Systems**
6. **Simulations**
7. **Custom Memory Allocators for Libraries**

## Example Usages
Note that we chose to implement the allocator as a singleton, so it can be accessed everywhere in the system, and also to prevent copy/assigning it.
```cpp
#include "BuddyAllocator.hpp"

int main() {
    BuddyAllocator allocator == BuddyAllocator::getInstance();
    void* memoryBlock = allocator.malloc(100); // Allocate 100 bytes.
    // ... use the memory ...
    allocator.free(memoryBlock);
    return 0;
}
```

## Performance Considerations
For low counts of allocations, the Buddy Allocator might sometimes be slower than the standard new and delete operations due to:

Initial Memory Pooling: Sets up an initial memory pool.
Overhead of Block Management: Maintains free lists and performs block splitting/coalescing.
Memory Wastage: Rounds up sizes to the nearest power of two.
Standard Library Optimizations: Modern C++ standard libraries have highly optimized memory allocators tailored for general use.
For larger numbers of allocations or specialized use cases, the Buddy Allocator can provide more predictable performance and reduced fragmentation.

## Pros and Cons
### Pros:
* Reduced Fragmentation: The allocator's coalescing feature reduces external fragmentation.
* Predictable Performance: Allocation and deallocation times are generally constant.
* Simple Free Block Management: Uses a free list for each block size, simplifying block management.
### Cons:
* Space Overhead: Memory can be wasted if the requested size is much smaller than the allocated block size.
* Possible Slower Performance: For small numbers of allocations, the overhead might outweigh the benefits.


## Disclaimer

This code is provided as is for educational purposes and as a reference. The author is not responsible for any consequences, including but not limited to academic misconduct, that may arise from inappropriate use of this code. It is the user's responsibility to abide by their institution's academic integrity policies. Copying this code and using it without understanding or crediting the source may be considered plagiarism by your institution.

## Authors

@EgbariaMohammad
