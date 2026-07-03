/***********************************************************************************************//**
 * \file cyabs_rtos_freertos.c
 *
 * \brief
 * Implementation for FreeRTOS abstraction
 *
 ***************************************************************************************************
 * \copyright
 * (c) 2019-2025, Infineon Technologies AG, or an affiliate of Infineon
 * Technologies AG.  SPDX-License-Identifier: Apache-2.0
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 **************************************************************************************************/

#include <cy_utils.h>
#include <cy_result.h>
#include <cyabs_rtos.h>
#include <FreeRTOS.h>
#include <task.h>
#include "cyabs_rtos_internal.h"

/* FreeRTOS pdTRUE/pdFALSE macros expand to boolean-essential literals but are assigned to/compared
   with BaseType_t (signed 32-bit), which is inherent to the FreeRTOS API design. */
CY_MISRA_FP_BLOCK_START("MISRA C-2012 Rule 10.3", 13,
                        "pdTRUE/pdFALSE are boolean-essential but used as BaseType_t per FreeRTOS API.");
CY_MISRA_FP_BLOCK_START("MISRA C-2012 Rule 10.4", 18,
                        "Comparison of BaseType_t with pdTRUE/pdFALSE (boolean) is required by FreeRTOS API.");

static const uint32_t  TASK_IDENT = 0xABCDEF01U;


#if (configUSE_TIMERS == 1)
typedef struct
{
    cy_timer_callback_t     cb;
    cy_timer_callback_arg_t arg;
} callback_data_t;


// Wrapper function to convert FreeRTOS callback signature to match expectation
// for our cyabs_rtos abstraction API.
static void timer_callback(TimerHandle_t arg)
{
    CY_MISRA_DEVIATE_BLOCK_START("MISRA C-2012 Rule 11.5", 1,
                                 "pvTimerGetTimerID returns void*; conversion to callback_data_t* is required to restore the callback context set during timer creation.");
    const callback_data_t* cb_arg = (const callback_data_t*)pvTimerGetTimerID(arg);
    CY_MISRA_BLOCK_END("MISRA C-2012 Rule 11.5");
    if (NULL != cb_arg->cb)
    {
        cb_arg->cb(cb_arg->arg);
    }
}


#endif /* configUSE_TIMERS */


//==================================================================================================
// Error Converter
//==================================================================================================

//--------------------------------------------------------------------------------------------------
// cy_rtos_last_error
//--------------------------------------------------------------------------------------------------
cy_rtos_error_t cy_rtos_last_error(void)
{
    return pdFALSE;
}


//==================================================================================================
// Threads
//==================================================================================================

typedef struct
{
    StaticTask_t      task;
    SemaphoreHandle_t sema;
    uint32_t          magic;
    void*             memptr;
} cy_task_wrapper_t;


//--------------------------------------------------------------------------------------------------
// cy_rtos_thread_create
//--------------------------------------------------------------------------------------------------
cy_rslt_t cy_rtos_thread_create(cy_thread_t* thread, cy_thread_entry_fn_t entry_function,
                                const char* name, void* stack, uint32_t stack_size,
                                cy_thread_priority_t priority, cy_thread_arg_t arg)
{
    cy_rslt_t status;

    CY_MISRA_DEVIATE_BLOCK_START("MISRA C-2012 Rule 11.6", 1,
                                 "Void-pointer-to-integer ((uint32_t)stack) cast validates stack alignment at system boundary.");
    if ((thread == NULL) || (stack_size < CY_RTOS_MIN_STACK_SIZE))
    {
        status = CY_RTOS_BAD_PARAM;
    }
    else if ((stack != NULL) && (0UL != (((uint32_t)stack) & CY_RTOS_ALIGNMENT_MASK)))
    {
        status = CY_RTOS_ALIGNMENT_ERROR;
    }
    else
    {
        // If the user provides a stack, we need to allocate memory for the StaticTask_t. If we
        // allocate memory we also need to clean it up. This is true when the task exits itself or
        // when it is killed. In the case it is killed is fairly straight forward. In the case
        // where it exits, we can't clean up any allocated memory since we can't free it before
        // calling vTaskDelete() and vTaskDelete() never returns. Thus we need to do it in join.
        // However, if the task exited itself it has also released any memory it allocated. Thus
        // in order to be able to reliably free memory as part of join, we need to know that the
        // data we are accessing (the StaticTask_t) has not been freed. We therefore need to always
        // allocate that object ourselves. This means we also need to allocate the stack if the
        // user did not provide one.
        uint32_t offset = (stack == NULL)
            ? (stack_size & ~CY_RTOS_ALIGNMENT_MASK)
            : 0U;
        uint32_t size  = offset + sizeof(cy_task_wrapper_t);
        CY_MISRA_DEVIATE_BLOCK_START("MISRA C-2012 Rule 11.5", 1,
                                     "pvPortMalloc returns void*; conversion to uint8_t* is required for byte-wise offset computation within one allocated block.");
        uint8_t* ident = (uint8_t*)pvPortMalloc(size);
        CY_MISRA_BLOCK_END("MISRA C-2012 Rule 11.5");

        if (ident == NULL)
        {
            status = CY_RTOS_NO_MEMORY;
        }
        else
        {
            StackType_t stack_size_rtos =
                ((stack_size & ~CY_RTOS_ALIGNMENT_MASK) / sizeof(StackType_t));
            CY_MISRA_FP_BLOCK_START("MISRA C-2012 Rule 11.3", 1,
                                    "Cast from uint8_t* to StackType_t* within single allocated block with guaranteed alignment.");
            CY_MISRA_DEVIATE_BLOCK_START("MISRA C-2012 Rule 11.5", 1,
                                         "Stack storage is represented as void* or uint8_t*; conversion to StackType_t* is required by xTaskCreateStatic API and alignment is validated.");
            StackType_t* stack_rtos = (stack == NULL)
                ? (StackType_t*)ident
                : (StackType_t*)stack;
            CY_MISRA_BLOCK_END("MISRA C-2012 Rule 11.5");
            CY_MISRA_BLOCK_END("MISRA C-2012 Rule 11.3");
            CY_MISRA_DEVIATE_BLOCK_START("MISRA C-2012 Rule 11.3", 1,
                                         "Cast from uint8_t* to cy_task_wrapper_t* within single allocated block. Offset guarantees alignment.");
            CY_MISRA_DEVIATE_BLOCK_START("MISRA C-2012 Rule 18.4", 1,
                                         "Pointer arithmetic computes wrapper address within a single allocated memory block.");
            cy_task_wrapper_t* wrapper = (cy_task_wrapper_t*)(ident + offset);
            CY_MISRA_BLOCK_END("MISRA C-2012 Rule 18.4");
            CY_MISRA_BLOCK_END("MISRA C-2012 Rule 11.3");
            wrapper->sema = xSemaphoreCreateBinary();
            CY_ASSERT(wrapper->sema != NULL);
            wrapper->magic  = TASK_IDENT;
            wrapper->memptr = ident;
            CY_MISRA_DEVIATE_BLOCK_START("MISRA C-2012 Rule 11.4", 1,
                                         "Pointer-to-integer cast required to verify alignment at runtime.");
            CY_ASSERT(((uint32_t)wrapper & CY_RTOS_ALIGNMENT_MASK) == 0UL);
            CY_MISRA_BLOCK_END("MISRA C-2012 Rule 11.4");
            CY_MISRA_FP_BLOCK_START("MISRA C-2012 Rule 11.2", 1,
                                    "xTaskCreateStatic returns TaskHandle_t (incomplete struct pointer) assigned to cy_thread_t.");
            *thread = xTaskCreateStatic((TaskFunction_t)entry_function, name, stack_size_rtos, arg,
                                        (UBaseType_t)priority, stack_rtos, &(wrapper->task));
            CY_MISRA_BLOCK_END("MISRA C-2012 Rule 11.2");
            CY_MISRA_DEVIATE_BLOCK_START("MISRA C-2012 Rule 11.2", 1,
                                         "TaskHandle_t is an opaque FreeRTOS pointer type; debug-only assert compares returned handle identity with wrapper task storage.");
            CY_ASSERT(((void*)*thread == (void*)&(wrapper->task)) || (*thread == NULL));
            CY_MISRA_BLOCK_END("MISRA C-2012 Rule 11.2");
            status = CY_RSLT_SUCCESS;
        }
    }
    CY_MISRA_BLOCK_END("MISRA C-2012 Rule 11.6");

    return status;
}


