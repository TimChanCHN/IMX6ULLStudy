#ifndef  __BSP_KEY_H
#define __BSP_KEY_H

enum get_key_value{
    KEY_NONE = 0U,
    KEY0_VALUE,
    KEY1_VALUE,
    KEY2_VALUE,
};

void init_key(void);
int get_key_value(void);

#endif // ! __KEY_H
