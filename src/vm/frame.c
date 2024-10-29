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

void *allocate_frame(enum palloc_flags flagies)
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
            lock_release(&frame_lock);
            return kpage;
        } 
    else 
        {
            // eviction
            PANIC("couldn't find a free frame");
        }
}

// void init_frame_table (enum palloc_flags flagies)
// {
//     lock_init(&frame_lock);
//     // ASK
//     frames = malloc (user_pool_size * sizeof(struct frame_table_entry));
//     for (int i = 0; i < user_pool_size; i++)
//     {
//         /*
//         Set the frame to Null
//         Check if we are getting from UserPool (if flags and PAL_USER)
//             if (the palloc flags and PAL_ZERO)
//                 frame = palloc_get_page (ZERO or USER)
//             else 
//                 frame = palloc_get_page(USER)
//         */
        
//         frames[i].paddr = palloc_get_page(PAL_USER | PAL_ZERO);
//         // if (flagies & PAL_USER)
//         // {
//         //     if (flagies & PAL_ZERO) 
//         //     {
//         //         frames[i].paddr = palloc_get_page(PAL_USER | PAL_ZERO);
//         //     }
//         //     else 
//         //     {
//         //         frames[i].paddr = palloc_get_page(PAL_USER);
//         //     }
//         // }

//         /* If we do above then we do not need the line below right? */
//         frames[i].vaddr = NULL;
//         frames[i].owner = NULL; 
//         frames[i].in_use = false;  
//     }
//     // PANIC("got to end init frame table");
// }

// // add parameter of vaddr
// void *allocate_frame()
// {
//     // PANIC("beg of allocate frame");
//     lock_acquire(&frame_lock);
//     for (int i = 0; i < user_pool_size; i++)
//     {
//         // PANIC("%d", user_pool_size);
//         if (!frames[i].in_use)
//         {
//             frames[i].in_use = true;
//             frames[i].owner = thread_current();
//             // set pagedir[vaddr] = frams[i].paddr
//             //add mapping
//             lock_release(&frame_lock);
//             // PANIC("found a free frame");
//             return frames[i].paddr;
            
//         }

//     }
//     lock_release(&frame_lock);
//     //eviction
//     //found no free frames
//     // PANIC("couldn't find a free frame");
// }

// bool add_map()
