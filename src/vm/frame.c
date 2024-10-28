#include "frame.h"

struct lock frame_lock; /* Lock for frames */
/* struct list frame_table; */
struct frame_table_entry frames[]; /* BOOOOOOO ARRAY :( */

void frame_init(void)
{
    lock_init(&frame_lock);
}

void get_frame()
{
    
}