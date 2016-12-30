
#define USE_LIBNOC

#include "comm_layer.h"

#ifdef USE_LIBNOC

#include "HAL/hal/hal.h"
#include <mppa_bsp.h>
#include <mppa_noc.h>
#include <mppa_routing.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void comm_init() {
/* NOITHING TO DO IN THIS VERSION OF KALRAY SDK*/
}

void comm_rx_rearm(int rx_id) {
  mppa_noc_wait_clear_event(0, MPPA_NOC_INTERRUPT_LINE_DNOC_RX, rx_id);
  mppa_noc_dnoc_rx_lac_event_counter(0, rx_id);
  mppa_noc_dnoc_rx_lac_item_counter(0, rx_id);
}

#ifdef linux
uint64_t * comm_buffer_alloc(size_t num_dwords) {
  return (uint64_t *)mppa_noc_buffer_alloc(num_dwords * sizeof(uint64_t));
}
#endif


int comm_tx_open(int rx_id, int source_cluster, int target_cluster) {
  unsigned tx_id = 0;
  if(mppa_noc_dnoc_tx_alloc_auto(0, &tx_id, MPPA_NOC_BLOCKING) != MPPA_NOC_RET_SUCCESS) {
    printf("mppa_noc_dnoc_tx_alloc_auto failed\n");
    return -1;
  }
   
  mppa_dnoc_channel_config_t config = { 0 };
  mppa_dnoc_header_t header = { 0 };
  header._.tag = rx_id; header._.valid = 1; 

#ifdef __k1a__
  MPPA_NOC_DNOC_TX_CONFIG_INITIALIZER(config, mppa_noc_dnoc_get_window_length(0), 0);
#else
  MPPA_NOC_DNOC_TX_CONFIG_INITIALIZER_DEFAULT(config, 0);
#endif

  if (mppa_routing_get_dnoc_unicast_route(source_cluster, target_cluster, &config, &header) != 0) {
    printf("mppa_routing_get_dnoc_unicast_route failed\n");
    return -1;
  }

  int status = mppa_noc_dnoc_tx_configure(0, tx_id, header, config);

  return tx_id;
}


void comm_tx_write(int tx_id, const void *buffer_data, size_t buffer_size, int offset) {
  mppa_noc_dnoc_tx_send_data(0, tx_id, buffer_size, buffer_data);
}


void comm_tx_eot(int tx_id) {
  mppa_noc_dnoc_tx_flush_eot(0, tx_id);
}


void comm_tx_write_eot(int tx_id, const void *buffer_data, size_t buffer_size, int offset) {

  mppa_noc_dnoc_tx_flush(0, tx_id);
  __builtin_k1_fence();
  mppa_noc_dnoc_tx_send_data(0, tx_id, buffer_size, buffer_data);
  mppa_noc_dnoc_tx_flush_eot(0, tx_id);
}



int comm_rx_open(int rx_id, int source_cluster, int target_cluster) {

  if(mppa_noc_dnoc_rx_alloc(0, rx_id) != MPPA_NOC_RET_SUCCESS) {
    printf("Allocation of tag %d failed.\n", rx_id);
    return -1;
  }
  return rx_id;
}


int comm_rx_read(int rx_id,
                 uint64_t *buffer,
                 uint64_t buffer_size,
                 uint64_t buffer_offset) {

  mppa_noc_dnoc_rx_configuration_t rx_configuration = {
                .buffer_base = (uintptr_t) buffer,
                .buffer_size = buffer_size,
                .current_offset = buffer_offset,
                .item_reload = 0,
                .item_counter = 0,
                .event_counter = 0,
                .reload_mode = MPPA_NOC_RX_RELOAD_MODE_INCR_DATA_NOTIF,
                .activation = MPPA_NOC_ACTIVATED,
                .counter_id = 0
  };

  if(mppa_noc_dnoc_rx_configure(0, rx_id, rx_configuration) != 0) {
    printf("Configuration of tag %d failed.\n", rx_id);
    return -1;
  }

  mppa_noc_dnoc_rx_lac_event_counter(0, rx_id);

  return rx_id;
}

