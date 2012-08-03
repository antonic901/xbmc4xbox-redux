/*
 * Copyright (C) 2000 Rich Wareham <richwareham@users.sourceforge.net>
 *
 * This file is part of libdvdnav, a DVD navigation library.
 *
 * libdvdnav is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * libdvdnav is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
 *
 * $Id: searching.c 1135 2008-09-06 21:55:51Z rathann $
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include "dvd_types.h"
#include <dvdread/nav_types.h>
#include <dvdread/ifo_types.h>
#include "remap.h"
#include "vm/decoder.h"
#include "vm/vm.h"
#include "dvdnav.h"
#include "dvdnav_internal.h"
#include <dvdread/ifo_read.h>

#define RESULT_FALSE        0
#define RESULT_TRUE         1
#define CELL_FIND_SECTOR    1
#define CELL_FIND_TIME      2
#define CELL_FIND_INDEX     3
#define TMAP_IDX_EDGE_BGN  -1
#define TMAP_IDX_EDGE_END  -2

/*
#define LOG_DEBUG
*/

/* Searching API calls */

/* Scan the ADMAP for a particular block number. */
/* Return placed in vobu. */
/* Returns error status */
/* FIXME: Maybe need to handle seeking outside current cell. */
static dvdnav_status_t dvdnav_scan_admap(dvdnav_t *this, int32_t domain, uint32_t seekto_block, int next, uint32_t *vobu) {
  vobu_admap_t *admap = NULL;

#ifdef LOG_DEBUG
  fprintf(MSG_OUT, "libdvdnav: Seeking to target %u ...\n", seekto_block);
#endif
  *vobu = -1;

  /* Search through the VOBU_ADMAP for the nearest VOBU
   * to the target block */
  switch(domain) {
  case FP_DOMAIN:
  case VMGM_DOMAIN:
    admap = this->vm->vmgi->menu_vobu_admap;
    break;
  case VTSM_DOMAIN:
    admap = this->vm->vtsi->menu_vobu_admap;
    break;
  case VTS_DOMAIN:
    admap = this->vm->vtsi->vts_vobu_admap;
    break;
  default:
    fprintf(MSG_OUT, "libdvdnav: Error: Unknown domain for seeking.\n");
  }
  if(admap) {
    uint32_t address = 0;
    uint32_t vobu_start, next_vobu;
    int admap_entries = (admap->last_byte + 1 - VOBU_ADMAP_SIZE)/VOBU_ADMAP_SIZE;

    /* Search through ADMAP for best sector */
    vobu_start = SRI_END_OF_CELL;
    /* FIXME: Implement a faster search algorithm */
    while(address < admap_entries) {
      next_vobu = admap->vobu_start_sectors[address];

      /* fprintf(MSG_OUT, "libdvdnav: Found block %u\n", next_vobu); */

      if(vobu_start <= seekto_block && next_vobu > seekto_block)
        break;
      vobu_start = next_vobu;
      address++;
    }
    *vobu = next ? next_vobu : vobu_start;
    return DVDNAV_STATUS_OK;
  }
  fprintf(MSG_OUT, "libdvdnav: admap not located\n");
  return DVDNAV_STATUS_ERR;
}

dvdnav_status_t dvdnav_time_search(dvdnav_t *this,
				   uint64_t time) {

  uint32_t target;
  uint64_t length = 0;
  uint32_t first_cell_nr, last_cell_nr, cell_nr;
  int32_t found = 0;
  cell_playback_t *cell;
  dvd_state_t *state;

  if(this->position_current.still != 0) {
    printerr("Cannot seek in a still frame.");
    return DVDNAV_STATUS_ERR;
  }

  pthread_mutex_lock(&this->vm_lock);
  state = &(this->vm->state);
  if(!state->pgc) {
    printerr("No current PGC.");
    pthread_mutex_unlock(&this->vm_lock);
    return DVDNAV_STATUS_ERR;
  }

  if((state->pgc->prohibited_ops.title_or_time_play == 1) ||
      (this->pci.pci_gi.vobu_uop_ctl.title_or_time_play == 1 )){
    printerr("operation forbidden.");
    pthread_mutex_unlock(&this->vm_lock);
    return DVDNAV_STATUS_ERR;
  }

  /* setup what cells we should be working within */
  if (this->pgc_based) {
    first_cell_nr = 1;
    last_cell_nr = state->pgc->nr_of_cells;
  } else {
    /* Find start cell of program. */
    first_cell_nr = state->pgc->program_map[state->pgN-1];
    /* Find end cell of program */
    if(state->pgN < state->pgc->nr_of_programs)
      last_cell_nr = state->pgc->program_map[state->pgN] - 1;
    else
      last_cell_nr = state->pgc->nr_of_cells;
  }

  /* FIXME: using time map is not going to work unless we are pgc_based */
  /*        we'd need to recalculate the time to be relative to full pgc first*/
  if(!this->pgc_based)
  {
#ifdef LOG_DEBUG
    fprintf(MSG_OUT, "libdvdnav: time_search - not pgc based\n");
#endif
    goto timemapdone;
  }

  if(!this->vm->vtsi->vts_tmapt){
    /* no time map for this program chain */
#ifdef LOG_DEBUG
    fprintf(MSG_OUT, "libdvdnav: time_search - no time map for this program chain\n");
#endif
    goto timemapdone;
  }

  if(this->vm->vtsi->vts_tmapt->nr_of_tmaps < state->pgcN){
    /* to few time maps for this program chain */
#ifdef LOG_DEBUG
    fprintf(MSG_OUT, "libdvdnav: time_search - to few time maps for this program chain\n");
#endif
    goto timemapdone;
  }

  /* get the tmpat corresponding to the pgc */
  vts_tmap_t *tmap = &(this->vm->vtsi->vts_tmapt->tmap[state->pgcN-1]);

  if(tmap->tmu == 0){
    /* no time unit for this time map */
#ifdef LOG_DEBUG
    fprintf(MSG_OUT, "libdvdnav: time_search - no time unit for this time map\n");
#endif
    goto timemapdone;
  }

  /* time is in pts (90khz clock), get the number of tmu's that represent */
  /* first entry defines at time tmu not time zero */
  int entry = time / tmap->tmu / 90000 - 1;
  if(entry > tmap->nr_of_entries)
    entry = tmap->nr_of_entries -1;

  if(entry > 0)
  {
    /* get the table entry, disregarding marking of discontinuity */
    target = tmap->map_ent[entry] & 0x7fffffff;
  }
  else
  {
    /* start from first vobunit */
    target = state->pgc->cell_playback[first_cell_nr-1].first_sector;;
  }

  /* if we have an additional entry we can interpolate next position */
  /* allowed only if next entry isn't discontinious */

  if( entry < tmap->nr_of_entries - 1)
  {
    const uint32_t target2 = tmap->map_ent[entry+1];
    const uint64_t timeunit = tmap->tmu*90000;
    if( !( target2 & 0x80000000) )
    {
      length = target2 - target;
      target += (uint32_t) (length * ( time - (entry+1)*timeunit ) / timeunit);
    }
  }
  found = 1;

timemapdone:


  for(cell_nr = first_cell_nr; cell_nr <= last_cell_nr; cell_nr ++) {
    cell =  &(state->pgc->cell_playback[cell_nr-1]);

    if(cell->block_type == BLOCK_TYPE_ANGLE_BLOCK && cell->block_mode != BLOCK_MODE_FIRST_CELL)
      continue;

    if(found) {

      if (target >= cell->first_sector
      &&  target <= cell->last_sector)
         break;

    } else {

      length = dvdnav_convert_time(&cell->playback_time);
      if (time >= length) {
        time -= length;
      } else {
        /* FIXME: there must be a better way than interpolation */
        target = time * (cell->last_sector - cell->first_sector + 1) / length;
        target += cell->first_sector;

  #ifdef LOG_DEBUG
        if( cell->first_sector > target || target > cell->last_sector )
          fprintf(MSG_OUT, "libdvdnav: time_search - sector is not within cell min:%u, max:%u, cur:%u\n", cell->first_sector, cell->last_sector, target);
  #endif

        found = 1;
        break;
      }
    }
  }

  if(found) {
    uint32_t vobu;
#ifdef LOG_DEBUG
    fprintf(MSG_OUT, "libdvdnav: Seeking to cell %i from choice of %i to %i\n",
	    cell_nr, first_cell_nr, last_cell_nr);
#endif
    if (dvdnav_scan_admap(this, state->domain, target, 0, &vobu) == DVDNAV_STATUS_OK) {
      uint32_t start = state->pgc->cell_playback[cell_nr-1].first_sector;

      if (vm_jump_cell_block(this->vm, cell_nr, vobu - start)) {
#ifdef LOG_DEBUG
        fprintf(MSG_OUT, "libdvdnav: After cellN=%u blockN=%u target=%x vobu=%x start=%x\n" ,
          state->cellN, state->blockN, target, vobu, start);
#endif
        this->vm->hop_channel += HOP_SEEK;
        pthread_mutex_unlock(&this->vm_lock);
        return DVDNAV_STATUS_OK;
      }
    }
  }

  fprintf(MSG_OUT, "libdvdnav: Error when seeking\n");
  printerr("Error when seeking.");
  pthread_mutex_unlock(&this->vm_lock);
  return DVDNAV_STATUS_ERR;
}

