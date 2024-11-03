#include "vm/frame.h"
#include "threads/loader.h"
#include "threads/palloc.h" 
#include "threads/malloc.h"
#include "threads/vaddr.h"

 
/*
    functions that we may... possibly need:
    - Initiliaze frame table
    - Add frame to frame table/ get a frame
    - Remove and free frame from frame table
    - Select frame for eviction (PRA)
    - Size of frame table (because physical memory is diff each time)

    Initiliaze Frame Table
        Initialize our array
        Initialize our lock(s)?

    Allocation
        Set the frame to Null
        Check if we are getting from UserPool (if flags and PAL_USER)
            if (the palloc flags and PAL_ZERO)
                frame = palloc_get_page (ZERO or USER)
            else 
                frame = palloc_get_page(USER)
        after if frame is not Null (add frame)
        else evict frame
            if eviction is NULL 
                do we panic here
        return the frame
*/

struct lock frame_lock; /* Lock for frames */
struct frame_table_entry *frames; 

void init_frame_table ()
{
    lock_init(&frame_lock);
    frames = malloc (user_pool_size * sizeof(struct frame_table_entry));
}

void *allocate_frame(enum palloc_flags flagies, void *upage)
{
    uint8_t *kpage = palloc_get_page (flagies);
    if (kpage != NULL) 
        { 
            lock_acquire(&frame_lock);
            // find correct index in frame table
            uint8_t index = (*kpage - LOADER_PHYS_BASE) / PGSIZE;
            // set up frame
            frames[index].paddr = kpage;
            frames[index].owner = thread_current ();  
            frames[index].in_use = true;
            frames[index].vaddr = upage;
            lock_release(&frame_lock);
            return kpage;
        } 
    else 
        {
            // eviction
            PANIC("couldn't find a free frame");
        }
}

void free_frame(void *kpage)
{
    lock_acquire(&frame_lock);
    uint8_t index = (*((uint8_t *) kpage) - LOADER_PHYS_BASE) / PGSIZE;
    frames[index].in_use = false;
    frames[index].vaddr = NULL;
    frames[index].owner = NULL;
    frames[index].paddr = NULL;
    lock_release(&frame_lock);
}