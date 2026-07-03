
# Abstraction-RTOS layer Middleware

# Overview

The RTOS abstraction layer provides simple RTOS services like threads, semaphores, mutexes, queues, and timers. It is not intended to be a full-featured RTOS interface, but to provide just enough support to allow for RTOS independent drivers and middleware. This allows middleware applications to be as portable as possible within ModusToolbox™. This library provides a unified API around the actual RTOS. This allows middleware libraries to be written once independent of the RTOS actually selected for the application. While the primary purpose of the library is for middleware, the abstraction layer can be used by the application code. However, since this API does not provide all RTOS features and the application generally knows what RTOS is being used, this is typically an unnecessary overhead. 


# Features

The Abstraction-RTOS middleware allows other middleware applications to be as portable as possible within ModusToolbox™. This library provides a unified API around the actual RTOS. This allows middleware libraries to be written once independent of the RTOS actually selected for the application.   
* APIs for interacting with common RTOS features including:
    * Threads
    * Mutexes
    * Semaphores
    * Timers
    * Queues
    * Events
    * Time management
    * Scheduler
* Worker Thread utility for running callbacks in a dedicated thread
* Implementations are provided for
    * FreeRTOS (FreeRTOS Kernel)
    * RTX (CMSIS-RTX)
    * ThreadX (Eclipse ThreadX)

# When to Use

Use Abstraction-RTOS middleware if you want to allow other middleware applications to be as portable as possible within ModusToolbox™, without dependency on a particular RTOS.

## Use Cases

* **Portable middleware development**: This is the primary use case of Abstraction-RTOS. The main idea is to give dependent middleware libraries maximum portability and independence from any specific RTOS. A middleware library that uses this abstraction can be integrated into any application regardless of which RTOS is selected, with no changes to the middleware source code. 

* **RTOS migration**: Using Abstraction-RTOS allows building an application that is independent of any specific RTOS. This makes it possible to switch from one RTOS to another with minimal effort — only the `COMPONENTS` makefile variable needs to be updated, while the application and middleware code remain unchanged.

# How to use

