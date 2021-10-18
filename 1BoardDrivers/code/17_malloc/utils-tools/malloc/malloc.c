#include "malloc.h"

// 内存池,32字节对齐
#pragma pack(4)
u8 mem1base[MEM1_MAX_SIZE];
#pragma pack()

// 内存管理表
u16 mem1mapbase[MEM1_ALLOC_TABLE_SIZE];

// 内存管理参数
// 内存表大小
const u32 memtblsize[MEMPOOL_BANK] = {MEM1_ALLOC_TABLE_SIZE};
// 内存分块大小
const u32 memblksize[MEMPOOL_BANK] = {MEM1_BLOCK_SIZE};
// 内存池大小
const u32 memsize[MEMPOOL_BANK] = {MEM1_MAX_SIZE};

// 内存管理控制器
struct _m_malloc_dev malloc_dev = 
{
    my_mem_init,                    // 内存初始化
    my_mem_perused,                 // 内存使用率
    {mem1base},                       // 内存池基地址
    {mem1mapbase},                    // 内存管理状态表
    {0},                              // 内存管理未就绪
};

// 复制内存
void mymemcpy(void *des, void *src, u32 n)
{
    u8 *xdes = des;
    u8 *xsrc = src;
    while (n--)
    {
        *xdes++ = *xsrc++;
    }
}

// 设置内存
void mymemset(void *des, u8 c, u32 count)                      // 设置内存
{   
    u8 *xs = des;
    while (count--)
    {
        *xs++ = c;
    }
}

// 内存管理初始化
void my_mem_init(u8 memx)
{
    mymemset(malloc_dev.memmap[memx], 0, memtblsize[memx]*2);       // 内存状态表数据清零
    mymemset(malloc_dev.membase[memx], 0, memsize[memx]);           // 内存池所有数据清零
    malloc_dev.memrdy[memx] = 1;                                    // 内存管理就绪 
}

// 获得内存使用率
u8 my_mem_perused(u8 memx)
{
    u32 used = 0;
    u32 i;
    for (i = 0; i < memtblsize[memx]; i++)
    {
        if (malloc_dev.memmap[memx][i])
            used++;
    }
    return used*100 / memtblsize[memx];
}                                     

// 内存分配
u32 my_mem_malloc(u8 memx, u32 size)
{
    long offset = 0;
    u32 nmemb;              // 所需的内存块数
    u32 cmemb = 0;              // 连续的内存块数
    u32 i;
    if (!malloc_dev.memrdy[memx])       // 内存池若未就绪，则先初始化
        malloc_dev.init(memx);
    
    if (size == 0)
        return 0xFFFFFFFF;
    
    nmemb = size / memblksize[memx];
    if (size % memblksize[memx])
        nmemb++;

    for (offset = memtblsize[memx]-1; offset >= 0; offset--)
    {
        if (!malloc_dev.memmap[memx][offset])
            cmemb++;
        else
            cmemb = 0;
        
        if (cmemb == nmemb)
        {
            for (i = 0; i < nmemb; i++)
            {
                malloc_dev.memmap[memx][offset+i] = nmemb;
            }
            return (offset*memblksize[memx]);               // 返回偏移地址
        }
    }
    return 0xFFFFFFFF;
}

// 释放内存
u8 my_mem_free(u8 memx, u32 offset)
{
    int i;
    if (!malloc_dev.memrdy[memx])
    {
        malloc_dev.init(memx);  
        return 1;               // 未初始化内存池
    }

    if (offset < memsize[memx])
    {
        int index = offset / memblksize[memx];          // 偏移所在内存块号码
        int nmemb = malloc_dev.memmap[memx][index];     // 内存块数量

        for (i = 0; i < nmemb; i++)
        {
            malloc_dev.memmap[memx][offset+i] = 0;
        }
        return 0;
    }
    else
        return 2;           // 偏移超区
}                           

// 内存释放
void myfree(u8 memx, void *ptr)
{
    u32 offset;
    if (ptr == NULL)
        return;
    offset = (u32)ptr - (u32)malloc_dev.membase[memx];
    my_mem_free(memx, offset);
}                               

// 内存分配
void *mymalloc(u8 memx, u32 size)
{
    u32 offset;
    offset = my_mem_malloc(memx, size);
    if (offset == 0xFFFFFFFF)
        return NULL;
    else
        return (void*)((u32)malloc_dev.membase[memx]+offset);
}                              

// 重新分配内存
void *myrealloc(u8 memx, void *ptr, u32 size)
{
    u32 offset;
    offset = my_mem_malloc(memx, size);
    if (offset == 0xFFFFFFFF)
        return NULL;
    else
    {
        mymemcpy((void*)((u32)malloc_dev.membase[memx]+offset), ptr, size);
        myfree(memx, ptr);
        return (void*)((u32)malloc_dev.membase[memx]+offset);
    }
}             

