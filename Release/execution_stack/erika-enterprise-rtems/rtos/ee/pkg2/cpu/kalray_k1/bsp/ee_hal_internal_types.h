/*
 * ee_hal_types.h
 *
 *  Created on: 06/dic/2014
 *      Author: AmministratoreErrico
 */

#ifndef EE_HAL_INTERNAL_TYPES_H_
#define EE_HAL_INTERNAL_TYPES_H_

#include "ee_platform_types.h"

/* Stack Entry Type (I use uint32 to match the type of the filler pattern,
   otherwise I would have used uint8) */
typedef EE_UREG EE_STACK_T;

typedef struct EE_stack_context_tag EE_stack_context;
typedef struct EE_SCB_tag EE_SCB;
typedef struct EE_SDB_tag EE_SDB;
typedef struct EE_TA_tag  EE_TA;
typedef struct EE_HDB_tag EE_HDB;

typedef struct EE_stack_context_tag {
  EE_UREG R8;
  EE_UREG R9;
  EE_UREG R10;
  EE_UREG R11;
  EE_UREG R12;
  EE_UREG R13;
  EE_UREG R14;
  EE_UREG R15;
  EE_UREG R16;
  EE_UREG R17;
  EE_UREG R18;
  EE_UREG R19;
  EE_UREG R20;
  EE_UREG R21;
  EE_UREG R22;
  EE_UREG R24;
  EE_UREG R25;
  EE_UREG R26;
  EE_UREG R27;
  EE_UREG R28;
  EE_UREG R29;
  EE_UREG R30;
  EE_UREG R31;
  EE_UREG RA;
  EE_UREG PS;
  struct EE_stack_context_tag * pTOS;
  EE_UREG dummy;
} EE_stack_context;

/* Stack Control Block: contine le informazioni dinamiche relative allo stack */
typedef struct EE_SCB_tag {
  EE_stack_context * tos;
} EE_SCB;

typedef struct EE_SDB_tag {
  EE_SCB *           p_SCB;
  EE_stack_context * bos; /* Base Of Stack */
  EE_UREG            stack_size;
} EE_SDB;

typedef struct EE_TA_tag {
  EE_BOOL         stacked;
  EE_isr2_prio    isr2_prio;
} EE_TA;

typedef struct EE_HDB_tag {
  EE_SDB *     p_SDB;
  EE_TA *      p_TA;
} EE_HDB;

typedef struct EE_HPB_tag {
  EE_ADDR              pool_base;
  EE_mem_size          residual_mem;
  EE_SDB *             p_sdb_array;
  EE_SCB *             p_scb_array;
  EE_array_size const  stack_descriptor_size;
  EE_array_index       free_stack_descriptor_index;
  EE_HDB *             p_hdb_array;
  EE_TA  *             p_ta_array;
  EE_array_size const  hal_descriptor_size;
  EE_array_index       free_hal_descriptor_index;
} EE_HPB;

#endif /* !EE_HAL_INTERNAL_TYPES_H_ */
