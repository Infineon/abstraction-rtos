/***************************************************************************//**
* \file cyabs_rtos_freertos.c
*
* \brief
* Implementation for FreeRTOS abstraction
*
********************************************************************************
* \copyright
* Copyright 2018-2019 Cypress Semiconductor Corporation
* SPDX-License-Identifier: Apache-2.0
*
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
*******************************************************************************/

#include "cy_utils.h"
#include "cy_result.h"
#include "cyabs_rtos.h"
#include <stdlib.h>
#include <FreeRTOS.h>
#include <task.h>

static cy_rtos_error_t last_error;

typedef struct
{
    cy_timer_callback_t cb;
    cy_timer_callback_arg_t arg;
} callback_data_t;

// Wrapper function to convert FreeRTOS callback signature to match expectation
// for our cyabs_rtos abstraction API.
static void timer_callback(TimerHandle_t arg)
{
    callback_data_t *cb_arg = (callback_data_t*)pvTimerGetTimerID(arg);
    if (NULL != cb_arg->cb)
    {
        cb_arg->cb(cb_arg->arg);
    }
}

/******************************************************
*                 Error Converter
******************************************************/

cy_rtos_error_t cy_rtos_last_error()
{
    return last_error;
}

/******************************************************
*                 Threads
******************************************************/

