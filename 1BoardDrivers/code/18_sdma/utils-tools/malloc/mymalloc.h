#ifndef __MALLOC_H
#define __MALLOC_H

#include "fsl_common.h"
#include "fsl_iomuxc.h"
#include "MCIMX6Y2.h"
#include "stdio.h"

#ifndef NULL
#define NULL 0
#endif

// 定义内存池
#define MEMPOOL_DDR     0

#define MEMPOOL_BANK   1

// MEMPOOL内存参数设定
#define MEM1_BLOCK_SIZE         4                                   // 内存块大小为4字节
#define MEM1_MAX_SIZE           5*1024*1024                         // 最大管理内存5MB
#define MEM1_ALLOC_TABLE_SIZE   MEM1_MAX_SIZE/MEM1_BLOCK_SIZE       // 内存表大小

// 内存管理控制器
struct _m_malloc_dev
{
    void (*init)(u8);               // 内存池初始化
    u8 (*perused)(u8);              // 内存使用率
    u8 *membase[MEMPOOL_BANK];      // 内存基址
    u16 *memmap[MEMPOOL_BANK];      // 内存管理状态表
    u8 memrdy[MEMPOOL_BANK];        // 内存池就绪状态
};
extern struct _m_malloc_dev malloc_dev;

void mymemset(void *des, u8 c, u32 count);                      // 设置内存
void mymemcpy(void *des, void *src, u32 n);                     // 复制内存
void my_mem_init(u8 memx);                                      // 内存管理初始化
u32 my_mem_malloc(u8 memx, u32 size);                           // 内存分配
u8 my_mem_free(u8 memx, u32 offset);                            // 释放内存
u8 my_mem_perused(u8 memx);                                     // 获得内存使用率

// 用户调用
void myfree(u8 memx, void *ptr);                                // 内存释放
void *mymalloc(u8 memx, u32 size);                              // 内存分配
void *myrealloc(u8 memx, void *ptr, u32 size);                  // 重新分配内存

#endif
