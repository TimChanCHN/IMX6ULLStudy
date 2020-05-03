.global _start
_start:
    /* 设置处理器进入SVC模式，该模式下可以访问CPU所有资源 */
    mrs r0, cpsr
    bic r0, r0, #0x1f
    orr r0, r0, #0x13
    msr cpsr, r0

    /* 设置SP指针，对于IMS6ULL而言，可以直接对SP指针赋值地址值，但是其他CPU需要确认栈的初始化等等 */
    ldr sp, =0x80200000             @栈向下增长，范围使0x80200000-0x80000000
    b main
