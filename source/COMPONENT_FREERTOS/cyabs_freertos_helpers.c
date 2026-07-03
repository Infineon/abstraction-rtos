/***********************************************************************************************//**
 * \file cyabs_freertos_helpers.c
 *
 * \brief
 * Provides implementations for functions required to enable static allocation and
 * tickless mode in FreeRTOS.
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
#include "FreeRTOS.h"
#include "task.h"
#include "cyabs_rtos.h"
#include "cy_utils.h"
#if defined(CY_USING_HAL)
#include "cyhal.h"
#elif defined(COMPONENT_MTB_HAL)
#include "mtb_hal.h"
#define cyhal_system_critical_section_enter() mtb_hal_system_critical_section_enter()
#define cyhal_system_critical_section_exit(x) mtb_hal_system_critical_section_exit(x)
#endif

// This is included to allow the user to control the idle task behavior via the configurator
// System->Power->RTOS->System Idle Power Mode setting.
#include "cybsp.h"

#if (configUSE_TICKLESS_IDLE != 0)
uint32_t cyabs_rtos_get_deepsleep_latency(void);
#endif
uint32_t cyabs_rtos_get_sleep_latency(void);

#if (configUSE_TICKLESS_IDLE != 0)
// By default, the device will deep-sleep in the idle task unless if the device
// configurator overrides the behavior to sleep in the System->Power->RTOS->System
// Idle Power Mode setting.
    #if defined(CY_CFG_PWR_MODE_DEEPSLEEP) && \
    (CY_CFG_PWR_SYS_IDLE_MODE == CY_CFG_PWR_MODE_DEEPSLEEP)
        #define ABS_RTOS_DEEPSLEEP_ENABLED
    #elif (defined(CY_CFG_PWR_MODE_DEEPSLEEP_RAM) && \
    (CY_CFG_PWR_SYS_IDLE_MODE == CY_CFG_PWR_MODE_DEEPSLEEP_RAM)) || \
    (defined(CY_CFG_PWR_MODE_HIBERNATE_RAM) && \
    (CY_CFG_PWR_SYS_IDLE_MODE == CY_CFG_PWR_MODE_HIBERNATE_RAM))
// We don't care here about the difference between deep-sleep and deep-sleep RAM
        #define ABS_RTOS_DEEPSLEEP_ENABLED
    #elif (defined(CY_CFG_PWR_MODE_SLEEP) && (CY_CFG_PWR_SYS_IDLE_MODE == CY_CFG_PWR_MODE_SLEEP))
        #define ABS_RTOS_SLEEP_ENABLED
    #endif // if defined(CY_CFG_PWR_MODE_DEEPSLEEP) && (CY_CFG_PWR_SYS_IDLE_MODE ==
// CY_CFG_PWR_MODE_DEEPSLEEP)
// We can support sleep in a non-tickless fashion if there is no LPTimer.
// But we must have it for DeepSleep because the overhead of entering and
// exiting DeepSleep is too high to do so in a tickful fashion.
    #if defined(ABS_RTOS_DEEPSLEEP_ENABLED)
    #if defined(MTB_HAL_API_VERSION) && ((MTB_HAL_API_VERSION) >= 3)
        #if !(defined(MTB_HAL_DRIVER_AVAILABLE_SYSPM)) || (MTB_HAL_DRIVER_AVAILABLE_SYSPM == 0)
            #error "Tickless idle depends on the SysPm HAL driver, but it is not available"
        #endif
    #else
       #if !(defined(CYHAL_DRIVER_AVAILABLE_SYSPM)) || (CYHAL_DRIVER_AVAILABLE_SYSPM == 0)
           #error "Tickless idle depends on the SysPm HAL driver, but it is not available"
       #endif
    #endif // defined(MTB_HAL_API_VERSION) && ((MTB_HAL_API_VERSION) >= 3)

    #if defined(MTB_HAL_API_VERSION) && ((MTB_HAL_API_VERSION) >= 3)
       #if !(defined(MTB_HAL_DRIVER_AVAILABLE_LPTIMER)) || (MTB_HAL_DRIVER_AVAILABLE_LPTIMER == 0)
                #error "Tickless idle depends on the LPTimer HAL driver, but it is not available"
        #endif
    #else
        #if !(defined(CYHAL_DRIVER_AVAILABLE_LPTIMER)) || (CYHAL_DRIVER_AVAILABLE_LPTIMER == 0)
            #error "Tickless idle depends on the LPTimer HAL driver, but it is not available"
        #endif
    #endif // defined(MTB_HAL_API_VERSION) && ((MTB_HAL_API_VERSION) >= 3)
    #endif // defined(ABS_RTOS_DEEPSLEEP_ENABLED)

    #if defined(ABS_RTOS_DEEPSLEEP_ENABLED) || defined(ABS_RTOS_SLEEP_ENABLED)
    #if defined(MTB_HAL_API_VERSION) && ((MTB_HAL_API_VERSION) >= 3)
        #if (defined(MTB_HAL_DRIVER_AVAILABLE_SYSPM) && (MTB_HAL_DRIVER_AVAILABLE_SYSPM)) && \
    (defined(MTB_HAL_DRIVER_AVAILABLE_LPTIMER) && (MTB_HAL_DRIVER_AVAILABLE_LPTIMER))
            #define ABS_RTOS_TICKLESS_ENABLED
        #endif
    #else
        #if (defined(CYHAL_DRIVER_AVAILABLE_SYSPM) && (CYHAL_DRIVER_AVAILABLE_SYSPM)) && \
    (defined(CYHAL_DRIVER_AVAILABLE_LPTIMER) && (CYHAL_DRIVER_AVAILABLE_LPTIMER))
            #define ABS_RTOS_TICKLESS_ENABLED
        #endif
    #endif // defined(MTB_HAL_API_VERSION) && ((MTB_HAL_API_VERSION) >= 3)
    #endif // if defined(ABS_RTOS_DEEPSLEEP_ENABLED) || defined(ABS_RTOS_SLEEP_ENABLED)
#endif // if (configUSE_TICKLESS_IDLE != 0)

#ifndef pdTICKS_TO_MS
#define pdTICKS_TO_MS(xTicks)    ( ( ( TickType_t ) ( xTicks ) * 1000u ) / configTICK_RATE_HZ )
#endif

#if defined(MTB_HAL_API_VERSION) && ((MTB_HAL_API_VERSION) >= 3)
#if defined(MTB_HAL_DRIVER_AVAILABLE_LPTIMER) && (MTB_HAL_DRIVER_AVAILABLE_LPTIMER)
static mtb_hal_lptimer_t* lptimer = NULL;

#if defined(CY_CFG_PWR_MODE_HIBERNATE_RAM)
static cy_en_syspm_hibernate_wakeup_source_t _hibernate_wakeup_source = CY_SYSPM_HIBERNATE_WDT0;

//--------------------------------------------------------------------------------------------------
// cyabs_rtos_set_hibernate_wakeup_source
//--------------------------------------------------------------------------------------------------
/** Sets the wakeup source for hibernate mode.
 *
 * Configures which wakeup source will be used to wake the device from hibernate mode.
 * This setting is applied when the device enters hibernation through the RTOS idle task.
 * The default wakeup source is the Watchdog Timer (CY_SYSPM_HIBERNATE_WDT0).
 *
 * @param[in] source    The wakeup source to use for hibernation. This should be one of the
 *                      cy_en_syspm_hibernate_wakeup_source_t enumeration values.
 *
 *
 * @see cyabs_rtos_get_hibernate_wakeup_source
 */
