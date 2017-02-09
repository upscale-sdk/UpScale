/*
 * cluster.h
 *
 *  Created on: Nov 5, 2015
 *      Author: e.guidieri
 */

#ifndef CLUSTER_H_
#define CLUSTER_H_

/* OMP DATA STRUCTURES */

// command descriptor
struct _command {
  int kernel; // if >=0, it represents an index in kernel pointers table
  unsigned long in_data_counter;    // number of input parameters
  unsigned long inout_data_counter; // number of inout parameters
  unsigned long out_data_counter;   // number of output parameters
  unsigned long in_data_size;       // total byte size of input data
  unsigned long inout_data_size;    // total byte size of inout data
  unsigned long out_data_size;      // total byte size of output data
};

// kernel parameter descriptor
struct _omp_param {
  void *ptr;           // pointer to parameter data
  unsigned long size;  // parameter size (in bytes)
  char type;           // 0 = IN, 1 = OUT, 2 = INOUT
};

struct _omp_data_msg {
  char *memory_area;   // Area for libgomp global structures
  char *function_args; // Args for the offloaded function (from the host)
};

/* KERNEL POINTERS */
typedef void (*omp_fn)(struct _omp_param *);
extern omp_fn omp_functions[];

typedef struct EE_k1_omp_message_tag {
  struct _omp_param *params;
  omp_fn             func;
} __attribute__ ((aligned(_K1_DCACHE_LINE_SIZE))) EE_k1_omp_message;

#endif /* CLUSTER_H_ */
