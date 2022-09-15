/***********************************************************************************************//**
 * \file cyabs_rtos_wiced_rtos.c
 *
 * \brief
 * Implementation for WICED RTOS abstraction
 *
 ***************************************************************************************************
 * \copyright
 * Copyright 2018-2022 Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation
 *
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
 **************************************************************************************************/

#include <cy_result.h>
#include <cy_utils.h>
#include <cyabs_rtos.h>
#include <stdlib.h>
#include "wiced_rtos.h"
#include "wiced_timer.h"

static const uint32_t MAX_QUEUE_MESSAGE_SIZE = 1;
static const uint32_t SEMAPHORE_INIT_COUNT = 0;

static cy_rtos_error_t last_error;

// WICED_RTOS functions are documented to return either WICED_SUCCESS or WICED_ERROR,
// but there are many sub-errors.  Most are generic, e.g. WICED_MUTEX_ERROR.  The queue
// interface is the only to return specific errors.
static cy_rslt_t convert_return_type(wiced_result_t retVal)
{
    switch (retVal)
    {
        case (WICED_SUCCESS):
            return CY_RSLT_SUCCESS;

        case (WICED_NO_MEMORY):
            return CY_RTOS_NO_MEMORY;

        case (WICED_TIMEOUT):
            return CY_RTOS_TIMEOUT;

        case (WICED_OPTION_ERROR):
            return CY_RTOS_BAD_PARAM;

        case (WICED_QUEUE_EMPTY):
            return CY_RTOS_QUEUE_EMPTY;

        case (WICED_QUEUE_FULL):
            return CY_RTOS_QUEUE_FULL;

        default:
            return CY_RTOS_GENERAL_ERROR;
    }
}


//--------------------------------------------------------------------------------------------------
// cy_rtos_last_error
//--------------------------------------------------------------------------------------------------
cy_rtos_error_t cy_rtos_last_error(void)
{
    return last_error;
}


//--------------------------------------------------------------------------------------------------
// cy_rtos_create_thread
//--------------------------------------------------------------------------------------------------
cy_rslt_t cy_rtos_thread_create(cy_thread_t* thread, cy_thread_entry_fn_t entry_function,
                                const char* name, void* stack, uint32_t stack_size,
                                cy_thread_priority_t priority, cy_thread_arg_t arg)
{
    if ((stack_size < CY_RTOS_MIN_STACK_SIZE) || (thread == NULL))
    {
        return CY_RTOS_BAD_PARAM;
    }
    else if ((stack != NULL) && (0 != (((uint32_t)stack) & CY_RTOS_ALIGNMENT_MASK)))
    {
        return CY_RTOS_ALIGNMENT_ERROR;
    }

    *thread = wiced_rtos_create_thread();
    if (thread == NULL)
    {
        return CY_RTOS_GENERAL_ERROR;
    }
    return convert_return_type(wiced_rtos_init_thread(*thread, priority, name, entry_function,
                                                      stack_size, &arg));
}


//--------------------------------------------------------------------------------------------------
// cy_rtos_thread_exit
//--------------------------------------------------------------------------------------------------
cy_rslt_t cy_rtos_thread_exit(void)
{
    // WICED doesn't seem to need anything before closing threads
    return CY_RSLT_SUCCESS;
}


//--------------------------------------------------------------------------------------------------
// cy_rtos_thread_terminate
//--------------------------------------------------------------------------------------------------
cy_rslt_t cy_rtos_thread_terminate(cy_thread_t* thread)
{
    if (thread == NULL)
    {
        return CY_RTOS_BAD_PARAM;
    }

    // No manual thread termination support
    return CY_RTOS_UNSUPPORTED;
}


//--------------------------------------------------------------------------------------------------
// cy_rtos_thread_is_running
//--------------------------------------------------------------------------------------------------
cy_rslt_t cy_rtos_thread_is_running(cy_thread_t* thread, bool* running)
{
    if ((thread == NULL) || (running == NULL))
    {
        return CY_RTOS_BAD_PARAM;
    }
    // Only true when the given thread is the current one
    *running = !wiced_rtos_is_current_thread(*thread);
    // ! because function returns SUCCESS (0x0) when the thread is running

    return CY_RSLT_SUCCESS;
}