//--------------------------------------------------------------------------------------------------
// cy_rtos_thread_exit
//--------------------------------------------------------------------------------------------------
cy_rslt_t cy_rtos_thread_exit(void)
{
    TaskHandle_t handle = xTaskGetCurrentTaskHandle();
    // Ideally this would just call vTaskDelete(NULL); however FreeRTOS
    // does not provide any way to know when the task is actually cleaned
    // up. It will tell you that it has been deleted, but when delete is
    // called from the thread itself it doesn't actually get deleted at
    // that time. It just gets added to the list of items that will be
    // deleted when the idle task runs, but there is no way of knowing
    // that the idle task ran unless you add an application hook which is
    // not something that can be done here. This means that
    // cy_rtos_join_thread() has no way of knowing that it is actually
    // save to cleanup memory. So, instad of deleting here, we use a
    // semaphore to indicate that we can delete and then join waits on
    // the semaphore.

    // This cast is ok because the handle internally represents the TCB that we created in the
    // thread create function.
    CY_MISRA_DEVIATE_BLOCK_START("MISRA C-2012 Rule 11.2", 1,
                                 "TaskHandle_t is an opaque incomplete-type pointer; cast to wrapper is safe by design.");
    const cy_task_wrapper_t* wrapper = ((const cy_task_wrapper_t*)handle);
    CY_MISRA_BLOCK_END("MISRA C-2012 Rule 11.2");

    CY_ASSERT(wrapper->magic == TASK_IDENT);
    if (wrapper->magic == TASK_IDENT)
    {
        // This signals to the thread deleting the current thread that it it is safe to delete the
        // current thread.
        (void)xSemaphoreGive(wrapper->sema);
    }

    // This function is not expected to return and calling cy_rtos_join_thread will call vTaskDelete
    // on this thread and clean up.
    for (;;)
    {
        #if defined(INCLUDE_vTaskSuspend)
        vTaskSuspend(handle);
        #else
        vTaskDelay(10000);
        #endif
    }
}


//--------------------------------------------------------------------------------------------------
// cy_rtos_thread_terminate
//--------------------------------------------------------------------------------------------------
cy_rslt_t cy_rtos_thread_terminate(const cy_thread_t* thread)
{
    cy_rslt_t status;
    if (thread == NULL)
    {
        status = CY_RTOS_BAD_PARAM;
    }
    else
    {
        vTaskDelete(*thread);
        // Check to see if we allocated the task and if so free it up.
        CY_MISRA_DEVIATE_BLOCK_START("MISRA C-2012 Rule 11.2", 1,
                                     "TaskHandle_t is an opaque incomplete-type pointer; cast to wrapper is safe by design.");
        cy_task_wrapper_t* wrapper = ((cy_task_wrapper_t*)*thread);
        CY_MISRA_BLOCK_END("MISRA C-2012 Rule 11.2");
        vTaskSuspendAll();
        if (wrapper->magic == TASK_IDENT)
        {
            wrapper->magic = 0U;
            vSemaphoreDelete(wrapper->sema);
            vPortFree(wrapper->memptr);
        }
        (void)xTaskResumeAll();
        status = CY_RSLT_SUCCESS;
    }
    return status;
}


//--------------------------------------------------------------------------------------------------
// cy_rtos_thread_is_running
//--------------------------------------------------------------------------------------------------
cy_rslt_t cy_rtos_thread_is_running(const cy_thread_t* thread, bool* running)
{
    cy_rslt_t status;
    if ((thread == NULL) || (running == NULL))
    {
        status = CY_RTOS_BAD_PARAM;
    }
    else
    {
        eTaskState st = eTaskGetState(*thread);
        *running = (st == (eTaskState)eRunning);
        status   = CY_RSLT_SUCCESS;
    }
    return status;
}


