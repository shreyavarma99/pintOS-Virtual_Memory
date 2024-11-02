#include "page.h"
#include "threads/malloc.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "filesys/file.h"
#include <string.h>
#include <stdio.h>

struct hash *spt_init (void) {
    struct hash *spt = malloc(sizeof(struct hash));
    if (!spt) {
        return NULL;
    }
    if(!hash_init (spt, suppl_hash_hash, suppl_hash_less, NULL))
    {
      free(spt); 
      return NULL;
    }
    if(!spt){
        PANIC("spt was null");
    }
    return spt;
}

/* Compares the value of two hash elements A and B, given
   auxiliary data AUX.  Returns true if A is less than B, or
   false if A is greater than or equal to B. */
bool suppl_hash_less (const struct hash_elem *a, const struct hash_elem *b, UNUSED void *aux)
{
  struct spt_entry *entry_a = hash_entry (a, struct spt_entry, hash_elem);
  struct spt_entry *entry_b = hash_entry (b, struct spt_entry, hash_elem);
  return (entry_a->vaddr < entry_b->vaddr);
}

/* Computes and returns the hash value for hash element E, given
   auxiliary data AUX. */
unsigned suppl_hash_hash (const struct hash_elem *e, UNUSED void *aux)
{
  const struct spt_entry *pte = hash_entry (e, struct spt_entry, hash_elem);
  return hash_bytes (&pte->vaddr, sizeof(pte->vaddr));
}

struct spt_entry *page_lookup (const void *address, struct thread *owner)
{
    //ASSERT(owner);
    //ASSERT(owner->spt != NULL);  // Ensure spt is initialized

    struct spt_entry p;
    struct hash_elem *e;

    p.vaddr = pg_round_down(address);
    e = hash_find(owner->spt, &p.hash_elem);
    return e != NULL ? hash_entry(e, struct spt_entry, hash_elem) : NULL;
}

bool spt_handle_file_fault(struct spt_entry *entry){
 uint8_t *kpage = allocate_frame(PAL_USER);
    if (kpage == NULL) {
        printf("Failed to allocate frame\n");
        return false;
    }
    entry->in_resident = true;

    if (file_read_at(entry->file, kpage, entry->read_bytes, entry->offset) != (int) entry->read_bytes) {
        palloc_free_page(kpage);
        printf("Failed to read file data into frame\n");
        return false;
    }
    memset(kpage + entry->read_bytes, 0, entry->zero_bytes);

    struct thread *t = entry->owner;
    if (t == NULL || t->pagedir == NULL) {
        palloc_free_page(kpage);
        printf("Thread or page directory is NULL\n");
        return false;
    }

    if (!(pagedir_get_page(t->pagedir, entry->vaddr) == NULL &&
          pagedir_set_page(t->pagedir, entry->vaddr, kpage, entry->writable))) {
        palloc_free_page(kpage);
        printf("Failed to map page to address space\n");
        return false;
    }
    return true;
}