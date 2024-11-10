#include "lib/kernel/bitmap.h"
#include "devices/block.h"
#include "threads/vaddr.h"
#include "threads/synch.h"
#include <stdbool.h>
#include "vm/page.h"
#include "userprog/pagedir.h"
#include "threads/thread.h"

static struct bitmap *swap_partition; /* swap partition */
static struct lock swap_lock; /* swap lock */
struct block *swap; /* actual block for swap */

/* Initializes the swap partition in virtual memory */
void swap_init (void)
{
    /* Jyotsna, and Shreya Varma driving */
    /* get the swap block */
    swap = block_get_role (BLOCK_SWAP);
    if (!swap) 
        {
            PANIC ("SWAP IS NULL");
        }
    
    swap_partition = bitmap_create (block_size (swap) / 8);
    lock_init (&swap_lock);
}

/* Kick out a page from physical memory and store it into the swap partition */
void swap_into_disk (struct spt_entry *entry, uint8_t *kpage)
{
    /* Jyotsna, and Shreya Varma driving */
    /* make sure writing to swap is atomic */
    if (!lock_held_by_current_thread (&swap_lock))
        {
            lock_acquire(&swap_lock);
        }
    entry->in_swap = true;
    entry->in_resident = false;
    /* find free slot in bitmap and mark as occupied */
    size_t swap_index = bitmap_scan_and_flip (swap_partition, 0, 1, false);
    entry->swap_index = swap_index;

    if (swap_index == BITMAP_ERROR)
        {
            PANIC("swap partition is full");
        }

    /* write to each sector of the block for this swap slot */
    for (int i = 0; i < 8; i++)
        {   
            block_write (swap, swap_index * 8 + i, 
                    kpage + (i * 512));
        }

    if (lock_held_by_current_thread (&swap_lock))
        {
            lock_release (&swap_lock);
        }
}

/* Bring in page from swap partition by reading the block */
void swap_out_of_disk (uint8_t *kpage, size_t index)
{
    /* Jyotsna, and Shreya Varma driving */
    /* make sure reading from swap is atomic */
    if (!lock_held_by_current_thread (&swap_lock))
        {
            lock_acquire (&swap_lock);
        }

    size_t swap_index = index;
    
    /* set bitmap spot as free */
    bitmap_reset (swap_partition, swap_index);

    /* read each sector from the block for this swap slot */
    for (int i = 0; i < 8; i++)
        {
            block_read (swap, swap_index * 8 + i, 
                kpage + (i * 512));
        }

    if (lock_held_by_current_thread (&swap_lock))
        {
            lock_release (&swap_lock);
        }
}

