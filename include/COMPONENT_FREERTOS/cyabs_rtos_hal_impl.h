/***********************************************************************************************//**
 * \file cyabs_rtos_hal_impl.h
 *
 * \brief
 * Internal definitions for RTOS abstraction layer for working with HAL
 *
 ***************************************************************************************************
 * \copyright
 * (c) 2019-2026, Infineon Technologies AG, or an affiliate of Infineon
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

#ifndef CYABS_RTOS_HAL_IMPL_H
#define CYABS_RTOS_HAL_IMPL_H

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

#if defined(CY_USING_HAL)
#include "cyhal.h"
#elif defined(COMPONENT_MTB_HAL)
#include "mtb_hal.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CYHAL_DRIVER_AVAILABLE_LPTIMER) && (CYHAL_DRIVER_AVAILABLE_LPTIMER)
/** Stores a reference to an lptimer instance for use with vApplicationSleep().
 *
 * @param[in] timer  Pointer to the lptimer handle
 */
void cyabs_rtos_set_lptimer(cyhal_lptimer_t* timer);

/** Gets a reference to the lptimer instance object used by vApplicationSleep(). This instance is
 * what was explicitly set by @ref cyabs_rtos_set_lptimer or, if none was set, what was
 * automatically allocated by the first call to vApplicationSleep().
 *
 * @return Pointer to the lptimer handle
 */
cyhal_lptimer_t* cyabs_rtos_get_lptimer(void);

#endif // defined(CYHAL_DRIVER_AVAILABLE_LPTIMER) && (CYHAL_DRIVER_AVAILABLE_LPTIMER)

#if defined(MTB_HAL_DRIVER_AVAILABLE_LPTIMER) && (MTB_HAL_DRIVER_AVAILABLE_LPTIMER)
/** Stores a reference to an lptimer instance for use with vApplicationSleep().
 *
 * @param[in] timer  Pointer to the lptimer handle
 */
void cyabs_rtos_set_lptimer(mtb_hal_lptimer_t* timer);

/** Gets a reference to the lptimer instance object used by vApplicationSleep(). This instance is
 * what was explicitly set by @ref cyabs_rtos_set_lptimer or, if none was set, what was
 * automatically allocated by the first call to vApplicationSleep().
 *
 * @return Pointer to the lptimer handle
 */
mtb_hal_lptimer_t* cyabs_rtos_get_lptimer(void);

#endif // defined(MTB_HAL_DRIVER_AVAILABLE_LPTIMER) && (MTB_HAL_DRIVER_AVAILABLE_LPTIMER)

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* CYABS_RTOS_HAL_IMPL_H */
