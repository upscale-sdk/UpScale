#ifndef __SHARED_DEFS_H__
#define __SHARED_DEFS_H__

#define CLUSTER_COUNT 16
#define MAX_OFFLOADS_PER_CLUSTER 2

/* OMP DATA STRUCTURES */

// kernel parameter descriptor
struct _omp_param {
  void *ptr;           // pointer to parameter data
  unsigned long size;  // parameter size (in bytes)
  char type;           // 0 = IN, 1 = OUT, 2 = INOUT
};

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
