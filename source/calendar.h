#ifndef WORLD_CALENDAR_H
#define WORLD_CALENDAR_H
#include <stdbool.h>
typedef struct {int year,month,day;} Date;
bool date_valid(Date d);
int date_days(int year,int month);
int date_weekday(Date d);
int date_pack(Date d);
Date date_unpack(int packed);
Date date_move(Date d,int days);
Date date_month(Date d,int delta);
#endif
