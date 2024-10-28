/* imports */
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "threads/tests.h"
#include "threads/synch.h"
#include "threads/palloc.h"


struct frame_table_entry {
    uintptr_t pointer_to_frame; /* physical address of frame */ 
    void *vaddr; /* virtual address */
    /* struct list_elem elem; */ 
    struct thread *owner;
};

/*
functions that we may... possibly need:
    - Initiliaze frame table
    - Add frame to frame table/ get a frame
    - Remove and free frame from frame table
    - Select frame for eviction (PRA)
    - Size of frame table (because physical memory is diff each time)
*/