cy_rslt_t cy_rtos_create_thread(cy_thread_t *thread, cy_thread_entry_fn_t entry_function,
    const char *name, void *stack, uint32_t stack_size, cy_thread_priority_t priority, cy_thread_arg_t arg)
{
    cy_rslt_t status;
    CY_UNUSED_PARAMETER(stack);
    if (thread == NULL || stack_size < CY_RTOS_MIN_STACK_SIZE)
    {
        status = CY_RTOS_BAD_PARAM;
    }
    else
    {
        uint32_t stack_deapth = ((stack_size + (sizeof(StackType_t) - 1)) / sizeof(StackType_t));
        last_error = xTaskCreate((TaskFunction_t)entry_function, name, stack_deapth, (void *)arg, priority, thread);
        if (last_error == errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY)
        {
            status = CY_RTOS_NO_MEMORY;
        }
        else if (last_error != pdPASS)
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

cy_rslt_t cy_rtos_exit_thread()
{
    vTaskDelete(NULL);
    return CY_RSLT_SUCCESS;
}

cy_rslt_t cy_rtos_terminate_thread(cy_thread_t *thread)
{
    cy_rslt_t status;
    if (thread == NULL)
    {
        status = CY_RTOS_BAD_PARAM;
    }
    else
    {
        vTaskDelete(*thread);
        status = CY_RSLT_SUCCESS;
    }
    return status;
}

cy_rslt_t cy_rtos_is_thread_running(cy_thread_t *thread, bool *running)
{
    cy_rslt_t status;
    if (thread == NULL || running == NULL)
    {
        status = CY_RTOS_BAD_PARAM;
    }
    else
    {
        eTaskState st = eTaskGetState(*thread);
        *running = (st == eRunning);
        status = CY_RSLT_SUCCESS;
    }
    return status;
}

cy_rslt_t cy_rtos_get_thread_state(cy_thread_t *thread, cy_thread_state_t *state)
{
    cy_rslt_t status;
    if (thread == NULL || state == NULL)
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

cy_rslt_t cy_rtos_join_thread(cy_thread_t *thread)
{
    cy_rslt_t status;
    if (thread == NULL)
    {
        status = CY_RTOS_BAD_PARAM;
    }
    else
    {
        TickType_t ticks = pdMS_TO_TICKS(1);

        while (eDeleted != eTaskGetState(*thread))
        {
            vTaskDelay(ticks);
        }

        status = CY_RSLT_SUCCESS;
    }
    return status;
}

cy_rslt_t cy_rtos_get_thread_handle(cy_thread_t *thread)
{
    cy_rslt_t status = CY_RSLT_SUCCESS;

    if (thread == NULL)
        status = CY_RTOS_BAD_PARAM;
    else
        *thread = xTaskGetCurrentTaskHandle();

    return status;
}


/******************************************************
*                 Mutexes
******************************************************/

cy_rslt_t cy_rtos_init_mutex(cy_mutex_t *mutex)
{
    cy_rslt_t status;
    if (mutex == NULL)
    {
        status = CY_RTOS_BAD_PARAM;
    }
    else
    {
        *mutex = xSemaphoreCreateRecursiveMutex();
        if (*mutex == NULL)
            status = CY_RTOS_NO_MEMORY;
        else
            status = CY_RSLT_SUCCESS;
    }
    return status;
}

cy_rslt_t cy_rtos_get_mutex(cy_mutex_t *mutex, cy_time_t timeout_ms)
{
    cy_rslt_t status;
    if (mutex == NULL)
    {
        status = CY_RTOS_BAD_PARAM;
    }
    else
    {
        TickType_t ticks = pdMS_TO_TICKS(timeout_ms);

        if (xSemaphoreTakeRecursive(*mutex, ticks) == pdFALSE)
            status = CY_RTOS_TIMEOUT;
        else
            status = CY_RSLT_SUCCESS;
    }
    return status;
}

cy_rslt_t cy_rtos_set_mutex(cy_mutex_t *mutex)
{
    cy_rslt_t status;
    if (mutex == NULL)
    {
        status = CY_RTOS_BAD_PARAM;
    }
    else
    {
        if (xSemaphoreGiveRecursive(*mutex) == pdFALSE)
            status = CY_RTOS_GENERAL_ERROR;
        else
            status = CY_RSLT_SUCCESS;
    }
    return status;
}

cy_rslt_t cy_rtos_deinit_mutex(cy_mutex_t *mutex)
{
    cy_rslt_t status;
    if (mutex == NULL)
    {
        status = CY_RTOS_BAD_PARAM;
    }
    else
    {
        vSemaphoreDelete(*mutex);
        status = CY_RSLT_SUCCESS;
    }
    return status;
}


/******************************************************
*                 Semaphores
******************************************************/

cy_rslt_t cy_rtos_init_semaphore(cy_semaphore_t *semaphore, uint32_t maxcount, uint32_t initcount)
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
            status = CY_RTOS_NO_MEMORY;
        else
            status = CY_RSLT_SUCCESS;
    }
    return status;
}

cy_rslt_t cy_rtos_get_semaphore(cy_semaphore_t *semaphore, uint32_t timeout_ms, bool in_isr)
{
    cy_rslt_t status;
    if (semaphore == NULL)
    {
        status = CY_RTOS_BAD_PARAM;
    }
    else
    {
        TickType_t ticks = pdMS_TO_TICKS(timeout_ms);
        status = CY_RSLT_SUCCESS;

        if (in_isr)
        {
            if (pdFALSE == xSemaphoreTakeFromISR(*semaphore, NULL))
                status = CY_RTOS_TIMEOUT;
        }
        else
        {
            if (pdFALSE == xSemaphoreTake(*semaphore, ticks))
                status = CY_RTOS_TIMEOUT;
        }
    }

    return status;
}

cy_rslt_t cy_rtos_set_semaphore(cy_semaphore_t *semaphore, bool in_isr)
{
    cy_rslt_t status;
    if (semaphore == NULL)
    {
        status = CY_RTOS_BAD_PARAM;
    }
    else
    {
        BaseType_t ret;

        if (in_isr)
        {
            ret = xSemaphoreGiveFromISR(*semaphore, NULL);
        }
        else
        {
            ret = xSemaphoreGive(*semaphore);
        }

        if (ret == pdFALSE)
            status = CY_RTOS_GENERAL_ERROR;
        else
            status = CY_RSLT_SUCCESS;
    }
    return status;
}

cy_rslt_t cy_rtos_deinit_semaphore(cy_semaphore_t *semaphore)
{
    cy_rslt_t status;
    if (semaphore == NULL)
    {
        status = CY_RTOS_BAD_PARAM;
    }
    else
    {
        vSemaphoreDelete(*semaphore);
        status = CY_RSLT_SUCCESS;
    }
    return status;
}


/******************************************************
*                 Events
******************************************************/

cy_rslt_t cy_rtos_init_event(cy_event_t *event)
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
            status = CY_RTOS_NO_MEMORY;
        else
            status = CY_RSLT_SUCCESS;
    }
    return status;
}

cy_rslt_t cy_rtos_setbits_event(cy_event_t *event, uint32_t bits, bool in_isr)
{
    cy_rslt_t status;
    if (event == NULL)
    {
        status = CY_RTOS_BAD_PARAM;
    }
    else
    {
        BaseType_t ret;
        if (in_isr)
        {
            BaseType_t bt = pdFALSE;
            ret = xEventGroupSetBitsFromISR(*event, bits, &bt);
        }
        else
        {
            ret = xEventGroupSetBits(*event, bits);
        }

        if (ret == pdFALSE)
            status = CY_RTOS_GENERAL_ERROR;
        else
            status = CY_RSLT_SUCCESS;
    }
    return status;
}

