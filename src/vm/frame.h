/* imports */
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "threads/synch.h"
#include "threads/palloc.h"

struct frame_table_entry 
{
    uint8_t *paddr; /* physical address of frame */ 
    void *vaddr; /* virtual address */ 
    struct thread *owner; /* owner thread of frame */
    bool in_use; /* frame currently being used */
    bool pinned; /* is pinned */
};

void init_frame_table (void);
uint8_t *allocate_frame (void *upage);
struct frame_table_entry *evict_frame (void);
void free_frame (void *kpage);
bool pin_frame (uint8_t *kpage);
void unpin_frame (uint8_t *kpage);

extern size_t clock;