dvdnav_status_t dvdnav_sector_search(dvdnav_t *this,
				     uint64_t offset, int32_t origin) {
  uint32_t target = 0;
  uint32_t current_pos;
  uint32_t cur_sector;
  uint32_t cur_cell_nr;
  uint32_t length = 0;
  uint32_t first_cell_nr, last_cell_nr, cell_nr;
  int32_t found;
  int forward = 0;
  cell_playback_t *cell;
  dvd_state_t *state;
  dvdnav_status_t result;

  if(this->position_current.still != 0) {
    printerr("Cannot seek in a still frame.");
    return DVDNAV_STATUS_ERR;
  }

  result = dvdnav_get_position(this, &target, &length);
  if(!result) {
    printerr("Cannot get current position");
    return DVDNAV_STATUS_ERR;
  }

  pthread_mutex_lock(&this->vm_lock);
  state = &(this->vm->state);
  if(!state->pgc) {
    printerr("No current PGC.");
    pthread_mutex_unlock(&this->vm_lock);
    return DVDNAV_STATUS_ERR;
  }
#ifdef LOG_DEBUG
  fprintf(MSG_OUT, "libdvdnav: seeking to offset=%llu pos=%u length=%u\n", offset, target, length);
  fprintf(MSG_OUT, "libdvdnav: Before cellN=%u blockN=%u\n", state->cellN, state->blockN);
#endif

  current_pos = target;
  cur_sector = this->vobu.vobu_start + this->vobu.blockN;
  cur_cell_nr = state->cellN;

  switch(origin) {
   case SEEK_SET:
    if(offset >= length) {
      printerr("Request to seek behind end.");
      pthread_mutex_unlock(&this->vm_lock);
      return DVDNAV_STATUS_ERR;
    }
    target = offset;
    break;
   case SEEK_CUR:
    if(target + offset >= length) {
      printerr("Request to seek behind end.");
      pthread_mutex_unlock(&this->vm_lock);
      return DVDNAV_STATUS_ERR;
    }
    target += offset;
    break;
   case SEEK_END:
    if(length < offset) {
      printerr("Request to seek before start.");
      pthread_mutex_unlock(&this->vm_lock);
      return DVDNAV_STATUS_ERR;
    }
    target = length - offset;
    break;
   default:
    /* Error occured */
    printerr("Illegal seek mode.");
    pthread_mutex_unlock(&this->vm_lock);
    return DVDNAV_STATUS_ERR;
  }
  forward = target > current_pos;

  this->cur_cell_time = 0;
  if (this->pgc_based) {
    first_cell_nr = 1;
    last_cell_nr = state->pgc->nr_of_cells;
  } else {
    /* Find start cell of program. */
    first_cell_nr = state->pgc->program_map[state->pgN-1];
    /* Find end cell of program */
    if(state->pgN < state->pgc->nr_of_programs)
      last_cell_nr = state->pgc->program_map[state->pgN] - 1;
    else
      last_cell_nr = state->pgc->nr_of_cells;
  }

  found = 0;
  for(cell_nr = first_cell_nr; (cell_nr <= last_cell_nr) && !found; cell_nr ++) {
    cell =  &(state->pgc->cell_playback[cell_nr-1]);
    if(cell->block_type == BLOCK_TYPE_ANGLE_BLOCK && cell->block_mode != BLOCK_MODE_FIRST_CELL)
      continue;
    length = cell->last_sector - cell->first_sector + 1;
    if (target >= length) {
      target -= length;
    } else {
      /* convert the target sector from Cell-relative to absolute physical sector */
      target += cell->first_sector;
      if (forward && (cell_nr == cur_cell_nr)) {
        uint32_t vobu;
        /* if we are seeking forward from the current position, make sure
         * we move to a new position that is after our current position.
         * simply truncating to the vobu will go backwards */
        if (dvdnav_scan_admap(this, state->domain, target, 0, &vobu) != DVDNAV_STATUS_OK)
          break;
        if (vobu <= cur_sector) {
          if (dvdnav_scan_admap(this, state->domain, target, 1, &vobu) != DVDNAV_STATUS_OK)
            break;
          if (vobu > cell->last_sector) {
            if (cell_nr == last_cell_nr)
              break;
            cell_nr++;
            cell =  &(state->pgc->cell_playback[cell_nr-1]);
            target = cell->first_sector;
          } else {
            target = vobu;
          }
        }
      }
      found = 1;
      break;
    }
  }

  if(found) {
    uint32_t vobu;
#ifdef LOG_DEBUG
    fprintf(MSG_OUT, "libdvdnav: Seeking to cell %i from choice of %i to %i\n",
	    cell_nr, first_cell_nr, last_cell_nr);
#endif
    if (dvdnav_scan_admap(this, state->domain, target, 0, &vobu) == DVDNAV_STATUS_OK) {
      int32_t start = state->pgc->cell_playback[cell_nr-1].first_sector;

      if (vm_jump_cell_block(this->vm, cell_nr, vobu - start)) {
#ifdef LOG_DEBUG
        fprintf(MSG_OUT, "libdvdnav: After cellN=%u blockN=%u target=%x vobu=%x start=%x\n" ,
          state->cellN, state->blockN, target, vobu, start);
#endif
        this->vm->hop_channel += HOP_SEEK;
        pthread_mutex_unlock(&this->vm_lock);
        return DVDNAV_STATUS_OK;
      }
    }
  }

  fprintf(MSG_OUT, "libdvdnav: Error when seeking\n");
  fprintf(MSG_OUT, "libdvdnav: FIXME: Implement seeking to location %u\n", target);
  printerr("Error when seeking.");
  pthread_mutex_unlock(&this->vm_lock);
  return DVDNAV_STATUS_ERR;
}

