/***********************************************************************************************//**
 * \file cyabs_rtos_types.h
 *
 * \brief
 * Internal type definitions for RTOS abstraction layer
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

#ifndef CYABS_RTOS_TYPES_H
#define CYABS_RTOS_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include "tx_api.h"

#ifdef __cplusplus
extern "C"
{
#endif

#include "cyabs_rtos_device_caps.h"
#if !defined (CY_RTOS_IS_H1_DEVICE)
#include <cmsis_compiler.h>
#else
#include "cyabs_rtos_impl_h1.h"
#endif

/******************************************************
*                 Type Definitions
******************************************************/

#if !defined (CY_RTOS_IS_H1_DEVICE)
// RTOS thread priority. See /ref cy_thread_priority_t in the
// cyabs_rtos_impl_h1.h for the Hatchet 1 thread priority definitions
typedef enum
{
    CY_RTOS_PRIORITY_MIN         = TX_MAX_PRIORITIES - 1,         /**< Minimum allowable Thread
                                                                     priority */
    CY_RTOS_PRIORITY_LOW         = (TX_MAX_PRIORITIES * 6 / 7),   /**< A low priority Thread */
    CY_RTOS_PRIORITY_BELOWNORMAL = (TX_MAX_PRIORITIES * 5 / 7),   /**< A slightly below normal
                                                                     Thread priority */
    CY_RTOS_PRIORITY_NORMAL      = (TX_MAX_PRIORITIES * 4 / 7),   /**< The normal Thread priority */
    CY_RTOS_PRIORITY_ABOVENORMAL = (TX_MAX_PRIORITIES * 3 / 7),   /**< A slightly elevated Thread
                                                                     priority */
    CY_RTOS_PRIORITY_HIGH        = (TX_MAX_PRIORITIES * 2 / 7),   /**< A high priority Thread */
    CY_RTOS_PRIORITY_REALTIME    = (TX_MAX_PRIORITIES * 1 / 7),   /**< Realtime Thread priority */
    CY_RTOS_PRIORITY_MAX         = 0                              /**< Maximum allowable Thread
                                                                     priority */
} cy_thread_priority_t;
#endif // if !defined (CY_RTOS_IS_H1_DEVICE)

typedef struct
{
    uint32_t     maxcount;
    TX_SEMAPHORE tx_semaphore;
} cy_semaphore_t;

typedef struct
{
    ULONG* mem;
    // ThreadX buffer size is a power of 2 times word size,
    // this is used to prevent memory corruption when get message from queue.
    size_t   itemsize;
    TX_QUEUE tx_queue;
} cy_queue_t;

typedef struct
{
    bool     oneshot;
    TX_TIMER tx_timer;
} cy_timer_t;

typedef TX_THREAD*              cy_thread_t;
typedef ULONG                   cy_thread_arg_t;
typedef TX_MUTEX                cy_mutex_t;
typedef TX_EVENT_FLAGS_GROUP    cy_event_t;
typedef ULONG                   cy_timer_callback_arg_t;
typedef uint32_t                cy_time_t;
typedef UINT                    cy_rtos_error_t;

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* CYABS_RTOS_TYPES_H */
