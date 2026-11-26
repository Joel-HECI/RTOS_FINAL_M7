#include "RtosIncludes.h"

static SemaphoreHandler_t SemaphoreRegistry[RTOS_MAX_SEMAPHORES];
static u16 SemaphoreCount;

static s16 Semaphore_FindRegistered(SemaphoreHandler_t Semaphore) {
  u16 index;

  for (index = 0; index < SemaphoreCount; index++) {
    if (SemaphoreRegistry[index] == Semaphore) {
      return (s16)index;
    }
  }
  return -1;
}

static SemaphorePermission_t *Semaphore_FindPermission(
    SemaphoreHandler_t Semaphore, TCB_t *Task) {
  u16 index;

  for (index = 0; index < SEMAPHORE_MAX_OBJECTS; index++) {
    if (Semaphore->Permissions[index].Task == Task) {
      return &Semaphore->Permissions[index];
    }
  }
  return NULL;
}

u16 Semaphore_Init(SemaphoreHandler_t Semaphore, TCB_t *Administrator) {
  u16 index;

  if (Semaphore == NULL || Administrator == NULL) {
    return SEMAPHORE_ERROR_NULL_PARAM;
  }
  if (Semaphore_FindRegistered(Semaphore) >= 0) {
    return SEMAPHORE_ERROR_ALREADY_EXISTS;
  }
  if (SemaphoreCount >= RTOS_MAX_SEMAPHORES) {
    return SEMAPHORE_ERROR_FULL;
  }

  Semaphore->IsTaken = FALSE;
  Semaphore->Owner = NULL;
  Semaphore->Administrator = Administrator;
  for (index = 0; index < SEMAPHORE_MAX_OBJECTS; index++) {
    Semaphore->Permissions[index].Task = NULL;
    Semaphore->Permissions[index].Access = 0u;
  }
  Semaphore->Permissions[0].Task = Administrator;
  Semaphore->Permissions[0].Access =
      SEMAPHORE_ACCESS_READ | SEMAPHORE_ACCESS_WRITE;
  SemaphoreRegistry[SemaphoreCount++] = Semaphore;
  return SEMAPHORE_OK;
}

u16 Semaphore_Deinit(SemaphoreHandler_t Semaphore) {
  s16 registeredIndex;
  TCB_t *CurrentTask;
  u16 index;

  if (Semaphore == NULL) {
    return SEMAPHORE_ERROR_NULL_PARAM;
  }
  registeredIndex = Semaphore_FindRegistered(Semaphore);
  if (registeredIndex < 0) {
    return SEMAPHORE_ERROR_NOT_FOUND;
  }
  if (Semaphore->IsTaken != FALSE) {
    return SEMAPHORE_ERROR_BUSY;
  }

  CurrentTask = Scheduler_GetCurrentTask();
  if (CurrentTask != NULL && CurrentTask != Semaphore->Administrator) {
    return SEMAPHORE_ERROR_SECURITY_FAILURE;
  }

  for (index = (u16)registeredIndex; index + 1u < SemaphoreCount; index++) {
    SemaphoreRegistry[index] = SemaphoreRegistry[index + 1u];
  }
  SemaphoreRegistry[--SemaphoreCount] = NULL;
  memset(Semaphore, 0, sizeof(*Semaphore));
  return SEMAPHORE_OK;
}

u16 Semaphore_Grant(SemaphoreHandler_t Semaphore, TCB_t *Task, u8 Access) {
  TCB_t *CurrentTask;
  SemaphorePermission_t *Permission;
  u16 index;

  if (Semaphore == NULL || Task == NULL) {
    return SEMAPHORE_ERROR_NULL_PARAM;
  }
  if (Semaphore_FindRegistered(Semaphore) < 0) {
    return SEMAPHORE_ERROR_NOT_FOUND;
  }
  if (Access == 0u ||
      (Access & (u8)~(SEMAPHORE_ACCESS_READ | SEMAPHORE_ACCESS_WRITE)) != 0u) {
    return SEMAPHORE_ERROR_INVALID_PARAM;
  }

  CurrentTask = Scheduler_GetCurrentTask();
  if (CurrentTask == NULL || CurrentTask != Semaphore->Administrator) {
    return SEMAPHORE_ERROR_SECURITY_FAILURE;
  }

  Permission = Semaphore_FindPermission(Semaphore, Task);
  if (Permission != NULL) {
    Permission->Access = Access;
    return SEMAPHORE_OK;
  }
  for (index = 0; index < SEMAPHORE_MAX_OBJECTS; index++) {
    if (Semaphore->Permissions[index].Task == NULL) {
      Semaphore->Permissions[index].Task = Task;
      Semaphore->Permissions[index].Access = Access;
      return SEMAPHORE_OK;
    }
  }
  return SEMAPHORE_ERROR_FULL;
}