dvdnav_status_t dvdnav_part_search(dvdnav_t *this, int32_t part) {
  int32_t title, old_part;

  if (dvdnav_current_title_info(this, &title, &old_part) == DVDNAV_STATUS_OK)
    return dvdnav_part_play(this, title, part);
  return DVDNAV_STATUS_ERR;
}

dvdnav_status_t dvdnav_prev_pg_search(dvdnav_t *this) {
  pthread_mutex_lock(&this->vm_lock);
  if(!this->vm->state.pgc) {
    printerr("No current PGC.");
    pthread_mutex_unlock(&this->vm_lock);
    return DVDNAV_STATUS_ERR;
  }

#ifdef LOG_DEBUG
  fprintf(MSG_OUT, "libdvdnav: previous chapter\n");
#endif
  if (!vm_jump_prev_pg(this->vm)) {
    fprintf(MSG_OUT, "libdvdnav: previous chapter failed.\n");
    printerr("Skip to previous chapter failed.");
    pthread_mutex_unlock(&this->vm_lock);
    return DVDNAV_STATUS_ERR;
  }
  this->cur_cell_time = 0;
  this->position_current.still = 0;
  this->vm->hop_channel++;
#ifdef LOG_DEBUG
  fprintf(MSG_OUT, "libdvdnav: previous chapter done\n");
#endif
  pthread_mutex_unlock(&this->vm_lock);

  return DVDNAV_STATUS_OK;
}

dvdnav_status_t dvdnav_top_pg_search(dvdnav_t *this) {
  pthread_mutex_lock(&this->vm_lock);
  if(!this->vm->state.pgc) {
    printerr("No current PGC.");
    pthread_mutex_unlock(&this->vm_lock);
    return DVDNAV_STATUS_ERR;
  }

#ifdef LOG_DEBUG
  fprintf(MSG_OUT, "libdvdnav: top chapter\n");
#endif
  if (!vm_jump_top_pg(this->vm)) {
    fprintf(MSG_OUT, "libdvdnav: top chapter failed.\n");
    printerr("Skip to top chapter failed.");
    pthread_mutex_unlock(&this->vm_lock);
    return DVDNAV_STATUS_ERR;
  }
  this->cur_cell_time = 0;
  this->position_current.still = 0;
  this->vm->hop_channel++;
#ifdef LOG_DEBUG
  fprintf(MSG_OUT, "libdvdnav: top chapter done\n");
#endif
  pthread_mutex_unlock(&this->vm_lock);

  return DVDNAV_STATUS_OK;
}

dvdnav_status_t dvdnav_next_pg_search(dvdnav_t *this) {
  vm_t *try_vm;

  pthread_mutex_lock(&this->vm_lock);
  if(!this->vm->state.pgc) {
    printerr("No current PGC.");
    pthread_mutex_unlock(&this->vm_lock);
    return DVDNAV_STATUS_ERR;
  }

#ifdef LOG_DEBUG
  fprintf(MSG_OUT, "libdvdnav: next chapter\n");
#endif
  /* make a copy of current VM and try to navigate the copy to the next PG */
  try_vm = vm_new_copy(this->vm);
  if (!vm_jump_next_pg(try_vm) || try_vm->stopped) {
    vm_free_copy(try_vm);
    /* next_pg failed, try to jump at least to the next cell */
    try_vm = vm_new_copy(this->vm);
    vm_get_next_cell(try_vm);
    if (try_vm->stopped) {
      vm_free_copy(try_vm);
      fprintf(MSG_OUT, "libdvdnav: next chapter failed.\n");
      printerr("Skip to next chapter failed.");
      pthread_mutex_unlock(&this->vm_lock);
      return DVDNAV_STATUS_ERR;
    }
  }
  this->cur_cell_time = 0;
  /* merge changes on success */
  vm_merge(this->vm, try_vm);
  vm_free_copy(try_vm);
  this->position_current.still = 0;
  this->vm->hop_channel++;
#ifdef LOG_DEBUG
  fprintf(MSG_OUT, "libdvdnav: next chapter done\n");
#endif
  pthread_mutex_unlock(&this->vm_lock);

  return DVDNAV_STATUS_OK;
}

dvdnav_status_t dvdnav_menu_call(dvdnav_t *this, DVDMenuID_t menu) {
  vm_t *try_vm;

  pthread_mutex_lock(&this->vm_lock);
  if(!this->vm->state.pgc) {
    printerr("No current PGC.");
    pthread_mutex_unlock(&this->vm_lock);
    return DVDNAV_STATUS_ERR;
  }

  this->cur_cell_time = 0;
  /* make a copy of current VM and try to navigate the copy to the menu */
  try_vm = vm_new_copy(this->vm);
  if ( (menu == DVD_MENU_Escape) && (this->vm->state.domain != VTS_DOMAIN)) {
    /* Try resume */
    if (vm_jump_resume(try_vm) && !try_vm->stopped) {
        /* merge changes on success */
        vm_merge(this->vm, try_vm);
        vm_free_copy(try_vm);
        this->position_current.still = 0;
        this->vm->hop_channel++;
        pthread_mutex_unlock(&this->vm_lock);
        return DVDNAV_STATUS_OK;
    }
  }
  if (menu == DVD_MENU_Escape) menu = DVD_MENU_Root;

  if (vm_jump_menu(try_vm, menu) && !try_vm->stopped) {
    /* merge changes on success */
    vm_merge(this->vm, try_vm);
    vm_free_copy(try_vm);
    this->position_current.still = 0;
    this->vm->hop_channel++;
    pthread_mutex_unlock(&this->vm_lock);
    return DVDNAV_STATUS_OK;
  } else {
    vm_free_copy(try_vm);
    printerr("No such menu or menu not reachable.");
    pthread_mutex_unlock(&this->vm_lock);
    return DVDNAV_STATUS_ERR;
  }
}