void cyabs_rtos_set_hibernate_wakeup_source(cy_en_syspm_hibernate_wakeup_source_t source)
{
    _hibernate_wakeup_source = source;
}


//--------------------------------------------------------------------------------------------------
// cyabs_rtos_get_hibernate_wakeup_source
//--------------------------------------------------------------------------------------------------
/** Retrieves the currently configured wakeup source for hibernate mode.
 *
 * Returns the wakeup source that is currently set to wake the device from hibernation.
 * This allows the application to verify the current hibernation configuration.
 *
 * @return The configured wakeup source as a cy_en_syspm_hibernate_wakeup_source_t value.
 *
 * @see cyabs_rtos_set_hibernate_wakeup_source
 */
cy_en_syspm_hibernate_wakeup_source_t cyabs_rtos_get_hibernate_wakeup_source(void)
{
    return _hibernate_wakeup_source;
}


#endif // defined(CY_CFG_PWR_MODE_HIBERNATE_RAM)


//--------------------------------------------------------------------------------------------------
// cyabs_rtos_set_lptimer
//--------------------------------------------------------------------------------------------------
void cyabs_rtos_set_lptimer(mtb_hal_lptimer_t* timer)
{
    lptimer = timer;
}


//--------------------------------------------------------------------------------------------------
// cyabs_rtos_get_lptimer
//--------------------------------------------------------------------------------------------------
mtb_hal_lptimer_t* cyabs_rtos_get_lptimer(void)
{
    return lptimer;
}


