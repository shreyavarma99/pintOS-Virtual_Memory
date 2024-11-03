#include "lib/kernel/bitmap.h"
#include "devices/block.h"
#include "threads/vaddr.h"
#include "threads/synch.h"
#include <stdbool.h>
#include "vm/page.h"
#include "userprog/pagedir.h"
#include "threads/thread.h"

static struct bitmap *swap_partition;
static struct lock lock;
struct block *swap;

void swap_init (void)
{
    swap = block_get_role(BLOCK_SWAP);
    if (!swap) {
        PANIC ("SWAP IS NULL");
    }
    swap_partition = bitmap_create(block_size (swap) / 8);
}

void swap_into_disk (struct spt_entry *entry)
{
    entry->in_swap = true;
    entry->in_resident = false;
    size_t swap_index = bitmap_scan_and_flip(swap_partition, 0, 1, false);
    entry->swap_index = swap_index;

    if (swap_index == BITMAP_ERROR)
    {
        PANIC("swap partition is full");
    }

    for (int i = 0; i < 8; i++)
    {
        block_write(swap, swap_index * 8 + i, 
                pagedir_get_page(entry->owner->pagedir, entry->vaddr) + i * 512);
    }

}

void swap_out_of_disk(struct spt_entry *entry)
{
    entry->in_swap = false;
    entry->in_resident = true;
    size_t swap_index = entry->swap_index;
    
    for (int i = 0; i < 8; i++)
    {
        block_read(swap, swap_index * 8 + i, 
            pagedir_get_page(entry->owner->pagedir, entry->vaddr) + i * 512);
    }
    bitmap_reset (swap_partition, swap_index);
    entry->swap_index = -1;
}

