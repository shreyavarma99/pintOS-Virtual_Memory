// populate spt
// check for stack growth
// handle stack growth

#include "page.h"
#include "threads/malloc.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include <stdio.h>


static bool suppl_hash_less (const struct hash_elem *a,
                             const struct hash_elem *b, void *aux);

static unsigned suppl_hash_hash (const struct hash_elem *e, void *aux);

/*TODO: give every process an spt */

/* initialize supplemental page table */
struct hash *spt_init () {
    struct hash *spt = malloc(sizeof(struct hash));
    if(spt == NULL || !hash_init (spt, &suppl_hash_hash, &suppl_hash_less, NULL)){
      free(spt);
      return NULL;
    }
    return spt;
}

bool install_page(void *upage, void *kpage, bool writable){
  
}

// void sup_destroy(struct hash *spt){
//   if(spt){
//     hash_destroy(spt, )
//   }
// }

//add a file backed spt entry to the page table
bool spt_add_file_entry(struct hash *spt, void *upage, struct file *file, off_t offset, size_t read_bytes, size_t zero_bytes, bool writable) {
  //Shreya Varma driving
  struct spt_entry *new_entry =  malloc(sizeof(struct spt_entry));

  if(!new_entry){
    //did not add successfully to spt
    return false;
  }


  //added successfully
  new_entry->file = file;
  new_entry->vaddr = upage;
  new_entry->offset = offset;
  new_entry->read_bytes = read_bytes;
  new_entry->zero_bytes = zero_bytes;
  new_entry->writable = writable;
  new_entry->location = FILE_BACKED;
  new_entry->owner = thread_current();

  //insert to spt; true if successful false otherwise
  if(hash_insert(spt, &new_entry->hash_elem)){
    return true;
  }

  return false;
}

//set entry to swap
bool set_spt_entry_to_swap(void *upage, int swap_index){
  struct spt_entry *spt_entry = find_page_entry(upage, thread_current());
  if(!spt_entry || spt_entry->location != FILE_BACKED){
    //did not already exist in the spt handle accordingly
    //or is not file backed
    return false;
  }

  //TODO: do smtg with swap index when swap implemented
  spt_entry->file = NULL;
  spt_entry->location = SWAP;
  spt_entry->offset = 0;
  spt_entry->read_bytes = 0;
  spt_entry->zero_bytes = 0;

  return true;
}

//find spt entry given upage and owner thread
struct spt_entry *find_page_entry(void *upage, struct thread *owner){
  struct spt_entry *temp_entry;
  temp_entry->vaddr = upage;
  // struct hash_elem *match = hash_find(thread_current()->spt, &temp_entry.hash_elem);
  struct hash_elem *match = hash_find(owner->spt, &temp_entry->hash_elem);

  if(match){
    return match;
  }
  return NULL;  
}

//add a page in disk to the page table

//check for stack growth

//add a page in swap to the page table

/* Compares the value of two hash elements A and B, given
   auxiliary data AUX.  Returns true if A is less than B, or
   false if A is greater than or equal to B. */
bool suppl_hash_less (const struct hash_elem *a, const struct hash_elem *b, void *aux)
{
  struct spt_entry *entry_a = hash_entry (a, struct spt_entry, hash_elem);
  struct spt_entry *entry_b = hash_entry (b, struct spt_entry, hash_elem);
  return (entry_a->vaddr < entry_b->vaddr);
}

/* Computes and returns the hash value for hash element E, given
   auxiliary data AUX. */
unsigned suppl_hash_hash (const struct hash_elem *e, void *aux)
{
  const struct spt_entry *pte = hash_entry (e, struct spt_entry, hash_elem);
  unsigned bytes = hash_bytes (&pte->vaddr, sizeof(pte->vaddr));
  return bytes;
}