#if (configUSE_TICKLESS_IDLE != 0)
//--------------------------------------------------------------------------------------------------
// cyabs_rtos_get_deepsleep_latency
//--------------------------------------------------------------------------------------------------
uint32_t cyabs_rtos_get_deepsleep_latency(void)
{
    uint32_t latency = 0U;

    #if defined(CY_CFG_PWR_DEEPSLEEP_LATENCY)
    latency = CY_CFG_PWR_DEEPSLEEP_LATENCY;
    #endif //defined(CY_CFG_PWR_DEEPSLEEP_LATENCY)

    #if defined (MTB_HAL_API_AVAILABLE_SYSPM_GET_DEEPSLEEP_MODE)
    mtb_hal_syspm_system_deep_sleep_mode_t deep_sleep_mode = mtb_hal_syspm_get_deepsleep_mode();

    switch (deep_sleep_mode)
    {
        case MTB_HAL_SYSPM_SYSTEM_DEEPSLEEP:
        case MTB_HAL_SYSPM_SYSTEM_DEEPSLEEP_OFF:
        case MTB_HAL_SYSPM_SYSTEM_DEEPSLEEP_NONE:
            #if defined(CY_CFG_PWR_DEEPSLEEP_LATENCY)
            latency = CY_CFG_PWR_DEEPSLEEP_LATENCY;
            #endif //defined(CY_CFG_PWR_DEEPSLEEP_LATENCY)
            break;

        case MTB_HAL_SYSPM_SYSTEM_DEEPSLEEP_RAM:
            #if defined(CY_CFG_PWR_DEEPSLEEP_RAM_LATENCY)
            latency = CY_CFG_PWR_DEEPSLEEP_RAM_LATENCY;
            #endif //defined(CY_CFG_PWR_DEEPSLEEP_RAM_LATENCY)
            break;

        default:
            #if defined(CY_CFG_PWR_DEEPSLEEP_LATENCY)
            latency = CY_CFG_PWR_DEEPSLEEP_LATENCY;
            #endif //defined(CY_CFG_PWR_DEEPSLEEP_LATENCY)
            break;
    }
    #endif // if defined (MTB_HAL_API_AVAILABLE_SYSPM_GET_DEEPSLEEP_MODE)
    return latency;
}


#endif //(configUSE_TICKLESS_IDLE != 0)
#endif // defined(MTB_HAL_DRIVER_AVAILABLE_LPTIMER) && (MTB_HAL_DRIVER_AVAILABLE_LPTIMER)
#else // (HAL API <= 2.0)
#if defined(CYHAL_DRIVER_AVAILABLE_LPTIMER) && (CYHAL_DRIVER_AVAILABLE_LPTIMER)
static cyhal_lptimer_t* lptimer = NULL;

//--------------------------------------------------------------------------------------------------
// cyabs_rtos_set_lptimer
//--------------------------------------------------------------------------------------------------
void cyabs_rtos_set_lptimer(cyhal_lptimer_t* timer)
{
    lptimer = timer;
}


//--------------------------------------------------------------------------------------------------
// cyabs_rtos_get_lptimer
//--------------------------------------------------------------------------------------------------
cyhal_lptimer_t* cyabs_rtos_get_lptimer(void)
{
    return lptimer;
}


