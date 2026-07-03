/***********************************************************************************************//**
 * \file cyabs_rtos_impl.h
 *
 * \brief
 * Internal definitions for RTOS abstraction layer
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

#ifndef CYABS_RTOS_IMPL_H
#define CYABS_RTOS_IMPL_H

#include <FreeRTOS.h>
#include <semphr.h>
#include <task.h>
#include <event_groups.h>
#include <timers.h>
#include "stdbool.h"
#include "cyabs_rtos_device_caps.h"
#if !defined (CY_RTOS_IS_H1_DEVICE)
#include <cmsis_compiler.h>
#endif
#include "cyabs_rtos_types.h"
#include "cyabs_rtos_hal_impl.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
*                 Constants
******************************************************/
#define CY_RTOS_MIN_STACK_SIZE      300U           /**< Minimum stack size in bytes */
#define CY_RTOS_ALIGNMENT           0x00000008UL   /**< Minimum alignment for RTOS objects */
#define CY_RTOS_ALIGNMENT_MASK      0x00000007UL   /**< Mask for checking the alignment of
                                                        created RTOS objects */
#if !defined(CY_RTOS_MAX_SUSPEND_NESTING)
#define CY_RTOS_MAX_SUSPEND_NESTING 3U             /**< Maximum nesting allowed for calls
                                                        to scheduler suspend from ISR */
#endif

cy_time_t convert_ms_to_ticks(cy_time_t timeout_ms);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* CYABS_RTOS_IMPL_H */