//--------------------------------------------------------------------------------------------------
// cy_rtos_thread_get_state
//--------------------------------------------------------------------------------------------------
cy_rslt_t cy_rtos_thread_get_state(const cy_thread_t* thread, cy_thread_state_t* state)
{
    cy_rslt_t status;
    if ((thread == NULL) || (state == NULL))
    {
        status = CY_RTOS_BAD_PARAM;
    }
    else
    {
        eTaskState st = eTaskGetState(*thread);
        switch (st)
        {
            case eSuspended:
                *state = CY_THREAD_STATE_INACTIVE;
                break;

            case eReady:
                *state = CY_THREAD_STATE_READY;
                break;

            case eRunning:
                *state = CY_THREAD_STATE_RUNNING;
                break;

            case eBlocked:
                *state = CY_THREAD_STATE_BLOCKED;
                break;

            case eDeleted:
                *state = CY_THREAD_STATE_TERMINATED;
                break;

            case eInvalid:
            default:
                *state = CY_THREAD_STATE_UNKNOWN;
                break;
        }

        status = CY_RSLT_SUCCESS;
    }
    return status;
}


//--------------------------------------------------------------------------------------------------
// cy_rtos_thread_join
//--------------------------------------------------------------------------------------------------
cy_rslt_t cy_rtos_thread_join(cy_thread_t* thread)
{
    cy_rslt_t status = CY_RSLT_SUCCESS;
    if (thread == NULL)
    {
        status = CY_RTOS_BAD_PARAM;
    }
    else
    {
        CY_MISRA_DEVIATE_BLOCK_START("MISRA C-2012 Rule 11.2", 1,
                                     "TaskHandle_t is an opaque incomplete-type pointer; cast to wrapper is safe by design.");
        const cy_task_wrapper_t* wrapper = ((const cy_task_wrapper_t*)(*thread));
        CY_MISRA_BLOCK_END("MISRA C-2012 Rule 11.2");
        // This makes sure that the thread to be deleted has completed.  See cy_rtos_exit_thread()
        // for description of why this is done.
        if (wrapper->magic == TASK_IDENT)
        {
            (void)xSemaphoreTake(wrapper->sema, portMAX_DELAY);
            status = cy_rtos_terminate_thread(thread);
        }
        *thread = NULL;
    }
    return status;
}


//--------------------------------------------------------------------------------------------------
// cy_rtos_thread_get_handle
//--------------------------------------------------------------------------------------------------
cy_rslt_t cy_rtos_thread_get_handle(cy_thread_t* thread)
{
    cy_rslt_t status = CY_RSLT_SUCCESS;

    if (thread == NULL)
    {
        status = CY_RTOS_BAD_PARAM;
    }
    else
    {
        *thread = xTaskGetCurrentTaskHandle();
    }

    return status;
}


//--------------------------------------------------------------------------------------------------
// cy_rtos_thread_wait_notification
//--------------------------------------------------------------------------------------------------
#if (configUSE_TASK_NOTIFICATIONS != 0)
cy_rslt_t cy_rtos_thread_wait_notification(cy_time_t timeout_ms)
{
    uint32_t ret;

    ret = ulTaskNotifyTake(pdTRUE, (timeout_ms == CY_RTOS_NEVER_TIMEOUT) ?
                           portMAX_DELAY : convert_ms_to_ticks(timeout_ms));
    if (0U != ret)
    {
        /* Received notify from another thread or ISR */
        return CY_RSLT_SUCCESS;
    }
    else
    {
        return CY_RTOS_TIMEOUT;
    }
}


#endif /* configUSE_TASK_NOTIFICATIONS */


//--------------------------------------------------------------------------------------------------
// cy_rtos_thread_set_notification
//--------------------------------------------------------------------------------------------------
#if (configUSE_TASK_NOTIFICATIONS != 0)
cy_rslt_t cy_rtos_thread_set_notification(const cy_thread_t* thread)
{
    cy_rslt_t status;
    if (thread == NULL)
    {
        status = CY_RTOS_BAD_PARAM;
    }
    else
    {
        if (is_in_isr())
        {
            BaseType_t taskWoken = pdFALSE;
            /* No error checking as this function always returns pdPASS. */
            vTaskNotifyGiveFromISR(*thread, &taskWoken);
            portEND_SWITCHING_ISR(taskWoken);
        }
        else
        {
            /* No error checking as this function always returns pdPASS. */
            CY_MISRA_DEVIATE_BLOCK_START("MISRA_CAST", 1,
                                         "Signedness conversion inside FreeRTOS macro expansion. Cannot modify third-party macro.");
            (void)xTaskNotifyGive(*thread);
            CY_MISRA_BLOCK_END("MISRA_CAST");
        }
        status = CY_RSLT_SUCCESS;
    }
    return status;
}


#endif /* configUSE_TASK_NOTIFICATIONS */


//--------------------------------------------------------------------------------------------------
// cy_rtos_thread_get_name
//--------------------------------------------------------------------------------------------------
cy_rslt_t cy_rtos_thread_get_name(const cy_thread_t* thread, const char** thread_name)
{
    *thread_name = pcTaskGetName(*thread);
    return CY_RSLT_SUCCESS;
}


//==================================================================================================
// Scheduler
//==================================================================================================
static uint16_t cy_rtos_suspend_count = 0;
static uint16_t cy_rtos_suspend_count_from_ISR = 0;
static UBaseType_t uxSavedInterruptStatus[CY_RTOS_MAX_SUSPEND_NESTING];
//--------------------------------------------------------------------------------------------------
// cy_rtos_scheduler_suspend
//--------------------------------------------------------------------------------------------------
cy_rslt_t cy_rtos_scheduler_suspend(void)
{
    cy_rslt_t status = CY_RSLT_SUCCESS;
    if (is_in_isr())
    {
        if (cy_rtos_suspend_count_from_ISR < (CY_RTOS_MAX_SUSPEND_NESTING - 1U))
        {
            /* Suspend the Scheduler before interrupts are disabled */
            vTaskSuspendAll();
            uxSavedInterruptStatus[cy_rtos_suspend_count_from_ISR] = taskENTER_CRITICAL_FROM_ISR();
            ++cy_rtos_suspend_count_from_ISR;
        }
        else
        {
            status = CY_RTOS_BAD_PARAM;
        }
    }
    else
    {
        /* Suspend the Scheduler before interrupts are disabled */
        vTaskSuspendAll();
        taskENTER_CRITICAL();
    }

    if (status == CY_RSLT_SUCCESS)
    {
        ++cy_rtos_suspend_count;
    }

    return status;
}


