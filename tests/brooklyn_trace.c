#include "../source/brooklyn.h"
#include <stdio.h>
#include <stdlib.h>
int main(int argc,char**argv){Brooklyn g;brooklyn_reset(&g);g.wind=argc>1?atoi(argv[1]):0;g.weather_left=10000;int x,z,j,d,h;while(scanf("%d %d %d %d %d",&x,&z,&j,&d,&h)==5){brooklyn_step(&g,x,z,j,d,h);printf("%.7g %.7g %.7g %.7g %.7g %d %d\n",g.x,g.z,g.y,g.vy,g.cam,g.checkpoint,g.won);}return 0;}
