/* imports */
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
// #include "threads/tests.h"
#include "threads/synch.h"
#include "threads/palloc.h"
#include <hash.h>
#include "lib/kernel/hash.h"
#include "filesys/file.h"

enum page_location{
    FILE_BACKED,
    SWAP,
    ZERO_PAGE,
};

struct spt_entry {
    struct thread *owner;
    enum page_location location;//location of page: SWAP, DISK, or FILE backed
    void *vaddr;
    struct file *file; /* file from which page was loaded */
    off_t offset; /* where in the file are we? */
    size_t zero_bytes; /* number of bytes that are zeroed out */
    size_t read_bytes; /* number of bytes that are read from file */
    struct hash_elem hash_elem;
    bool writable;
};
