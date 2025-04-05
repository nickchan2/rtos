#pragma once

#include <assert.h>
#include <stddef.h>

// Context switch stack frame:
// See 2-27
// 
// ----- Saved on exception entry only if using FP
// FPSCR
// S15
// ...
// S0
//
// ----- Always saved on exception entry
// xPSR
// PC
// LR
// R12
// R3
// ...
// R0
// 
// ----- Always saved by context switch handler
// R11
// ...
// R4
//
// ----- Saved by the context switch handler using FP
// S31
// ...
// S16
//
// ----- Always saved by context switch handler
// EXC_RETURN (indicates if FP is used)

typedef struct {
    // Saved by context switch handler
    size_t exc_return;
    size_t r4;
    size_t r5;
    size_t r6;
    size_t r7;
    size_t r8;
    size_t r9;
    size_t r10;
    size_t r11;
    // Saved on exception entry
    size_t r0;
    size_t r1;
    size_t r2;
    size_t r3;
    size_t r12;
    size_t lr;
    size_t pc;
    size_t xpsr;
} switch_frame_nofp_t;

static_assert(sizeof(switch_frame_nofp_t) == 4 * 17, "");

typedef struct {
    // Saved by context switch handler
    size_t exc_return;
    float  s16_s31[16];
    size_t r4_r11[8];
    // Saved on exception entry
    size_t r0_r3[4];
    size_t r12;
    size_t lr;
    size_t pc;
    size_t xpsr;
    float  s0_s15[16];
    size_t fpscr;
} switch_frame_fp_t;

static_assert(sizeof(switch_frame_fp_t) == 4 * 50, "");

typedef union {
    switch_frame_nofp_t nofp;
    switch_frame_fp_t fp;
} switch_frame_t;

typedef struct {
    size_t r0;
    size_t r1;
    size_t r2;
    size_t r3;
    size_t r12;
    size_t lr;
    size_t pc;
    size_t xpsr;
} exception_entry_stack_t;
