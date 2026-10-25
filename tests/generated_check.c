#include "../source/generated_world.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static bool edge(const GeneratedWorld*w,unsigned a,unsigned b){WorldPlayer p;world_reset(&p,w);const WorldNode*s=&w->nodes[a];p.x=s->x;p.z=s->z;p.y=s->y;p.ground=(int)a;p.checkpoint=(int)a;bool air=false;
 for(int i=0;i<240;i++){const WorldNode*t=&w->nodes[b];float dx=t->x-p.x,dz=t->z-p.z,d=hypotf(dx,dz);world_step(&p,w,d>.08f?dx/d:0,d>.08f?dz/d:0,true,false);if(p.ground<0)air=true;if(p.falls)return false;if(air&&p.ground>=0)return (unsigned)p.ground==b;}return false;}
static unsigned reach(bool edges[WORLD_MAX_NODES][WORLD_MAX_NODES],unsigned count,unsigned start,int omit,unsigned goal){bool seen[WORLD_MAX_NODES]={0};seen[start]=true;for(unsigned pass=0;pass<count;pass++)for(unsigned i=0;i<count;i++)if(seen[i])for(unsigned j=0;j<count;j++)if(edges[i][j]&&(int)j!=omit)seen[j]=true;return seen[goal];}
int main(int argc,char**argv){
 if(argc<2)return 2;
 FILE*f=fopen(argv[1],"rb");if(!f)return 2;unsigned char data[4096];size_t n=fread(data,1,sizeof(data),f);fclose(f);
 if(n<16)return 2;
 uint32_t checksum=(uint32_t)data[12]|((uint32_t)data[13]<<8)|((uint32_t)data[14]<<16)|((uint32_t)data[15]<<24);
 GeneratedWorld w;if(!world_parse(&w,data,n,checksum)){fprintf(stderr,"Package invalid\n");return 1;}
 bool edges[WORLD_MAX_NODES][WORLD_MAX_NODES]={0};for(unsigned i=0;i<w.count;i++)for(unsigned j=0;j<w.count;j++)if(i!=j)edges[i][j]=edge(&w,i,j);
 for(unsigned i=0;i<w.count;i++)if(!reach(edges,w.count,0,-1,i)||!reach(edges,w.count,i,-1,w.goal)){fprintf(stderr,"Runtime cannot traverse platform %u\n",i);return 1;}
 for(unsigned i=1;i<w.count;i++)if(i!=w.goal&&!reach(edges,w.count,0,(int)i,w.goal)){fprintf(stderr,"Runtime route depends on platform %u\n",i);return 1;}
 // Check failure recovery and jump height using the shipped native simulation.
 WorldPlayer p;world_reset(&p,&w);p.ground=-1;p.y=-20;world_step(&p,&w,0,0,false,false);assert(p.ground==0&&p.falls==1);
 WorldPlayer shortjump,longjump;world_reset(&shortjump,&w);world_reset(&longjump,&w);float sh=0,lh=0;
 for(int i=0;i<60;i++){world_step(&shortjump,&w,0,0,i==0,false);world_step(&longjump,&w,0,0,true,false);sh=fmaxf(sh,shortjump.y);lh=fmaxf(lh,longjump.y);}assert(lh>sh+.4f);
 assert(!world_parse(&w,data,n,checksum^1));data[n-1]^=1;assert(!world_parse(&w,data,n,checksum));data[n-1]^=1;
 printf("Native physics: both routes, all nodes, recovery, variable jump and package validation pass\n");
 if(argc>2){world_parse(&w,data,n,checksum);world_reset(&p,&w);uint16_t pixels[256*192];world_render(pixels,&w,&p);f=fopen(argv[2],"wb");if(!f)return 2;fprintf(f,"P6\n256 192\n255\n");for(unsigned i=0;i<256*192;i++){unsigned char rgb[3];for(int j=0;j<3;j++){unsigned c=(pixels[i]>>(j*5))&31;rgb[j]=(c<<3)|(c>>2);}fwrite(rgb,1,3,f);}fclose(f);}
 return 0;
}
