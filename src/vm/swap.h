#include <vm/page.h>

void swap_init (void);
void swap_into_disk (struct spt_entry *entry);
void swap_out_of_disk(uint8_t *kpage, size_t index);
