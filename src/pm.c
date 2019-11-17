/* AUTEURS
 * MATHIEU MATOS
 * NICOLAS RICHARD */

#include <stdio.h>
#include <string.h>


#include "conf.h"
#include "pm.h"

static FILE *pm_backing_store;
static FILE *pm_log;
static char pm_memory[PHYSICAL_MEMORY_SIZE];
static unsigned int download_count = 0;
static unsigned int backup_count = 0;
static unsigned int read_count = 0;
static unsigned int write_count = 0;


// Initialise la mémoire physique
void pm_init (FILE *backing_store, FILE *log)
{
  pm_backing_store = backing_store;
  pm_log = log;
  memset (pm_memory, '\0', sizeof (pm_memory));
}


// Charge la page demandée du backing store
void pm_download_page (unsigned int page_number, unsigned int frame_number)
{
  download_count++;
  /* ¡ TODO: DONE ! */
  char page_read[PAGE_FRAME_SIZE];
  void *pr_ptr;
  pr_ptr = page_read;
  unsigned int read_offset = page_number * PAGE_FRAME_SIZE;
  unsigned int pm_offset = frame_number * PAGE_FRAME_SIZE;

  fseek(pm_backing_store, read_offset, SEEK_SET);
  fread(pr_ptr, 1, PAGE_FRAME_SIZE, pm_backing_store);
  
  for (unsigned int i = 0; i < sizeof(page_read); i++)
    {
      pm_memory[pm_offset + i] = page_read[i];
    }
}

// Sauvegarde la frame spécifiée dans la page du backing store
void pm_backup_page (unsigned int frame_number, unsigned int page_number)
{
  backup_count++;
  /* ¡ TODO: DONE ! */
  char page_write[PAGE_FRAME_SIZE];
  void *pw_ptr;
  pw_ptr = page_write;
  unsigned int frame_offset = frame_number * PAGE_FRAME_SIZE;
  unsigned int write_offset = page_number * PAGE_FRAME_SIZE;
  
  for(unsigned int i = 0; i < sizeof(page_write); i++)
  {
    page_write[i] = pm_memory[frame_offset + i];
  }
  
  fseek(pm_backing_store, write_offset, SEEK_SET);
  fwrite(pw_ptr, 1, PAGE_FRAME_SIZE, pm_backing_store); 
}

char pm_read (unsigned int physical_address)
{
  read_count++;
  /* ¡ TODO: DONE ! */
  return pm_memory[physical_address];
}

void pm_write (unsigned int physical_address, char c)
{
  write_count++;
  /* ¡ TODO: DONE ! */
  pm_memory[physical_address] = c;
}


void pm_clean (void)
{
  // Enregistre l'état de la mémoire physique.
  if (pm_log)
    {
      for (unsigned int i = 0; i < PHYSICAL_MEMORY_SIZE; i++)
	{
	  if (i % 80 == 0)
	    fprintf (pm_log, "%c\n", pm_memory[i]);
	  else
	    fprintf (pm_log, "%c", pm_memory[i]);
	}
    }
  fprintf (stdout, "Page downloads: %2u\n", download_count);
  fprintf (stdout, "Page backups  : %2u\n", backup_count);
  fprintf (stdout, "PM reads : %4u\n", read_count);
  fprintf (stdout, "PM writes: %4u\n", write_count);
}
