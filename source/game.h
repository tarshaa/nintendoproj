#ifndef PHOTO_GAME_H
#define PHOTO_GAME_H
#include <stdbool.h>
#include <stdint.h>
typedef struct { float x,y,vx,vy; int checkpoint,ticks,collected; bool grounded,duck,won,lost; } Game;
void game_reset(Game *g);
void game_step(Game *g, int direction, bool jump, bool duck);
bool game_ground(float x);
#endif
