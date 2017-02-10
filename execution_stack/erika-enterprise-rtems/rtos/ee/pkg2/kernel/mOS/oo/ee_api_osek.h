/*
 * ee_api_osek.h
 *
 *  Created on: 07/dic/2014
 *      Author: AmministratoreErrico
 */

#ifndef EE_API_OSEK_H_
#define EE_API_OSEK_H_

#include "ee_api_types.h"

#define OSDEFAULTAPPMODE 0U

/* TODO: Add all others hooks declaration */

#if (defined(EE_HAS_SHUTDOWN_HOOK))
extern void ShutdownHook ( StatusType Error );
#endif /* EE_HAS_SHUTDOWN_HOOK */

StatusType StartOS ( AppModeType Mode );
StatusType ActivateTask ( TaskType taskId );
StatusType TerminateTask ( void );
StatusType GetTaskID ( TaskRefType TaskID );
StatusType ShutdownOS ( StatusType Error );

EE_INLINE__ CoreIdType GetCoreID ( void )
{
  return  EE_get_curr_core_id();
}

#endif /* EE_API_OSEK_H_ */
