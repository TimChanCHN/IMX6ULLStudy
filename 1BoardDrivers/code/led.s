

.global _start @全局变量

_start:
/* 使能所有外设时钟 */
    ldr r0, =0x020c4068         @CCGR0
    ldr r1, = 0xffffffff        @向CCGR0写入的数据
    str r1, [r0]                @将0xffffffff写入到CCGR0

    ldr r0, = 0x020c406c        @CCGR1
    str r1, [r0]

    ldr r0, =0x020c4070         @CCGR2
    str r1, [r0]

    ldr r0, =0x020c4074         @CCGR3
    str r1, [r0]

    ldr r0, =0x020c4078         @CCGR4
    str r1, [r0]

    ldr r0, =0x020c407C         @CCGR5
    str r1, [r0]

    ldr r0, =0x020c4080         @CCGR6
    str r1, [r0]

/* 配置GPIO1为复用GPIO */
    ldr r0, =0x020e0068         @CCGR6
    ldr r1, =0x05                  
    str r1, [r0]

/* 配置GPIO1——IO3的电气属性，如推挽、上拉、输出速率等设置 */
    ldr r0, =0x020e02f4
    ldr r1, =0x10b0
    str r1, [r0]

/* 设置GPIO为输出 */
    ldr r0, =0x0209c004
    ldr r1, = 0x08
    str r1, [r0]

/* 设置GPIO1的数据寄存器，说明输出高电平 */
    ldr r0, =0x0209c000
    ldr r1, = 0x00
    str r1, [r0]

loop:
    b loop