u16 Semaphore_Revoke(SemaphoreHandler_t Semaphore, TCB_t *Task) {
  TCB_t *CurrentTask;
  SemaphorePermission_t *Permission;

  if (Semaphore == NULL || Task == NULL) {
    return SEMAPHORE_ERROR_NULL_PARAM;
  }
  if (Semaphore_FindRegistered(Semaphore) < 0) {
    return SEMAPHORE_ERROR_NOT_FOUND;
  }

  CurrentTask = Scheduler_GetCurrentTask();
  if (CurrentTask == NULL || CurrentTask != Semaphore->Administrator) {
    return SEMAPHORE_ERROR_SECURITY_FAILURE;
  }

  Permission = Semaphore_FindPermission(Semaphore, Task);
  if (Permission == NULL) {
    return SEMAPHORE_ERROR_NOT_FOUND;
  }
  if (Semaphore->Owner == Task) {
    Semaphore->Owner = NULL;
    Semaphore->IsTaken = FALSE;
  }
  Permission->Task = NULL;
  Permission->Access = 0u;
  return SEMAPHORE_OK;
}

u16 Semaphore_Take(SemaphoreHandler_t Semaphore, u8 Access) {
  TCB_t *CurrentTask;
  SemaphorePermission_t *Permission;

  if (Semaphore == NULL) {
    return SEMAPHORE_ERROR_NULL_PARAM;
  }
  if (Semaphore_FindRegistered(Semaphore) < 0) {
    return SEMAPHORE_ERROR_NOT_FOUND;
  }
  if (Access == 0u ||
      (Access & (u8)~(SEMAPHORE_ACCESS_READ | SEMAPHORE_ACCESS_WRITE)) != 0u) {
    return SEMAPHORE_ERROR_INVALID_PARAM;
  }

  CurrentTask = Scheduler_GetCurrentTask();
  if (CurrentTask == NULL) {
    return SEMAPHORE_ERROR_INVALID_STATE;
  }
  Permission = Semaphore_FindPermission(Semaphore, CurrentTask);
  if (Permission == NULL || (Permission->Access & Access) != Access) {
    return SEMAPHORE_ERROR_SECURITY_FAILURE;
  }
  if (Semaphore->IsTaken != FALSE) {
    return SEMAPHORE_ERROR_BUSY;
  }

  Semaphore->Owner = CurrentTask;
  Semaphore->IsTaken = TRUE;
  return SEMAPHORE_OK;
}

u16 Semaphore_Release(SemaphoreHandler_t Semaphore) {
  TCB_t *CurrentTask;

  if (Semaphore == NULL) {
    return SEMAPHORE_ERROR_NULL_PARAM;
  }
  if (Semaphore_FindRegistered(Semaphore) < 0) {
    return SEMAPHORE_ERROR_NOT_FOUND;
  }
  CurrentTask = Scheduler_GetCurrentTask();
  if (CurrentTask == NULL) {
    return SEMAPHORE_ERROR_INVALID_STATE;
  }
  if (Semaphore->IsTaken == FALSE || Semaphore->Owner != CurrentTask) {
    return SEMAPHORE_ERROR_SECURITY_FAILURE;
  }

  Semaphore->Owner = NULL;
  Semaphore->IsTaken = FALSE;
  return SEMAPHORE_OK;
}

u16 Semaphore_OnTaskDelete(TCB_t *Task) {
  u16 semaphoreIndex;
  u16 permissionIndex;

  if (Task == NULL) {
    return SEMAPHORE_ERROR_NULL_PARAM;
  }
  for (semaphoreIndex = 0; semaphoreIndex < SemaphoreCount;
       semaphoreIndex++) {
    SemaphoreHandler_t Semaphore = SemaphoreRegistry[semaphoreIndex];

    if (Semaphore->Owner == Task) {
      Semaphore->Owner = NULL;
      Semaphore->IsTaken = FALSE;
    }
    if (Semaphore->Administrator == Task) {
      Semaphore->Administrator = NULL;
    }
    for (permissionIndex = 0; permissionIndex < SEMAPHORE_MAX_OBJECTS;
         permissionIndex++) {
      if (Semaphore->Permissions[permissionIndex].Task == Task) {
        Semaphore->Permissions[permissionIndex].Task = NULL;
        Semaphore->Permissions[permissionIndex].Access = 0u;
      }
    }
  }
  return SEMAPHORE_OK;
}