//--------------------------------------------------------------------------------------------------
// cy_rtos_thread_get_state
//--------------------------------------------------------------------------------------------------
cy_rslt_t cy_rtos_thread_get_state(cy_thread_t* thread, cy_thread_state_t* state)
{
    if ((thread == NULL) || (state == NULL))
    {
        return CY_RTOS_BAD_PARAM;
    }

    bool running;
    cy_rslt_t rslt = cy_rtos_is_thread_running(thread, &running);
    if (CY_RSLT_SUCCESS != rslt)
    {
        return rslt;
    }
    else if (running)
    {
        *state = CY_THREAD_STATE_RUNNING;
        return CY_RSLT_SUCCESS;
    }

    // WICED doesn't give us any additional info
    *state = CY_THREAD_STATE_UNKNOWN;
    return CY_RSLT_SUCCESS;
}


//--------------------------------------------------------------------------------------------------
// cy_rtos_thread_join
//--------------------------------------------------------------------------------------------------
cy_rslt_t cy_rtos_thread_join(cy_thread_t* thread)
{
    if (thread == NULL)
    {
        return CY_RTOS_BAD_PARAM;
    }
    // Note: wiced_rtos_thread_join() does not return until the thread is terminated.
    return convert_return_type(wiced_rtos_thread_join(*thread));
}


//--------------------------------------------------------------------------------------------------
// cy_rtos_thread_get_handle
//--------------------------------------------------------------------------------------------------
cy_rslt_t cy_rtos_thread_get_handle(cy_thread_t* thread)
{
    if (thread == NULL)
    {
        return CY_RTOS_BAD_PARAM;
    }
    return CY_RTOS_UNSUPPORTED;
}


//--------------------------------------------------------------------------------------------------
// cy_rtos_thread_wait_notification
//--------------------------------------------------------------------------------------------------
cy_rslt_t cy_rtos_thread_wait_notification(cy_time_t timeout_ms)
{
    cy_rtos_error_t ret;
    cy_rslt_t status = CY_RSLT_SUCCESS;

    ret = wiced_rtos_delay_milliseconds(timeout_ms, ALLOW_THREAD_TO_SLEEP);
    /* Update the last known error status */
    last_error = (cy_rtos_error_t)ret;

    if (ret == WICED_SUCCESS)
    {
        status = CY_RTOS_TIMEOUT;
    }
    else if (ret != WICED_ERROR)
    {
        status = CY_RTOS_GENERAL_ERROR;
    }
    return status;
}


//--------------------------------------------------------------------------------------------------
// cy_rtos_thread_set_notification
//--------------------------------------------------------------------------------------------------
cy_rslt_t cy_rtos_thread_set_notification(cy_thread_t* thread)
{
    if (thread == NULL)
    {
        return CY_RTOS_BAD_PARAM;
    }
    // This may cause the other thread to return an error if it is waiting on a task
    return convert_return_type(wiced_rtos_thread_force_awake(*thread));
}


/******************************************************
*                 Mutexes
******************************************************/

//--------------------------------------------------------------------------------------------------
// cy_rtos_mutex_init
//--------------------------------------------------------------------------------------------------
cy_rslt_t cy_rtos_mutex_init(cy_mutex_t* mutex, bool recursive)
{
    // All WICED mutexes are recursive
    CY_UNUSED_PARAMETER(recursive);

    if (mutex == NULL)
    {
        return CY_RTOS_BAD_PARAM;
    }
    *mutex = wiced_rtos_create_mutex();
    if (mutex == NULL)
    {
        return CY_RTOS_GENERAL_ERROR;
    }
    return convert_return_type(wiced_rtos_init_mutex(*mutex));
}


