#include "bsp_int.h"

static unsigned int irqNesting;

/* 中断处理函数表 */
static sys_irq_handle_t irqTable[NUMBER_OF_INT_VECTORS];


/* 初始化中断处理函数表 */
void system_irqtable_init(void)
{
    unsigned int i = 0;

    irqNesting = 0;
    for(i = 0; i < NUMBER_OF_INT_VECTORS; i++ )
    {
        irqTable[i].irqHandler = default_irqhandler;
        irqTable[i].userParam = NULL;
    }

}

/* 注册中断处理函数 */
void system_register_irqhandler(IRQn_Type irq, system_irq_handler_t handler, void *userParam)
{
    irqTable[irq].irqHandler = handler;
    irqTable[irq].userParam = userParam;
}

/* 中断初始化函数 */
void int_init(void)
{
    GIC_Init();
    system_irqtable_init();

    /* 中断向量偏移设置 */
    __set_VBAR((uint32_t)0X87800000);
}

/* 具体的中断处理函数，IRQ_Handler会调用此函数 */
void system_irqhandler(unsigned int gicciar)
{
    uint32_t intNum = gicciar & 0x3ff;

    /* 检查中断ID */
    if((intNum == 1023) || (intNum >= NUMBER_OF_INT_VECTORS))
    {
        return;
    }

    irqNesting++;

    /* 根据中断ID号，读取中断处理函数，然后执行 */
    irqTable[intNum].irqHandler(intNum, irqTable[intNum].userParam);
    irqNesting--;
}


/* 默认中断处理函数 */
void default_irqhandler(unsigned int gicciar, void *userParam)
{
    while(1)
    {

    }
}
