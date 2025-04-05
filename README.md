# RTOS

A real-time operating system written in C designed for ARM Cortex-M4F based
microcontrollers.

## Scheduling

The RTOS uses multi-priority preemptive scheduling. The RTOS will always run
the available task with the highest priority. If multiple tasks at the same 
priority level are available, they will be time-sliced.

## Using the RTOS

Interrupt priorities must be configured such that **PendSV < SysTick < SVC**
where PendSV is set to be the lowest possible priority.

`SysTick_Handler()` must call `rtos_tick()`.

Note that the RTOS implements `SVC_Handler()` and `PendSV_Handler()`.