//--------------------------------------------------------------------------------------------------
// cy_rtos_mutex_get
//--------------------------------------------------------------------------------------------------
cy_rslt_t cy_rtos_mutex_get(cy_mutex_t* mutex, cy_time_t timeout_ms)
{
    if (mutex == NULL)
    {
        return CY_RTOS_BAD_PARAM;
    }

    return convert_return_type(wiced_rtos_lock_mutex(*mutex));
}


//--------------------------------------------------------------------------------------------------
// cy_rtos_mutex_set
//--------------------------------------------------------------------------------------------------
cy_rslt_t cy_rtos_mutex_set(cy_mutex_t* mutex)
{
    if (mutex == NULL)
    {
        return CY_RTOS_BAD_PARAM;
    }

    return convert_return_type(wiced_rtos_unlock_mutex(*mutex));
}


//--------------------------------------------------------------------------------------------------
// cy_rtos_deinit_mutex
//--------------------------------------------------------------------------------------------------
cy_rslt_t cy_rtos_mutex_deinit(cy_mutex_t* mutex)
{
    if (mutex == NULL)
    {
        return CY_RTOS_BAD_PARAM;
    }

    // There is a wiced_rtos_deinit_mutex(), but it isn't functional. WICED_RTOS doesn't support
    // properly freeing resources, only way to get them back is restarting device
    return CY_RTOS_UNSUPPORTED;
}


/******************************************************
*                 Semaphores
******************************************************/

//--------------------------------------------------------------------------------------------------
// cy_rtos_semaphore_init
//--------------------------------------------------------------------------------------------------
cy_rslt_t cy_rtos_semaphore_init(cy_semaphore_t* semaphore, uint32_t maxcount, uint32_t initcount)
{
    // WICED_RTOS hard-codes the initial count to 0.  These are counting semaphores
    if ((semaphore == NULL) || (initcount != SEMAPHORE_INIT_COUNT))
    {
        return CY_RTOS_BAD_PARAM;
    }

    semaphore->semaphore = wiced_rtos_create_semaphore();
    if (semaphore == NULL)
    {
        return CY_RTOS_GENERAL_ERROR;
    }
    semaphore->max_count = maxcount;
    semaphore->count = SEMAPHORE_INIT_COUNT;
    return convert_return_type(wiced_rtos_init_semaphore(semaphore->semaphore));
}


//--------------------------------------------------------------------------------------------------
// cy_rtos_semaphore_get
//--------------------------------------------------------------------------------------------------
cy_rslt_t cy_rtos_semaphore_get(cy_semaphore_t* semaphore, cy_time_t timeout_ms)
{
    wiced_result_t wiced_rslt;
    cy_rslt_t cy_rslt;
    // Calling from ISR context is not supported
    if ((semaphore == NULL) || (semaphore->semaphore == NULL))
    {
        return CY_RTOS_BAD_PARAM;
    }
    // The get function handles waiting for the semaphore to be non-zero
    wiced_rslt = wiced_rtos_get_semaphore(semaphore->semaphore, timeout_ms);
    if (WICED_NO_INSTANCE == wiced_rslt)
    {
        // Semaphore must be set before it can be get.  If unset, WICED RTOS treats it as though it
        // doesn't exist, which is equivalent to a timeout for us.
        return CY_RTOS_TIMEOUT;
    }

    cy_rslt = convert_return_type(wiced_rslt);
    if (CY_RSLT_SUCCESS == cy_rslt)
    {
        semaphore->count -= 1;
        return CY_RSLT_SUCCESS;
    }
    return cy_rslt;
}


