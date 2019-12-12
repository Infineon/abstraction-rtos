### Cypress RTOS Abstraction Library 1.1
The RTOS Abstraction APIs allow middleware to be written to be RTOS aware, but not need to care about what the actual RTOS is.
 
### What's New In This Release?
* Fixed an issue with the FreeRTOS implementation where it would always allocate memory for the thread stack, even if a pre-allocated buffer was provided.
* Removed usage of assert() and replaced with CY_ASSERT()

### What's Included?
The this release of the RTOS Abstraction API includes support for the following:
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

### Supported Software and Tools
This version of the RTOS Abstraction API was validated for compatibility with the following Software and Tools:

| Software and Tools                        | Version |
| :---                                      | :----:  |
| ModusToolbox Software Environment         | 2.0     |
| GCC Compiler                              | 7.4     |
| IAR Compiler                              | 8.32    |
| ARM Compiler                              | 6.11    |


### More information
Use the following links for more information, as needed:
* [API Reference Guide](https://cypresssemiconductorco.github.io/abstraction-rtos/html/modules.html)
* [Cypress Semiconductor](http://www.cypress.com)
* [Cypress Semiconductor GitHub](https://github.com/cypresssemiconductorco)
* [ModusToolbox](https://www.cypress.com/products/modustoolbox-software-environment)

---
© Cypress Semiconductor Corporation, 2019.