# -buddy-memory-allocator

every allocation will contain meta-data, which is for our own use only and the user shouldn’t be aware of its existence. 

this technique divides memory into partitions of sizes that are always powers of 2, to try and satisfy a memory request as suitably as possible.
The basic concept behind it is as follows - memory is broken up into large blocks where each block is a power of two number of bytes.
Upon allocation request, if a block of the desired size is not available, a large block is broken up in half and the two blocks are said to be buddies 
to each other. One half is used for the allocation and the other is free. 
The blocks are continuously halved as necessary until a block of the desired size is available. When a block is later
freed, its buddy is examined and the two are merged if it is also free.
our buddy allocator initially increase the program break to obtain the memory region it will
use for future allocations. we initially have 32 free blocks of size 128kb (kb = 1024 bytes).
We will use the notation of “orders” to talk about the sizes of our memory blocks. The smallest
possible order is 0 for blocks of size 128 bytes, the next order is 1 for blocks of size 256 bytes, and so
on. The biggest blocks will be of order MAX_ORDER=10 and of size 128kb.
Sizes of blocks include the size of your metadata structure.


Challenges i had:
● Challenge 0 (Memory utilization):
searching for an empty block in ascending order will cause internal fragmentation. To mitigate this problem, we shall allocate the smallest block possible that
fits the requested memory, that way the internal fragmentation caused by this allocation will be as small as possible.

● Challenge 1 (Memory utilization):
If we reuse freed memory sectors with bigger sizes than required, we’ll be wasting memory (internal fragmentation).
Solution: Implement a function that smalloc() will use, such that if a pre-allocated block
is reused and is large enough, the function will cut the block in half to two blocks (buddies)
with two separate meta-data structs. One will serve the current allocation, and the other will
remain unused for later (marked free and added to the free blocks data structure). This
process should be done iteratively until the allocated block is no longer “large enough”.
Definition of “large enough”: The allocated block is large enough if it is of order > 0, and if
after splitting it to two equal sized blocks, the requested user allocation is small enough to
fit entirely inside the first block, so the second block will be free.

● Challenge 2 (Memory utilization):
Many allocations and de-allocations might cause two buddy blocks to be free, but separate.
Solution: Implement a function that sfree() will use, such that if we free a block which
has a free buddy block, the function will automatically combine both free buddy blocks (the
current one and the adjacent one) into one large free block. If after merging we find out that
the new free block also has a free buddy block, we should iteratively merge blocks until no
two buddy blocks are free. (we should not merge free blocks of order MAX_ORDER)

● Challenge 3 (Large allocations):
Modern dynamic memory managers not only use sbrk() but also mmap(). This process helps reduce the negative effects of memory
fragmentation when large blocks of memory are freed but locked by smaller, more recently
allocated blocks lying between them and the end of the allocated space. In this case, had the
block been allocated with sbrk(), it would have probably remained unused by the system
for some time (or at least most of it).
Solution: use mmap for allocations that require 128kb space or more (128*1024 B).

● Challenge 4 (Safety & Security):
Consider the following case – a buffer overflow happens in the heap memory area (either on
accident or on purpose), and this overflow overwrites the metadata bytes of some allocation
with arbitrary junk (or worse). Think – which problems can happen if we access this
overwritten metadata?
Solution: We can detect (but not prevent) heap overflows using “cookies” – 32bit integers
that are placed in the metadata of each allocation. If an overflow happens, the cookie value
will change and we can detect this before accessing the allocation’s metadata by comparing
the cookie value with the expected cookie value.
Sulution: add cookies to the allocations’ metadata.