//--------------------------------------------------------------------------------------------------
// cy_rtos_semaphore_set
//--------------------------------------------------------------------------------------------------
cy_rslt_t cy_rtos_semaphore_set(cy_semaphore_t* semaphore)
{
    if ((semaphore == NULL) || (semaphore->semaphore == NULL))
    {
        return CY_RTOS_BAD_PARAM;
    }
    // WICED semaphore has no built-in max count support
    if (semaphore->count >= semaphore->max_count)
    {
        return CY_RTOS_GENERAL_ERROR;
    }

    if (CY_RSLT_SUCCESS == convert_return_type(wiced_rtos_set_semaphore(semaphore->semaphore)))
    {
        semaphore->count += 1;
        return CY_RSLT_SUCCESS;
    }
    return CY_RTOS_GENERAL_ERROR;
}


//--------------------------------------------------------------------------------------------------
// cy_rtos_semaphore_get_count
//--------------------------------------------------------------------------------------------------
cy_rslt_t cy_rtos_semaphore_get_count(cy_semaphore_t* semaphore, size_t* count)
{
    if ((semaphore == NULL) || (semaphore->semaphore == NULL) || (count == NULL))
    {
        return CY_RTOS_BAD_PARAM;
    }
    *count = (size_t)semaphore->count;
    return CY_RSLT_SUCCESS;
}


//--------------------------------------------------------------------------------------------------
// cy_rtos_semaphore_deinit
//--------------------------------------------------------------------------------------------------
cy_rslt_t cy_rtos_semaphore_deinit(cy_semaphore_t* semaphore)
{
    if (semaphore == NULL)
    {
        return CY_RTOS_BAD_PARAM;
    }
    return CY_RTOS_UNSUPPORTED;
}


/******************************************************
*                 Events
******************************************************/

//--------------------------------------------------------------------------------------------------
// cy_rtos_event_init
//--------------------------------------------------------------------------------------------------
cy_rslt_t cy_rtos_event_init(cy_event_t* event)
{
    if (event == NULL)
    {
        return CY_RTOS_BAD_PARAM;
    }
    *event = wiced_rtos_create_event_flags();
    if (*event == NULL)
    {
        return CY_RTOS_GENERAL_ERROR;
    }
    return CY_RSLT_SUCCESS;
    // wiced_rtos_init_event_flags intentionally not used here.
}


//--------------------------------------------------------------------------------------------------
// cy_rtos_event_setbits
//--------------------------------------------------------------------------------------------------
cy_rslt_t cy_rtos_event_setbits(cy_event_t* event, uint32_t bits)
{
    if (event == NULL)
    {
        return CY_RTOS_BAD_PARAM;
    }
    return convert_return_type(wiced_rtos_set_event_flags(*event, bits));
}


//--------------------------------------------------------------------------------------------------
// cy_rtos_event_clearbits
//--------------------------------------------------------------------------------------------------
cy_rslt_t cy_rtos_event_clearbits(cy_event_t* event, uint32_t bits)
{
    if (event == NULL)
    {
        return CY_RTOS_BAD_PARAM;
    }
    return convert_return_type(wiced_rtos_set_event_flags(*event, ~bits));
}


//--------------------------------------------------------------------------------------------------
// cy_rtos_event_getbits
//--------------------------------------------------------------------------------------------------
cy_rslt_t cy_rtos_event_getbits(cy_event_t* event, uint32_t* bits)
{
    if ((event == NULL) || (bits == NULL))
    {
        return CY_RTOS_BAD_PARAM;
    }
    return CY_RTOS_UNSUPPORTED;
}


//--------------------------------------------------------------------------------------------------
// cy_rtos_event_waitbits
//--------------------------------------------------------------------------------------------------
cy_rslt_t cy_rtos_event_waitbits(cy_event_t* event, uint32_t* bits, bool clear, bool all,
                                 cy_time_t timeout_ms)
{
    uint32_t get_option;

    if ((event == NULL) || (bits == NULL))
    {
        return CY_RTOS_BAD_PARAM;
    }

    return convert_return_type(wiced_rtos_wait_for_event_flags(*event, *bits, bits, clear,
                                                               (all ? WAIT_FOR_ALL_EVENTS :
                                                                WAIT_FOR_ANY_EVENT), timeout_ms));
}


