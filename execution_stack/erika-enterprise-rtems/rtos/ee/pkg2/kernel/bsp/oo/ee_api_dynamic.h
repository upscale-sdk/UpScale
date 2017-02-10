/*
 * ee_api_dynamic.h
 *
 *  Created on: 07/dic/2014
 *      Author: AmministratoreErrico
 */

#ifndef EE_API_DYNAMIC_H_
#define EE_API_DYNAMIC_H_

#include "eecfg.h"
#ifdef EE_API_DYNAMIC
#include "ee_api_types.h"

#define OS_SERVICE_ID_DYNAMIC OS_SERVICE_ID_EXTENSION

StatusType CreateTask ( TaskTypeRef taskIdRef, TaskExecutionType taskType,
  TaskFunc taskFunc, TaskPrio readyPrio, TaskPrio dispatchPrio,
  TaskActivation maxNumOfAct, MemSize stackSize );

StatusType SetISR2Source ( TaskType taskId, IRS2SourceId ISR2Id );

#endif /* EE_API_DYNAMIC */
#define OS_SERVICE_ID_DYNAMIC OS_SERVICE_ID_EXTENSION
#endif /* EE_API_DYNAMIC_H_ */
