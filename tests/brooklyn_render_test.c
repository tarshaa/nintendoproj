#include "../source/brooklyn_render.h"
#include <assert.h>
#include <stdio.h>
int main(void){static uint16_t pixels[256*192+2];Brooklyn g;brooklyn_reset(&g);pixels[0]=123;pixels[256*192+1]=456;
 for(int i=0;i<=75;i++){g.z=g.cam=i;g.x=i%3-1;g.y=i%5;g.wind=i%3;g.gliding=i%2;g.won=i==65;g.time=i;g.reward=1;brooklyn_render(pixels+1,&g);assert(pixels[0]==123&&pixels[256*192+1]==456);for(int p=1;p<=256*192;p++)assert(pixels[p]&32768);}
 puts("Renderer covers viewport and stays within framebuffer across route");}