#if (configUSE_TICKLESS_IDLE != 0)
//--------------------------------------------------------------------------------------------------
// cyabs_rtos_get_deepsleep_latency
//--------------------------------------------------------------------------------------------------
uint32_t cyabs_rtos_get_deepsleep_latency(void)
{
    uint32_t latency = 0U;

    #if defined(CY_CFG_PWR_DEEPSLEEP_LATENCY)
    latency = CY_CFG_PWR_DEEPSLEEP_LATENCY;
    #endif //defined(CY_CFG_PWR_DEEPSLEEP_LATENCY)

    #if defined (CYHAL_API_AVAILABLE_SYSPM_GET_DEEPSLEEP_MODE)
    cyhal_syspm_system_deep_sleep_mode_t deep_sleep_mode = cyhal_syspm_get_deepsleep_mode();

    switch (deep_sleep_mode)
    {
        case CYHAL_SYSPM_SYSTEM_DEEPSLEEP:
        case CYHAL_SYSPM_SYSTEM_DEEPSLEEP_OFF:
        case CYHAL_SYSPM_SYSTEM_DEEPSLEEP_NONE:
            #if defined(CY_CFG_PWR_DEEPSLEEP_LATENCY)
            latency = CY_CFG_PWR_DEEPSLEEP_LATENCY;
            #endif //defined(CY_CFG_PWR_DEEPSLEEP_LATENCY)
            break;

        case CYHAL_SYSPM_SYSTEM_DEEPSLEEP_RAM:
            #if defined(CY_CFG_PWR_DEEPSLEEP_RAM_LATENCY)
            latency = CY_CFG_PWR_DEEPSLEEP_RAM_LATENCY;
            #endif //defined(CY_CFG_PWR_DEEPSLEEP_RAM_LATENCY)
            break;

        default:
            #if defined(CY_CFG_PWR_DEEPSLEEP_LATENCY)
            latency = CY_CFG_PWR_DEEPSLEEP_LATENCY;
            #endif //defined(CY_CFG_PWR_DEEPSLEEP_LATENCY)
            break;
    }
    #endif // if defined (CYHAL_API_AVAILABLE_SYSPM_GET_DEEPSLEEP_MODE)
    return latency;
}


#endif //(configUSE_TICKLESS_IDLE != 0)
#endif //defined(CYHAL_DRIVER_AVAILABLE_LPTIMER) && (CYHAL_DRIVER_AVAILABLE_LPTIMER)
#endif //defined(MTB_HAL_API_VERSION) && ((MTB_HAL_API_VERSION) >= 3)

//--------------------------------------------------------------------------------------------------
// cyabs_rtos_get_sleep_latency
//--------------------------------------------------------------------------------------------------
uint32_t cyabs_rtos_get_sleep_latency(void)
{
    #if defined(CY_CFG_PWR_SLEEP_LATENCY)
    return CY_CFG_PWR_SLEEP_LATENCY;
    #else
    return 0U;
    #endif
}


CY_MISRA_DEVIATE_BLOCK_START("MISRA C-2012 Rule 8.4", 1,
                             "vApplicationGetIdleTaskMemory is a FreeRTOS hook; prototype is in FreeRTOS internal headers not visible here.");
//--------------------------------------------------------------------------------------------------
// vApplicationGetIdleTaskMemory
//
// configSUPPORT_STATIC_ALLOCATION is set to 1, so the application must provide an implementation of
// vApplicationGetIdleTaskMemory() to provide the memory that is used by the Idle task.
//--------------------------------------------------------------------------------------------------
__WEAK void vApplicationGetIdleTaskMemory(StaticTask_t** ppxIdleTaskTCBBuffer,
                                          StackType_t** ppxIdleTaskStackBuffer,
                                          uint32_t* pulIdleTaskStackSize)
{
    // If the buffers to be provided to the Idle task are declared inside this function then they
    // must be declared static - otherwise they will be allocated on the stack and so not exists
    // after this function exits.
    static StaticTask_t xIdleTaskTCB;
    CY_MISRA_DEVIATE_BLOCK_START("MISRA C-2012 Rule 8.13", 1,
                                 "Idle task stack buffer must remain writable because FreeRTOS updates stack contents at runtime.");
    static StackType_t  uxIdleTaskStack[configMINIMAL_STACK_SIZE];
    CY_MISRA_BLOCK_END("MISRA C-2012 Rule 8.13");

    // Pass out a pointer to the StaticTask_t structure in which the Idle task's state will be
    // stored.
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

    // Pass out the array that will be used as the Idle task's stack.
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;

    // Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.  Note that, as the
    // array is necessarily of type StackType_t, configMINIMAL_STACK_SIZE is specified in words, not
    // bytes.
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}