//--------------------------------------------------------------------------------------------------
// cy_rtos_scheduler_resume
//--------------------------------------------------------------------------------------------------
cy_rslt_t cy_rtos_scheduler_resume(void)
{
    cy_rslt_t status;
    if (cy_rtos_suspend_count > 0U)
    {
        if (is_in_isr())
        {
            if (cy_rtos_suspend_count_from_ISR > 0U) //We have at least one suspend from ISR call
            {
                taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptStatus[cy_rtos_suspend_count_from_ISR -
                                                                  1U]);
                --cy_rtos_suspend_count_from_ISR;
                status = CY_RSLT_SUCCESS;
            }
            else
            {
                status = CY_RTOS_BAD_PARAM;
            }
        }
        else
        {
            taskEXIT_CRITICAL();
            status = CY_RSLT_SUCCESS;
        }
        --cy_rtos_suspend_count;
        /* Resume the Scheduler after interrupt are re-enabled */
        (void)xTaskResumeAll();
    }
    else
    {
        status = CY_RTOS_BAD_PARAM;
    }
    return status;
}


//--------------------------------------------------------------------------------------------------
// cy_rtos_scheduler_get_state
//--------------------------------------------------------------------------------------------------
cy_rslt_t cy_rtos_scheduler_get_state(cy_scheduler_state_t* state)
{
    cy_rslt_t status;
    if (state == NULL)
    {
        status = CY_RTOS_BAD_PARAM;
    }
    else
    {
        BaseType_t st = xTaskGetSchedulerState();
        switch (st)
        {
            case taskSCHEDULER_NOT_STARTED:
                *state = CY_SCHEDULER_STATE_NOT_STARTED;
                break;

            case taskSCHEDULER_SUSPENDED:
                *state = CY_SCHEDULER_STATE_SUSPENDED;
                break;

            case taskSCHEDULER_RUNNING:
                *state = CY_SCHEDULER_STATE_RUNNING;
                break;

            default:
                *state = CY_SCHEDULER_STATE_UNKNOWN;
                break;
        }

        status = CY_RSLT_SUCCESS;
    }
    return status;
}


//==================================================================================================
// Mutexes
//==================================================================================================

#if (configUSE_MUTEXES == 1)
//--------------------------------------------------------------------------------------------------
// cy_rtos_mutex_init
//--------------------------------------------------------------------------------------------------
cy_rslt_t cy_rtos_mutex_init(cy_mutex_t* mutex, bool recursive)
{
    cy_rslt_t status;
    if (mutex == NULL)
    {
        status = CY_RTOS_BAD_PARAM;
    }
    #if (configUSE_RECURSIVE_MUTEXES != 1)
    else if (recursive)
    {
        /* Recursive mutexes disabled in FreeRTOSConfig */
        status = CY_RTOS_BAD_PARAM;
    }
    #endif /* configUSE_RECURSIVE_MUTEXES */
    else
    {
        mutex->is_recursive = recursive;
        #if (configUSE_RECURSIVE_MUTEXES == 1)
        mutex->mutex_handle = recursive
            ? xSemaphoreCreateRecursiveMutex()
            : xSemaphoreCreateMutex();
        #else
        mutex->mutex_handle = xSemaphoreCreateMutex();
        #endif /* configUSE_RECURSIVE_MUTEXES */
        if (mutex->mutex_handle == NULL)
        {
            status = CY_RTOS_NO_MEMORY;
        }
        else
        {
            status = CY_RSLT_SUCCESS;
        }
    }
    return status;
}


#if defined(FREERTOS_COMMON_SECTION_BEGIN)
FREERTOS_COMMON_SECTION_BEGIN
#endif
//--------------------------------------------------------------------------------------------------
// cy_rtos_mutex_get
//--------------------------------------------------------------------------------------------------
cy_rslt_t cy_rtos_mutex_get(const cy_mutex_t* mutex, cy_time_t timeout_ms)
{
    cy_rslt_t status;
    if (mutex == NULL)
    {
        status = CY_RTOS_BAD_PARAM;
    }
    else
    {
        TickType_t ticks  = convert_ms_to_ticks(timeout_ms);
        BaseType_t result;
        #if (configUSE_RECURSIVE_MUTEXES == 1)
        result = (mutex->is_recursive)
                    ? xSemaphoreTakeRecursive(mutex->mutex_handle, ticks)
                    : xSemaphoreTake(mutex->mutex_handle, ticks);
        #else
        result = xSemaphoreTake(mutex->mutex_handle, ticks);
        #endif /* configUSE_RECURSIVE_MUTEXES */

        status = (result == pdFALSE)
                ? CY_RTOS_TIMEOUT
                : CY_RSLT_SUCCESS;
    }
    return status;
}


#if defined(FREERTOS_COMMON_SECTION_END)
FREERTOS_COMMON_SECTION_END
#endif


#if defined(FREERTOS_COMMON_SECTION_BEGIN)
FREERTOS_COMMON_SECTION_BEGIN
#endif
//--------------------------------------------------------------------------------------------------
// cy_rtos_mutex_set
//--------------------------------------------------------------------------------------------------
cy_rslt_t cy_rtos_mutex_set(const cy_mutex_t* mutex)
{
    cy_rslt_t status;
    if (mutex == NULL)
    {
        status = CY_RTOS_BAD_PARAM;
    }
    else
    {
        BaseType_t result;
        #if (configUSE_RECURSIVE_MUTEXES == 1)
        result = (mutex->is_recursive)
                    ? xSemaphoreGiveRecursive(mutex->mutex_handle)
                    : xSemaphoreGive(mutex->mutex_handle);
        #else
        result = xSemaphoreGive(mutex->mutex_handle);
        #endif /* configUSE_RECURSIVE_MUTEXES */

        status = (result == pdFALSE)
                ? CY_RTOS_GENERAL_ERROR
                : CY_RSLT_SUCCESS;
    }
    return status;
}


#if defined(FREERTOS_COMMON_SECTION_END)
FREERTOS_COMMON_SECTION_END
#endif

CY_MISRA_DEVIATE_BLOCK_START("MISRA C-2012 Rule 8.13", 1,
                             "Public API signature in cyabs_rtos.h is non-const for cross-kernel compatibility; some kernels modify the handle object in deinit paths.");
//--------------------------------------------------------------------------------------------------
// cy_rtos_mutex_deinit
//--------------------------------------------------------------------------------------------------
cy_rslt_t cy_rtos_mutex_deinit(cy_mutex_t* mutex)
{
    cy_rslt_t status;
    if (mutex == NULL)
    {
        status = CY_RTOS_BAD_PARAM;
    }
    else
    {
        vSemaphoreDelete(mutex->mutex_handle);
        status = CY_RSLT_SUCCESS;
    }
    return status;
}


