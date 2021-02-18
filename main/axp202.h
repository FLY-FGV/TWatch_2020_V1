#ifndef __AXP_202_MY__
#define __AXP_202_MY__
void init_axp202();
void get_TimeAndDate(uint8_t *HMS,uint8_t *DMY);
void set_new_Time(uint8_t Hh,uint8_t Mm,uint8_t Ss);
void set_new_Date(uint8_t Dd,uint8_t Mm,uint8_t Yy);
uint8_t get_DayOfWeek();
void set_DayOfWeek(uint8_t nd);
int16_t axp202_getdischargecurrent();
int16_t axp202_getchargecurrent();
int16_t axp202_getBatV();
uint8_t axp202_getpowerstatus();
void disable_ldo2();
void  enable_ldo2();
void disable_all_ldo();
#endif