CY_MISRA_BLOCK_END("MISRA C-2012 Rule 8.4");


/*--------------------*/

CY_MISRA_DEVIATE_BLOCK_START("MISRA C-2012 Rule 8.4", 1,
                             "vApplicationGetTimerTaskMemory is a FreeRTOS hook; prototype is in FreeRTOS internal headers not visible here.");
//--------------------------------------------------------------------------------------------------
// vApplicationGetTimerTaskMemory
//
// configSUPPORT_STATIC_ALLOCATION and configUSE_TIMERS are both set to 1, so the application must
// provide an implementation of vApplicationGetTimerTaskMemory() to provide the memory that is used
// by the Timer service task.
//--------------------------------------------------------------------------------------------------
__WEAK void vApplicationGetTimerTaskMemory(StaticTask_t** ppxTimerTaskTCBBuffer,
                                           StackType_t** ppxTimerTaskStackBuffer,
                                           uint32_t* pulTimerTaskStackSize)
{
    // If the buffers to be provided to the Timer task are declared inside this function then they
    // must be declared static - otherwise they will be allocated on the stack and so not exists
    // after this function exits.
    static StaticTask_t xTimerTaskTCB;
    CY_MISRA_DEVIATE_BLOCK_START("MISRA C-2012 Rule 8.13", 1,
                                 "Timer task stack buffer must remain writable because FreeRTOS updates stack contents at runtime.");
    static StackType_t  uxTimerTaskStack[configTIMER_TASK_STACK_DEPTH];
    CY_MISRA_BLOCK_END("MISRA C-2012 Rule 8.13");

    // Pass out a pointer to the StaticTask_t structure in which the Timer task's state will be
    // stored.
    *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;

    // Pass out the array that will be used as the Timer task's stack.
    *ppxTimerTaskStackBuffer = uxTimerTaskStack;

    // Pass out the size of the array pointed to by *ppxTimerTaskStackBuffer.  Note that, as the
    // array is necessarily of type StackType_t, configTIMER_TASK_STACK_DEPTH is specified in words,
    // not bytes.
    CY_MISRA_DEVIATE_BLOCK_START("MISRA C-2012 Rule 10.4", 1,
                                 "configTIMER_TASK_STACK_DEPTH macro expansion mixes unsigned and signed operands.");
    const uint32_t timer_task_stack_depth = configTIMER_TASK_STACK_DEPTH;
    CY_MISRA_BLOCK_END("MISRA C-2012 Rule 10.4");
    *pulTimerTaskStackSize = timer_task_stack_depth;
}


CY_MISRA_BLOCK_END("MISRA C-2012 Rule 8.4");


#if (configUSE_TICKLESS_IDLE != 0)
CY_MISRA_DEVIATE_BLOCK_START("MISRA C-2012 Rule 8.4", 1,
                             "vApplicationSleep is a FreeRTOS hook; prototype is in FreeRTOS internal headers not visible here.");
//--------------------------------------------------------------------------------------------------
// vApplicationSleep
//
/** User defined tickless idle sleep function.
 *
 * Provides a implementation for portSUPPRESS_TICKS_AND_SLEEP macro that allows
 * the device to attempt to deep-sleep for the idle time the kernel expects before
 * the next task is ready. This function disables the system timer and enables low power
 * timer that can operate in deep-sleep mode to wake the device from deep-sleep after
 * expected idle time has elapsed.
 *
 * @param[in] xExpectedIdleTime    Total number of tick periods before
 *                                  a task is due to be moved into the Ready state.
 */
