/* imports */
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "threads/tests.h"
#include "threads/synch.h"
#include "threads/palloc.h"

struct spt_entry {
    struct thread *owner;
    struct file *file; /* file from which page was loaded */
    __off_t offset; /* where in the file are we? */
    size_t zero_bytes; /* number of bytes that are zeroed out */
    size_t read_bytes; /* number of bytes that are read from file */
    

};