int comm_rx_wait(int rx_id) {
  int value = 0;
  while(value == 0) {
   value = mppa_noc_dnoc_rx_get_event_counter(0, rx_id);
  }
  mppa_noc_dnoc_rx_lac_event_counter(0, rx_id);
  mppa_noc_dnoc_rx_lac_item_counter(0, rx_id);
  return 0;
}


int comm_rx_ready(int rx_id, size_t data_size) {
  size_t val = mppa_noc_dnoc_rx_get_item_counter(0, rx_id);
  return val == data_size;
}

int comm_rx_pending(int rx_id) {
  return mppa_noc_dnoc_rx_get_item_counter(0, rx_id);
} 



///////////////////////////////////////////////////////////////////////////////
//// SYNCHRONIZATION
/////////////////////////////////////////////////////////////////////////////////


int comm_sync_rx_open(int rx_id, int source_cluster, int target_cluster) {

  if(mppa_noc_cnoc_rx_alloc(0, rx_id) != 0) {
    printf("Allocation failed.\n");
    return -1;
  }
  
#ifdef __k1b__
  mppa_cnoc_mailbox_notif_t notif;
  memset(&notif, 0, sizeof(mppa_cnoc_mailbox_notif_t));
  notif . _ . enable = 1;
  notif . _ . evt_en = 1;
  notif . _ . rm = 1<<__k1_get_cpu_id();
#endif
 
  mppa_noc_cnoc_rx_configuration_t config = { 0 };
  config.mode = MPPA_NOC_CNOC_RX_MAILBOX;
  config.init_value = 0;

#ifdef __k1a__
  if (mppa_noc_cnoc_rx_configure(0, rx_id, config) != 0) {
    printf("mppa_noc_cnoc_rx_configure failed.\n");
    return -1;
  }
#else
  if (mppa_noc_cnoc_rx_configure(0, rx_id, config, &notif) != 0) {
    printf("mppa_noc_cnoc_rx_configure failed.\n");
    return -1;
  }
#endif

  return rx_id;
}


int comm_sync_tx_open(int rx_id, int source_cluster, int target_cluster) {
  mppa_cnoc_config_t config = { 0 };
  mppa_cnoc_header_t header = { 0 };

  mppa_routing_get_cnoc_unicast_route(source_cluster, target_cluster, &config, &header);
  header._.tag = rx_id;

  unsigned tx_id = 0;
  mppa_noc_ret_t ret __attribute__((unused)) = mppa_noc_cnoc_tx_alloc_auto(0, &tx_id, MPPA_NOC_BLOCKING);
  assert(ret == MPPA_NOC_RET_SUCCESS);
  mppa_noc_cnoc_tx_configure(0, tx_id, config, header);

  return tx_id;
}


void comm_sync_tx_write(int tx_id) {
  uint64_t data = 1;
  mppa_noc_cnoc_tx_push_eot(0, tx_id, data);
}


int comm_sync_rx_wait(int rx_id) {

  mppa_noc_wait_clear_event(0, MPPA_NOC_INTERRUPT_LINE_CNOC_RX, rx_id);

#ifdef __k1b__
  mppa_cnoc_mailbox_notif_t notif;
  memset(&notif, 0, sizeof(mppa_cnoc_mailbox_notif_t));
  notif . _ . enable = 1;
  notif . _ . evt_en = 1;
  notif . _ . rm = 1<<__k1_get_cpu_id();
#endif

  mppa_noc_cnoc_rx_configuration_t config = { 0 };
  config.mode = MPPA_NOC_CNOC_RX_MAILBOX;
  config.init_value = 0;

#ifdef __k1a__
  if (mppa_noc_cnoc_rx_configure(0, rx_id, config) != 0) {
    printf("mppa_noc_cnoc_rx_configure failed.\n");
    return -1;
  }
#else
  if (mppa_noc_cnoc_rx_configure(0, rx_id, config, &notif) != 0) {
    printf("mppa_noc_cnoc_rx_configure failed.\n");
    return -1;
  }
#endif

  return 0;
}

