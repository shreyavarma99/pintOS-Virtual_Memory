#include "vm/frame.h"
#include "threads/loader.h"
#include "threads/palloc.h" 
#include "threads/malloc.h"
#include "threads/vaddr.h"
#include "threads/thread.h"
#include "userprog/pagedir.h"
#include "vm/swap.h"
#include <string.h>
 
struct lock frame_lock; /* Lock for frames */
struct frame_table_entry *frames; /* Array of frame table entries */
size_t clock; /* index of the clock hand in the frame table */

/* Create frame table array and set intial fields of each frame table entry
*/
void init_frame_table ()
{
    /* Garv, Jyotsna, and Shreya Agrawal Driving */
    lock_init (&frame_lock);
    frames = malloc ((user_pool_size - 1) * sizeof(struct frame_table_entry));
    /* clock hand starts at the first frame table entry */
    clock = 0;
    for (size_t i = 0; i < user_pool_size - 1; i++)
        {
            uint8_t *kpage = palloc_get_page (PAL_USER | PAL_ZERO);
            frames[i].paddr = kpage;    
            frames[i].vaddr = NULL;
            frames[i].in_use = false;
            frames[i].owner = NULL;
            frames[i].pinned = false;
        }
}

/* Find a free frame in the frame table to use for allocation and 
 * return the kpage, or evict a frame and return its kpage */
uint8_t *allocate_frame (void *upage)
{
    /* Jyotsna Driving */
    /* allocating in frame table should be atomic */
    bool held = lock_held_by_current_thread (&frame_lock);
    if (!held)
        {
            lock_acquire (&frame_lock);
        }

    uint8_t *kpage = NULL;
    /* search for free frame */
    size_t i = 0;
    for (i = 0; i < user_pool_size - 1; i++)
        {
            if (!frames[i].in_use)
                {
                    /* found a free frame */
                    frames[i].in_use = true;
                    frames[i].owner = thread_current ();  
                    frames[i].vaddr = upage;
                    kpage = frames[i].paddr;
                    break;
                }
        }
    /* return free frame if one was found */
    if (kpage != NULL)
        {
            if (!held)
                {
                    lock_release (&frame_lock);
                }
            return kpage;
        }

    /* eviction case, no free frames */
    struct frame_table_entry *frame_to_replace = evict_frame ();
    frame_to_replace->owner = thread_current ();  
    frame_to_replace->in_use = true;
    frame_to_replace->vaddr = upage;
    kpage = frame_to_replace->paddr;

    if (!held)
        {
            lock_release (&frame_lock);
        }
    return kpage;
}

/* mark a frame based on kpage passed in as pinned, 
 * in order to prevent eviction  */
bool pin_frame (uint8_t *kpage)
{
    /* Jyotsna driving */
    bool prev = false;
    bool held = lock_held_by_current_thread (&frame_lock);
    if (!held) 
        {
            lock_acquire (&frame_lock);
        }
    /* find the frame in the frame table array */
    for (size_t i = 0; i < user_pool_size - 1; i++)
        {
            if (frames[i].paddr == kpage)
                {
                    prev = frames[i].pinned;
                    frames[i].pinned = true;
                    break;
                }
        }
    if (!held)
        {
            lock_release (&frame_lock);
        }
    /* return if it was previously pinned */
    return prev;
}

/* unpin the frame with the given kpage */
void unpin_frame (uint8_t *kpage)
{
    /* Jyotsna driving */
    bool held = lock_held_by_current_thread (&frame_lock);
    if (!held) 
        {
            lock_acquire (&frame_lock);
        }
    /* find the frame with parameter kpage in frame table */
    for (size_t i = 0; i < user_pool_size - 1; i++)
        {
            if (frames[i].paddr == kpage)
            {
                frames[i].pinned = false;
                break;
            }
        }
    if (!held)
        {
            lock_release (&frame_lock);
        }
    return;
}

/* select frame to evict, remove from current page in it from pagedir, 
 * perform any saving to swap necessary for eviction */
struct frame_table_entry *evict_frame (void)
{
    /* Jyotsna driving */
    struct frame_table_entry *evicted = NULL;
    while (evicted == NULL)
        {
            if (clock == user_pool_size - 1) 
                {
                    clock = 0;
                }
            if (!frames[clock].in_use)
                {
                    /* move clock hand */
                    clock++;
                    continue;
                }
            if (frames[clock].pinned)
                {
                    /* move clock hand, don't want to evict a pinned frame */
                    clock++;
                    continue;
                }
            /* if accessed, clear accessed bit */
            if (pagedir_is_accessed (frames[clock].owner->pagedir, 
                frames[clock].vaddr))
                {
                    /* frame was accessed */
                    pagedir_set_accessed (frames[clock].owner->pagedir, 
                        frames[clock].vaddr, false);
                    /* move clock hand */
                    clock++;
                }
            else 
                {
                    /* found a not accessed frame */
                    /* save dirty bit from before */
                    bool dirty = pagedir_is_dirty 
                        (frames[clock].owner->pagedir, frames[clock].vaddr);
                    struct spt_entry *entry = page_lookup 
                        (frames[clock].vaddr, frames[clock].owner);
                    if (dirty)
                        {
                            /* must be put to swap */
                            swap_into_disk (entry, frames[clock].paddr);
                        }
                    /* clear frame metadata */
                    entry->in_resident = false;
                    entry->in_swap = true;
                    frames[clock].in_use = false;
                    frames[clock].owner = NULL;
                    frames[clock].vaddr = NULL;
                    evicted = &frames[clock];
                    /* remove mapping from pagedir */
                    pagedir_clear_page (entry->owner->pagedir, entry->vaddr);
                    /* move clock hand */
                    clock++;
                    break;
                }
        }
    return evicted;
}

/* used for resource reclamation, removing mappings and 
 * clearing frame metadata */
void free_frame (void *kpage)
{
    /* Garv driving */
    /* changes to frame table must be atomic */
    bool held = lock_held_by_current_thread (&frame_lock);
    if (!held)  
        {
            lock_acquire (&frame_lock);
        }
    /* go through frame table array and clear mapping and frame metadata 
       for each frame */
    for (size_t i = 0; i < user_pool_size - 1; i++)
        {
            if (frames[i].paddr == kpage)
                {
                    pagedir_clear_page (frames[i].owner->pagedir, 
                        frames[i].vaddr);
                    frames[i].in_use = false;
                    frames[i].vaddr = NULL;
                    frames[i].owner = NULL;
                    break;
                }
        }
    if (!held)
        {
            lock_release (&frame_lock);
        }
    return;
}
