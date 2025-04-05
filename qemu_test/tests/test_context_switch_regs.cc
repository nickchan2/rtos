#include "rtos.hh"
#include "rtos_test.hh"

asm(
    ".section .data \n"
    "counter:       \n"
    "   .word 0     \n"
);

[[gnu::naked]] static void task0_function() {
    asm volatile(
        // Ensure this task is entered first
        "   ldr     r0, =counter            \n"
        "   ldr     r1, [r0]                \n"
        "   cmp     r1, #0x0                \n"
        "   it      ne                      \n"
        "   bne     rtos_test_fail_asm      \n"

        // Increment the counter to indicate task0 has started
        "   ldr     r1, =0x1                \n"
        "   str     r1, [r0]                \n"

        // Set known values to the general purpose registers
        "   ldr     r0, =0x0                \n"
        "   ldr     r1, =0x1                \n"
        "   ldr     r2, =0x2                \n"
        "   ldr     r3, =0x3                \n"
        "   ldr     r4, =0x4                \n"
        "   ldr     r5, =0x5                \n"
        "   ldr     r6, =0x6                \n"
        "   ldr     r7, =0x7                \n"
        "   ldr     r8, =0x8                \n"
        "   ldr     r9, =0x9                \n"
        "   ldr     r10, =0xA               \n"
        "   ldr     r11, =0xB               \n"
        "   ldr     r12, =0xC               \n"

        // Push some of the registers to the stack
        "   push    {r2-r12}                \n"

        // Yield to task1. Also sets r14 (LR) to the address of the next
        // instruction. Note that bit 0 of of r14 is also set to indicate
        // to return to thumb mode.
        "   bl      rtos_task_yield         \n"
        "task0_after_yield:                 \n"

        // Check r0 and r1 were restored
        "   cmp     r0, #0x0                \n"
        "   it      ne                      \n"
        "   bne     rtos_test_fail_asm      \n"
        "   cmp     r1, #0x1                \n"
        "   it      ne                      \n"
        "   bne     rtos_test_fail_asm      \n"

        // Check that task1 was entered
        "   ldr     r0, =counter            \n"
        "   ldr     r1, [r0]                \n"
        "   cmp     r1, #0x2                \n"
        "   it      ne                      \n"
        "   bne     rtos_test_fail_asm      \n"

        "task0_check_regs:                  \n"
        // Check general purpose registers were restored
        "   cmp     r2, #0x2                \n"
        "   it      ne                      \n"
        "   bne     rtos_test_fail_asm      \n"
        "   cmp     r3, #0x3                \n"
        "   it      ne                      \n"
        "   bne     rtos_test_fail_asm      \n"
        "   cmp     r4, #0x4                \n"
        "   it      ne                      \n"
        "   bne     rtos_test_fail_asm      \n"
        "   cmp     r5, #0x5                \n"
        "   it      ne                      \n"
        "   bne     rtos_test_fail_asm      \n"
        "   cmp     r6, #0x6                \n"
        "   it      ne                      \n"
        "   bne     rtos_test_fail_asm      \n"
        "   cmp     r7, #0x7                \n"
        "   it      ne                      \n"
        "   bne     rtos_test_fail_asm      \n"
        "   cmp     r8, #0x8                \n"
        "   it      ne                      \n"
        "   bne     rtos_test_fail_asm      \n"
        "   cmp     r9, #0x9                \n"
        "   it      ne                      \n"
        "   bne     rtos_test_fail_asm      \n"
        "   cmp     r10, #0xA               \n"
        "   it      ne                      \n"
        "   bne     rtos_test_fail_asm      \n"
        "   cmp     r11, #0xB               \n"
        "   it      ne                      \n"
        "   bne     rtos_test_fail_asm      \n"
        "   cmp     r12, #0xC               \n"
        "   it      ne                      \n"
        "   bne     rtos_test_fail_asm      \n"

        // r0 == 0 indicates that the registers after the context switch were
        // checked. After this, check pop some regs from te stack and check
        // they are also correct. This indirectly checks that the stack pointer
        // was restored correctly.
        "   cmp     r0, #0x0                \n"
        "   it      ne                      \n"
        "   bne     rtos_test_pass_asm      \n"

        // Check that LR was restored correctly
        "   ldr     r0, =0x1                \n"
        "   ldr     r0, =task0_after_yield  \n"
        "   orr     r0, #0x1                \n" // LR[0]: thumb mode
        "   cmp     r14, r0                 \n"
        "   it      ne                      \n"
        "   bne     rtos_test_fail_asm      \n"

        "   ldr     r0, =0x1                \n"
        "   pop     {r2-r12}                \n"
        "   b       task0_check_regs        \n"
    );
}

[[gnu::naked]] static void task1_function() {
    asm volatile(
        // Ensure this task is entered second
        "   ldr     r0, =counter            \n"
        "   ldr     r1, [r0]                \n"
        "   cmp     r1, #0x1                \n"
        "   it      ne                      \n"
        "   bne     rtos_test_fail_asm      \n"

        // Increment the counter to indicate task1 has started
        "   ldr     r1, =0x2                \n"
        "   str     r1, [r0]                \n"

        // Set garbage values to the registers
        "   ldr     r0, =0xDEADBEEF         \n"
        "   ldr     r1, =0xDEADBEEF         \n"
        "   ldr     r2, =0xDEADBEEF         \n"
        "   ldr     r3, =0xDEADBEEF         \n"
        "   ldr     r4, =0xDEADBEEF         \n"
        "   ldr     r5, =0xDEADBEEF         \n"
        "   ldr     r6, =0xDEADBEEF         \n"
        "   ldr     r7, =0xDEADBEEF         \n"
        "   ldr     r8, =0xDEADBEEF         \n"
        "   ldr     r9, =0xDEADBEEF         \n"
        "   ldr     r10, =0xDEADBEEF        \n"
        "   ldr     r11, =0xDEADBEEF        \n"
        "   ldr     r12, =0xDEADBEEF        \n"
        "   ldr     r14, =0xDEADBEEF        \n"

        "   b       rtos_task_exit          \n"
    );
}

int main() {
    rtos_test::setup();
    rtos_test::TaskWithStack task0(1, false, task0_function);
    rtos_test::TaskWithStack task1(1, false, task1_function);
    rtos::start();
}
