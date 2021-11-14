#ifndef __BSP_RTC_H
#define __BSP_RTC_H
#include "imx6u.h"

/* 跟时间有关的红定义 */
#define SECONDS_IN_A_DAY      (86400)
#define SECONDS_IN_A_HOUR     (3600)
#define SECONDS_IN_A_MINUTE   (60)
#define DAYS_IN_A_YEAR        (365)
#define YEAR_RANGE_START      (1970)
#define YEAR_RANGE_END        (2099)

/* 跟时间有关的结构体 */
struct rtc_datetime{
    unsigned short year;
    unsigned char  month;
    unsigned char  day;
    unsigned char  hour;
    unsigned char  minute;
    unsigned char  second;
};

void rtc_enable(void);
void rtc_disable(void);
uint64_t rtc_coverdate_to_seconds(struct rtc_datetime *datetime);
void rtc_setdatetime(struct rtc_datetime *datatime);
uint64_t rtc_getseconds(void);
void rtc_convertseconds_to_datetime(unsigned int seconds, struct rtc_datetime *datetime);
void rtc_getdatetime(struct rtc_datetime *datetime);
unsigned char rtc_isleapyear(unsigned short year);
#endif