dvdnav_status_t dvdnav_get_position(dvdnav_t *this, uint32_t *pos,
				    uint32_t *len) {
  uint32_t cur_sector;
  int32_t cell_nr, first_cell_nr, last_cell_nr;
  cell_playback_t *cell;
  dvd_state_t *state;

  if(!this->started) {
    printerr("Virtual DVD machine not started.");
    return DVDNAV_STATUS_ERR;
  }

  pthread_mutex_lock(&this->vm_lock);
  state = &(this->vm->state);
  if(!state->pgc || this->vm->stopped) {
    printerr("No current PGC.");
    pthread_mutex_unlock(&this->vm_lock);
    return DVDNAV_STATUS_ERR;
  }
  if (this->position_current.hop_channel  != this->vm->hop_channel ||
      this->position_current.domain       != state->domain         ||
      this->position_current.vts          != state->vtsN           ||
      this->position_current.cell_restart != state->cell_restart) {
    printerr("New position not yet determined.");
    pthread_mutex_unlock(&this->vm_lock);
    return DVDNAV_STATUS_ERR;
  }

  /* Get current sector */
  cur_sector = this->vobu.vobu_start + this->vobu.blockN;

  if (this->pgc_based) {
    first_cell_nr = 1;
    last_cell_nr = state->pgc->nr_of_cells;
  } else {
    /* Find start cell of program. */
    first_cell_nr = state->pgc->program_map[state->pgN-1];
    /* Find end cell of program */
    if(state->pgN < state->pgc->nr_of_programs)
      last_cell_nr = state->pgc->program_map[state->pgN] - 1;
    else
      last_cell_nr = state->pgc->nr_of_cells;
  }

  *pos = -1;
  *len = 0;
  for (cell_nr = first_cell_nr; cell_nr <= last_cell_nr; cell_nr++) {
    cell = &(state->pgc->cell_playback[cell_nr-1]);
    if (cell_nr == state->cellN) {
      /* the current sector is in this cell,
       * pos is length of PG up to here + sector's offset in this cell */
      *pos = *len + cur_sector - cell->first_sector;
    }
    *len += cell->last_sector - cell->first_sector + 1;
  }

  assert((signed)*pos != -1);

  pthread_mutex_unlock(&this->vm_lock);

  return DVDNAV_STATUS_OK;
}

dvdnav_status_t dvdnav_get_position_in_title(dvdnav_t *this,
					     uint32_t *pos,
					     uint32_t *len) {
  uint32_t cur_sector;
  uint32_t first_cell_nr;
  uint32_t last_cell_nr;
  cell_playback_t *first_cell;
  cell_playback_t *last_cell;
  dvd_state_t *state;

  state = &(this->vm->state);
  if(!state->pgc) {
    printerr("No current PGC.");
    return DVDNAV_STATUS_ERR;
  }

  /* Get current sector */
  cur_sector = this->vobu.vobu_start + this->vobu.blockN;

  /* Now find first and last cells in title. */
  first_cell_nr = state->pgc->program_map[0];
  first_cell = &(state->pgc->cell_playback[first_cell_nr-1]);
  last_cell_nr = state->pgc->nr_of_cells;
  last_cell = &(state->pgc->cell_playback[last_cell_nr-1]);

  *pos = cur_sector - first_cell->first_sector;
  *len = last_cell->last_sector - first_cell->first_sector;

  return DVDNAV_STATUS_OK;
}

uint32_t dvdnav_describe_title_chapters(dvdnav_t *this, int32_t title, uint64_t **times, uint64_t *duration) {
  int32_t retval=0;
  uint16_t parts, i;
  title_info_t *ptitle = NULL;
  ptt_info_t *ptt = NULL;
  ifo_handle_t *ifo;
  pgc_t *pgc;
  cell_playback_t *cell;
  uint64_t length, *tmp=NULL;

  *times = NULL;
  *duration = 0;
  pthread_mutex_lock(&this->vm_lock);
  if(!this->vm->vmgi) {
    printerr("Bad VM state or missing VTSI.");
    goto fail;
  }
  if(!this->started) {
    /* don't report an error but be nice */
    vm_start(this->vm);
    this->started = 1;
  }
  ifo = vm_get_title_ifo(this->vm, title);
  if(!ifo || !ifo->vts_pgcit) {
    printerr("Couldn't open IFO for chosen title, exit.");
    goto fail;
  }

  ptitle = &this->vm->vmgi->tt_srpt->title[title-1];
  parts = ptitle->nr_of_ptts;
  ptt = ifo->vts_ptt_srpt->title[ptitle->vts_ttn-1].ptt;

  tmp = calloc(1, sizeof(uint64_t)*parts);
  if(!tmp)
    goto fail;

  length = 0;
  for(i=0; i<parts; i++) {
    uint32_t cellnr, endcellnr;
    pgc = ifo->vts_pgcit->pgci_srp[ptt[i].pgcn-1].pgc;
    if(ptt[i].pgn > pgc->nr_of_programs) {
      printerr("WRONG part number.");
      goto fail;
    }

    cellnr = pgc->program_map[ptt[i].pgn-1];
    if(ptt[i].pgn < pgc->nr_of_programs)
      endcellnr = pgc->program_map[ptt[i].pgn];
    else
      endcellnr = 0;

    do {
      cell = &pgc->cell_playback[cellnr-1];
      if(!(cell->block_type == BLOCK_TYPE_ANGLE_BLOCK &&
           cell->block_mode != BLOCK_MODE_FIRST_CELL
      ))
      {
        tmp[i] = length + dvdnav_convert_time(&cell->playback_time);
        length = tmp[i];
      }
      cellnr++;
    } while(cellnr < endcellnr);
  }
  *duration = length;
  vm_ifo_close(ifo);
  retval = parts;
  *times = tmp;

fail:
  pthread_mutex_unlock(&this->vm_lock);
  if(!retval && tmp)
    free(tmp);
  return retval;
}

