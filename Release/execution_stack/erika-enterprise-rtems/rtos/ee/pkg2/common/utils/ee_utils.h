#ifndef EE_UTILS_H_
#define EE_UTILS_H_

#include "ee_platform_types.h"

/******************************************************************************
                        Useful Generic Defines
 ******************************************************************************/

#ifndef EE_TRUE
#define EE_TRUE 1
#endif /* !EE_TRUE */

#ifndef EE_FALSE
#define EE_FALSE 0
#endif /* !EE_FALSE */

#define EE_KILO         1000U
#define EE_MEGA         1000000U
/* Single bit bitmask generator. */
#define EE_BIT(bit)     (1U << ((EE_UREG)(bit)))

#define EE_STRINGIFY(m) #m
/* One step indirection more to let a macro explode */
#define EE_S(m)         EE_STRINGIFY(m)

#define EE_STRING_JOIN(s1,s2) s1##s2
#define EE_S_J(s1,s2)         EE_STRING_JOIN(s1,s2)

/* Utility Macro that Count the number of element for an Array */
#define EE_ARRAY_ELEMENT_COUNT(ar) \
  ((sizeof(ar)/sizeof(0[ar])) / (!(sizeof(ar) % sizeof(0[ar]))))

/* Stack Alignment Macros */

/* Used to initialize stack arrays with the right size. */
#define EE_STACK_WORD_LEGHT(size) (((((EE_UREG)size) + EE_STACK_ALIGN_SIZE) - 1U) \
  / sizeof(EE_STACK_T))

/*******************************************************************************
                             Time to Ticks Utilities
 ******************************************************************************/
/** Utility Macro that convert an amount of ms in number of ticks of a given
    frequency **/
#define EE_MILLI_TO_TICKS(X_MS, REF_FREQ_HZ)  \
  ((X_MS) * ((REF_FREQ_HZ) / 1000UL))

/** Utility Macro that convert an amount of us in number of ticks of a given
    frequency **/
#define EE_MICRO_TO_TICKS(X_US, REF_FREQ_HZ)              \
  (((X_US) / 1000UL)?                                     \
      EE_MILLI_TO_TICKS(((X_US) / 1000UL), REF_FREQ_HZ):  \
      EE_MILLI_TO_TICKS(X_US, REF_FREQ_HZ) / 1000UL)

/** Utility Macro that convert an amount of us in number of ticks of a given
    frequency **/
#define MICROSECONDS_TO_TICKS(X_MICROSECS, REF_FREQ_HZ)   \
  EE_MICRO_TO_TICKS(X_MICROSECS, REF_FREQ_HZ)

/** Utility Macro that convert an amount of ms in number of ticks of a given
    frequency **/
#define MILLISECONDS_TO_TICKS(X_MILLISECS, REF_FREQ_HZ)   \
  EE_MILLI_TO_TICKS(X_MILLISECS, REF_FREQ_HZ)


#endif /* !EE_UTILS_H_ */
