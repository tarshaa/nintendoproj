#include "calendar.h"
int date_days(int y,int m){static const int ds[]={31,28,31,30,31,30,31,31,30,31,30,31};if(m<1||m>12)return 0;return ds[m-1]+(m==2&&y%4==0&&(y%100!=0||y%400==0));}
bool date_valid(Date d){return d.year>=2000&&d.year<=2099&&d.month>=1&&d.month<=12&&d.day>=1&&d.day<=date_days(d.year,d.month);}
int date_weekday(Date d){static const int t[]={0,3,2,5,0,3,5,1,4,6,2,4};int y=d.year-(d.month<3);return(y+y/4-y/100+y/400+t[d.month-1]+d.day)%7;}
int date_pack(Date d){return d.year*10000+d.month*100+d.day;}
Date date_unpack(int n){return(Date){n/10000,n/100%100,n%100};}
Date date_month(Date d,int delta){int m=d.year*12+d.month-1+delta;if(m<2000*12)m=2000*12;if(m>2099*12+11)m=2099*12+11;d.year=m/12;d.month=m%12+1;if(d.day>date_days(d.year,d.month))d.day=date_days(d.year,d.month);return d;}
Date date_move(Date d,int n){while(n>0){if(d.year==2099&&d.month==12&&d.day==31)break;if(++d.day>date_days(d.year,d.month)){d.day=1;if(++d.month==13){d.month=1;d.year++;}}n--;}
while(n<0){if(d.year==2000&&d.month==1&&d.day==1)break;if(--d.day<1){if(--d.month==0){d.month=12;d.year--;}d.day=date_days(d.year,d.month);}n++;}return d;}
