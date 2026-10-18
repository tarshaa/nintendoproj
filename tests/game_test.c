#include "../source/game.h"
#include <assert.h>
#include <stdio.h>
int main(void){
 Game g;game_reset(&g);assert(g.grounded);
 game_step(&g,0,true,false);assert(g.y<140&&!g.grounded);
 game_reset(&g);g.x=268;for(int i=0;i<12;i++)game_step(&g,1,false,false);assert(g.x<270);
 for(int i=0;i<55;i++){game_step(&g,1,false,true);}
 assert(g.x>315);
 game_reset(&g);g.x=290;game_step(&g,0,true,false);assert(g.duck&&g.y==140);
 game_reset(&g);g.x=185;g.checkpoint=240;for(int i=0;i<80;i++)game_step(&g,0,false,false);assert(g.x==240&&g.y==140);
 game_reset(&g);g.ticks=5399;game_step(&g,0,false,false);assert(g.lost);
 game_reset(&g);g.x=690;game_step(&g,0,false,false);assert(!g.won);g.collected=7;game_step(&g,0,false,false);assert(g.won);
 // Full deterministic route: collect all three, jump both gaps, duck under canopy.
 game_reset(&g);for(int i=0;i<1500&&!g.won&&!g.lost;i++){
 bool jump=g.grounded&&((g.x>=156&&g.x<175)||(g.x>=386&&g.x<405));
 game_step(&g,1,jump,g.x>260&&g.x<326);
 }assert(g.won&&g.collected==7);puts("game tests passed");
}
