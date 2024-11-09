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
struct frame_table_entry *frames; 
size_t clock;

void init_frame_table ()
{
    lock_init(&frame_lock);
    frames = malloc ((user_pool_size - 1) * sizeof(struct frame_table_entry));
    clock = 0;
    for (size_t i = 0; i < user_pool_size - 1; i++)
    {
        uint8_t *kpage = palloc_get_page(PAL_USER | PAL_ZERO);
        frames[i].paddr = kpage;    
        frames[i].vaddr = NULL;
        frames[i].in_use = false;
        frames[i].owner = NULL;
        frames[i].pinned = false;
        lock_init(&frames[i].lock);
    }
}

uint8_t *allocate_frame(void *upage)
{
    if (!lock_held_by_current_thread(&frame_lock))
        lock_acquire(&frame_lock);
    uint8_t *kpage = NULL;
    // search for free frame
    size_t i = 0;
    for (i = 0; i < user_pool_size - 1; i++)
    {
        if(!frames[i].in_use)
        {
            frames[i].in_use = true;
            frames[i].owner = thread_current ();  
            frames[i].vaddr = upage;
            kpage = frames[i].paddr;
            break;
        }
    }
    
    if (kpage != NULL)
    {
        // pagedir set page
        lock_release(&frame_lock);
        return kpage;
    }
    
    struct frame_table_entry *frame_to_replace = evict_frame();

    frame_to_replace->owner = thread_current ();  
    frame_to_replace->in_use = true;
    frame_to_replace->vaddr = upage;
    kpage = frame_to_replace->paddr;
    struct spt_entry *spte = page_lookup(upage, frame_to_replace->owner);
    lock_release(&frame_lock);
    return kpage;
}

void pin_frame(uint8_t *kpage)
{
    if (!lock_held_by_current_thread(&frame_lock))
        lock_acquire(&frame_lock);
    for (size_t i = 0; i < user_pool_size - 1; i++)
    {
        if (frames[i].paddr == kpage)
        {
            frames[i].pinned = true;
            break;
        }
    }
    lock_release(&frame_lock);
    return;
}

void unpin_frame(uint8_t *kpage)
{
    if (!lock_held_by_current_thread(&frame_lock))
        lock_acquire(&frame_lock);
    for (size_t i = 0; i < user_pool_size - 1; i++)
    {
        if (frames[i].paddr == kpage)
        {
            frames[i].pinned = false;
            break;
        }
    }
    lock_release(&frame_lock);
    return;
}

struct frame_table_entry *evict_frame(void)
{
    struct frame_table_entry *evicted = NULL;
    while (evicted == NULL)
    {
        if (clock == user_pool_size - 1) 
        {
            clock = 0;
        }
        if (!frames[clock].in_use)
        {
            // PANIC("comes in here %d", clock);
            clock++;
            continue;
        }
        if (frames[clock].pinned)
        {
            // PANIC("pinned we don't want to ");
            clock++;
            continue;
        }

        if(pagedir_is_accessed (frames[clock].owner->pagedir, frames[clock].vaddr))
        {
            //frame was accessed
            pagedir_set_accessed(frames[clock].owner->pagedir, frames[clock].vaddr, false);
            clock++;
        }
        else 
        {
            frames[clock].pinned = true;
           // PANIC("comes in here too");
            //frame was not accessed...evict frame
            bool dirty = pagedir_is_dirty(frames[clock].owner->pagedir, frames[clock].vaddr);
            struct spt_entry *entry = page_lookup(frames[clock].vaddr, frames[clock].owner);
            if (dirty)
            {
                swap_into_disk(entry);
            }
            entry->in_resident = false;
            entry->in_swap = true;
            frames[clock].in_use = false;
            frames[clock].owner = NULL;
            frames[clock].vaddr = NULL;
            evicted = &frames[clock];
            pagedir_clear_page(entry->owner->pagedir, entry->vaddr);
            frames[clock].pinned = false;
            clock++;
            break;
        }
    }
    return evicted;
}

void free_frame(void *kpage)
{
    if (!lock_held_by_current_thread(&frame_lock))
        lock_acquire(&frame_lock);
    for (size_t i = 0; i < user_pool_size - 1; i++)
    {
        if (frames[i].paddr == kpage)
        {
            lock_acquire(&frames[i].lock);
            pagedir_clear_page(frames[i].owner->pagedir, frames[i].vaddr);
            frames[i].in_use = false;
            frames[i].vaddr = NULL;
            frames[i].owner = NULL;
            lock_release(&frames[i].lock);
            break;
        }
    }
    lock_release(&frame_lock);
    return;
}
