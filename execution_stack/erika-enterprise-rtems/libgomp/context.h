#ifndef __SWITCH_H__
#define __SWITCH_H__

unsigned int start_context(void *task, void *stack, void *ctx);
unsigned int swap_context(void *, void *);
unsigned int load_context(void *);

#endif // __SWITCH_H__