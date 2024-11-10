#include "lib/kernel/hash.h"
#include "filesys/file.h"
#include "threads/thread.h"
#include <stdbool.h>

struct spt_entry 
{
    struct thread *owner; /* which thread has this spt entry */
    bool in_resident; /* is it in physical memory */
    bool in_swap; /* is it in swap partition */
    bool in_file; /* originates from the filesys */
    bool is_zero; /* zero page */
    void *vaddr; /* virtual address */
    struct file *file; /* file from which page was loaded */
    off_t offset; /* where in the file are we? */
    size_t zero_bytes; /* number of bytes that are zeroed out */
    size_t read_bytes; /* number of bytes that are read from file */
    struct hash_elem hash_elem; /* hash elem used hash spt hash table */
    size_t swap_index; /* swap slot this spt_entry is at if it in swap */
    bool writable; /* is writable */
};

struct hash *spt_init (void);
void destroy_table (void);
struct spt_entry *page_lookup (const void *address, struct thread *owner);
hash_less_func suppl_hash_less;
hash_hash_func suppl_hash_hash;
hash_action_func destruct;
bool add_new_spt_entry (struct hash *spt, struct spt_entry *new_entry);
bool spt_handle_page_fault (struct spt_entry *found);
bool spt_handle_file_fault (struct spt_entry *entry);