cy_rslt_t cy_rtos_clearbits_event(cy_event_t *event, uint32_t bits, bool in_isr)
{
    cy_rslt_t status;
    if (event == NULL)
    {
        status = CY_RTOS_BAD_PARAM;
    }
    else
    {
        BaseType_t ret;
        if (in_isr)
        {
            ret = xEventGroupClearBitsFromISR(*event, bits);
        }
        else
        {
            ret = xEventGroupClearBits(*event, bits);
        }

        if (ret == pdFALSE)
            status = CY_RTOS_GENERAL_ERROR;
        else
            status = CY_RSLT_SUCCESS;
    }
    return status;
}

cy_rslt_t cy_rtos_getbits_event(cy_event_t *event, uint32_t *bits)
{
    cy_rslt_t status;
    if (event == NULL || bits == NULL)
    {
        status = CY_RTOS_BAD_PARAM;
    }
    else
    {
        *bits = xEventGroupGetBits(*event);
        status = CY_RSLT_SUCCESS;
    }
    return status;
}

cy_rslt_t cy_rtos_waitbits_event(cy_event_t *event, uint32_t *waitfor, bool clear, bool allset, cy_time_t timeout)
{
    cy_rslt_t status;
    if (event == NULL || waitfor == NULL)
    {
        status = CY_RTOS_BAD_PARAM;
    }
    else
    {
        TickType_t ticks = pdMS_TO_TICKS(timeout);
        uint32_t bits = *waitfor;

        *waitfor = xEventGroupWaitBits(*event, bits, clear, allset, ticks);
        status = (((bits & *waitfor) == bits) || (((bits & *waitfor) > 0) & !allset))
            ? CY_RSLT_SUCCESS
            : CY_RTOS_TIMEOUT;
    }
    return status;
}

cy_rslt_t cy_rtos_deinit_event(cy_event_t *event)
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


/******************************************************
*                 Queues
******************************************************/

cy_rslt_t cy_rtos_init_queue(cy_queue_t *queue, size_t length, size_t itemsize)
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
            status = CY_RTOS_NO_MEMORY;
        else
            status = CY_RSLT_SUCCESS;
    }
    return status;
}

cy_rslt_t cy_rtos_put_queue(cy_queue_t *queue, const void *item_ptr, cy_time_t timeoutms, bool in_isr)
{
    cy_rslt_t status;
    if (queue == NULL || item_ptr == NULL)
    {
        status = CY_RTOS_BAD_PARAM;
    }
    else
    {
        BaseType_t ret;
        if (in_isr)
        {
            ret = xQueueSendToBackFromISR(*queue, item_ptr, NULL);
        }
        else
        {
            TickType_t ticks = pdMS_TO_TICKS(timeoutms);
            ret = xQueueSendToBack(*queue, item_ptr, ticks);
        }

        if (ret == pdFALSE)
            status = CY_RTOS_GENERAL_ERROR;
        else
            status = CY_RSLT_SUCCESS;
    }
    return status;
}

cy_rslt_t cy_rtos_get_queue(cy_queue_t *queue, void *item_ptr, cy_time_t timeoutms, bool in_isr)
{
    cy_rslt_t status;
    if (queue == NULL || item_ptr == NULL)
    {
        status = CY_RTOS_BAD_PARAM;
    }
    else
    {
        BaseType_t ret;
        if (in_isr)
        {
            ret = xQueueReceiveFromISR(*queue, item_ptr, NULL);
        }
        else
        {
            TickType_t ticks = pdMS_TO_TICKS(timeoutms);
            ret = xQueueReceive(*queue, item_ptr, ticks);
        }

        if (ret == pdFALSE)
            status = CY_RTOS_GENERAL_ERROR;
        else
            status = CY_RSLT_SUCCESS;
    }
    return status;
}

cy_rslt_t cy_rtos_count_queue(cy_queue_t *queue, size_t *num_waiting)
{
    cy_rslt_t status;
    if (queue == NULL || num_waiting == NULL)
    {
        status = CY_RTOS_BAD_PARAM;
    }
    else
    {
        *num_waiting = uxQueueMessagesWaiting(*queue);
        status = CY_RSLT_SUCCESS;
    }
    return status;
}