CY_MISRA_BLOCK_END("MISRA C-2012 Rule 8.13");


#endif /* configUSE_MUTEXES */


//==================================================================================================
// Semaphores
//==================================================================================================

//--------------------------------------------------------------------------------------------------
// cy_rtos_semaphore_init
//--------------------------------------------------------------------------------------------------
#if (configUSE_COUNTING_SEMAPHORES == 1)
cy_rslt_t cy_rtos_semaphore_init(cy_semaphore_t* semaphore, uint32_t maxcount, uint32_t initcount)
{
    cy_rslt_t status;
    if (semaphore == NULL)
    {
        status = CY_RTOS_BAD_PARAM;
    }
    else
    {
        *semaphore = xSemaphoreCreateCounting(maxcount, initcount);
        if (*semaphore == NULL)
        {
            status = CY_RTOS_NO_MEMORY;
        }
        else
        {
            status = CY_RSLT_SUCCESS;
        }
    }
    return status;
}


#endif /* configUSE_COUNTING_SEMAPHORES */


//--------------------------------------------------------------------------------------------------
// cy_rtos_semaphore_get
//--------------------------------------------------------------------------------------------------
cy_rslt_t cy_rtos_semaphore_get(const cy_semaphore_t* semaphore, cy_time_t timeout_ms)
{
    cy_rslt_t status;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    // xSemaphoreTakeFromISR does not take timeout as a parameter
    // since it cannot block. Hence we return an error if the user
    // tries to set a timeout.
    if ((semaphore == NULL) || (is_in_isr() && (timeout_ms != 0U)))
    {
        status = CY_RTOS_BAD_PARAM;
    }
    else
    {
        TickType_t ticks = convert_ms_to_ticks(timeout_ms);
        status = CY_RSLT_SUCCESS;

        if (is_in_isr())
        {
            CY_MISRA_DEVIATE_BLOCK_START("MISRA C-2012 Rule 11.2", 1,
                                         "FreeRTOS ISR API uses opaque handle types; semaphore handle is passed through API as required by contract.");
            if (pdFALSE == xSemaphoreTakeFromISR(*semaphore, &xHigherPriorityTaskWoken))
            {
                status = CY_RTOS_TIMEOUT;
            }
            else
            {
                portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
            }
            CY_MISRA_BLOCK_END("MISRA C-2012 Rule 11.2");
        }
        else
        {
            if (pdFALSE == xSemaphoreTake(*semaphore, ticks))
            {
                status = CY_RTOS_TIMEOUT;
            }
        }
    }

    return status;
}


//--------------------------------------------------------------------------------------------------
// cy_rtos_semaphore_set
//--------------------------------------------------------------------------------------------------
cy_rslt_t cy_rtos_semaphore_set(const cy_semaphore_t* semaphore)
{
    cy_rslt_t status;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    if (semaphore == NULL)
    {
        status = CY_RTOS_BAD_PARAM;
    }
    else
    {
        BaseType_t ret;

        if (is_in_isr())
        {
            CY_MISRA_DEVIATE_BLOCK_START("MISRA C-2012 Rule 11.2", 1,
                                         "FreeRTOS ISR API uses opaque handle types; semaphore handle is passed through API as required by contract.");
            ret = xSemaphoreGiveFromISR(*semaphore, &xHigherPriorityTaskWoken);
            CY_MISRA_BLOCK_END("MISRA C-2012 Rule 11.2");

            if (ret == pdTRUE)
            {
                portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
            }
        }
        else
        {
            CY_MISRA_DEVIATE_BLOCK_START("MISRA C-2012 Rule 11.2", 1,
                                         "FreeRTOS API uses opaque handle types; semaphore handle is passed through API as required by contract.");
            ret = xSemaphoreGive(*semaphore);
            CY_MISRA_BLOCK_END("MISRA C-2012 Rule 11.2");
        }

        if (ret == pdFALSE)
        {
            status = CY_RTOS_GENERAL_ERROR;
        }
        else
        {
            status = CY_RSLT_SUCCESS;
        }
    }

    return status;
}


//--------------------------------------------------------------------------------------------------
// cy_rtos_semaphore_get_count
//--------------------------------------------------------------------------------------------------
cy_rslt_t cy_rtos_semaphore_get_count(const cy_semaphore_t* semaphore, size_t* count)
{
    cy_rslt_t status;
    if (semaphore == NULL)
    {
        status = CY_RTOS_BAD_PARAM;
    }
    else
    {
        CY_MISRA_DEVIATE_BLOCK_START("MISRA C-2012 Rule 11.2", 1,
                                     "FreeRTOS API uses opaque handle types; semaphore handle is passed through API as required by contract.");
        *count = uxSemaphoreGetCount(*semaphore);
        CY_MISRA_BLOCK_END("MISRA C-2012 Rule 11.2");
        status = CY_RSLT_SUCCESS;
    }
    return status;
}


#if (configUSE_COUNTING_SEMAPHORES == 1)
CY_MISRA_DEVIATE_BLOCK_START("MISRA C-2012 Rule 8.13", 1,
                             "Public API signature in cyabs_rtos.h is non-const for cross-kernel compatibility; some kernels modify the handle object in deinit paths.");
//--------------------------------------------------------------------------------------------------
// cy_rtos_semaphore_deinit
//--------------------------------------------------------------------------------------------------
cy_rslt_t cy_rtos_semaphore_deinit(cy_semaphore_t* semaphore)
{
    cy_rslt_t status;
    if (semaphore == NULL)
    {
        status = CY_RTOS_BAD_PARAM;
    }
    else
    {
        CY_MISRA_DEVIATE_BLOCK_START("MISRA C-2012 Rule 11.2", 1,
                                     "FreeRTOS API uses opaque handle types; semaphore handle is passed through API as required by contract.");
        vSemaphoreDelete(*semaphore);
        CY_MISRA_BLOCK_END("MISRA C-2012 Rule 11.2");
        status = CY_RSLT_SUCCESS;
    }
    return status;
}


CY_MISRA_BLOCK_END("MISRA C-2012 Rule 8.13");


#endif /* configUSE_COUNTING_SEMAPHORES */

