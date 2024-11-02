/* imports */
#include "vm/frame.h"
#include "lib/kernel/hash.h"
#include "filesys/file.h"
#include "threads/thread.h"

struct spt_entry {
    struct thread *owner;
    bool in_resident;
    bool in_swap;
    bool in_file;
    void *vaddr;
    struct file *file; /* file from which page was loaded */
    off_t offset; /* where in the file are we? */
    size_t zero_bytes; /* number of bytes that are zeroed out */
    size_t read_bytes; /* number of bytes that are read from file */
    struct hash_elem hash_elem;
    bool writable;
};

struct hash *spt_init (void);
struct spt_entry *page_lookup (const void *address, struct thread *owner);
hash_less_func suppl_hash_less;
hash_hash_func suppl_hash_hash;
bool spt_handle_file_fault(struct spt_entry *entry);