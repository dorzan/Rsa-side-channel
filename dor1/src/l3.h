/*
 * Copyright 2016 CSIRO
 *
 * This file is part of Mastik.
 *
 * Mastik is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Mastik is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Mastik.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __L3_H__
#define __L3_H__ 1

typedef void (*l3progressNotification_t)(int count, int est, void *data);
struct l3info {
  int associativity;
  int slices;
  int setsperslice;
  int bufsize;
  int flags;
  l3progressNotification_t progressNotification;
  void *progressNotificationData;
};

typedef struct l3pp *l3pp_t;
typedef struct l3info *l3info_t;


#define L3FLAG_NOHUGEPAGES	0x01
#define L3FLAG_USEPTE		0x02 
#define L3FLAG_NOPROBE		0x04 


l3pp_t l3_prepare(l3info_t l3info);
void l3_release(l3pp_t l3);

// Returns the number of probed sets in the LLC
int l3_getSets(l3pp_t l3);

// Returns the number slices
int l3_getSlices(l3pp_t l3);

// Returns the LLC associativity
int l3_getAssociativity(l3pp_t l3);

int l3_monitor(l3pp_t l3, int line);
void l3_unmonitorall(l3pp_t l3);
int l3_unmonitor(l3pp_t l3, int line);
int l3_getmonitoredset(l3pp_t l3, int *lines, int nlines);

void l3_randomise(l3pp_t l3);

void l3_probe(l3pp_t l3, uint16_t *results);
void l3_bprobe(l3pp_t l3, uint16_t *results);
void l3_probecount(l3pp_t l3, uint16_t *results);
void l3_bprobecount(l3pp_t l3, uint16_t *results);
void * l3_getline(l3pp_t l3, int set, int line);
void * l3_getbuffer(l3pp_t l3);
void * l3_swapslices(l3pp_t l3, int sliceA, int sliceB);

int l3_repeatedprobe(l3pp_t l3, int nrecords, uint16_t *results, int slot);
int l3_repeatedprobecount(l3pp_t l3, int nrecords, uint16_t *results, int slot);
int l3_repeatedprobecount_with_times(l3pp_t l3, int nrecords, uint16_t *results, uint64_t *, int slot);

void *sethead(l3pp_t l3, int set);
int probecount(void *pp);
int bprobecount(void *pp);


#endif // __L3_H__