//==================================================================================================
// Events
//==================================================================================================

//--------------------------------------------------------------------------------------------------
// cy_rtos_init_event
//--------------------------------------------------------------------------------------------------
cy_rslt_t cy_rtos_event_init(cy_event_t* event)
{
    cy_rslt_t status;
    if (event == NULL)
    {
        status = CY_RTOS_BAD_PARAM;
    }
    else
    {
        *event = xEventGroupCreate();
        if (*event == NULL)
        {
            status = CY_RTOS_NO_MEMORY;
        }
        else
        {
            status = CY_RSLT_SUCCESS;
        }
    }
    return status;
}


//--------------------------------------------------------------------------------------------------
// cy_rtos_event_setbits
//--------------------------------------------------------------------------------------------------
cy_rslt_t cy_rtos_event_setbits(const cy_event_t* event, uint32_t bits)
{
    cy_rslt_t status;
    if (event == NULL)
    {
        status = CY_RTOS_BAD_PARAM;
    }
    else
    {
        BaseType_t ret;
        if (is_in_isr())
        {
            #if (configUSE_TIMERS == 1)
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            ret = xEventGroupSetBitsFromISR(*event, (EventBits_t)bits, &xHigherPriorityTaskWoken);

            if (ret == pdTRUE)
            {
                portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
            }
            #else
            /* xEventGroupSetBitsFromISR requires configUSE_TIMERS == 1 */
            status = CY_RTOS_UNSUPPORTED;
            #endif /* configUSE_TIMERS */
        }
        else
        {
            // xEventGroupSetBits does not return pass/fail, but instead returns the value of the
            // event bits at the time the function returns. There is potential to
            // return 0 (value equal to pdFALSE), so instead treat it as successful after the call
            // to xEventGroupSetBits to avoid false error.
            (void)xEventGroupSetBits(*event, (EventBits_t)bits);
            ret = pdTRUE;
        }

        if (ret == pdFALSE)
        {
            status = CY_RTOS_GENERAL_ERROR;
        }
        else
        {
            status = CY_RSLT_SUCCESS;
        }
    }
    return status;
}


//--------------------------------------------------------------------------------------------------
// cy_rtos_event_clearbits
//--------------------------------------------------------------------------------------------------
cy_rslt_t cy_rtos_event_clearbits(const cy_event_t* event, uint32_t bits)
{
    cy_rslt_t status;
    if (event == NULL)
    {
        status = CY_RTOS_BAD_PARAM;
    }
    else
    {
        BaseType_t ret;
        if (is_in_isr())
        {
            #if (configUSE_TIMERS == 1)
            ret = xEventGroupClearBitsFromISR(*event, (EventBits_t)bits);
            #else
            /* xEventGroupClearBitsFromISR requires configUSE_TIMERS == 1 */
            status = CY_RTOS_UNSUPPORTED;
            #endif /* configUSE_TIMERS */
        }
        else
        {
            // xEventGroupClearBits does not return pass/fail, but instead returns the value of the
            // event bits before the requested bits were cleared. There is potential to
            // return 0 (value equal to pdFALSE), so instead treat it as successful after the call
            // to xEventGroupClearBits to avoid false error.
            (void)xEventGroupClearBits(*event, (EventBits_t)bits);
            ret = pdTRUE;
        }

        if (ret == pdFALSE)
        {
            status = CY_RTOS_GENERAL_ERROR;
        }
        else
        {
            status = CY_RSLT_SUCCESS;
        }
    }
    return status;
}


//--------------------------------------------------------------------------------------------------
// cy_rtos_event_getbits
//--------------------------------------------------------------------------------------------------
cy_rslt_t cy_rtos_event_getbits(const cy_event_t* event, uint32_t* bits)
{
    cy_rslt_t status;
    if ((event == NULL) || (bits == NULL))
    {
        status = CY_RTOS_BAD_PARAM;
    }
    else
    {
        *bits  = xEventGroupClearBits(*event, 0U);
        status = CY_RSLT_SUCCESS;
    }
    return status;
}


//--------------------------------------------------------------------------------------------------
// cy_rtos_event_waitbits
//--------------------------------------------------------------------------------------------------
cy_rslt_t cy_rtos_event_waitbits(const cy_event_t* event, uint32_t* bits, bool clear, bool all,
                                 cy_time_t timeout_ms)
{
    cy_rslt_t status;
    if ((event == NULL) || (bits == NULL))
    {
        status = CY_RTOS_BAD_PARAM;
    }
    else
    {
        TickType_t ticks = convert_ms_to_ticks(timeout_ms);
        uint32_t   bitsVal  = *bits;

        *bits = xEventGroupWaitBits(*event, (EventBits_t)bitsVal, (BaseType_t)clear,
                                    (BaseType_t)all, ticks);
        status   = (((bitsVal & *bits) == bitsVal) || (((bitsVal & *bits) != 0U) && (!all)))
            ? CY_RSLT_SUCCESS
            : CY_RTOS_TIMEOUT;
    }
    return status;
}


CY_MISRA_DEVIATE_BLOCK_START("MISRA C-2012 Rule 8.13", 1,
                             "Public API signature in cyabs_rtos.h is non-const for cross-kernel compatibility; some kernels modify the handle object in deinit paths.");
//--------------------------------------------------------------------------------------------------
// cy_rtos_event_deinit
//--------------------------------------------------------------------------------------------------
cy_rslt_t cy_rtos_event_deinit(cy_event_t* event)
{
    cy_rslt_t status;
    if (event == NULL)
    {
        status = CY_RTOS_BAD_PARAM;
    }
    else
    {
        vEventGroupDelete(*event);
        status = CY_RSLT_SUCCESS;
    }
    return status;
}


CY_MISRA_BLOCK_END("MISRA C-2012 Rule 8.13");


//==================================================================================================
// Queues
//==================================================================================================

//--------------------------------------------------------------------------------------------------
// cy_rtos_queue_init
//--------------------------------------------------------------------------------------------------
cy_rslt_t cy_rtos_queue_init(cy_queue_t* queue, size_t length, size_t itemsize)
{
    cy_rslt_t status;
    if (queue == NULL)
    {
        status = CY_RTOS_BAD_PARAM;
    }
    else
    {
        *queue = xQueueCreate(length, itemsize);
        if (*queue == NULL)
        {
            status = CY_RTOS_NO_MEMORY;
        }
        else
        {
            status = CY_RSLT_SUCCESS;
        }
    }
    return status;
}