//--------------------------------------------------------------------------------------------------
// cy_rtos_event_deinit
//--------------------------------------------------------------------------------------------------
cy_rslt_t cy_rtos_event_deinit(cy_event_t* event)
{
    if (event == NULL)
    {
        return CY_RTOS_BAD_PARAM;
    }
    return CY_RTOS_UNSUPPORTED;
}


/******************************************************
*                 Queues
******************************************************/

//--------------------------------------------------------------------------------------------------
// cy_rtos_queue_init
//--------------------------------------------------------------------------------------------------
cy_rslt_t cy_rtos_queue_init(cy_queue_t* queue, size_t length, size_t itemsize)
{
    // Keeping "itemsize == 0" to avoid future issues should the 1-byte limitation ever be removed
    if ((queue == NULL) || (itemsize == 0) || (itemsize > MAX_QUEUE_MESSAGE_SIZE))
    {
        return CY_RTOS_BAD_PARAM;
    }

    queue->queue = wiced_rtos_create_queue();
    if (queue->queue == NULL)
    {
        return CY_RTOS_GENERAL_ERROR;
    }

    queue->size = length;
    queue->itemsize = itemsize;
    // WICED RTOS limitation - "message_size : size in bytes of objects that will be held in the
    // queue(currently only 1 byte is acccepted)"
    return convert_return_type(wiced_rtos_init_queue(queue->queue, NULL, 1, length));
}


//--------------------------------------------------------------------------------------------------
// cy_rtos_queue_put
//--------------------------------------------------------------------------------------------------
cy_rslt_t cy_rtos_queue_put(cy_queue_t* queue, const void* item_ptr, cy_time_t timeout_ms)
{
    // Calling from ISR context is not supported
    if ((queue == NULL) || (queue->queue == NULL) || (item_ptr == NULL))
    {
        return CY_RTOS_BAD_PARAM;
    }

    return convert_return_type(wiced_rtos_push_to_queue((queue->queue), item_ptr, timeout_ms));
}


//--------------------------------------------------------------------------------------------------
// cy_rtos_queue_get
//--------------------------------------------------------------------------------------------------
cy_rslt_t cy_rtos_queue_get(cy_queue_t* queue, void* item_ptr, cy_time_t timeout_ms)
{
    uint32_t buffer[MAX_QUEUE_MESSAGE_SIZE];
    // Calling from ISR context is not supported
    if ((queue == NULL) || (queue->queue == NULL) || (item_ptr == NULL))
    {
        return CY_RTOS_BAD_PARAM;
    }

    if (WICED_ERROR == wiced_rtos_pop_from_queue(queue->queue, &buffer, timeout_ms))
    {
        return CY_RTOS_GENERAL_ERROR;
    }
    memcpy(item_ptr, (void*)buffer, queue->itemsize);
    return CY_RSLT_SUCCESS;
}


//--------------------------------------------------------------------------------------------------
// cy_rtos_queue_count
//--------------------------------------------------------------------------------------------------
cy_rslt_t cy_rtos_queue_count(cy_queue_t* queue, size_t* num_waiting)
{
    if ((queue == NULL) || (queue->queue == NULL) || (num_waiting == NULL))
    {
        return CY_RTOS_BAD_PARAM;
    }

    // size_t is unsigned long
    return convert_return_type(wiced_rtos_get_queue_occupancy(queue->queue,
                                                              (uint32_t*)num_waiting));
}


//--------------------------------------------------------------------------------------------------
// cy_rtos_queue_space
//--------------------------------------------------------------------------------------------------
cy_rslt_t cy_rtos_queue_space(cy_queue_t* queue, size_t* num_spaces)
{
    if ((queue == NULL) || (queue->queue == NULL) || (num_spaces == NULL))
    {
        return CY_RTOS_BAD_PARAM;
    }

    // size_t is unsigned long
    if (WICED_SUCCESS != wiced_rtos_get_queue_occupancy(queue->queue, (uint32_t*)num_spaces))
    {
        return CY_RTOS_GENERAL_ERROR;
    }

    *num_spaces = (queue->size - *num_spaces);
    return CY_RSLT_SUCCESS;
}


