#include "../source/brooklyn.h"
#include "../source/calendar.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
static bool route(int wind){Brooklyn g;brooklyn_reset(&g);g.wind=wind;g.weather_left=10000;
 for(int i=0;i<4000&&!g.won;i++){
 bool jump=g.grounded&&((g.z>=11.5f&&g.z<13)||(g.z>=34.5f&&g.z<36)||(g.z>=47.5f&&g.z<49));
 bool glide=(g.z>=11.5f&&g.z<16.5f)||(g.z>=34.5f&&g.z<44)||(g.z>=47.5f&&g.z<53);
 int side=g.x>.2f?-1:g.x<-.2f?1:0;
 brooklyn_step(&g,side,1,jump,g.z>25&&g.z<30,glide);
 }
 if(!g.won||g.falls)printf("route wind %d z %f x %f falls %d\n",wind,g.z,g.x,g.falls);
 return g.won&&g.falls==0;}
int main(void){
 for(int i=0;i<3;i++)assert(route(i));
 Brooklyn g;brooklyn_reset(&g);brooklyn_step(&g,1,0,false,false,false);assert(g.x>0&&g.z==2);brooklyn_step(&g,0,1,false,false,false);assert(g.z>2);brooklyn_step(&g,0,-1,false,false,false);assert(fabsf(g.z-2)<.001f);
 brooklyn_reset(&g);g.z=25.99f;brooklyn_step(&g,0,1,false,false,false);assert(g.z==25.99f);for(int i=0;i<60;i++)brooklyn_step(&g,0,1,false,true,false);assert(g.z>29);
 brooklyn_reset(&g);g.z=37;g.y=1;brooklyn_step(&g,0,0,false,false,true);assert(g.vy>1.8f&&g.gliding);
 brooklyn_reset(&g);g.z=37;g.checkpoint=2;brooklyn_step(&g,0,0,false,false,false);assert(g.z==32&&g.falls==1);
 brooklyn_reset(&g);g.x=5.3f;brooklyn_step(&g,0,0,false,false,false);assert(g.x==0&&g.falls==1);
 brooklyn_reset(&g);g.z=65;g.x=3;brooklyn_step(&g,0,0,false,false,false);assert(!g.won);g.x=0;brooklyn_step(&g,0,0,false,false,false);assert(g.won);
 brooklyn_reset(&g);g.weather_left=.001f;brooklyn_step(&g,0,0,false,false,false);assert(g.wind==1&&g.weather_left>=7&&g.weather_left<=12);
 assert(date_days(2024,2)==29&&date_days(2026,2)==28);assert(date_weekday((Date){2026,10,18})==0);assert(date_pack(date_move((Date){2026,12,31},1))==20270101);assert(date_pack(date_move((Date){2024,3,1},-1))==20240229);assert(date_pack(date_month((Date){2026,1,31},1))==20260228);
 puts("Original route, steering, awning, gliding, weather, recovery and calendar passed");
}
