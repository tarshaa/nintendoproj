#include "brooklyn.h"
#include <math.h>
#include <string.h>
static const float checkpoints[]={2,20,32,48};
static bool gap(float z){return(z>13&&z<16)||(z>36&&z<43)||(z>49&&z<52);}
void brooklyn_reset(Brooklyn*g){memset(g,0,sizeof(*g));g->z=g->cam=2;g->grounded=true;g->weather_left=8;g->random=0xb400c1;}
void brooklyn_step(Brooklyn*g,int sideways,int forward,bool jump,bool duck,bool glide){
 const float dt=1.f/60;
 if(g->won||g->lost)return;
 if(jump&&g->y==0){g->y=.01f;g->vy=10;}
 g->ticks++;g->time+=dt;g->weather_left-=dt;
 if(g->weather_left<0){g->wind=(g->wind+1)%3;g->random=g->random*1664525u+1013904223u;g->weather_left=7+(g->random>>8)*(5.f/16777216.f);}
 g->duck=duck||(g->z>26&&g->z<29&&g->y<1);
 float diagonal=sideways&&forward?.70710678118f:1,speed=g->duck?3.8f:g->wind==1?9:8;
 float next=fmaxf(1,g->z+forward*diagonal*speed*dt);
 if(!duck&&((g->z<=26&&next>26)||(g->z>=29&&next<29)))next=g->z;
 g->z=next;g->x+=sideways*diagonal*speed*dt;
 if(g->wind==2&&g->z>18&&!g->duck)g->x+=1.8f*dt;
 g->duck=duck||(g->z>26&&g->z<29&&g->y<1);
 g->gliding=glide&&g->y>0&&!g->duck;
 if(g->gliding&&g->z>35&&g->z<45&&g->y<4)g->vy=fmaxf(g->vy,2);
 if(g->y>0){g->vy-=(g->gliding?5:22)*dt;if(g->gliding)g->vy=fmaxf(-1.6f,g->vy);g->y=fmaxf(0,g->y+g->vy*dt);if(g->y==0)g->vy=0;}
 if(fabsf(g->x)>5.2f||(g->y==0&&gap(g->z))){g->x=0;g->z=checkpoints[g->checkpoint];g->y=g->vy=0;g->falls++;g->gliding=false;}
 for(int i=1;i<4;i++)if(g->z>=checkpoints[i]&&g->y==0&&i>g->checkpoint)g->checkpoint=i;
 g->grounded=g->y==0;g->cam+=(g->z-g->cam)*(dt*5);
 if(g->z>=65&&fabsf(g->x)<2.3f&&g->y==0)g->won=true;
}
