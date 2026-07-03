/***********************************************************************************************//**
 * \file cyabs_freertos_dsram.c
 *
 * \brief
 * Provides implementations for functions required to enable deepsleep ram.
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
#include <cmsis_compiler.h> // For __WEAK
#include <cy_utils.h>
#include "FreeRTOS.h"
#include "cyabs_rtos.h"
#include "cyabs_rtos_dsram.h"
#if defined(CY_RTOS_HAS_DEEPSLEEP_RAM)
#include "cy_syslib.h"
#endif

CY_MISRA_FP_BLOCK_START("MISRA C-2012 Rule 2.2", 2,
                        "Empty weak DSRAM hook implementations are intentional extension points that can be overridden by the application or BSP.");

CY_MISRA_FP_BLOCK_START("MISRA C-2012 Rule 8.6", 1,
                        "vPortSetupTimerInterrupt is implemented in the FreeRTOS port layer; this extern declaration is required for cross-module linkage.");
void vPortSetupTimerInterrupt(void);
CY_MISRA_BLOCK_END("MISRA C-2012 Rule 8.6");

#if defined(CY_RTOS_HAS_DEEPSLEEP_RAM)
//--------------------------------------------------------------------------------------------------
// vStoreDSRAMContextWithWFI
//--------------------------------------------------------------------------------------------------
__WEAK void vStoreDSRAMContextWithWFI(void)
{
}


//--------------------------------------------------------------------------------------------------
// vRestoreDSRAMContext
//--------------------------------------------------------------------------------------------------
__WEAK void vRestoreDSRAMContext(void)
{
}


CY_RAMFUNC_BEGIN
//--------------------------------------------------------------------------------------------------
// Cy_SysPm_StoreDSContext_Wfi
// Cy_SysPm_StoreDSContext_Wfi is defined as a weak function in pdl.
// This implementation under abstraction rtos implements FreeRTOS
// specific context store required for deep sleep entry.
//--------------------------------------------------------------------------------------------------
void Cy_SysPm_StoreDSContext_Wfi(void)
{
    System_Store_NVIC_Reg();
    /* Clear the Warm Boot Entry status Flag */
    #if !defined(CYBSP_FREERTOS_DSRAM_WARMBOOT_SUPPORT) || \
    (defined(CYBSP_FREERTOS_DSRAM_WARMBOOT_SUPPORT) && \
    (CYBSP_FREERTOS_DSRAM_WARMBOOT_SUPPORT))
    Cy_SysLib_ClearDSRAMWarmBootEntryStatus();
    #endif \
    // !defined(CYBSP_FREERTOS_DSRAM_WARMBOOT_SUPPORT) ||
    // (defined(CYBSP_FREERTOS_DSRAM_WARMBOOT_SUPPORT) &&
    // (CYBSP_FREERTOS_DSRAM_WARMBOOT_SUPPORT)
    vStoreDSRAMContextWithWFI();
    System_Restore_NVIC_Reg();
}


CY_RAMFUNC_END


//--------------------------------------------------------------------------------------------------
// cyabs_dsram_exit_dsram
//--------------------------------------------------------------------------------------------------
__WEAK void cyabs_rtos_exit_dsram(void)
{
    vPortSetupTimerInterrupt();

    vRestoreDSRAMContext();
}


CY_MISRA_BLOCK_END("MISRA C-2012 Rule 2.2");

#endif \
    // if defined(CY_RTOS_HAS_DEEPSLEEP_RAM)
