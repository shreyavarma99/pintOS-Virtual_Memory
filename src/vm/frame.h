/* imports */
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "threads/synch.h"
#include "threads/palloc.h"

struct frame_table_entry {
    uint8_t *paddr; /* physical address of frame */ 
    void *vaddr; /* virtual address TODO also include spte */ 
    struct thread *owner;
    bool in_use;
    struct lock lock;
    bool pinned;
};

void init_frame_table (void);
uint8_t *allocate_frame(void *upage);
struct frame_table_entry *evict_frame(void);
void free_frame(void *kpage);
extern size_t clock;
bool pin_frame(uint8_t *kpage);
void unpin_frame(uint8_t *kpage);

// void *allocate_frame(enum palloc_flags flagies, void *upage);
// void free_frame(void *kpage);
// struct frame_table_entry *evict_frame(void);