/*
Check if pointer is null, and log it if it is
*/
int32_t is_null(void *val, const char *val_name) {
  if (val == NULL) {
    fprintf(MSG_OUT, "ERR:NULL REF: %s\n", val_name);
    return RESULT_TRUE;
  }
  else {
    return RESULT_FALSE;
  }
}
/*
Get an admap and admap_len
*/
vobu_admap_t* dvdnav_admap_get(dvdnav_t *this, dvd_state_t *state
   , int32_t *out_admap_len) {
  vobu_admap_t *admap = NULL;
  domain_t domain = state->domain;
  switch(domain) {
  case FP_DOMAIN:
  case VMGM_DOMAIN:
    admap = this->vm->vmgi->menu_vobu_admap;
    break;
  case VTSM_DOMAIN:
    admap = this->vm->vtsi->menu_vobu_admap;
    break;
  case VTS_DOMAIN:
    admap = this->vm->vtsi->vts_vobu_admap;
    break;
  default: {
    fprintf(MSG_OUT, "ERR:Unknown domain\n");
    return NULL;
  }
  }
  if (is_null(admap, "admap")) return NULL;
 
  *out_admap_len = (admap->last_byte + 1 - VOBU_ADMAP_SIZE) / VOBU_ADMAP_SIZE;
  if (*out_admap_len <= 0) {
    fprintf(MSG_OUT, "ERR:admap_len <= 0\n");
    return NULL;
  }
  return admap;
}
/*
Get a tmap, tmap_len and tmap_interval
*/
vts_tmap_t* dvdnav_tmap_get(dvdnav_t *this, dvd_state_t *state
  , int32_t *out_tmap_len, int32_t *out_tmap_interval) {
  int32_t vts_idx = state->vtsN;

  domain_t domain = state->domain;
  ifo_handle_t *ifo = NULL;
  switch(domain) {
  case FP_DOMAIN:
  case VTSM_DOMAIN:
  case VMGM_DOMAIN: {
    // NOTE: ifo = this->vm->vmgi seems to be always null
    ifo = this->vm->vmgi;
    break;
  }
  case VTS_DOMAIN: {
    // NOTE: ifo = this->vm->vtsi doesn't work
    ifo = ifoOpen(this->vm->dvd, vts_idx);
    break;
  }
  default: {
    fprintf(MSG_OUT, "ERR:unknown domain for tmap\n");
    return NULL;
  }
  }
  if (is_null(ifo, "ifo")) return NULL;

  vts_tmapt_t *tmapt = ifo->vts_tmapt;
  if (is_null(tmapt, "tmapt")) return NULL;

  uint16_t tmap_count = tmapt->nr_of_tmaps;
 
  // get pgcN; -1 b/c pgcN is base1
  int32_t pgcN = state->pgcN - 1;
  if (pgcN < 0) {
    fprintf(MSG_OUT, "ERR:pgcN < 0\n");
    return NULL;
  }
 
  // get tmap
  vts_tmap_t *tmap = NULL;
  switch(domain) {
  case FP_DOMAIN:
  case VMGM_DOMAIN:
  case VTSM_DOMAIN: {
    if (tmap_count == 0) {
      fprintf(MSG_OUT, "ERR:tmap_count == 0\n");
      return NULL;
    }
    tmap = &tmapt->tmap[0]; // ASSUME: vmgi only has one time map
    break;
  }
  case VTS_DOMAIN: {
    if (pgcN >= tmap_count) {
      fprintf(MSG_OUT, "ERR:pgcN >= tmap_count; pgcN=%i tmap_count=%i\n"
         , pgcN, tmap_count);
      return NULL;
    }
    tmap = &tmapt->tmap[pgcN];
    break;
  }
  }
  if (is_null(tmap, "tmap")) return NULL;

  // get tmap_interval; tmap->tmu is in seconds; convert to millisecs
  *out_tmap_interval = tmap->tmu * 1000;
  if (*out_tmap_interval == 0) {
    fprintf(MSG_OUT, "ERR:tmap_interval == 0\n");
    return NULL;
  }
  // get tmap_len
  *out_tmap_len = tmap->nr_of_entries;
  if (*out_tmap_len == 0) {
    fprintf(MSG_OUT, "ERR:tmap_len == 0\n");
    return NULL;
  }
  return tmap;
}
/*
Get a sector from a tmap
*/
int32_t dvdnav_tmap_get_entry(vts_tmap_t *tmap, uint16_t tmap_len
   , int32_t idx, uint32_t *out_sector) {
   // tmaps start at idx 0 which represents a sector at time 1 * tmap_interval
   // this creates a "fake" tmap index at idx -1 for sector 0
  if (idx == TMAP_IDX_EDGE_BGN) {
    *out_sector = 0;
    return RESULT_TRUE;
  }
  if (idx < TMAP_IDX_EDGE_BGN || idx >= tmap_len) {
    fprintf(MSG_OUT, "ERR:idx out of bounds idx=%i %i\n", idx, tmap_len);
    return RESULT_FALSE;
  }
  // 0x7fffffff unsets discontinuity bit if present
  *out_sector = tmap->map_ent[idx] & 0x7fffffff;
  return RESULT_TRUE;
}
/*
Do a binary search for earlier admap index near find_sector
*/
int32_t dvdnav_admap_search(vobu_admap_t *admap, uint32_t admap_len
  , uint32_t find_sector, uint32_t *out_vobu) {       
  int32_t adj = 1, prv_pos = 0, prv_len = admap_len;
  while (1) {
    int32_t cur_len = prv_len / 2;
    // need to add 1 when prv_len == 3 (cur_len shoud go to 2, not 1)
    if (prv_len % 2 == 1) ++cur_len;
    int32_t cur_idx = prv_pos + (cur_len * adj);
    if      (cur_idx < 0)             cur_idx = 0;
    else if   (cur_idx >= admap_len)  cur_idx = admap_len - 1;

    uint32_t cur_sector = admap->vobu_start_sectors[cur_idx];
    if        (find_sector <  cur_sector) adj = -1;
    else if (find_sector >  cur_sector) adj =  1;
    else if (find_sector == cur_sector) {
      *out_vobu = cur_idx;
      return RESULT_TRUE;
    }         
    if (cur_len == 1) {// no smaller intervals left
      if (adj == -1) {// last comparison was greater; take lesser
          cur_idx -= 1;
          cur_sector = admap->vobu_start_sectors[cur_idx];
      }
      *out_vobu = cur_idx;
      return RESULT_TRUE;
    }
    prv_len = cur_len;
    prv_pos = cur_idx;
  }
}
/*
Do a binary search for the earlier tmap entry near find_sector
*/
int32_t dvdnav_tmap_search(vts_tmap_t *tmap, uint32_t tmap_len
   , uint32_t find_sector, int32_t *out_tmap, uint32_t *out_sector) {
  int32_t adj = 1; int32_t prv_pos = 0; int32_t prv_len = tmap_len;
  int32_t result = RESULT_FALSE;
  while (1) {
    int32_t cur_len = prv_len / 2;
    // need to add 1 when prv_len == 3 (cur_len shoud go to 2, not 1)
    if (prv_len % 2 == 1) ++cur_len;
    int32_t cur_idx = prv_pos + (cur_len * adj);
    if      (cur_idx < 0)             cur_idx = 0;
    else if (cur_idx >= tmap_len) cur_idx = tmap_len - 1;
    uint32_t cur_sector = 0;
    result = dvdnav_tmap_get_entry(tmap, tmap_len, cur_idx, &cur_sector);
    if (!result) return RESULT_FALSE;
    if        (find_sector <  cur_sector) adj = -1;
    else if (find_sector >  cur_sector) adj =  1;
    else if (find_sector == cur_sector) {
      *out_tmap = cur_idx;
      *out_sector = cur_sector;
      return RESULT_TRUE;
    }         
    if (cur_len == 1) {// no smaller intervals left
      if (adj == -1) {// last comparison was greater; take lesser
        if (cur_idx == 0) { // fake tmap index for sector 0
          cur_idx = TMAP_IDX_EDGE_BGN;
          cur_sector = 0;
        }
        else {
          cur_idx -= 1;
          result = dvdnav_tmap_get_entry(tmap, tmap_len, cur_idx, &cur_sector);
          if (!result) return RESULT_FALSE;
        }
      }
      *out_tmap = cur_idx;
      *out_sector = cur_sector;
      return RESULT_TRUE;
    }
    prv_len = cur_len;
    prv_pos = cur_idx;
  }
}