//--------------------------------------------------------------------------------------------------
__WEAK void vApplicationSleep(TickType_t xExpectedIdleTime)
{
    #if defined(ABS_RTOS_TICKLESS_ENABLED)
    uint32_t actual_idle_ms = 0U;
    cy_rslt_t result = CY_RSLT_SUCCESS;
    bool wfi_at_end = false;
    // The application is expected to populate this value by calling
    // `cyabs_rtos_set_lptimer` before the RTOS scheduler is started
    #if !(defined(MTB_HAL_API_VERSION)) || (MTB_HAL_API_VERSION < 3)
    if (NULL == cyabs_rtos_get_lptimer())
    {
        static cyhal_lptimer_t timer;
        result = cyhal_lptimer_init(&timer);
        if (result == CY_RSLT_SUCCESS)
        {
            lptimer = &timer;
        }
        else
        {
            CY_ASSERT(false);
        }
    }
    #endif // !defined(MTB_HAL_API_VERSION) && ((MTB_HAL_API_VERSION) >= 3)

    if (NULL != cyabs_rtos_get_lptimer())
    {
        /* Disable interrupts so that nothing can change the status of the RTOS while
         * we try to go to sleep or deep-sleep.
         */
        uint32_t interrupt_status = cyhal_system_critical_section_enter();
        eSleepModeStatus sleep_status = eTaskConfirmSleepModeStatus();

        if (sleep_status != (eSleepModeStatus)eAbortSleep)
        {
            // If the RTOS says we should sleep, we should WFI at the end of this function unless
            // something else attempts to enter a tickless sleep
            // Note, this is *attempts*, not *succeeds*.  If we determined that we should try to
            // enter tickless, but failed to do so, we want to stay awake and let the RTOS call
            // back into us again if there is time. It is possible that a low power transition
            // is prevented by a transient hardware condition (e.g. a UART not quite done sending)
            // that may resolved itself before a subsequent try.
            wfi_at_end = true;

            uint32_t requested_idle_ms = pdTICKS_TO_MS(xExpectedIdleTime);
            uint32_t sleep_latency = cyabs_rtos_get_sleep_latency();

            #if defined(ABS_RTOS_DEEPSLEEP_ENABLED)
            uint32_t deep_sleep_latency = cyabs_rtos_get_deepsleep_latency();
            if (requested_idle_ms > deep_sleep_latency)
            {
                wfi_at_end = false;
                #if defined(MTB_HAL_API_VERSION) && ((MTB_HAL_API_VERSION) >= 3)
                #if defined(CY_CFG_PWR_MODE_HIBERNATE_RAM)
                if (CY_CFG_PWR_SYS_IDLE_MODE == CY_CFG_PWR_MODE_HIBERNATE_RAM)
                {
                    result = mtb_hal_syspm_tickless_hibernate_ram(
                        cyabs_rtos_get_lptimer(), cyabs_rtos_get_hibernate_wakeup_source(),
                        (requested_idle_ms -
                         deep_sleep_latency),
                        &actual_idle_ms);
                }
                else
                #endif // defined(CY_CFG_PWR_MODE_HIBERNATE_RAM)
                {
                    result = mtb_hal_syspm_tickless_deepsleep(cyabs_rtos_get_lptimer(),
                                                              (requested_idle_ms -
                                                               deep_sleep_latency),
                                                              &actual_idle_ms);
                }
                #else // if defined(MTB_HAL_API_VERSION) && ((MTB_HAL_API_VERSION) >= 3)
                result = cyhal_syspm_tickless_deepsleep(cyabs_rtos_get_lptimer(),
                                                        (requested_idle_ms - deep_sleep_latency),
                                                        &actual_idle_ms);
                #endif // defined(MTB_HAL_API_VERSION) && ((MTB_HAL_API_VERSION) >= 3)
                #if defined(MTB_HAL_SYSPM_RSLT_DEEPSLEEP_LOCKED) || \
                defined(CYHAL_SYSPM_RSLT_DEEPSLEEP_LOCKED)
                #if defined(MTB_HAL_API_VERSION) && ((MTB_HAL_API_VERSION) >= 3)
                if (MTB_HAL_SYSPM_RSLT_DEEPSLEEP_LOCKED == result)
                #else
                if (CYHAL_SYSPM_RSLT_DEEPSLEEP_LOCKED == result)
                #endif // defined(MTB_HAL_API_VERSION) && ((MTB_HAL_API_VERSION) >= 3)
                {
                    // DeepSleep was locked by software. We know that there is no hardware
                    // event that could cause it to be unlocked, and we're in a critical section
                    // so we know that there is no interrupt handler that could unlock it.
                    // So in this specific case, we can safely infer that the most power-efficient
                    // action is to enter Sleep for the entire idle period.
                    if (requested_idle_ms > sleep_latency)
                    {
                        result =
                            #if defined(MTB_HAL_API_VERSION) && ((MTB_HAL_API_VERSION) >= 3)
                            mtb_hal_syspm_tickless_sleep(
                                cyabs_rtos_get_lptimer(), (requested_idle_ms - sleep_latency),
                                &actual_idle_ms);
                            #else
                            cyhal_syspm_tickless_sleep(
                                cyabs_rtos_get_lptimer(), (requested_idle_ms - sleep_latency),
                                &actual_idle_ms);
                        #endif // defined(MTB_HAL_API_VERSION) && ((MTB_HAL_API_VERSION) >= 3)
                    }
                }
                #else // if defined(MTB_HAL_SYSPM_RSLT_DEEPSLEEP_LOCKED) ||
                // defined(CYHAL_SYSPM_RSLT_DEEPSLEEP_LOCKED)
                CY_UNUSED_PARAMETER(sleep_latency);
                #endif // defined(MTB_HAL_SYSPM_RSLT_DEEPSLEEP_LOCKED)
            }
            #endif // defined(ABS_RTOS_DEEPSLEEP_ENABLED)
            #if defined(ABS_RTOS_SLEEP_ENABLED)
            #if defined(ABS_RTOS_DEEPSLEEP_ENABLED)
            // If we tried to DeepSleep, we don't want to also try to sleep. Either we
            // went to DeepSleep and then were woken by an interrupt (possibly prematurely),
            // or we tried to DeepSleep and were rejected by hardware not being ready
            // (which might now be ready if we try DeepSleep again). In either of those cases,
            // we should not also try to enter Sleep; we should return from this function
            // and let the RTOS scheduler sort out whether to call us again.
            else
            #endif // defined(ABS_RTOS_DEEPSLEEP_ENABLED)
            if (requested_idle_ms > sleep_latency)
            {
                wfi_at_end = false;
                #if defined(MTB_HAL_API_VERSION) && ((MTB_HAL_API_VERSION) >= 3)
                result = mtb_hal_syspm_tickless_sleep(
                    cyabs_rtos_get_lptimer(), (requested_idle_ms - sleep_latency),
                    &actual_idle_ms);
                #else
                result = cyhal_syspm_tickless_sleep(
                    cyabs_rtos_get_lptimer(), (requested_idle_ms - sleep_latency),
                    &actual_idle_ms);
                #endif // defined(MTB_HAL_API_VERSION) && ((MTB_HAL_API_VERSION) >= 3)
            }
            #endif // if defined(ABS_RTOS_SLEEP_ENABLED)

            CY_UNUSED_PARAMETER(result);
            // The return value of tickless sleep is disregarded since SysTick timer is stopped,
            // before sleep, regardless of the sleep's success or failure, therefore idle time
            // must be updated in any case.
            if (actual_idle_ms > 0U)
            {
                // If you hit this assert, the latency time (CY_CFG_PWR_DEEPSLEEP_LATENCY) should
                // be increased. This can be set though the Device Configurator, or by manually
                // defining the variable in cybsp.h for the TARGET platform.
                CY_ASSERT(actual_idle_ms <= pdTICKS_TO_MS(xExpectedIdleTime));
                vTaskStepTick(convert_ms_to_ticks(actual_idle_ms));
            }
        }
        cyhal_system_critical_section_exit(interrupt_status);
    }
    else
    {
        /* If LPtimer is not defined, only do a WFI to preserve compatibility */
        wfi_at_end = true;
    }

    if (true == wfi_at_end)
    #else // defined(ABS_RTOS_TICKLESS_ENABLED)
    CY_UNUSED_PARAMETER(xExpectedIdleTime);
    #endif // defined(ABS_RTOS_TICKLESS_ENABLED)
    {
        __WFI();
    }
}


CY_MISRA_BLOCK_END("MISRA C-2012 Rule 8.4");


#endif // (configUSE_TICKLESS_IDLE != 0)
