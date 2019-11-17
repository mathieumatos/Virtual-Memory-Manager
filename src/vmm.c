/* AUTEURS
 * MATHIEU MATOS
 * NICOLAS RICHARD */

#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "conf.h"
#include "common.h"
#include "vmm.h"
#include "tlb.h"
#include "pt.h"
#include "pm.h"

static unsigned int read_count = 0;
static unsigned int write_count = 0;
static FILE* vmm_log;


// added
unsigned int free_frame_index = 0;
int pm_lru_index_queue[NUM_FRAMES];


void vmm_init (FILE *log)
{
  // Initialise le fichier de journal.
  vmm_log = log;
}


// NE PAS MODIFIER CETTE FONCTION
static void vmm_log_command (FILE *out, const char *command,
                            unsigned int laddress, /* Logical address. */
		                        unsigned int page,
                            unsigned int frame,
                            unsigned int offset,
                            unsigned int paddress, /* Physical address.  */
		                        char c) /* Caractère lu ou écrit.  */
{
  if (out)
    fprintf (out, "%s[%c]@%05d: p=%d, o=%d, f=%d pa=%d\n", command, c, laddress,
	     page, offset, frame, paddress);
}



/* Effectue une lecture à l'adresse logique `laddress`.  */
char vmm_read (unsigned int laddress)
{
  char c = '!';
  read_count++;
  /* ¡ TODO: DONE ! */

  unsigned int page_num = decode_laddress('p', laddress);
  unsigned int offset = decode_laddress('o', laddress);
  int frame_num = tlb_lookup(page_num, false);
  
  /* TLB MISS */
  if(frame_num < 0)
  {
    frame_num = pt_lookup(page_num);

    /* PAGE FAULT */
    if(frame_num < 0)
    {
      // Find free frame
      frame_num = page_fault_manager(page_num, free_frame_index, 'r');
      
      if(free_frame_index < PHYSICAL_MEMORY_SIZE)
      {
      	free_frame_index += PAGE_FRAME_SIZE;
      }
    }
  }

  pm_update_lru(frame_num);
  tlb_add_entry(page_num, frame_num, true);
  unsigned int paddress = (frame_num * PAGE_FRAME_SIZE) + offset;
  c = pm_read(paddress);

  // TODO: DONE.
  vmm_log_command (stdout, "READING", laddress, page_num, frame_num, offset, paddress, c);
  return c;
}



/* finds a suitable frame and downloads the page in it */
int page_fault_manager(unsigned int page_num, unsigned int index, char mode)
{
  int frame_num;
  
  /* find suitable frame */
  if (index >= PHYSICAL_MEMORY_SIZE)
  {
    /* if pm full */
    frame_num = pm_lru_index_queue[NUM_FRAMES-1];

    /* if page was modified */
    if (!pt_readonly_p(page_num))
    {
      pm_backup_page(frame_num, page_num);
    }
  }
  else 
  {
    /* if pm has empty frame */
    frame_num = index / PAGE_FRAME_SIZE;

    // add frame to pm_lru_index_queue
    pm_lru_index_queue[frame_num] = frame_num;
    pm_update_lru(frame_num);
  }

  /* free the frame */
  pt_unset_entry(page_num);

  /* download new page */
  pm_download_page(page_num, frame_num);
  pt_set_entry(page_num, frame_num);

  return frame_num;
}


/* Effectue une écriture à l'adresse logique `laddress`.  */
void vmm_write (unsigned int laddress, char c)
{
  write_count++;
  /* ¡ TODO: DONE ! */

  unsigned int page_num = decode_laddress('p', laddress);
  unsigned int offset = decode_laddress('o', laddress);
  int frame_num = tlb_lookup(page_num, true);
  
  /* TLB MISS */
  if (frame_num < 0)
  {
    frame_num = pt_lookup(page_num);

    /* PAGE FAULT */
    if (frame_num < 0)
    {
      frame_num = page_fault_manager(page_num, free_frame_index, 'w');
      
      if (free_frame_index < PHYSICAL_MEMORY_SIZE)
      {
        free_frame_index += PAGE_FRAME_SIZE;
      }
    }
  }
  
  tlb_add_entry(page_num, frame_num, false);
  unsigned int paddress = (frame_num * PAGE_FRAME_SIZE) + offset;
  pm_write(paddress, c);
  pt_set_readonly(page_num, false);

  // TODO: DONE.
  vmm_log_command (stdout, "WRITING", laddress, page_num, frame_num, offset, paddress, c);
}


/* returns offset or page number */
int decode_laddress(char operator, unsigned int laddress) {
  return (operator == 'o' ? laddress % PAGE_FRAME_SIZE : laddress / PAGE_FRAME_SIZE);
}


/* decale elements du tableau et place l'element specifie en tete de tableau */
void pm_update_lru(int f_num){
  int index = -1;
  /* find specified frame index */
  for(int i = 0; i < NUM_FRAMES; i++){
    if(pm_lru_index_queue[i] == f_num){
      index = i;
      break;
    }
  }

  if(index == -1){
    printf("ERROR AT PM_UPDATE_LRU : FRAME NOT FOUND");
    exit(1);
  }
  else{
    int temp = pm_lru_index_queue[index];
    for(int j = index; j > 0; j--){
      pm_lru_index_queue[j] = pm_lru_index_queue[j-1];
    }
    pm_lru_index_queue[0] = temp;
  }
}


/* backup toutes les pages avant la fin de l'exécution 
 * cette fonction est appelee dans "parse.y" */
void vmm_backup_final (void)
{
  for (unsigned int i = 0; i < NUM_PAGES; i++)
  {
    int frame_num = pt_lookup_final(i);
    if (frame_num >= 0)
    {
      if (!pt_readonly_p(i))
      {
        pm_backup_page(frame_num, i);
      }
    }
  }
}


// NE PAS MODIFIER CETTE FONCTION
void vmm_clean (void)
{
  fprintf (stdout, "VM reads : %4u\n", read_count);
  fprintf (stdout, "VM writes: %4u\n", write_count);
}
