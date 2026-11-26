#include "RtosIncludes.h"

static SemaphoreHandle_t SemaphoreRegistry[RTOS_MAX_SEMAPHORES];
static u16 SemaphoreCount;

static s16 Semaphore_FindRegistered(SemaphoreHandle_t Semaphore) {
  u16 index;

  for (index = 0; index < SemaphoreCount; index++) {
    if (SemaphoreRegistry[index] == Semaphore) {
      return (s16)index;
    }
  }
  return -1;
}

static SemaphorePermission_t *Semaphore_FindPermission(
    SemaphoreHandle_t Semaphore, TCB_t *Task) {
  u16 index;

  for (index = 0; index < RTOS_MAX_TASKS; index++) {
    if (Semaphore->Permissions[index].Task == Task) {
      return &Semaphore->Permissions[index];
    }
  }
  return NULL;
}

u16 Semaphore_Init(SemaphoreHandle_t Semaphore) {
  u16 index;

  if (Semaphore == NULL) {
    return SEMAPHORE_ERROR_NULL_PARAM;
  }
  if (Semaphore_FindRegistered(Semaphore) >= 0) {
    return SEMAPHORE_ERROR_ALREADY_EXISTS;
  }
  if (SemaphoreCount >= RTOS_MAX_SEMAPHORES) {
    return SEMAPHORE_ERROR_FULL;
  }

  Semaphore->ReaderCount = 0u;
  Semaphore->Writer = NULL;
  for (index = 0; index < RTOS_MAX_TASKS; index++) {
    Semaphore->Permissions[index].Task = NULL;
    Semaphore->Permissions[index].Access = 0u;
    Semaphore->Permissions[index].IsReading = FALSE;
  }
  SemaphoreRegistry[SemaphoreCount++] = Semaphore;
  return SEMAPHORE_OK;
}

u16 Semaphore_Deinit(SemaphoreHandle_t Semaphore) {
  s16 registeredIndex;
  u16 index;

  if (Semaphore == NULL) {
    return SEMAPHORE_ERROR_NULL_PARAM;
  }
  registeredIndex = Semaphore_FindRegistered(Semaphore);
  if (registeredIndex < 0) {
    return SEMAPHORE_ERROR_NOT_FOUND;
  }
  if (Semaphore->ReaderCount != 0u || Semaphore->Writer != NULL) {
    return SEMAPHORE_ERROR_BUSY;
  }

  for (index = (u16)registeredIndex; index + 1u < SemaphoreCount; index++) {
    SemaphoreRegistry[index] = SemaphoreRegistry[index + 1u];
  }
  SemaphoreRegistry[--SemaphoreCount] = NULL;
  memset(Semaphore, 0, sizeof(*Semaphore));
  return SEMAPHORE_OK;
}

u16 Semaphore_Grant(SemaphoreHandle_t Semaphore, TCB_t *Task, u8 Access) {
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
  if (Task_IsRegistered(Task) != TASK_OK) {
    return SEMAPHORE_ERROR_NOT_FOUND;
  }

  Permission = Semaphore_FindPermission(Semaphore, Task);
  if (Permission != NULL) {
    Permission->Access = Access;
    return SEMAPHORE_OK;
  }
  for (index = 0; index < RTOS_MAX_TASKS; index++) {
    if (Semaphore->Permissions[index].Task == NULL) {
      Semaphore->Permissions[index].Task = Task;
      Semaphore->Permissions[index].Access = Access;
      return SEMAPHORE_OK;
    }
  }
  return SEMAPHORE_ERROR_FULL;
}

u16 Semaphore_Revoke(SemaphoreHandle_t Semaphore, TCB_t *Task) {
  SemaphorePermission_t *Permission;

  if (Semaphore == NULL || Task == NULL) {
    return SEMAPHORE_ERROR_NULL_PARAM;
  }
  if (Semaphore_FindRegistered(Semaphore) < 0) {
    return SEMAPHORE_ERROR_NOT_FOUND;
  }

  Permission = Semaphore_FindPermission(Semaphore, Task);
  if (Permission == NULL) {
    return SEMAPHORE_ERROR_NOT_FOUND;
  }
  Permission->Access = 0u;
  if (Permission->IsReading == FALSE) {
    Permission->Task = NULL;
  }
  return SEMAPHORE_OK;
}

u16 Semaphore_Acquire(SemaphoreHandle_t Semaphore, u8 Access) {
  TCB_t *CurrentTask;
  SemaphorePermission_t *Permission;

  if (Semaphore == NULL) {
    return SEMAPHORE_ERROR_NULL_PARAM;
  }
  if (Semaphore_FindRegistered(Semaphore) < 0) {
    return SEMAPHORE_ERROR_NOT_FOUND;
  }
  if (Access != SEMAPHORE_ACCESS_READ && Access != SEMAPHORE_ACCESS_WRITE) {
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

  if (Access == SEMAPHORE_ACCESS_READ) {
    if (Semaphore->Writer != NULL) {
      return SEMAPHORE_ERROR_BUSY;
    }
    if (Permission->IsReading != FALSE) {
      return SEMAPHORE_ERROR_INVALID_OPERATION;
    }
    Permission->IsReading = TRUE;
    Semaphore->ReaderCount++;
    return SEMAPHORE_OK;
  }

  if (Semaphore->Writer != NULL || Semaphore->ReaderCount != 0u) {
    return SEMAPHORE_ERROR_BUSY;
  }
  Semaphore->Writer = CurrentTask;
  return SEMAPHORE_OK;
}

u16 Semaphore_Release(SemaphoreHandle_t Semaphore) {
  TCB_t *CurrentTask;
  SemaphorePermission_t *Permission;

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
  if (Semaphore->Writer == CurrentTask) {
    Semaphore->Writer = NULL;
    return SEMAPHORE_OK;
  }

  Permission = Semaphore_FindPermission(Semaphore, CurrentTask);
  if (Permission == NULL || Permission->IsReading == FALSE) {
    return SEMAPHORE_ERROR_INVALID_OPERATION;
  }
  Permission->IsReading = FALSE;
  Semaphore->ReaderCount--;
  if (Permission->Access == 0u) {
    Permission->Task = NULL;
  }
  return SEMAPHORE_OK;
}

void Semaphore_OnTaskDelete(TCB_t *Task) {
  u16 semaphoreIndex;
  u16 permissionIndex;

  if (Task == NULL) {
    return;
  }
  for (semaphoreIndex = 0; semaphoreIndex < SemaphoreCount; semaphoreIndex++) {
    SemaphoreHandle_t Semaphore = SemaphoreRegistry[semaphoreIndex];

    if (Semaphore->Writer == Task) {
      Semaphore->Writer = NULL;
    }
    for (permissionIndex = 0; permissionIndex < RTOS_MAX_TASKS;
         permissionIndex++) {
      if (Semaphore->Permissions[permissionIndex].Task == Task) {
        Semaphore->Permissions[permissionIndex].Task = NULL;
        Semaphore->Permissions[permissionIndex].Access = 0u;
        if (Semaphore->Permissions[permissionIndex].IsReading != FALSE) {
          Semaphore->Permissions[permissionIndex].IsReading = FALSE;
          Semaphore->ReaderCount--;
        }
      }
    }
  }
}
