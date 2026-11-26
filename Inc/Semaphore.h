#ifndef SEMAPHORE_H_
#define SEMAPHORE_H_

#include "Task.h"

#ifndef SEMAPHORE_BASE_ERROR
#define SEMAPHORE_BASE_ERROR 0x0500
#endif

#define SEMAPHORE_OK ERROR_OK
#define SEMAPHORE_ERROR_FAIL (SEMAPHORE_BASE_ERROR | ERROR_FAIL)
#define SEMAPHORE_ERROR_NULL_PARAM (SEMAPHORE_BASE_ERROR | ERROR_NULL_PARAM)
#define SEMAPHORE_ERROR_INVALID_PARAM (SEMAPHORE_BASE_ERROR | ERROR_INVALID_PARAM)
#define SEMAPHORE_ERROR_ALREADY_EXISTS \
  (SEMAPHORE_BASE_ERROR | ERROR_ALREADY_EXISTS)
#define SEMAPHORE_ERROR_BUSY (SEMAPHORE_BASE_ERROR | ERROR_BUSSY)
#define SEMAPHORE_ERROR_SECURITY_FAILURE \
  (SEMAPHORE_BASE_ERROR | ERROR_SECURITY_FAILURE)
#define SEMAPHORE_ERROR_INVALID_STATE \
  (SEMAPHORE_BASE_ERROR | ERROR_INVALID_STATE)
#define SEMAPHORE_ERROR_FULL (SEMAPHORE_BASE_ERROR | ERROR_FULL)
#define SEMAPHORE_ERROR_NOT_FOUND (SEMAPHORE_BASE_ERROR | ERROR_NOT_FOUND)
#define SEMAPHORE_ERROR_INVALID_OPERATION \
  (SEMAPHORE_BASE_ERROR | ERROR_INVALID_OPERATION)

#define SEMAPHORE_ACCESS_READ 0x01u
#define SEMAPHORE_ACCESS_WRITE 0x02u
#define SEMAPHORE_MAX_OBJECTS RTOS_MAX_TASKS

/* Exclusive, non-blocking access to a resource, gated by per-task permissions. */
typedef struct {
  TCB_t *Task;
  u8 Access;
} SemaphorePermission_t;

typedef struct {
  u8 IsTaken;
  TCB_t *Owner;
  TCB_t *Administrator;
  SemaphorePermission_t Permissions[RTOS_MAX_TASKS];
} Semaphore_t, *SemaphoreHandler_t;

u16 Semaphore_Init(SemaphoreHandler_t Semaphore, TCB_t *Administrator);
u16 Semaphore_Deinit(SemaphoreHandler_t Semaphore);
u16 Semaphore_Grant(SemaphoreHandler_t Semaphore, TCB_t *Task, u8 Access);
u16 Semaphore_Revoke(SemaphoreHandler_t Semaphore, TCB_t *Task);
u16 Semaphore_Take(SemaphoreHandler_t Semaphore, u8 Access);
u16 Semaphore_Release(SemaphoreHandler_t Semaphore);
u16 Semaphore_OnTaskDelete(TCB_t *Task);

#endif /* SEMAPHORE_H_ */