/*
Given a sector/time/idx find the cell that encloses it
*/
int32_t dvdnav_cell_find(dvdnav_t *this, dvd_state_t *state
   , int32_t find_mode, uint64_t find_val
  , int32_t *out_cell_idx
  , uint64_t *out_bgn_time, uint32_t *out_bgn_sector
  , uint64_t *out_end_time, uint32_t *out_end_sector) {   
  pgc_t *pgc = state->pgc;
  if (is_null(pgc, "pgc")) return RESULT_FALSE;

  // get cells_len
  uint32_t cells_len = pgc->nr_of_cells;
  if (cells_len == 0) {
    fprintf(MSG_OUT, "ERR:cells_len == 0\n");
    return RESULT_FALSE;
  }

  // get cells_bgn, cells_end
  uint32_t cells_bgn, cells_end;
  if (this->pgc_based) {
    cells_bgn = 1;
    cells_end = cells_len;
  }
  else {
    int pgN = state->pgN;
    cells_bgn = pgc->program_map[pgN - 1]; // -1 b/c pgN is 1 based?   
    int program_count = pgc->nr_of_programs;
    if (pgN < program_count) {
      cells_end = pgc->program_map[pgN] - 1;
    }
    else {
      cells_end = cells_len;
    }
  }

  // search cells
  uint32_t cell_idx, sector_bgn = 0, sector_end = 0;
  uint64_t time_bgn = 0, time_end = 0;
  cell_playback_t *cell;
  int found = RESULT_FALSE;
  for (cell_idx = cells_bgn; cell_idx <= cells_end; cell_idx++) {
    cell =  &(pgc->cell_playback[cell_idx-1]);  // -1 b/c cell is base1
    // if angle block, only consider first angleBlock
    // (others are "redundant" for purpose of search)
    if ( cell->block_type == BLOCK_TYPE_ANGLE_BLOCK
       && cell->block_mode != BLOCK_MODE_FIRST_CELL) {
      continue;
    }
    sector_bgn = cell->first_sector;
    sector_end = cell->last_sector;
    time_end += (dvdnav_convert_time(&cell->playback_time) / 90);// 90 pts to ms
    if (find_mode == CELL_FIND_SECTOR) {
      if (find_val >= sector_bgn && find_val <= sector_end) {
        found = RESULT_TRUE;
        break;
      }   
    }
    else if (find_mode == CELL_FIND_TIME) {
      if (find_val >= time_bgn && find_val <= time_end) {
        found = RESULT_TRUE;
        break;
      }
    }
    else if (find_mode == CELL_FIND_INDEX) {
      if (find_val == cell_idx) {
        found = RESULT_TRUE;
        break;
      }
    }
    time_bgn = time_end;
  }
 
  // found cell: set *out vars
  if (found) {
    *out_cell_idx = cell_idx;
    *out_bgn_sector = sector_bgn;
    *out_end_sector = sector_end;
    *out_bgn_time = time_bgn;
    *out_end_time = time_end;
  }
  else
    fprintf(MSG_OUT, "ERR:cell not found\n");
  return found;
}
/*
Given two sectors and a fraction, calc the corresponding vobu
*/
int32_t dvdnav_admap_interpolate_vobu(dvdnav_t *this
   , vobu_admap_t *admap, int32_t admap_len
   , uint32_t sector_bgn, uint32_t sector_end, double fraction
   , uint32_t *out_jump_sector) {

  // get vobu_bgn
  int32_t result;
  uint32_t vobu_bgn;
  result = dvdnav_admap_search(admap, admap_len, sector_bgn, &vobu_bgn);
  if (!result) {
    fprintf(MSG_OUT, "ERR admap_interpolate: could not find sector_bgn");
    return RESULT_FALSE;
  }
 
  // get vobu_end
  uint32_t vobu_end;
  result = dvdnav_admap_search(admap, admap_len, sector_end, &vobu_end);
  if (!result) {
    fprintf(MSG_OUT, "ERR admap_interpolate: could not find sector_end");
    return RESULT_FALSE;
  }

  // get vobu_dif
  uint32_t vobu_dif = vobu_end - vobu_bgn;

  // get vobu_off; +.5 to round up else 74% of a 4 sec interval = 2 sec
  uint32_t vobu_off = (fraction * ((double)vobu_dif + .5));
  // HACK: need to add +1, or else will land too soon (not sure why)
  vobu_off++;
  // get sector
  if (vobu_off >= admap_len) {
    fprintf(MSG_OUT, "ERR admap_interpolate: vobu_off >= admap_len");
    return RESULT_FALSE;
  }
 
  int32_t vobu_idx = vobu_bgn + vobu_off;
  *out_jump_sector = admap->vobu_start_sectors[vobu_idx];
  return RESULT_TRUE;
}
/*
Given two tmap entries and a known time, calc the time for the lo tmap entry
*/
int32_t dvdnav_tmap_calc_time_for_tmap_entry(vobu_admap_t *admap
   , uint32_t admap_len, int32_t tmap_interval
   , uint32_t cell_sector, uint32_t lo_sector, uint32_t hi_sector
   , uint64_t cell_time, uint64_t *out_lo_time
   ) {
  if (lo_sector == hi_sector) {
    fprintf(MSG_OUT, "ERR:lo_sector == hi_sector: %i\n", lo_sector);
    return RESULT_FALSE;
  }
 
  // get vobus corresponding to lo, hi, cell
  int32_t result = RESULT_FALSE;
  uint32_t lo_vobu;
  result = dvdnav_admap_search(admap, admap_len, lo_sector, &lo_vobu);
  if (!result) {
    fprintf(MSG_OUT, "ERR lo_vobu: lo_sector=%i", lo_sector);
    return RESULT_FALSE;
  }
  uint32_t hi_vobu;
  result = dvdnav_admap_search(admap, admap_len, hi_sector, &hi_vobu);
  if (!result) {
    fprintf(MSG_OUT, "ERR hi_vobu: hi_sector=%i", hi_sector);
    return RESULT_FALSE;
  }
  uint32_t cell_vobu;
  result = dvdnav_admap_search(admap, admap_len, cell_sector, &cell_vobu);
  if (!result) {
    fprintf(MSG_OUT, "ERR cell_vobu: cell_sector=%i", cell_sector);
    return RESULT_FALSE;
  }
 
  // calc position of cell relative to lo
  double vobu_pct = (double)(cell_vobu - lo_vobu)
                  / (double)(hi_vobu   - lo_vobu);
  if (vobu_pct < 0 || vobu_pct > 1) {
    fprintf(MSG_OUT, "ERR vobu_pct must be between 0 and 1");
    return RESULT_FALSE;
  }

  // calc time of lo
  uint64_t time_adj = (uint64_t)(tmap_interval * vobu_pct);
  *out_lo_time = cell_time - time_adj;
  return RESULT_TRUE;
}
/*
Find the tmap entries on either side of a given sector
*/
int32_t dvdnav_tmap_get_entries_for_sector(dvdnav_t *this, dvd_state_t *state
  , vobu_admap_t *admap, int32_t admap_len, vts_tmap_t *tmap, uint32_t tmap_len
  , int32_t cell_idx, uint32_t cell_end_sector, uint32_t find_sector
  , int32_t *lo_tmap, uint32_t *lo_sector
  , int32_t *hi_tmap, uint32_t *hi_sector
  ) {
  int32_t result = RESULT_FALSE;
  result = dvdnav_tmap_search(tmap, tmap_len, find_sector, lo_tmap, lo_sector);
  if (!result) {
    fprintf(MSG_OUT, "ERR:could not find lo idx: %i\n", find_sector);
     return RESULT_FALSE;
  }

  uint32_t out_sector = 0;
  // lo is last tmap entry; "fake" entry for one beyond
  // and mark it with cell_end_sector
  if (*lo_tmap == tmap_len - 1) {
     *hi_tmap = TMAP_IDX_EDGE_END;
     *hi_sector = cell_end_sector;
  }
  else {
     *hi_tmap = *lo_tmap + 1;
     result = dvdnav_tmap_get_entry(tmap, tmap_len, *hi_tmap, &out_sector);
     if (!result) {
      fprintf(MSG_OUT, "ERR:could not find hi sector: %i\n", *hi_tmap);
        return RESULT_FALSE;
     }
    *hi_sector = out_sector;
  }
  // HACK: handle cells that start at discontinuity entries
  // first check if discontinuity applies
  int32_t discontinuity = RESULT_FALSE;
  if (*lo_tmap == TMAP_IDX_EDGE_BGN) {
    int32_t out_cell_idx;
    // get cell 1
    uint64_t cell_1_bgn_time; uint32_t cell_1_bgn_sector;
    uint64_t cell_1_end_time; uint32_t cell_1_end_sector;
    result = dvdnav_cell_find(this, state, CELL_FIND_INDEX
       , 1, &out_cell_idx, &cell_1_bgn_time, &cell_1_bgn_sector
       , &cell_1_end_time, &cell_1_end_sector
       );
    if (!result) {
      fprintf(MSG_OUT, "ERR:could not retrieve cell 1\n");
      return RESULT_FALSE;
    }
    // if cell 1 does not start at sector 0 then assume discontinuity
    // NOTE: most DVDs should start at 0
    if (cell_1_bgn_sector != 0) {
      discontinuity = RESULT_TRUE;
    }
  }
  else {
    if ((tmap->map_ent[*lo_tmap] & (1 << 31)) != 0) {
      discontinuity = RESULT_TRUE;
    }
  }
  if (discontinuity) {
    // HACK: since there is either
    // (a) a non-zero sector start at cell_1
    // (b) a discontinuity entry
    // the vobu at lo_tmap is suspect. In order to find a vobu, assume that
    // lo_tmap to hi_tmap is separated by the same number of vobus as
    // hi_tmap to hi_tmap + 1
    // This is a hack. It works in practice but there must be a better way....

    // first get the vobu for tmap_2
    uint32_t tmap_2_sector = 0, tmap_2_vobu;
    result = dvdnav_tmap_get_entry(tmap, tmap_len
      , *hi_tmap + 1, &tmap_2_sector);
    if (!result) {
      fprintf(MSG_OUT, "ERR:no tmap_2_sector: %i\n"
        , *hi_tmap + 1);
      return RESULT_FALSE;
    }
    result = dvdnav_admap_search(admap, admap_len
      , tmap_2_sector, &tmap_2_vobu);
    if (!result) {
      fprintf(MSG_OUT, "ERR:no tmap_2_vobu: %i\n", tmap_2_vobu);
      return RESULT_FALSE;
    }

    // now get the vobu for tmap_1
    uint32_t tmap_1_vobu;
    result = dvdnav_admap_search(admap, admap_len, *hi_sector, &tmap_1_vobu);
    if (!result) {
      fprintf(MSG_OUT, "ERR:no find tmap_1_vobu: %i\n", tmap_1_vobu);
      return RESULT_FALSE;
    }

    // now calc the vobu for lo_tmap
    uint32_t vobu_diff = tmap_2_vobu - tmap_1_vobu;
    uint32_t tmap_neg1_vobu = tmap_1_vobu - vobu_diff;
    if (tmap_neg1_vobu < 0 || tmap_neg1_vobu >= admap_len) {
      fprintf(MSG_OUT, "ERR:tmap_neg1_vobu: %i\n", tmap_neg1_vobu);
      return RESULT_FALSE;
    }
    *lo_sector = admap->vobu_start_sectors[tmap_neg1_vobu];
  }
  if (!(find_sector >= *lo_sector && find_sector <= *hi_sector)) {
      fprintf(MSG_OUT, "ERR:find_sector not between lo/hi\n");
        return RESULT_FALSE;
  }
  return RESULT_TRUE;
}

