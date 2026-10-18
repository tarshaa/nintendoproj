#include "brooklyn.h"
#include <string.h>
static unsigned random_next(Brooklyn*g){g->random=g->random*1664525u+1013904223u;return g->random;}
bool brooklyn_ground(float x){return !((x>145&&x<179)||(x>405&&x<510)||(x>661&&x<702));}
bool brooklyn_in_fan(float x){return x>=355&&x<=510;}
void brooklyn_reset(Brooklyn*g){
 memset(g,0,sizeof(*g));g->x=24;g->y=BROOKLYN_FLOOR;g->grounded=true;
 g->random=0xB400C1u;g->weather_left=420;g->next_wind=1;
}
void brooklyn_step(Brooklyn*g,int direction,bool jump,bool duck,bool glide){
 if(g->won||g->lost)return;
 if(++g->ticks>=BROOKLYN_TIME){g->lost=true;return;}
 if(--g->weather_left<=0){g->wind=g->next_wind;g->next_wind=(g->wind+1+random_next(g)%2)%3;g->weather_left=420+random_next(g)%181;}
 bool beneath=g->x+6>230&&g->x-6<285&&g->y>136;
 g->duck=(duck||beneath)&&g->grounded;
 float oldx=g->x,oldy=g->y;
 if(jump&&g->grounded&&!g->duck){g->vy=-5.9f;g->grounded=false;}
 g->gliding=glide&&!g->grounded;
 float gust=g->grounded?0:(g->wind==1?0.25f:g->wind==2?-0.25f:0);
 g->x+=direction*(g->duck?1.05f:1.85f)+gust;
 if(g->x<8)g->x=8;
 if(g->x>BROOKLYN_END)g->x=BROOKLYN_END;
 int height=g->duck?9:20;
 if(g->x+6>230&&g->x-6<285&&g->y>105&&g->y-height<136)g->x=oldx;
 g->vy+=g->gliding?0.10f:0.29f;
 if(g->gliding&&g->vy>0.7f)g->vy=0.7f;
 if(g->gliding&&brooklyn_in_fan(g->x)){g->vy-=0.20f;if(g->vy<-1.4f)g->vy=-1.4f;}
 g->y+=g->vy;g->grounded=false;
 if(g->y<44){g->y=44;g->vy=0;}
 if(g->vy>=0&&oldy<=105&&g->y>=105&&g->x+5>230&&g->x-5<285){g->y=105;g->vy=0;g->grounded=true;}
 if(g->vy>=0&&oldy<=BROOKLYN_FLOOR&&g->y>=BROOKLYN_FLOOR&&brooklyn_ground(g->x)){g->y=BROOKLYN_FLOOR;g->vy=0;g->grounded=true;}
 if(g->grounded&&g->x>=195&&g->x<220)g->checkpoint=200;
 if(g->grounded&&g->x>=325&&g->x<355)g->checkpoint=335;
 if(g->grounded&&g->x>=545&&g->x<620)g->checkpoint=550;
 if(g->grounded&&g->x>=730)g->checkpoint=740;
 if(g->y>205){g->falls++;g->x=g->checkpoint?g->checkpoint:24;g->y=BROOKLYN_FLOOR;g->vy=0;g->grounded=true;g->gliding=false;}
 if(g->x>=875&&g->grounded)g->won=true;
}
