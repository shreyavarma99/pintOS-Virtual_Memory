#include "vm/frame.h"
#include "threads/loader.h"
#include "threads/palloc.h" 
#include "threads/malloc.h"
#include "threads/vaddr.h"
#include "threads/thread.h"
#include "userprog/pagedir.h"
#include "vm/swap.h"
#include <string.h>
 
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
uint8_t clock;

void init_frame_table ()
{
    lock_init(&frame_lock);
    frames = malloc (user_pool_size * sizeof(struct frame_table_entry));
    clock = 1;
}

void *allocate_frame(enum palloc_flags flagies, void *upage)
{
    uint8_t *kpage = palloc_get_page (flagies);
    if (kpage != NULL) 
        { 
            lock_acquire(&frame_lock);
            // find correct index in frame table
            uint8_t index = (kpage - user_base_addr) / PGSIZE;
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
            // // eviction
            struct frame_table_entry *frame_to_replace = evict_frame();
            if (!lock_held_by_current_thread(&frame_lock))
                lock_acquire(&frame_lock);
            frame_to_replace->owner = thread_current ();  
            frame_to_replace->in_use = true;
            frame_to_replace->vaddr = upage;
            lock_release(&frame_lock);
            return frame_to_replace->paddr;
        }
}

void free_frame(void *kpage)
{
    lock_acquire(&frame_lock);
    uint8_t index = (((uint8_t *) kpage) - user_base_addr) / PGSIZE;
    palloc_free_page(kpage);
    frames[index].in_use = false;
    frames[index].vaddr = NULL;
    frames[index].owner = NULL;
    frames[index].paddr = NULL;
    lock_release(&frame_lock);
}

struct frame_table_entry *evict_frame(void)
{
    if (!lock_held_by_current_thread(&frame_lock))
        lock_acquire(&frame_lock);
    while(true)
    {
        // reached end of frame table
        if (clock >= user_pool_size) {
            clock = 1;
        }
        
        struct frame_table_entry *frame = &frames[clock];
        if(frame->in_use){
            struct thread *owner = frame->owner;
            if(owner)
            {
                if(pagedir_is_accessed (owner->pagedir, frame->vaddr))
                {
                    //frame was accessed
                    pagedir_set_accessed(owner->pagedir, frame->vaddr, false);
                }
                else {
                    //frame was not accessed...evict frame
                    bool dirty = pagedir_is_dirty(owner->pagedir, frame->vaddr);
                    struct spt_entry *entry = page_lookup(frame->vaddr, owner);
                    entry->in_swap = true;
                    swap_into_disk(entry);
                    pagedir_clear_page(owner->pagedir, frame->vaddr);
                    frame->in_use = false;
                    frame->owner = NULL;
                    frame->vaddr = NULL;
                    clock++;
                    lock_release(&frame_lock);
                    return frame;
                }
            }
        }
        clock++;
    }
}