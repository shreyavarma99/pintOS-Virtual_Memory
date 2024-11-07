/* imports */
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "threads/synch.h"
#include "threads/palloc.h"

struct frame_table_entry {
    uint8_t *paddr; /* physical address of frame */ 
    void *vaddr; /* virtual address TODO also include spte */ 
    struct thread *owner;
    bool in_use;
    struct lock lock;
};

void init_frame_table (void);
void *allocate_frame(enum palloc_flags flagies, void *upage);
void free_frame(void *kpage);
struct frame_table_entry *evict_frame(void);


