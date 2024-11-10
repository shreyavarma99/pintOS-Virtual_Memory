#include "threads/malloc.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "filesys/file.h"
#include "vm/frame.h"
#include <string.h>
#include <stdio.h>
#include "userprog/syscall.h"
#include "vm/swap.h"

/* Used to create the spt hash table for a thread */
struct hash *spt_init (void) 
{
    /* Shreya Agrawal, and Jyotsna Driving */
    struct hash *spt = malloc (sizeof(struct hash));
    if (!spt) 
        {
            return NULL;
        }
    if (!hash_init (spt, suppl_hash_hash, suppl_hash_less, NULL))
        {
            free (spt); 
            return NULL;
        }
    if (!spt)
        {
            PANIC("spt was null");
        }
    return spt;
}

/* Overridden hash action function for destruction */
void destruct (struct hash_elem *e, void *aux UNUSED)
{
    /* Shreya Agrawal, Garv, and Jyotsna Driving */
    struct spt_entry *entry;
    entry = hash_entry (e, struct spt_entry, hash_elem);

    /* free frame this entry occupies */
    if (entry->in_resident)
        {
            free_frame (pagedir_get_page (thread_current ()->pagedir, 
                entry->vaddr));
        }
    free (entry);
}

/* Used to destory hash table and reclaim resources */
void destroy_table()
{
    /* Shreya Agrawal, Garv, and Jyotsna Driving */
    hash_destroy (thread_current ()->spt, destruct);
}

/* Compares the value of two hash elements A and B, given
   auxiliary data AUX.  Returns true if A is less than B, or
   false if A is greater than or equal to B. */
bool suppl_hash_less (const struct hash_elem *a, const struct hash_elem *b, 
    UNUSED void *aux)
{
  /* Shreya Agrawal, and Jyotsna Driving */
  struct spt_entry *entry_a = hash_entry (a, struct spt_entry, hash_elem);
  struct spt_entry *entry_b = hash_entry (b, struct spt_entry, hash_elem);
  return (entry_a->vaddr < entry_b->vaddr);
}

/* Computes and returns the hash value for hash element E, given
   auxiliary data AUX. */
unsigned suppl_hash_hash (const struct hash_elem *e, UNUSED void *aux)
{
  /* Shreya Agrawal, and Jyotsna Driving */
  const struct spt_entry *pte = hash_entry (e, struct spt_entry, hash_elem);
  return hash_bytes (&pte->vaddr, sizeof(pte->vaddr));
}

/* Find a spt entry in the spt table using the 
 * virtual address and owner thread */
struct spt_entry *page_lookup (const void *address, struct thread *owner)
{
    /* Shreya Agrawal, and Jyotsna Driving */
    struct spt_entry p;
    struct hash_elem *e;

    p.vaddr = pg_round_down (address);
    e = hash_find (owner->spt, &p.hash_elem);
    return e != NULL ? hash_entry (e, struct spt_entry, hash_elem) : NULL;
}

/* Handle bringing in the current spt entry's info from swap or 
 * setting zeros if it is a zero page */
bool spt_handle_page_fault (struct spt_entry *entry)
{
    /* Shreya Agrawal, and Jyotsna Driving */
    if (entry->in_file)
        {
            return spt_handle_file_fault (entry);
        }
    /* select a frame to bring the page into */
    void *kpage = allocate_frame (entry->vaddr);
    if (kpage == NULL) 
        {
            return false;
        }
    /* prevent eviction while being used */
    bool prev = pin_frame (kpage);
    if (entry->swap_index != -1)
        {
            /* bring in from swap */
            swap_out_of_disk (kpage, entry->swap_index);
            entry->swap_index = -1;
            pagedir_set_dirty (entry->owner->pagedir, entry->vaddr, true);
        } 
    else 
        {
            /* zero page */
            memset (kpage, 0, PGSIZE);
        }
    if (!prev)
        {
            unpin_frame (kpage);
        }
    struct thread *t = entry->owner;
    /* save dirty bit to use later */
    bool dirty = pagedir_is_dirty (t->pagedir, entry->vaddr);
    if (!(pagedir_get_page (t->pagedir, entry->vaddr) == NULL &&
        pagedir_set_page (t->pagedir, entry->vaddr, kpage, entry->writable))) 
        {
            /* if cannot be installed, then free it */
            free_frame (kpage);
            return false;
        } 
    /* set dirty bit */  
    pagedir_set_dirty (t->pagedir, entry->vaddr, dirty);
    /* not in swap anymore */ 
    entry->swap_index = -1;
    /* now in physical memory */
    entry->in_resident = true;
    return true;
}

/* Bring in spt entry's page from file */
bool spt_handle_file_fault (struct spt_entry *entry)
{
    /* Shreya Agrawal, Garv, and Jyotsna Driving */
    /* select frame to bring page into */
    uint8_t *kpage = allocate_frame (entry->vaddr);
    bool prev = pin_frame (kpage);

    if (kpage == NULL) 
        {
            return false;
        }
    
    struct thread *t = entry->owner;
    /* check if dirty */
    bool dirty = pagedir_is_dirty (t->pagedir, entry->vaddr);
    if (dirty)
        {
            /* must be brought in from swap */
            swap_out_of_disk (kpage, entry->swap_index);
            entry->swap_index = -1;
            entry->in_resident = true;
            pagedir_set_dirty (entry->owner->pagedir, entry->vaddr, true);
        }
    else 
        {
            bool held = lock_held_by_current_thread (&file_mutex);
            if (!held)
                {
                    lock_acquire (&file_mutex);
                }
            /* read it in from file, free if not read correctly */
            if (file_read_at (entry->file, kpage, entry->read_bytes, 
                entry->offset) != (int) entry->read_bytes) 
                {
                    free_frame (kpage);
                    lock_release (&file_mutex);
                    return false;
                }
            if (!held)
                {
                    lock_release (&file_mutex);
                }
            memset (kpage + entry->read_bytes, 0, entry->zero_bytes);
        }

    if (t == NULL || t->pagedir == NULL) 
        {
            free_frame (kpage);
            return false;
        }

    /* if not installed into pagedir corretly, free the frame */
    if (!(pagedir_get_page (t->pagedir, entry->vaddr) == NULL &&
          pagedir_set_page (t->pagedir, entry->vaddr, kpage, entry->writable))) 
        {
            free_frame (kpage);
            return false;
        }
    /* set dirty to bit saved before */
    pagedir_set_dirty (t->pagedir, entry->vaddr, dirty); 

    /* unpin now that we are done */ 
    if (!prev) 
        {
            unpin_frame (kpage);
        }
    /* not in swap anymore (if it was before), and in physical memory */
    entry->swap_index = -1;
    entry->in_resident = true;
    return true;
}

/* Used to insert a spt entry into the spt hash table */
bool add_new_spt_entry (struct hash *spt, struct spt_entry *new_entry)
{
    /* Shreya Agrawal, and Jyotsna Driving */
    if (!hash_insert (spt, &new_entry->hash_elem))
        {
            return true;
        }
    return false;
}