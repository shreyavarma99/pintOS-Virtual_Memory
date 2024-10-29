/* imports */
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "threads/synch.h"
#include "threads/palloc.h"



struct frame_table_entry {
    uintptr_t paddr; /* physical address of frame */ 
    void *vaddr; /* virtual address TODO also include spte */ 
    struct thread *owner;
    bool in_use;
};

void init_frame_table (enum palloc_flags flagies);
void *allocate_frame();

