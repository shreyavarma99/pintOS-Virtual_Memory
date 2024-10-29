#include "vm/frame.h"
#include "threads/loader.h"
#include "threads/palloc.h" 
#include "threads/malloc.h"

 
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

void init_frame_table (enum palloc_flags flagies)
{
    lock_init(&frame_lock);
    // ASK
    frames = malloc (user_pool_size * sizeof(struct frame_table_entry*));
    for (int i = 0; i < user_pool_size; i++)
    {
        /*
        Set the frame to Null
        Check if we are getting from UserPool (if flags and PAL_USER)
            if (the palloc flags and PAL_ZERO)
                frame = palloc_get_page (ZERO or USER)
            else 
                frame = palloc_get_page(USER)
        */
        
        frames[i].paddr = NULL;
        if (flagies & PAL_USER)
        {
            if (flagies & PAL_ZERO) 
            {
                frames[i].paddr = palloc_get_page(PAL_USER | PAL_ZERO);
            }
            else 
            {
                frames[i].paddr = palloc_get_page(PAL_USER);
            }
        }

        /* If we do above then we do not need the line below right? */
        frames[i].vaddr = NULL;
        frames[i].owner = NULL; 
        frames[i].in_use = false;  
    }
}

// add parameter of vaddr
void *allocate_frame()
{
    lock_acquire(&frame_lock);
    for (int i = 0; i < user_pool_size; i++)
    {
        if (!frames[i].in_use)
        {
            frames[i].in_use = true;
            frames[i].owner = thread_current();
            // set pagedir[vaddr] = frams[i].paddr
            //add mapping
            return frames[i].paddr;
            
        }

    }
    //eviction
    //found no free frames
    PANIC("couldn't find a free frame");
}

// bool add_map()
