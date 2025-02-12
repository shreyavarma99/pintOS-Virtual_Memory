**Implemented Features**
1️⃣ Supplemental Page Table (SPT)

Tracks virtual-to-physical memory mappings for user processes.
Supports multiple types of pages (stack, executable, mmap files).
Provides efficient lookup and eviction mechanisms.

2️⃣ Demand Paging
Lazy loading of pages → Loads memory pages only when accessed, instead of loading everything at once.
Handles page faults by fetching the required page from disk or swap when needed.

3️⃣ Page Swapping
Implements a swap table to store pages that don’t fit in physical memory.
Uses disk-backed swapping when RAM is full.
4️⃣ Page Eviction Algorithms
Implements a frame table to track allocated frames.
Uses an eviction policy (e.g., Clock Algorithm or Least Recently Used (LRU)) to select a page for eviction when memory is full.
5️⃣ Stack Growth
Supports dynamic stack growth, allowing user programs to use more stack space when needed.
Expands stack memory only up to a defined limit to prevent infinite growth.
6️⃣ Memory-Mapped Files (mmap)
Implements mmap() and munmap() to map files to virtual memory, enabling efficient file access.
Supports writing changes back to disk when unmapping.

**Challenges:**
- Efficiently handling page faults → Requires proper synchronization to avoid race conditions.
- Choosing the right page to evict → Implementing an efficient eviction policy is crucial for performance.
- Ensuring stack safety → Stack growth must be controlled to prevent infinite expansion.
- Managing swap space efficiently → Ensuring minimal disk I/O for performance optimization.

**How to Run**
cd pintos/src/userprog
make clean
make
pintos --qemu -- -q run your-program

**For testing virtual memory:**
pintos --qemu --swap-size=4 -- -q run test-virtual-memory