## Prerequisites
### Software Requirements
Abstraction-RTOS supports three popular kernels, but does not include them inside the asset.
Those kernels need to be downloaded separately into the APP folder.
Download links:
* [RTX (CMSIS-RTX)](https://github.com/ARM-software/CMSIS-RTX)
* [ThreadX (Eclipse ThreadX)](https://github.com/eclipse-threadx/threadx)

Note: No need to download FreeRTOS separately. It exists as an Infineon® library asset and can be added via Library Manager.  


### Configure Makefile
To connect any RTOS to the APP, add the chosen RTOS to the `COMPONENTS` parameter in the APP's main `Makefile`. For example:
```makefile
# Select one of the following:
COMPONENTS+=RTX
# or
COMPONENTS+=FREERTOS
# or
COMPONENTS+=THREADX
```

### Add Headers
You also need to include `cyabs_rtos.h` in your APP to use the Abstraction-RTOS APIs.

## Getting Started


**Step 1:** Create an empty project for your device.

**Step 2:** In Library Manager, select Abstraction-RTOS to add it to your APP. Wait until the update is finished.

**Step 3:** Also select FreeRTOS in Library Manager, or add another RTOS kernel to the project.

**Step 4:** Add the code below. Build and program.


```c
#include "cybsp.h"
#include "cyabs_rtos.h"

/* USER LED toggle period in milliseconds */
#define USER_LED_TOGGLE_PERIOD_MS   1000u

static cy_thread_t my_thread;

static void my_thread_func(cy_thread_arg_t arg)
{
    (void)arg;

    for (;;)
    {
        /* Toggle the USER LED state */
        Cy_GPIO_Inv(CYBSP_USER_LED_PORT, CYBSP_USER_LED_PIN);

        /* Thread body: add application logic here */
        cy_rtos_delay_milliseconds(USER_LED_TOGGLE_PERIOD_MS);
    }
}

int main(void)
{
    cy_rslt_t result;

    /* Initialize the device and board peripherals */
    result = cybsp_init();
    CY_ASSERT(result == CY_RSLT_SUCCESS);

    __enable_irq();

    /* Create the task. Let Abstraction-RTOS allocate an aligned stack. */
    result = cy_rtos_thread_create(&my_thread,
                                   my_thread_func,
                                   "MyThread",
                                   NULL,
                                   1024u,
                                   CY_RTOS_PRIORITY_NORMAL,
                                   0u);
    CY_ASSERT(result == CY_RSLT_SUCCESS);

    /* Start the RTOS scheduler - does not return. */
    vTaskStartScheduler();

    return 0;
}
```

**Step 5:** The LED starts blinking.



## RTOS Configuration Requirements
### FreeRTOS
To enable all functionality when using with FreeRTOS, the following configuration options must be enabled in FreeRTOSConfig.h:
* configSUPPORT_DYNAMIC_ALLOCATION
* configSUPPORT_STATIC_ALLOCATION
* configUSE_COUNTING_SEMAPHORES
* configUSE_MUTEXES
* configUSE_NEWLIB_REENTRANT
* configUSE_RECURSIVE_MUTEXES
* configUSE_TASK_NOTIFICATIONS
* configUSE_TICKLESS_IDLE
* configUSE_TIMERS
* configUSE_TRACE_FACILITY

* INCLUDE_vTaskDelay
* INCLUDE_vTaskDelete
* INCLUDE_vTaskPrioritySet
* INCLUDE_uxTaskPriorityGet
* INCLUDE_xTimerPendFunctionCall
* INCLUDE_vTaskSuspend

Enabling configSUPPORT_STATIC_ALLOCATION requires the application to provide implementations for `vApplicationGetIdleTaskMemory` and
`vApplicationGetTimerTaskMemory` functions. Weak implementations for these functions are provided as a part of this library. These can
be overridden by the application if custom implementations of these functions are desired.<br>

#### Low Power Configuration
This library provides an API `vApplicationSleep` which can be used to enable tickless support in FreeRTOS. In order to enable tickless mode with this API, the following changes need to be made in `FreeRTOSConfig.h`:
* Enables tickless mode with user specified `portSUPPRESS_TICKS_AND_SLEEP` implementation.<br>
\c \#define `configUSE_TICKLESS_IDLE                       2`
* Hook `portSUPPRESS_TICKS_AND_SLEEP` macro to `vApplicationSleep` implementation.<br>
\c \#define `portSUPPRESS_TICKS_AND_SLEEP( xIdleTime )    vApplicationSleep( xIdleTime )`

In tickless mode `vApplicationSleep` updates the number of RTOS ticks that have passed since the tick interrupt was stopped using `vTaskStepTick`, taking into account the sleep/deepsleep latency.
* The maximum tickless idle time can be calculated as follows:
    * `MaxIdleTime = LPtimerDelay + latency` where: `latency = PreSleepLatency + PostSleepLatency`, `LPtimerDelay = xExpectedIdleTime - latency`. 

    **Note**: The `LPtimerDelay` represents the duration in which the IP is configured to Wake-up the device. Wake-up can happen before its expiration if one other configured interrupt triggers during this time.

* In case of `xExpectedIdleTime <= latency` WFI is executed instead, and normal sleep is expected to be reached (SCR->SLEEPDEEP = 0).

* The actual idle time for which the SysTick timer is disabled is always updated in vTaskStepTick to keep the RTOS always up to date, independently of whether the tickless sleep is reached successfully or not. The reason for this is that in the sequence of operations to reach sleep, the SysTick is disabled before the (deep)sleep callbacks are called, so if one of them does not allow (deep)sleep, the ticks for which the check is performed still need to be counted in the RTOS.

Functions cy_rtos_scheduler_suspend/cy_rtos_scheduler_resume can be called from ISR but calls need to be paired to restore the saved interrupt status correctly so a structure to save these values has been implemented.
The size of this structure can be controlled with CY_RTOS_MAX_SUSPEND_NESTING. This macro is overridable and its default value is 3.

For further details on Low power support in FreeRTOS please refer to documentation [here](https://www.freertos.org/low-power-tickless-rtos.html)

#### Known Limitations
The cy_rtos_event_* functions accept a 32-bit event as an argument. However, FreeRTOS requires 8-bits(`configUSE_16_BIT_TICKS` == 1) or 24-bits (`configUSE_16_BIT_TICKS` == 0) to run successfully. Hence the application should not reference these unsupported bits. Check the FreeRTOS implementation of `eventEVENT_BITS_CONTROL_BYTES` in event_group.c for internal details.

### RTX / ThreadX
No specific requirements exist

## Porting Notes
In order to port to a new environment, the file cyabs_rtos_impl.h must be provided with definitions of some basic types for the abstraction layer.  The types expected to be defined are:

- `cy_thread_t` : typedef from underlying RTOS thread type
- `cy_thread_arg_t` : typedef from the RTOS type that is passed to the entry function of a thread.
- `cy_mutex_t` : typedef from the underlying RTOS mutex type
- `cy_semaphore_t`: typedef from the underlying RTOS semaphore type
- `cy_event_t` : typedef from the underlying RTOS event type
- `cy_queue_t` : typedef from the underlying RTOS queue type
- `cy_timer_callback_arg_t` : typedef from the RTOS type that is passed to the timer callback function
- `cy_timer_t` : typedef from the underlying RTOS timer type
- `cy_time_t` : count of time in milliseconds
- `cy_rtos_error_t` : typedef from the underlying RTOS error type

The enum `cy_thread_priority_t` needs to have the following priority values defined and mapped to RTOS specific values:
- `CY_RTOS_PRIORITY_MIN`
- `CY_RTOS_PRIORITY_LOW`
- `CY_RTOS_PRIORITY_BELOWNORMAL`
- `CY_RTOS_PRIORITY_NORMAL`
- `CY_RTOS_PRIORITY_ABOVENORMAL`
- `CY_RTOS_PRIORITY_HIGH`
- `CY_RTOS_PRIORITY_REALTIME`
- `CY_RTOS_PRIORITY_MAX`

Finally, the following macros need to be defined for memory allocations:
- `CY_RTOS_MIN_STACK_SIZE`
- `CY_RTOS_ALIGNMENT`
- `CY_RTOS_ALIGNMENT_MASK`

# Industry Standards and Compliance

Coding Standards:

* MISRA C:2012

## MISRA-C:2012 Compliance

This section describes MISRA-C:2012 compliance and deviations for the Abstraction-RTOS.

MISRA stands for Motor Industry Software Reliability Association. The MISRA specification covers a set of 10 mandatory rules, 110 required rules and 39 advisory rules that apply to firmware design and has been put together by the Automotive Industry to enhance the quality and robustness of the firmware code embedded in automotive devices.

The MISRA specification defines two categories of deviations (see section 5.4 of the MISRA-C:2012 specification):

* Project Deviations - deviations that are applicable for particular class of circumstances.
* Specific Deviations - deviations that are applicable for a single instance in a single file.

Project Deviations are documented in this section.
Specific deviations are documented in the source code, close to the deviation occurrence. For each deviation a special macro identifies the relevant rule or directive number, and reason.

### Verification Environment

This section provides MISRA compliance analysis environment description.

| Item | Description | Version |
| ---- | ----------- | ------- |
| Test Specification | MISRA-C:2012 Guidelines for the use of the C language in critical systems | March 2013 |
| MISRA Checking Tool | Coverity Static Analysis Tool | 2022.12.0 |

### Project Deviation

| MISRA Rule | Rule Description | Rationale |
| ---------- | ---------------- | --------- |
| Rule 3.1 | The character sequences /* and // shall not be used within a comment. | Using of the special comment symbols is need for Doxygen comment support, it does not have any impact. |

### Specific Deviation

Specific deviations for the following required rules are documented inline in the source code using `CY_MISRA_DEVIATE_*` macroses.

# Release Notes and Changelog
- RELEASE.md - Detailed release notes for all versions

# More information
* [Reference Guide](https://infineon.github.io/abstraction-rtos/html/modules.html)
* [FreeRTOS Kernel Page](https://github.com/FreeRTOS/FreeRTOS-Kernel)
* [RTX Page](https://github.com/ARM-software/CMSIS-RTX)
* [ThreadX Page](https://github.com/eclipse-threadx/threadx)
* [ModusToolbox Software Environment, Quick Start Guide, Documentation, and Videos](https://www.infineon.com/cms/en/design-support/tools/sdk/modustoolbox-software/)
* [ModusToolbox Introduction](https://documentation.infineon.com/modustoolbox/docs/zhf1731521206417/index.html)
* [Code Examples for ModusToolbox Software](https://github.com/Infineon/Code-Examples-for-ModusToolbox-Software)
* [Infineon Technologies AG](https://www.infineon.com)

# License
This library is released under the Apache License, Version 2.0. See the individual source files for the SPDX-License-Identifier or visit https://www.apache.org/licenses/LICENSE-2.0 for the full license text.

---

## Copyright
(c) Copyright 2019-2026, Infineon Technologies AG, or an affiliate of Infineon Technologies AG.  All rights reserved.
