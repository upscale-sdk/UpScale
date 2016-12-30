/*
 * ee_api_osek.h
 *
 *  Created on: 07/dic/2014
 *      Author: AmministratoreErrico
 */

#ifndef EE_API_OSEK_H_
#define EE_API_OSEK_H_

#include "ee_api_types.h"

#define OS_SERVICE_ID_OSEK 0U

StatusType StartOS ( TaskFunc idleTaskFunc );
StatusType ActivateTask ( TaskType taskId );
StatusType TerminateTask ( void );

EE_INLINE__ CoreIdType GetCoreID ( void )
{
  return  EE_get_curr_core_id();
}

#endif /* EE_API_OSEK_H_ */