cy_rslt_t cy_rtos_space_queue(cy_queue_t *queue, size_t *num_spaces)
{
    cy_rslt_t status;
    if (queue == NULL || num_spaces == NULL)
    {
        status = CY_RTOS_BAD_PARAM;
    }
    else
    {
        *num_spaces = uxQueueSpacesAvailable(*queue);
        status = CY_RSLT_SUCCESS;
    }
    return status;
}

cy_rslt_t cy_rtos_reset_queue(cy_queue_t *queue)
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
            status = CY_RTOS_GENERAL_ERROR;
        else
            status = CY_RSLT_SUCCESS;
    }
    return status;
}

cy_rslt_t cy_rtos_deinit_queue(cy_queue_t *queue)
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


/******************************************************
*                 Timers
******************************************************/

cy_rslt_t cy_rtos_init_timer(cy_timer_t *timer, cy_timer_trigger_type_t type,
    cy_timer_callback_t fun, cy_timer_callback_arg_t arg)
{
    cy_rslt_t status;
    if (timer == NULL || fun == NULL)
    {
        status = CY_RTOS_BAD_PARAM;
    }
    else
    {
        BaseType_t reload = (type == CY_TIMER_TYPE_PERIODIC) ? pdTRUE : pdFALSE;

        // Create a wrapper callback to make sure to call fun() with arg as opposed
        // to providing the timer reference as FreeRTOS does by default.
        callback_data_t *cb_arg = (callback_data_t*)malloc(sizeof(callback_data_t));
        if (NULL == cb_arg)
            status = CY_RTOS_NO_MEMORY;
        else
        {
            cb_arg->cb = fun;
            cb_arg->arg = arg;

            *timer = xTimerCreate("", 1, reload, cb_arg, &timer_callback);

            if (*timer == NULL)
                status = CY_RTOS_NO_MEMORY;
            else
                status = CY_RSLT_SUCCESS;
        }
    }
    return status;
}

cy_rslt_t cy_rtos_start_timer(cy_timer_t *timer, cy_time_t num_ms)
{
    cy_rslt_t status;
    if (timer == NULL)
    {
        status = CY_RTOS_BAD_PARAM;
    }
    else
    {
        TickType_t ticks = pdMS_TO_TICKS(num_ms);
        BaseType_t ret = xTimerChangePeriod(*timer, ticks, 0);

        if (ret == pdPASS)
            ret = xTimerStart(*timer, 0);

        if (ret == pdFALSE)
            status = CY_RTOS_GENERAL_ERROR;
        else
            status = CY_RSLT_SUCCESS;
    }
    return status;
}

cy_rslt_t cy_rtos_stop_timer(cy_timer_t *timer)
{
    cy_rslt_t status;
    if (timer == NULL)
    {
        status = CY_RTOS_BAD_PARAM;
    }
    else
    {
        BaseType_t ret = xTimerStop(*timer, 0);

        if (ret == pdFALSE)
            status = CY_RTOS_GENERAL_ERROR;
        else
            status = CY_RSLT_SUCCESS;
    }
    return status;
}

cy_rslt_t cy_rtos_is_running_timer(cy_timer_t *timer, bool *state)
{
    cy_rslt_t status;
    if (timer == NULL || state == NULL)
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

cy_rslt_t cy_rtos_deinit_timer(cy_timer_t *timer)
{
    cy_rslt_t status;
    if (timer == NULL)
    {
        status = CY_RTOS_BAD_PARAM;
    }
    else
    {
        void *cb = pvTimerGetTimerID(*timer);
        BaseType_t ret = xTimerDelete(*timer, 0);

        if (ret == pdFALSE)
            status = CY_RTOS_GENERAL_ERROR;
        else
        {
            free(cb);
            status = CY_RSLT_SUCCESS;
        }
    }
    return status;
}


/******************************************************
*                 Time
******************************************************/

cy_rslt_t cy_rtos_get_time(cy_time_t *tval)
{
    cy_rslt_t status;
    if (tval == NULL)
    {
        status = CY_RTOS_BAD_PARAM;
    }
    else
    {
        *tval = (cy_time_t)((xTaskGetTickCount() * 1000LL) / configTICK_RATE_HZ);
        status = CY_RSLT_SUCCESS;
    }
    return status;
}

cy_rslt_t cy_rtos_delay_milliseconds(uint32_t num_ms)
{
    vTaskDelay((TickType_t)(((uint64_t)num_ms * configTICK_RATE_HZ) / 1000));
    return CY_RSLT_SUCCESS;
}