#if defined(FREERTOS_COMMON_SECTION_BEGIN)
FREERTOS_COMMON_SECTION_BEGIN
#endif
//--------------------------------------------------------------------------------------------------
// cy_rtos_queue_put
//--------------------------------------------------------------------------------------------------
cy_rslt_t cy_rtos_queue_put(const cy_queue_t* queue, const void* item_ptr, cy_time_t timeout_ms)
{
    cy_rslt_t status;
    if ((queue == NULL) || (item_ptr == NULL))
    {
        status = CY_RTOS_BAD_PARAM;
    }
    else
    {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        BaseType_t ret;
        if (is_in_isr())
        {
            ret = xQueueSendToBackFromISR(*queue, item_ptr, &xHigherPriorityTaskWoken);
            if (ret == pdTRUE)
            {
                portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
            }
        }
        else
        {
            TickType_t ticks = convert_ms_to_ticks(timeout_ms);
            ret = xQueueSendToBack(*queue, item_ptr, ticks);
        }

        if (ret == pdFALSE)
        {
            status = CY_RTOS_GENERAL_ERROR;
        }
        else
        {
            status = CY_RSLT_SUCCESS;
        }
    }
    return status;
}


#if defined(FREERTOS_COMMON_SECTION_END)
FREERTOS_COMMON_SECTION_END
#endif


#if defined(FREERTOS_COMMON_SECTION_BEGIN)
FREERTOS_COMMON_SECTION_BEGIN
#endif
//--------------------------------------------------------------------------------------------------
// cy_rtos_queue_get
//--------------------------------------------------------------------------------------------------
cy_rslt_t cy_rtos_queue_get(const cy_queue_t* queue, void* item_ptr, cy_time_t timeout_ms)
{
    cy_rslt_t status;
    if ((queue == NULL) || (item_ptr == NULL))
    {
        status = CY_RTOS_BAD_PARAM;
    }
    else
    {
        BaseType_t ret;
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        if (is_in_isr())
        {
            ret = xQueueReceiveFromISR(*queue, item_ptr, &xHigherPriorityTaskWoken);
            if (ret == pdTRUE)
            {
                portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
            }
        }
        else
        {
            TickType_t ticks = convert_ms_to_ticks(timeout_ms);
            ret = xQueueReceive(*queue, item_ptr, ticks);
        }

        if (ret == pdFALSE)
        {
            status = CY_RTOS_GENERAL_ERROR;
        }
        else
        {
            status = CY_RSLT_SUCCESS;
        }
    }
    return status;
}


#if defined(FREERTOS_COMMON_SECTION_END)
FREERTOS_COMMON_SECTION_END
#endif


//--------------------------------------------------------------------------------------------------
// cy_rtos_queue_count
//--------------------------------------------------------------------------------------------------
cy_rslt_t cy_rtos_queue_count(const cy_queue_t* queue, size_t* num_waiting)
{
    cy_rslt_t status;
    if ((queue == NULL) || (num_waiting == NULL))
    {
        status = CY_RTOS_BAD_PARAM;
    }
    else
    {
        *num_waiting = is_in_isr()
            ? uxQueueMessagesWaitingFromISR(*queue)
            : uxQueueMessagesWaiting(*queue);
        status = CY_RSLT_SUCCESS;
    }
    return status;
}


//--------------------------------------------------------------------------------------------------
// cy_rtos_queue_space
//--------------------------------------------------------------------------------------------------
cy_rslt_t cy_rtos_queue_space(const cy_queue_t* queue, size_t* num_spaces)
{
    cy_rslt_t status;
    if ((queue == NULL) || (num_spaces == NULL))
    {
        status = CY_RTOS_BAD_PARAM;
    }
    else
    {
        *num_spaces = uxQueueSpacesAvailable(*queue);
        status      = CY_RSLT_SUCCESS;
    }
    return status;
}


//--------------------------------------------------------------------------------------------------
// cy_rtos_queue_reset
//--------------------------------------------------------------------------------------------------
cy_rslt_t cy_rtos_queue_reset(const cy_queue_t* queue)
{
    cy_rslt_t status;
    if (queue == NULL)
    {
        status = CY_RTOS_BAD_PARAM;
    }
    else
    {
        BaseType_t ret = xQueueReset(*queue);

        if (ret == pdFALSE)
        {
            status = CY_RTOS_GENERAL_ERROR;
        }
        else
        {
            status = CY_RSLT_SUCCESS;
        }
    }
    return status;
}


CY_MISRA_DEVIATE_BLOCK_START("MISRA C-2012 Rule 8.13", 1,
                             "Public API signature in cyabs_rtos.h is non-const for cross-kernel compatibility; some kernels modify the handle object in deinit paths.");
//--------------------------------------------------------------------------------------------------
// cy_rtos_queue_deinit
//--------------------------------------------------------------------------------------------------
cy_rslt_t cy_rtos_queue_deinit(cy_queue_t* queue)
{
    cy_rslt_t status;
    if (queue == NULL)
    {
        status = CY_RTOS_BAD_PARAM;
    }
    else
    {
        vQueueDelete(*queue);
        status = CY_RSLT_SUCCESS;
    }
    return status;
}


CY_MISRA_BLOCK_END("MISRA C-2012 Rule 8.13");


//==================================================================================================
// Timers
//==================================================================================================

