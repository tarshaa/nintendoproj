#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define WORLD_MAX_NODES 40
typedef struct {float x,z,y,r;uint16_t color;uint8_t shape,flags;char label[24];} WorldNode;
typedef struct {uint32_t photo_checksum;unsigned count,goal;char title[48],objective[96];WorldNode nodes[WORLD_MAX_NODES];} GeneratedWorld;
typedef struct {float x,z,y,vx,vz,vy,t,cam_x,cam_z,cam_y,buffer,coyote,reward;int ground,checkpoint;bool held,won;unsigned falls;} WorldPlayer;
bool world_parse(GeneratedWorld *world,const void *bytes,size_t size,uint32_t photo_checksum);
void world_reset(WorldPlayer *player,const GeneratedWorld *world);
void world_retry(WorldPlayer *player,const GeneratedWorld *world);
void world_step(WorldPlayer *player,const GeneratedWorld *world,float x,float z,bool jump,bool careful);
void world_render(uint16_t *pixels,const GeneratedWorld *world,const WorldPlayer *player);
