/***********************************************************************************************//**
 * \file cyabs_rtos_impl.h
 *
 * \brief
 * Internal definitions for RTOS abstraction layer
 *
 ***************************************************************************************************
 * \copyright
 * Copyright 2019-2021 Cypress Semiconductor Corporation (an Infineon company) or
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

#pragma once

#include "wiced_rtos.h"
#include "wiced_timer.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * \addtogroup group_abstraction_rtos_port RTOS Specific Types and Defines
 * \ingroup group_abstraction_rtos_common
 * \{
 * The following defines and types have values that are specific to each RTOS port.
 * The define values are specific to each RTOS. The types are simple aliases that
 * wrap RTOS specifc types. Code cannot assume anything about the values or internals
 * of any types.
 */

/******************************************************
*                 Constants
******************************************************/
/** WICED RTOS doesn't define or mention these, so using default values */
#define CY_RTOS_MIN_STACK_SIZE      300                     /**< Minimum stack size */
#define CY_RTOS_ALIGNMENT_MASK      0x00000007UL            /**< Checks for 8-bit alignment */

/******************************************************
*                 Type Definitions
******************************************************/

/** RTOS thread priority. WICED RTOS rejects priorities above 7 */
typedef enum
{
    CY_RTOS_PRIORITY_MIN         = 7,
    CY_RTOS_PRIORITY_LOW         = 6,
    CY_RTOS_PRIORITY_BELOWNORMAL = 5,
    CY_RTOS_PRIORITY_NORMAL      = 4,
    CY_RTOS_PRIORITY_ABOVENORMAL = 3,
    CY_RTOS_PRIORITY_HIGH        = 2,
    CY_RTOS_PRIORITY_REALTIME    = 1,
    CY_RTOS_PRIORITY_MAX         = 0
} cy_thread_priority_t;


/** WICED_RTOS interface functions assume all types are void*, the real types are inaccessible. */
typedef void* cy_thread_t;                  /**< WICED_RTOS thread object pointer */
typedef uint32_t cy_thread_arg_t;           /**< Thread entry function argument */
typedef void* cy_mutex_t;                   /**< WICED_RTOS mutex object pointer */
/**
 * @brief Semaphore object
 */
typedef struct
{
    void* semaphore;                        /**< WICED semaphore object pointer */
    uint32_t max_count;                     /**< Max semaphore value */
    uint32_t count;                         /**< Current semaphore value */
} cy_semaphore_t;
typedef void* cy_event_t;                   /**< WICED_RTOS event object pointer */
/**
 * @brief Queue object
 */
typedef struct
{
    uint32_t itemsize;                      /**< Size of each item */
    uint32_t size;                          /**< Max number of items */
    void* queue;                            /**< WICED queue object pointer */
} cy_queue_t;
typedef void* cy_timer_t;                   /**< WICED_RTOS timer object pointer */
typedef uint32_t cy_timer_callback_arg_t;   /**< Timer callback function argument */
typedef uint32_t cy_time_t;                 /**< Time unit */
typedef uint16_t cy_rtos_error_t;           /**< Error status */

/** \} group_abstraction_rtos_port */

#ifdef __cplusplus
} // extern "C"
#endif
