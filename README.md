# RTOS

A real-time operating system written in C designed for ARM Cortex-M4F based microcontrollers.

## Scheduling

The RTOS uses fixed-priority preemptive scheduling. The RTOS will always run the available task with the highest priority. If multiple tasks at the same priority level are available, they will be time-sliced.

## Using the RTOS

The following files must be copied into your project:
- rtos.h
- rtos.c
- alloc.h (alternatively provide your own alloc.h)

Interrupt priorities must be configured such that **PendSV < SysTick < SVC** where PendSV is set to be the lowest possible priority.

`SysTick_Handler()` must call `rtos_tick()`.

Note that the kernel implements `SVC_Handler()` and `PendSV_Handler()`.

## Demo Project

A project demonstrating the usage of the RTOS is set up in PlatformIO for an STM32-Nucleo board (STM32F401RE) in *demo_project*.

The application has two worker tasks, prints when the timer expires, and prints and flashes the on board LED when the button is pressed.