#else // Alternative: LIBMPPAIPC

#include <mppa/osconfig.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void comm_init() { }

static long long match[256];
static int sync_fd[256];
static int portal_fd[256];

int comm_sync_rx_open(int rx_id, int source_cluster, int target_cluster) {
  char io_sync[128];
  match[rx_id] = -1LL ^ (1LL << source_cluster);

  sprintf(io_sync, "/mppa/sync/%d:%d", target_cluster, rx_id);
  sync_fd[rx_id] = mppa_open(io_sync, O_RDONLY);
  mppa_ioctl(sync_fd[rx_id], MPPA_RX_SET_MATCH, match[rx_id]);

  return rx_id;
}

int comm_sync_tx_open(int rx_id, int source_cluster, int target_cluster) {
  int fd;
  char io_sync[128];

  sprintf(io_sync, "/mppa/sync/%d:%d", target_cluster, rx_id);
  fd = mppa_open(io_sync, O_WRONLY);

  return fd;
}


int comm_sync_rx_wait(int rx_id) {
  long long dummy = 0;
  int status = mppa_read(sync_fd[rx_id], &dummy, sizeof(dummy));;
  mppa_ioctl(sync_fd[rx_id], MPPA_RX_SET_MATCH, match[rx_id]);
  return status;
}


void comm_sync_tx_write(int tx_id) {
  long long mask = (long long)1 << __k1_get_cluster_id();
  mppa_write(tx_id, &mask, sizeof(mask));
}



int comm_rx_open(int rx_id, int source_cluster, int target_cluster) {
  char portal[128];
  sprintf(portal, "/mppa/portal/%d:%d", target_cluster, rx_id);
  portal_fd[rx_id] = mppa_open(portal, O_RDONLY);
  return  rx_id;
}

int comm_tx_open(int rx_id, int source_cluster, int target_cluster) {
  char portal[128];
  sprintf(portal, "/mppa/portal/%d:%d", target_cluster, rx_id);
  return  mppa_open(portal, O_WRONLY);
}


void comm_tx_write(int tx_id, const void *buffer_data, size_t buffer_size, int offset) {
  mppa_ioctl(tx_id, MPPA_TX_NOTIFY_OFF);
  mppa_pwrite(tx_id, buffer_data, buffer_size, offset);
}


void comm_tx_eot(int tx_id) {
  mppa_ioctl(tx_id, MPPA_TX_NOTIFY_ON);
  mppa_pwrite(tx_id, 0, 0, 0);
}


void comm_tx_write_eot(int tx_id, const void *buffer_data, size_t buffer_size, int offset) {
  mppa_ioctl(tx_id, MPPA_TX_NOTIFY_ON);
  mppa_pwrite(tx_id, buffer_data, buffer_size, offset);
}

static mppa_aiocb_t portal_aiocb[128];

int comm_rx_read(int rx_id,
                 uint64_t *buffer,
                 uint64_t buffer_size,
                 uint64_t buffer_offset) {

  mppa_aiocb_ctor(&portal_aiocb[rx_id], portal_fd[rx_id], buffer, buffer_size);
  mppa_aiocb_set_pwrite(&portal_aiocb[rx_id], buffer, buffer_size, 0);
  mppa_aiocb_set_trigger(&portal_aiocb[rx_id], 1);
  mppa_aio_read(&portal_aiocb[rx_id]);

}

int comm_rx_wait(int rx_id) {
  mppa_aio_wait(&portal_aiocb[rx_id]);
  return 0;
}


int comm_rx_ready(int rx_id, size_t data_size) {
  return mppa_aio_error(&portal_aiocb[rx_id]) == 0;
}

int comm_rx_pending(int rx_id) {
  return mppa_aio_error(&portal_aiocb[rx_id]) == 0;
} 

void comm_rx_rearm(int rx_id) {
  mppa_aio_return(&portal_aiocb[rx_id]);
  mppa_aio_read(&portal_aiocb[rx_id]);
}


#endif // LIBNOC / !LIBNOC
