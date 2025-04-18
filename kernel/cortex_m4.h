#pragma once

#include <stddef.h>

// Interrupt control and state register
static volatile size_t *const cm4_icsr = (volatile size_t *)0xE000ED04U;

static const size_t cm4_icsr_pendsvset_mask = 1U << 28U;

// Floating point context control register
static volatile size_t *const fpcsr = (volatile size_t *)0xE000EF34U;

static const size_t fpcsr_aspen_mask = 1U << 31U;
static const size_t fpcsr_lspen_mask = 1U << 30U;

// Return to thread mode, use PSP, no FP context
static const size_t cm4_exc_return_thread_psp_nofp = 0xFFFFFFFDU;

static const size_t cm4_epsr_thumb_mask = 1U << 24U;

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