/*
Find the nearest vobu by using the tmap
*/
int32_t dvdnav_find_vobu_by_tmap(dvdnav_t *this, dvd_state_t *state
  , vobu_admap_t *admap, int32_t admap_len
  , int32_t cell_idx
  , uint64_t cell_bgn_time, uint32_t cell_bgn_sector
  , uint32_t cell_end_sector
   , uint64_t time_in_ms, uint32_t *out_jump_sector) {
  // get tmap, tmap_len, tmap_interval
  int32_t tmap_len, tmap_interval;
  vts_tmap_t *tmap = dvdnav_tmap_get(this, state
    , &tmap_len, &tmap_interval);
  if (is_null(tmap, "tmap")) return RESULT_FALSE;

  // get tmap entries on either side of cell_bgn
  int32_t cell_bgn_lo_tmap; uint32_t cell_bgn_lo_sector;
  int32_t cell_bgn_hi_tmap; uint32_t cell_bgn_hi_sector;
  int32_t result = RESULT_FALSE;
  result = dvdnav_tmap_get_entries_for_sector(this, state, admap, admap_len
     , tmap, tmap_len, cell_idx, cell_end_sector, cell_bgn_sector
    , &cell_bgn_lo_tmap, &cell_bgn_lo_sector
    , &cell_bgn_hi_tmap, &cell_bgn_hi_sector
    );
  if (!result) return RESULT_FALSE;

   // calc time of cell_bgn_lo
  uint64_t cell_bgn_lo_time;
  result = dvdnav_tmap_calc_time_for_tmap_entry(admap, admap_len, tmap_interval
    , cell_bgn_sector, cell_bgn_lo_sector, cell_bgn_hi_sector
    , cell_bgn_time, &cell_bgn_lo_time
     );
  if (!result) return RESULT_FALSE;
 
  // calc tmap_time of jump_time relative to cell_bgn_lo
  uint64_t seek_offset = time_in_ms - cell_bgn_lo_time;
  uint32_t seek_idx, jump_lo_sector, jump_hi_sector;
  seek_idx = (uint32_t)(seek_offset / tmap_interval);
  uint32_t seek_remainder = seek_offset - (seek_idx * tmap_interval);
  double seek_pct = (double)seek_remainder / (double)tmap_interval;

  // get tmap entries on either side of jump_time
  uint32_t jump_lo_idx = (uint32_t)(cell_bgn_lo_tmap + seek_idx);
  result = dvdnav_tmap_get_entry(tmap, tmap_len, jump_lo_idx, &jump_lo_sector);
  if (!result) return RESULT_FALSE;

  uint32_t jump_hi_idx = jump_lo_idx + 1; // +1 handled by get_entry
  result = dvdnav_tmap_get_entry(tmap, tmap_len, jump_hi_idx, &jump_hi_sector);
  if (!result) return RESULT_FALSE;

  // interpolate sector
  result = dvdnav_admap_interpolate_vobu(this, admap, admap_len
    , jump_lo_sector, jump_hi_sector, seek_pct, out_jump_sector);
 
  if (!result) return RESULT_FALSE;
  return RESULT_TRUE;
}
/*
Find the nearest vobu by using the cell boundaries
*/
int32_t dvdnav_find_vobu_by_cell_boundaries(dvdnav_t *this
    , vobu_admap_t *admap, int32_t admap_len
    , uint64_t cell_bgn_time, uint32_t cell_bgn_sector
    , uint64_t cell_end_time, uint32_t cell_end_sector
    , uint64_t time_in_ms
    , uint32_t *out_jump_sector
    ) {
  // get jump_offset
  uint64_t jump_offset = time_in_ms - cell_bgn_time;
  if (jump_offset < 0) {
    fprintf(MSG_OUT, "ERR:jump_offset < 0\n");
    return RESULT_FALSE;
  }
 
  // get cell_len
  uint64_t cell_len = cell_end_time - cell_bgn_time;
  if (cell_len < 0) {
    fprintf(MSG_OUT, "ERR:cell_len < 0\n");
    return RESULT_FALSE;
  }

  // get jump_pct
  double jump_pct = (double)jump_offset / cell_len;
 
  // get sector
  // NOTE: end cell sector in VTS_PGC is last sector of cell
  // this last sector is not the start of a VOBU
  // +1 to get sector that is the start of a VOBU
  cell_end_sector += 1;
  int32_t result = RESULT_FALSE;
  result = dvdnav_admap_interpolate_vobu(this, admap, admap_len
    , cell_bgn_sector, cell_end_sector, jump_pct, out_jump_sector);
  if (!result) {
    fprintf(MSG_OUT, "ERR:find_by_admap.interpolate\n");
    return RESULT_FALSE;
  }
  return RESULT_TRUE;
}
/*
Find the nearest vobu and jump to it
*/
int32_t dvdnav_jump_to_sector_by_time(dvdnav_t *this
   , uint32_t time_in_pts_ticks) {
  int32_t result = RESULT_FALSE;

  // convert time to milliseconds
  uint64_t time_in_ms = time_in_pts_ticks / 90;

  // get variables that will be used across both functions
  dvd_state_t *state = &(this->vm->state);
  if (is_null(state, "state")) goto exit;

  // get cell info
  int32_t cell_idx;
  uint64_t cell_bgn_time; uint32_t cell_bgn_sector;
  uint64_t cell_end_time; uint32_t cell_end_sector;
  result = dvdnav_cell_find(this, state, CELL_FIND_TIME, time_in_ms
    , &cell_idx
     , &cell_bgn_time, &cell_bgn_sector
     , &cell_end_time, &cell_end_sector);
  if (!result) goto exit;

  // get admap
  int32_t admap_len;
  vobu_admap_t *admap = dvdnav_admap_get(this, state, &admap_len);
  if (is_null(admap, "admap")) goto exit;

  // find sector
  uint32_t jump_sector;
  result = dvdnav_find_vobu_by_tmap(this, state, admap, admap_len
     , cell_idx, cell_bgn_time, cell_bgn_sector, cell_end_sector
    , time_in_ms, &jump_sector);
  if (!result) {// bad tmap; interpolate over cell
    result = dvdnav_find_vobu_by_cell_boundaries(this, admap, admap_len
       , cell_bgn_time, cell_bgn_sector
       , cell_end_time, cell_end_sector
       , time_in_ms, &jump_sector);
    if (!result) {
      goto exit;
    }
  }
 
  // may need to reget time when jump goes to diff cell (occurs near time 0)
  if ( jump_sector < cell_bgn_sector
     || jump_sector > cell_end_sector) {
    result = dvdnav_cell_find(this, state, CELL_FIND_SECTOR, jump_sector
      , &cell_idx
       , &cell_bgn_time, &cell_bgn_sector
       , &cell_end_time, &cell_end_sector);
    if (!result) {
      fprintf(MSG_OUT, "ERR:unable to find cell for %i\n", jump_sector);
      goto exit;
    }
  }

  // jump to sector
  uint32_t sector_off = jump_sector - cell_bgn_sector;
  this->cur_cell_time = 0;
  if (vm_jump_cell_block(this->vm, cell_idx, sector_off)) {
    pthread_mutex_lock(&this->vm_lock);
    this->vm->hop_channel += HOP_SEEK;
    pthread_mutex_unlock(&this->vm_lock);
    result = RESULT_TRUE;
  }

exit:
  return result;
}