//--------------------------------------------------------------------------------------------------
// cy_rtos_queue_reset
//--------------------------------------------------------------------------------------------------
cy_rslt_t cy_rtos_queue_reset(cy_queue_t* queue)
{
    if ((queue == NULL) || (queue->queue == NULL))
    {
        return CY_RTOS_BAD_PARAM;
    }
    uint32_t buffer[MAX_QUEUE_MESSAGE_SIZE];
    while (!wiced_rtos_is_queue_empty(queue->queue))
    {
        wiced_rtos_pop_from_queue(queue->queue, &buffer, 0x0);
    }
    return CY_RSLT_SUCCESS;
}


//--------------------------------------------------------------------------------------------------
// cy_rtos_queue_deinit
//--------------------------------------------------------------------------------------------------
cy_rslt_t cy_rtos_queue_deinit(cy_queue_t* queue)
{
    if ((queue == NULL) || (queue->queue == NULL))
    {
        return CY_RTOS_BAD_PARAM;
    }
    // There is a wiced_rtos_deinit_queue, but it hasn't been implemented yet. WICED_RTOS doesn't
    // support properly freeing resources, the only way to regain the resource is to reset the chip
    return CY_RTOS_UNSUPPORTED;
}


/******************************************************
*                 Timers
******************************************************/

//--------------------------------------------------------------------------------------------------
// cy_rtos_timer_init
//--------------------------------------------------------------------------------------------------
cy_rslt_t cy_rtos_timer_init(cy_timer_t* timer, cy_timer_trigger_type_t type,
                             cy_timer_callback_t fun, cy_timer_callback_arg_t arg)
{
    return CY_RTOS_UNSUPPORTED;
}


//--------------------------------------------------------------------------------------------------
// cy_rtos_timer_start
//--------------------------------------------------------------------------------------------------
cy_rslt_t cy_rtos_timer_start(cy_timer_t* timer, cy_time_t num_ms)
{
    return CY_RTOS_UNSUPPORTED;
}


//--------------------------------------------------------------------------------------------------
// cy_rtos_timer_stop
//--------------------------------------------------------------------------------------------------
cy_rslt_t cy_rtos_timer_stop(cy_timer_t* timer)
{
    return CY_RTOS_UNSUPPORTED;
}


//--------------------------------------------------------------------------------------------------
// cy_rtos_timer_is_running
//--------------------------------------------------------------------------------------------------
cy_rslt_t cy_rtos_timer_is_running(cy_timer_t* timer, bool* state)
{
    return CY_RTOS_UNSUPPORTED;
}


//--------------------------------------------------------------------------------------------------
// cy_rtos_timer_deinit
//--------------------------------------------------------------------------------------------------
cy_rslt_t cy_rtos_timer_deinit(cy_timer_t* timer)
{
    return CY_RTOS_UNSUPPORTED;
}


/******************************************************
*                 Time
******************************************************/

// --------------------------------------------------------------------------------------------------
// cy_rtos_time_get
// --------------------------------------------------------------------------------------------------
cy_rslt_t cy_rtos_time_get(cy_time_t* tval)
{
    if (tval == NULL)
    {
        return CY_RTOS_BAD_PARAM;
    }
    return CY_RTOS_UNSUPPORTED;
}


//--------------------------------------------------------------------------------------------------
// cy_rtos_delay_milliseconds
//--------------------------------------------------------------------------------------------------
cy_rslt_t cy_rtos_delay_milliseconds(cy_time_t num_ms)
{
    // Function definition wants a bit more specific implementation, unsure how much translates into
    // wiced_rtos_delay_milliseconds ("thread should sleep for the longest period possible which is
    // less than the delay required, then makes up the difference with a tight loop").

    return convert_return_type(wiced_rtos_delay_milliseconds(num_ms, ALLOW_THREAD_TO_SLEEP));
}
