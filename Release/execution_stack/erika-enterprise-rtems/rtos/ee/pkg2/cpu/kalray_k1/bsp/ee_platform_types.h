#ifndef EE_PLATFORM_TYPES_H_
#define EE_PLATFORM_TYPES_H_

#include "eecfg.h"
#include <stdint.h>
#include <stddef.h>

#ifndef EE_API_DYNAMIC
#define EE_CONST const
#else
#define EE_CONST
#endif /* !EE_API_DYNAMIC */

/* Define HAL types */
typedef void *        EE_ADDR;
typedef char *        EE_PBYTE;
typedef const void *  EE_CONST_ADDR;
typedef uint32_t      EE_UREG;
typedef int32_t       EE_SREG;
typedef uint32_t      EE_FREG;

typedef EE_UREG       EE_CORE_ID;

typedef unsigned char EE_BIT;
typedef int           EE_BOOL; /* Il risultato di un espressione booleana: un intero. */

typedef void (*EE_task_func) ( void );
typedef void (*EE_kernel_callback) ( void );
typedef EE_UREG       EE_array_size;
typedef EE_UREG       EE_array_index;
typedef EE_UREG       EE_mem_size;
typedef EE_UREG       EE_isr2_source_id;
typedef uint8_t       EE_isr2_prio;
typedef uint8_t       EE_status_type;

typedef uint8_t       EE_service_id;

#ifndef EE_TID
#define EE_TID EE_UREG
#endif /* !EE_TID */

#ifndef EE_TASK_PRIO
#define EE_TASK_PRIO EE_UREG
#endif /* !EE_TASK_PRIO */

#ifndef EE_TASK_ACTIVATION_NUM
#define EE_TASK_ACTIVATION_NUM EE_UREG
#endif /* !EE_TASK_ACTIVATION_NUM */

typedef EE_TID                 EE_task_id;
typedef EE_TASK_PRIO           EE_task_prio;
typedef EE_TASK_ACTIVATION_NUM EE_task_nact;

#endif /* !EE_PLATFORM_TYPES_H_ */
