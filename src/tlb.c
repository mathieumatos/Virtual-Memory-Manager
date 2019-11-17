/* AUTEURS
 * MATHIEU MATOS
 * NICOLAS RICHARD */

#include <stdint.h>
#include <stdio.h>

#include "tlb.h"
#include "vmm.h"

#include "conf.h"

struct tlb_entry
{
  unsigned int page_number;
  int frame_number;             /* Invalide si négatif.  */
  bool readonly : 1;
};

static FILE *tlb_log = NULL;
static struct tlb_entry tlb_entries[TLB_NUM_ENTRIES]; 

static unsigned int tlb_hit_count = 0;
static unsigned int tlb_miss_count = 0;
static unsigned int tlb_mod_count = 0;

/* Initialise le TLB, et indique où envoyer le log des accès.  */
void tlb_init (FILE *log)
{
  for (int i = 0; i < TLB_NUM_ENTRIES; i++)
  {
    tlb_entries[i].frame_number = -1;
    tlb_entries[i].page_number = -1;
  }
  tlb_log = log;
}

/******************** ¡ NE RIEN CHANGER CI-DESSUS !  ******************/


// ADDED FOR LRU ALGORIGTHM -> REMPLACEMENT DU TLB
static int tlb_lru_index_queue[TLB_NUM_ENTRIES];

/* Recherche dans le TLB.
 * Renvoie le `frame_number`, si trouvé, ou un nombre négatif sinon.  */
static int tlb__lookup (unsigned int page_number, bool write)
{
  // TODO: DONE.
  int frame_num = -1;

  if(tlb_verify(page_number)){
    for (int i = 0; i < TLB_NUM_ENTRIES; i++){
      if(tlb_entries[i].page_number == page_number){
        tlb_entries[i].readonly = write;
        frame_num = tlb_entries[i].frame_number;
        break;
      }
    }
  }
  return frame_num;

}

/* Teste la présence d'une page dans le TLB.
 * Renvoie true, si trouvé, ou false sinon. */
static bool tlb__verify (unsigned int page_number)
{
  // TODO: DONE.
  bool is_present = false;
  for (int i = 0; i < TLB_NUM_ENTRIES; i++)
  {
    if(tlb_entries[i].page_number == page_number){
      is_present = true;
      break;
    }
  }
  return is_present;
}

/* Ajoute dans le TLB une entrée qui associe `frame_number` à
 * `page_number`.  */
static void tlb__add_entry (unsigned int page_number,
                            unsigned int frame_number, bool readonly)
{
  // TODO: DONE.
  
  int index = -1;

  // if page in tlb : update lru array
  if(tlb_verify(page_number))
  {
    // find index of page in tlb
    for(unsigned int i = 0; i < TLB_NUM_ENTRIES; i++)
    {
      if(tlb_entries[i].page_number == page_number)
      {
        index = i;
        break;
      }
    }

    // update tlb and its tlb_lru_index_queue
    update_lru(tlb_lru_index_queue, index);
  }

  /* 
  if page IS NOT in tlb : 
    if tlb has free space (simply add at begining of LRU queue) 
    VS 
    if tlb is full (remove LRU index, add new entry at begining of LRU queue)
  */
  else{
    // Create entry
    struct tlb_entry new_entry = 
    {
      page_number,
      frame_number,
      readonly
    };

    // free space in tlb?
    bool tlb_has_free_space = false;
    int index;

    for(unsigned int i = 0; i < TLB_NUM_ENTRIES; i++)
    {
      // index is free if frame_number == -1
      if(tlb_entries[i].frame_number == -1)
      {
        // if yes : determine index with free space
        tlb_has_free_space = true;
        index = i;
        break;
      }
    }

    
    // Free space in TLB
    if(tlb_has_free_space)
    {
      // add new entry at first free space in tlb
      tlb_entries[index] = new_entry;

      // add new index to tlb_lru_index_queue
      tlb_lru_index_queue[index] = index;

      // update tlb_lru_index_queue since it is the most recently used
      update_lru(tlb_lru_index_queue, index);
    }

    // No free space in TLB
    else
    {
      // changer le LRU entry du tlb pour new_entry
      tlb_entries[tlb_lru_index_queue[TLB_NUM_ENTRIES-1]] = new_entry;

      // le LRU est maintenant le MRU
      update_lru(tlb_lru_index_queue, TLB_NUM_ENTRIES-1);
    }
  }
}

/* decale les entrees du tableau et met l'index specifie en tete de tableau */
void update_lru(int *array, int index){
  int temp = array[index];
  for(int i = index; i > 0; i--){
    array[i] = array[i-1];
  }
  array[0] = temp;
}



/******************** ¡ NE RIEN CHANGER CI-DESSOUS !  ******************/

void tlb_add_entry (unsigned int page_number,
                    unsigned int frame_number, bool readonly)
{
  tlb_mod_count++;
  tlb__add_entry (page_number, frame_number, readonly);
}

int tlb_lookup (unsigned int page_number, bool write)
{
  int fn = tlb__lookup (page_number, write);
  (*(fn < 0 ? &tlb_miss_count : &tlb_hit_count))++;
  return fn;
}

bool tlb_verify (unsigned int page_number) {
  return tlb__verify (page_number);
}

/* Imprime un sommaires des accès.  */
void tlb_clean (void)
{
  fprintf (stdout, "TLB misses   : %3u\n", tlb_miss_count);
  fprintf (stdout, "TLB hits     : %3u\n", tlb_hit_count);
  fprintf (stdout, "TLB changes  : %3u\n", tlb_mod_count);
  fprintf (stdout, "TLB miss rate: %.1f%%\n",
           100 * tlb_miss_count
           /* Ajoute 0.01 pour éviter la division par 0.  */
           / (0.01 + tlb_hit_count + tlb_miss_count));
}
