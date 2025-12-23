#pragma once

#include <stdint.h>

static inline uint32_t cm4_get_control(void) {
    uint32_t control;
    __asm volatile("mrs %0, control" : "=r"(control));
    return control;
}

static inline void cm4_set_control(uint32_t control) {
    __asm volatile("msr control, %0" : : "r"(control));
}

static const uint32_t cm4_control_npriv_mask = 1U << 0U;

// Interrupt control and state register
static volatile uint32_t *const cm4_icsr = (volatile uint32_t *)0xE000ED04U;

static const uint32_t cm4_icsr_pendsvset_mask = 1U << 28U;

// Floating point context control register
static volatile uint32_t *const cm4_fpcsr = (volatile uint32_t *)0xE000EF34U;

static const uint32_t cm4_fpcsr_aspen_mask = 1U << 31U;
static const uint32_t cm4_fpcsr_lspen_mask = 1U << 30U;

// Return to thread mode, use PSP, no FP context
static const uint32_t cm4_exc_return_thread_psp_nofp = 0xFFFFFFFDU;

static const uint32_t cm4_epsr_thumb_mask = 1U << 24U;

static void cm4_wait_for_interrupt(void) {
    __asm volatile("wfi");
}

static void cm4_enable_irq(void) {
    __asm volatile("cpsie i");
}

// Equivalent to writing 0 to PRIMASK, which raises the execution priority to
// 0, effectively disabling all interrupts. Note that Hardfault, NMI, and
// Reset exceptions still have higher priority.
static void cm4_disable_irq(void) {
    __asm volatile("cpsid i");
}