dvdnav_status_t dvdnav_get_state(dvdnav_t *this, dvd_state_t *save_state)
{
  if(!this || !this->vm) return DVDNAV_STATUS_ERR;

  pthread_mutex_lock(&this->vm_lock);
  
  if( !vm_get_state(this->vm, save_state) )
  {
    printerr("Failed to get vm state.");
    pthread_mutex_unlock(&this->vm_lock);
    return DVDNAV_STATUS_ERR;
  }
  
  pthread_mutex_unlock(&this->vm_lock);
  return DVDNAV_STATUS_OK;
}

dvdnav_status_t dvdnav_set_state(dvdnav_t *this, dvd_state_t *save_state)
{
  if(!this || !this->vm)
  {
    printerr("Passed a NULL pointer.");
    return DVDNAV_STATUS_ERR;
  }

  if(!this->started) {
    printerr("Virtual DVD machine not started.");
    return DVDNAV_STATUS_ERR;
  }

  pthread_mutex_lock(&this->vm_lock);

  /* reset the dvdnav state */
  memset(&this->pci,0,sizeof(this->pci));
  memset(&this->dsi,0,sizeof(this->dsi));
  this->last_cmd_nav_lbn = SRI_END_OF_CELL;

  /* Set initial values of flags */  
  this->position_current.still = 0;
  this->skip_still = 0;
  this->sync_wait = 0;
  this->sync_wait_skip = 0;
  this->spu_clut_changed = 0;


  /* set the state. this will also start the vm on that state */
  /* means the next read block should be comming from that new */
  /* state */
  if( !vm_set_state(this->vm, save_state) )
  {
    printerr("Failed to set vm state.");
    pthread_mutex_unlock(&this->vm_lock);
    return DVDNAV_STATUS_ERR;
  } 

  pthread_mutex_unlock(&this->vm_lock);
  return DVDNAV_STATUS_OK;
}
