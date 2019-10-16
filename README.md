# Cypress RTOS Abstraction

## Overview

The RTOS Abstraction APIs allow middleware to be written to be RTOS aware, but without depending on any particular RTOS.

To use the RTOS Abstraction, simply include a reference to `cyabs_rtos.h`.

## Features

* APIs for interacting with common RTOS Features including:
    * Threads
    * Mutexes
    * Semaphores
    * Timers
    * Queues
    * Events
* Implementations are provided for
    * FreeRTOS
    * RTX (CMSIS RTOS)

## Porting Notes
In order to port to a new environment, the file cyabs_rtos_impl.h must be provided with
definitions of some basic types for the abstraction layer.  The types expected to be defined are:
 
- `cy_thread_t` : typedef from underlying RTOS thread type
- `cy_thread_arg_t` : typedef from the RTOS type that is passed to the entry function of a thread.
- `cy_mutex_t` : typedef from the underlying RTOS mutex type
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

## More information
* [API Reference Guide](https://cypresssemiconductorco.github.io/abstraction-rtos/html/modules.html)
* [Cypress Semiconductor](http://www.cypress.com)
* [Cypress Semiconductor GitHub](https://github.com/cypresssemiconductorco)
* [ModusToolbox](https://www.cypress.com/products/modustoolbox-software-environment)

---
© Cypress Semiconductor Corporation, 2019.
