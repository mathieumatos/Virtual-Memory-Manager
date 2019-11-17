#ifndef VMM_H
#define VMM_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "conf.h"

void vmm_init (FILE *log);
char vmm_read (unsigned int logical_address);
void vmm_write (unsigned int logical_address, char);
void vmm_clean (void);
int decode_laddress(char operator, unsigned int laddress);
int page_fault_manager(unsigned int page_num, unsigned int index, char mode);
void pm_update_lru(int f_num);
void vmm_backup_final(void);


#endif
