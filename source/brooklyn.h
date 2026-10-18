#ifndef BROOKLYN_H
#define BROOKLYN_H
#include <stdbool.h>
#include <stdint.h>
// Coordinates and movement match the recovered Special Delivery browser prototype.
typedef struct {
 float x,z,y,vy,cam,time,weather_left,reward;
 int ticks,checkpoint,falls,wind;
 uint32_t random;
 bool grounded,duck,gliding,won,lost;
} Brooklyn;
void brooklyn_reset(Brooklyn *g);
void brooklyn_step(Brooklyn *g,int sideways,int forward,bool jump,bool duck,bool glide);
#endif
