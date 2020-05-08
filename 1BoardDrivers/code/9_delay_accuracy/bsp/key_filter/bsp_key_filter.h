#ifndef  __BSP_KEY_FILTER_H
#define  __BSP_KEY_FILTER_H

/* 函数声明 */
void filterkey_init(void);
void filtertimer_init(unsigned int value);
void filtertimer_stop(void);
void filtertimer_restart(unsigned int value);
void filtertimer_irqhandler(void);
void gpio1_16_31_irqhandler(void);

#endif // ! __BSP_KEY_FILTER_H
