/*
 * ee_api_extension.h
 *
 *  Created on: 07/dic/2014
 *      Author: AmministratoreErrico
 */

#ifndef EE_API_EXTENSION_H_
#define EE_API_EXTENSION_H_

#include "eecfg.h"

#ifdef EE_API_EXTENSION

#define OS_SERVICE_ID_EXTENSION OS_SERVICE_ID_OSEK

#include "ee_compiler.h"
#include "ee_basic_data_structures.h"
#include "ee_api_types.h"

EE_INLINE__ void InitSem ( SemRefType pSem, CountType count )
{
  if ( pSem != NULL ) {
    pSem->blocked_queue = INVALID_INDEX;
    pSem->count = count;
  }
}

StatusType WaitSem(SemRefType Sem);
StatusType PostSem(SemRefType Sem);

#endif /* EE_API_EXTENSION */
#define OS_SERVICE_ID_EXTENSION OS_SERVICE_ID_OSEK
#endif /* !EE_API_EXTENSION_H_ */
