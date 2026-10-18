#ifndef BROOKLYN_H
#define BROOKLYN_H
#include <stdbool.h>
#include <stdint.h>
#define BROOKLYN_FLOOR 150
#define BROOKLYN_END 920
#define BROOKLYN_TIME (75*60)
typedef struct {
 float x,y,vy;
 int ticks,checkpoint,falls,wind,next_wind,weather_left;
 uint32_t random;
 bool grounded,duck,gliding,won,lost;
} Brooklyn;
void brooklyn_reset(Brooklyn *g);
void brooklyn_step(Brooklyn *g,int direction,bool jump,bool duck,bool glide);
bool brooklyn_ground(float x);
bool brooklyn_in_fan(float x);
#endif
