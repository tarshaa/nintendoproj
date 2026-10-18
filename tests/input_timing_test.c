#include "../source/input_queue.h"
#include "../source/brooklyn.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
// Inputs occur at 60 Hz. The drawing loop consumes them at different rates.
static Brooklyn run(unsigned draw_every){InputQueue q={0};Brooklyn g;brooklyn_reset(&g);
 for(unsigned tick=0;tick<360;tick++){
  InputSample s={tick==20?2u:0u, tick<120?1u:0u};input_push(&q,s);
  if((tick+1)%draw_every==0){InputSample batch[INPUT_CAPACITY];unsigned n=input_drain(&q,batch);for(unsigned i=0;i<n;i++)brooklyn_step(&g,0,!!(batch[i].held&1),batch[i].pressed&2,false,false);}
 }return g;}
int main(void){Brooklyn normal=run(1);for(unsigned rate=2;rate<=6;rate++){Brooklyn slow=run(rate);assert(memcmp(&normal,&slow,sizeof(normal))==0);}assert(normal.ticks==360);
 InputQueue q={0};InputSample samples[INPUT_CAPACITY];input_push(&q,(InputSample){2,2});input_push(&q,(InputSample){0,0});assert(input_drain(&q,samples)==2);assert(samples[0].pressed==2&&samples[1].held==0);assert(input_drain(&q,samples)==0);
 for(unsigned i=0;i<100;i++){input_push(&q,(InputSample){i,i});}
 assert(input_drain(&q,samples)==INPUT_CAPACITY);assert(samples[0].held==68&&samples[31].held==99);
 puts("Movement identical at 60/30/20/15/12/10 draw FPS; short taps retained; backlog bounded");}
