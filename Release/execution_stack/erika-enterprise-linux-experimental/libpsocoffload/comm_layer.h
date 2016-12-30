#ifndef __COMM_LAYER__
#define __COMM_LAYER__

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

void comm_init();

#ifdef linux
uint64_t * comm_buffer_alloc(size_t num_dwords);
#endif

#ifdef USE_LIBNOC
#include <mppa_power.h>
#define SPAWN(cluster, name, argv, envp) mppa_power_base_spawn(cluster, name, argv, envp, MPPA_POWER_SHUFFLING_ENABLED)
#else
#include <mppaipc.h>
#define SPAWN(cluster, name, argv, envp) mppa_spawn(cluster, NULL, name, argv, envp)
#endif

int comm_tx_open(int rx_id, int source_cluster, int target_cluster);
int comm_rx_wait(int rx_id);
void comm_tx_write(int tx_id, const void *buffer_data, size_t buffer_size, int offset);
void comm_tx_eot(int tx_id);
void comm_tx_write_eot(int tx_id, const void *buffer_data, size_t buffer_size, int offset);

int comm_rx_open(int rx_id, int source_cluster, int target_cluster);
int comm_rx_read(int rx_id,
                 uint64_t *buffer,
                 uint64_t buffer_size,
                 uint64_t buffer_offset);
int comm_rx_ready(int rx_id, size_t data_size);
int comm_rx_pending(int rx_id);
void comm_rx_rearm(int rx_id);

int comm_sync_rx_open(int rx_id, int source_cluster, int target_cluster);
int comm_sync_tx_open(int rx_id, int source_cluster, int target_cluster);
void comm_sync_tx_write(int tx_id);
int comm_sync_rx_wait(int rx_id);

#endif // __COMM_LAYER__
