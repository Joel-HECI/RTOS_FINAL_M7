#ifndef _RTOS_INCLUDES_H_
#define _RTOS_INCLUDES_H_

/*******************************************************
 *      DEFINICIONES PUBLICAS DEL _RTOS_INCLUDES_H_    *
 *******************************************************/

#define RTOS_MIN_STACK_SIZE 128
#define RTOS_MAX_TASKS 8
#define RTOS_MAX_TASKS_PRIORITIES 16
#define RTOS_MIN_PRIORITY RTOS_MAX_TASKS_PRIORITIES - 1
#define RTOS_MAX_SEMAPHORES 8

/*******************************************************
 *              INCLUSION LIBRERIAS  h                 *
 *******************************************************/

#include "AppTypes.h"
#include "List.h"
#include "Queue.h"
#include "Scheduler.h"
#include "Task.h"
#include "Semaphore.h"

#include <stddef.h>
#include <string.h>


#endif //_RTOS_INCLUDES_H_