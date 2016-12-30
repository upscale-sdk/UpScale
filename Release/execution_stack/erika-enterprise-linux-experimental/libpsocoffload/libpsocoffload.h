#ifndef INCLUDE_LIBPSOCOFFLOAD_H
#define INCLUDE_LIBPSOCOFFLOAD_H

#include "shared_defs.h"

extern void GOMP_init(int device);
extern void GOMP_target(int device, void (*fn) (void *), const void *target_fn, void *data, unsigned int slot_id, unsigned int mask);
extern void GOMP_target_wait(int device, void *data, unsigned int slot_id);
void GOMP_deinit(int device);

#endif /* INCLUDE_LIBPSOCOFFLOAD_H */
