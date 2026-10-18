#include "../source/brooklyn.h"
#include "../source/calendar.h"
#include <assert.h>
#include <stdio.h>
static bool route(int wind){
 Brooklyn g;brooklyn_reset(&g);g.wind=wind;g.weather_left=10000;
 for(int i=0;i<3000&&!g.won&&!g.lost;i++){
  bool jump=g.grounded&&((g.x>=125&&g.x<145)||(g.x>=376&&g.x<405)||(g.x>=639&&g.x<661));
  brooklyn_step(&g,1,jump,g.x>=217&&g.x<297,g.x>=355&&g.x<530);
 }
 return g.won&&g.falls==0;
}
int main(void){
 for(int wind=0;wind<3;wind++)assert(route(wind));
 Brooklyn g;brooklyn_reset(&g);g.x=224;
 for(int i=0;i<20;i++){brooklyn_step(&g,1,false,false,false);}assert(g.x<230);
 for(int i=0;i<70;i++){brooklyn_step(&g,1,false,true,false);}assert(g.x>285);
 brooklyn_reset(&g);g.x=250;brooklyn_step(&g,0,true,false,false);assert(g.duck&&g.y==150);
 brooklyn_reset(&g);g.x=425;g.y=120;g.grounded=false;brooklyn_step(&g,0,false,false,true);assert(g.gliding&&g.vy<0);
 brooklyn_reset(&g);g.checkpoint=335;g.x=450;g.y=203;g.vy=4;brooklyn_step(&g,0,false,false,false);assert(g.x==335&&g.falls==1);
 brooklyn_reset(&g);g.ticks=BROOKLYN_TIME-1;brooklyn_step(&g,0,false,false,false);assert(g.lost);
 brooklyn_reset(&g);g.weather_left=1;brooklyn_step(&g,0,false,false,false);assert(g.wind==1&&g.weather_left>=420);
 assert(date_days(2024,2)==29&&date_days(2026,2)==28);
 assert(date_weekday((Date){2026,10,18})==0);
 assert(date_pack(date_move((Date){2026,12,31},1))==20270101);
 assert(date_pack(date_move((Date){2024,3,1},-1))==20240229);
 assert(date_pack(date_month((Date){2026,1,31},1))==20260228);
 assert(date_pack(date_move((Date){2000,1,1},-1))==20000101);
 assert(date_pack(date_move((Date){2099,12,31},1))==20991231);
 puts("Brooklyn route, wind, canopy, recovery and calendar tests passed");
}
