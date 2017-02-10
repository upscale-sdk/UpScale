/* Copyright (C) 2015, 2017 DEI - Universita' di Bologna
   Contributed by:
   Alessandro Capotondi <alessandro.capotondi@unibo.it>
   Daniele Cesarini <daniele.cesarini@unibo.it>
   Andrea Marongiu  <a.marongiu@unibo.it>
   Giuseppe Tagliavini <giuseppe.tagliavini@unibo.it>
*/

/* This file is part of the GNU OpenMP Library (libgomp).

   Libgomp is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   Libgomp is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
   FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
   more details.

   Under Section 7 of GPL version 3, you are granted additional
   permissions described in the GCC Runtime Library Exception, version
   3.1, as published by the Free Software Foundation.

   You should have received a copy of the GNU General Public License and
   a copy of the GCC Runtime Library Exception along with this program;
   see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
   <http://www.gnu.org/licenses/>.  */

/* This file contains support structures to the OFFLOAD construct.  */

#ifndef __SHARED_DEFS_H__
#define __SHARED_DEFS_H__

#define CLUSTER_COUNT 16
#define MAX_OFFLOADS_PER_CLUSTER 1

/* OMP DATA STRUCTURES */

// kernel parameter descriptor
typedef struct _omp_param {
  void *ptr;           // pointer to parameter data
  unsigned long size;  // parameter size (in bytes)
  char type;           // 0 = IN, 1 = OUT, 2 = INOUT
} omp_param_t;

// outlined data structures
struct _params1 {
  int n;
  struct _omp_param params[1];
};

struct _params2 {
  int n;
  struct _omp_param params[2];
};

struct _params3 {
  int n;
  struct _omp_param params[3];
};

struct _params4 {
  int n;
  struct _omp_param params[4];
};

struct _params5 {
  int n;
  struct _omp_param params[5];
};

// command descriptor
struct _command {
  int kernel; // if >=0, it represents an index in kernel pointers table
  unsigned int slot_id;             // Offload slot
  unsigned int mask;                // core mask
  unsigned long in_data_counter;    // number of input parameters
  unsigned long inout_data_counter; // number of inout parameters
  unsigned long out_data_counter;   // number of output parameters
  unsigned long in_data_size;       // total byte size of input data
  unsigned long inout_data_size;    // total byte size of inout data
  unsigned long out_data_size;      // total byte size of output data
};

// Execution context
struct _gomp_context {
  char io_sync[20];
  int io_sync_fd;
  char command_portal[20];
  int command_portal_fd;
  char input_data_portal[MAX_OFFLOADS_PER_CLUSTER][20];
  int input_data_portal_fd[MAX_OFFLOADS_PER_CLUSTER];
  char output_data_portal[MAX_OFFLOADS_PER_CLUSTER][20];
  int output_data_portal_fd[MAX_OFFLOADS_PER_CLUSTER];
};

struct _gomp_context ctx[CLUSTER_COUNT];

/* KERNEL POINTERS */
typedef void (*omp_fn)(struct _omp_param *);
extern omp_fn omp_functions[];

#endif /* __SHARED_DEFS_H__ */