#if (configUSE_TIMERS == 1)
//--------------------------------------------------------------------------------------------------
// cy_rtos_timer_init
//--------------------------------------------------------------------------------------------------
cy_rslt_t cy_rtos_timer_init(cy_timer_t* timer, cy_timer_trigger_type_t type,
                             cy_timer_callback_t fun, cy_timer_callback_arg_t arg)
{
    cy_rslt_t status;
    if ((timer == NULL) || (fun == NULL))
    {
        status = CY_RTOS_BAD_PARAM;
    }
    else
    {
        // Create a wrapper callback to make sure to call fun() with arg as opposed
        // to providing the timer reference as FreeRTOS does by default.
        CY_MISRA_DEVIATE_BLOCK_START("MISRA C-2012 Rule 11.5", 1,
                                     "pvPortMalloc returns void* aligned to portBYTE_ALIGNMENT (>= sizeof(callback_data_t) alignment); conversion to callback_data_t* is safe - no alignment violation.");
        callback_data_t* cb_arg = (callback_data_t*)pvPortMalloc(sizeof(callback_data_t));
        CY_MISRA_BLOCK_END("MISRA C-2012 Rule 11.5");
        if (NULL == cb_arg)
        {
            status = CY_RTOS_NO_MEMORY;
        }
        else
        {
            cb_arg->cb  = fun;
            cb_arg->arg = arg;

            BaseType_t reload =
                (type == (cy_timer_trigger_type_t)CY_TIMER_TYPE_PERIODIC) ? pdTRUE : pdFALSE;
            *timer = xTimerCreate("", 1UL, (UBaseType_t)reload, cb_arg, &timer_callback);

            if (*timer == NULL)
            {
                vPortFree(cb_arg);
                status = CY_RTOS_NO_MEMORY;
            }
            else
            {
                status = CY_RSLT_SUCCESS;
            }
        }
    }
    return status;
}


//--------------------------------------------------------------------------------------------------
// cy_rtos_timer_start
//--------------------------------------------------------------------------------------------------
cy_rslt_t cy_rtos_timer_start(const cy_timer_t* timer, cy_time_t num_ms)
{
    cy_rslt_t status;
    if (timer == NULL)
    {
        status = CY_RTOS_BAD_PARAM;
    }
    else
    {
        TickType_t ticks = convert_ms_to_ticks(num_ms);
        BaseType_t ret;

        if (is_in_isr())
        {
            BaseType_t taskWoken = pdFALSE;
            ret = xTimerChangePeriodFromISR(*timer, ticks, &taskWoken);
            if (ret == pdPASS)
            {
                portYIELD_FROM_ISR(taskWoken);
                taskWoken = pdFALSE;
                ret = xTimerStartFromISR(*timer, &taskWoken);
                if (ret == pdPASS)
                {
                    portYIELD_FROM_ISR(taskWoken);
                }
            }
        }
        else
        {
            ret = xTimerChangePeriod(*timer, ticks, 0UL);
            if (ret == pdPASS)
            {
                ret = xTimerStart(*timer, 0UL);
            }
        }
        if (ret == pdFALSE)
        {
            status = CY_RTOS_GENERAL_ERROR;
        }
        else
        {
            status = CY_RSLT_SUCCESS;
        }
    }
    return status;
}


//--------------------------------------------------------------------------------------------------
// cy_rtos_timer_stop
//--------------------------------------------------------------------------------------------------
cy_rslt_t cy_rtos_timer_stop(const cy_timer_t* timer)
{
    cy_rslt_t status;
    if (timer == NULL)
    {
        status = CY_RTOS_BAD_PARAM;
    }
    else
    {
        BaseType_t ret;
        if (is_in_isr())
        {
            BaseType_t taskWoken = pdFALSE;
            CY_MISRA_DEVIATE_BLOCK_START("MISRA_CAST", 1,
                                         "Signedness conversion inside FreeRTOS macro expansion. Cannot modify third-party macro.");
            ret = xTimerStopFromISR(*timer, &taskWoken);
            CY_MISRA_BLOCK_END("MISRA_CAST");
            if (pdPASS == ret)
            {
                portYIELD_FROM_ISR(taskWoken);
            }
        }
        else
        {
            ret = xTimerStop(*timer, 0UL);
        }
        if (ret == pdFALSE)
        {
            status = CY_RTOS_GENERAL_ERROR;
        }
        else
        {
            status = CY_RSLT_SUCCESS;
        }
    }

    return status;
}


//--------------------------------------------------------------------------------------------------
// cy_rtos_timer_is_running
//--------------------------------------------------------------------------------------------------
cy_rslt_t cy_rtos_timer_is_running(const cy_timer_t* timer, bool* state)
{
    cy_rslt_t status;
    if ((timer == NULL) || (state == NULL))
    {
        status = CY_RTOS_BAD_PARAM;
    }
    else
    {
        BaseType_t active = xTimerIsTimerActive(*timer);
        *state = (active != pdFALSE);

        status = CY_RSLT_SUCCESS;
    }
    return status;
}


CY_MISRA_DEVIATE_BLOCK_START("MISRA C-2012 Rule 8.13", 1,
                             "Public API signature in cyabs_rtos.h is non-const for cross-kernel compatibility; some kernels modify the handle object in deinit paths.");
//--------------------------------------------------------------------------------------------------
// cy_rtos_timer_deinit
//--------------------------------------------------------------------------------------------------
cy_rslt_t cy_rtos_timer_deinit(cy_timer_t* timer)
{
    cy_rslt_t status;
    if (timer == NULL)
    {
        status = CY_RTOS_BAD_PARAM;
    }
    else
    {
        void*      cb  = pvTimerGetTimerID(*timer);
        BaseType_t ret = xTimerDelete(*timer, 0UL);

        if (ret == pdFALSE)
        {
            status = CY_RTOS_GENERAL_ERROR;
        }
        else
        {
            vPortFree(cb);
            status = CY_RSLT_SUCCESS;
        }
    }
    return status;
}


CY_MISRA_BLOCK_END("MISRA C-2012 Rule 8.13");


#endif /* configUSE_TIMERS */


//==================================================================================================
// Time
//==================================================================================================

//--------------------------------------------------------------------------------------------------
// cy_rtos_time_get
//--------------------------------------------------------------------------------------------------
cy_rslt_t cy_rtos_time_get(cy_time_t* tval)
{
    cy_rslt_t status;
    if (tval == NULL)
    {
        status = CY_RTOS_BAD_PARAM;
    }
    else
    {
        *tval  =
            (cy_time_t)(((uint64_t)xTaskGetTickCount() * 1000ULL) / (uint64_t)configTICK_RATE_HZ);
        status = CY_RSLT_SUCCESS;
    }
    return status;
}


//--------------------------------------------------------------------------------------------------
// cy_rtos_delay_milliseconds
//--------------------------------------------------------------------------------------------------
cy_rslt_t cy_rtos_delay_milliseconds(cy_time_t num_ms)
{
    vTaskDelay(convert_ms_to_ticks(num_ms));
    return CY_RSLT_SUCCESS;
}


CY_MISRA_BLOCK_END("MISRA C-2012 Rule 10.4");
CY_MISRA_BLOCK_END("MISRA C-2012 Rule 10.3");
