#include "generated_world.h"
#include <math.h>
#include <stdlib.h>
// Native 256x192 raster presentation of a three-coordinate world. This first
// network test reuses the existing bitmap display; it is not the final 3D art port.
typedef struct {float x,y,s;} Point;
static uint16_t *pixels;static const WorldPlayer *player;
static uint16_t shade(uint16_t c,float f){int r=(int)((c&31)*f),g=(int)(((c>>5)&31)*f),b=(int)(((c>>10)&31)*f);return (r>31?31:r)|((g>31?31:g)<<5)|((b>31?31:b)<<10)|32768;}
#define C(r,g,b) ((r)|((g)<<5)|((b)<<10)|32768)
static Point project(float x,float z,float y){float dz=z-player->cam_z,dy=y-player->cam_y,d=22+dz*.90f-dy*.42f;float s=190/fmaxf(6,d);return (Point){128+(x-player->cam_x)*s,134-(dy*.90f+dz*.42f)*s,s};}
static void rect(int x,int y,int w,int h,uint16_t c){int x1=x<0?0:x,y1=y<0?0:y,x2=x+w>256?256:x+w,y2=y+h>192?192:y+h;for(int yy=y1;yy<y2;yy++)for(int xx=x1;xx<x2;xx++)pixels[yy*256+xx]=c|32768;}
static void ellipse(float x,float y,float rx,float ry,uint16_t c){if(rx<.1f||ry<.1f)return;int a=(int)(y-ry),b=(int)(y+ry);if(a<0)a=0;if(b>191)b=191;for(int yy=a;yy<=b;yy++){float t=(yy-y)/ry;int half=(int)(rx*sqrtf(fmaxf(0,1-t*t)));rect((int)x-half,yy,2*half+1,1,c);}}
static void line(float x,float y,float xx,float yy,uint16_t c){float dx=xx-x,dy=yy-y;int steps=(int)fmaxf(fabsf(dx),fabsf(dy));if(steps>1024)return;for(int i=0;i<=steps;i++){float t=steps?(float)i/steps:0;rect((int)(x+dx*t),(int)(y+dy*t),1,1,c);}}
static void node(const WorldNode*n,bool goal,bool checkpoint,float time){
 Point q=project(n->x,n->z,n->y);float r=n->r*q.s;if(q.x+r<0||q.x-r>256||q.y+r<20||q.y-r>192)return;
 uint16_t color=n->color|32768;
 if(n->shape==0){ // Domed cap, visible ivory stem, cream rim and gill strokes.
  rect((int)(q.x-r*.22f),(int)q.y,(int)(r*.44f),(int)(r*.95f),C(29,28,24));
  rect((int)(q.x-r*.17f),(int)q.y,(int)(r*.1f),(int)(r*.95f),C(25,23,19));
  ellipse(q.x,q.y+r*.05f,r,r*.42f,C(30,29,25));
  for(int i=-4;i<=4;i++)line(q.x,q.y+r*.13f,q.x+i*r*.2f,q.y+r*.3f,C(24,22,18));
  ellipse(q.x,q.y-r*.20f,r,r*.52f,color);
  ellipse(q.x-r*.2f,q.y-r*.36f,r*.48f,r*.17f,shade(color,1.17f));
  for(int i=0;i<4;i++)rect((int)(q.x-r*.55f+i*r*.32f),(int)(q.y-r*.25f-(i%2)*r*.15f),2,1,shade(color,1.3f));
 }else if(n->shape==1){
  ellipse(q.x,q.y,r,r*.34f,shade(color,.72f));ellipse(q.x,q.y-2,r*.95f,r*.3f,color);
  line(q.x-r,q.y,q.x+r,q.y,C(22,28,14));for(int i=-3;i<=3;i++){float x=q.x+i*r*.22f;line(x,q.y,x+r*.12f,q.y-r*.18f,C(18,25,10));}
  if(n->flags&2){float pulse=fabsf(sinf(time*3));line(q.x-r*.3f,q.y-r*(.1f+.2f*pulse),q.x,q.y-r*(.3f+.2f*pulse),C(30,28,11));line(q.x,q.y-r*(.3f+.2f*pulse),q.x+r*.3f,q.y-r*(.1f+.2f*pulse),C(30,28,11));}
 }else if(n->shape==4){ellipse(q.x,q.y+r*.2f,r,r*.6f,shade(color,.7f));ellipse(q.x,q.y,r,r*.62f,color);ellipse(q.x-r*.25f,q.y-r*.2f,r*.28f,r*.13f,shade(color,1.25f));rect((int)q.x,(int)(q.y-r*.65f),2,(int)(r*.2f),C(12,16,5));}
 else if(n->shape==5){ellipse(q.x,q.y+3,r,r*.40f,shade(color,.8f));ellipse(q.x,q.y,r,r*.4f,color);ellipse(q.x,q.y,r*.78f,r*.29f,C(31,30,27));}
 else {rect((int)(q.x-r),(int)(q.y-r*.24f),(int)(2*r),(int)(r*.7f),shade(color,.72f));rect((int)(q.x-r),(int)(q.y-r*.4f),(int)(2*r),(int)(r*.4f),color);if(n->shape==2)for(int i=0;i<3;i++)line(q.x-r+2,q.y+i*2,q.x+r-2,q.y+i*2,C(31,30,26));if(n->shape==6)for(int i=-3;i<=3;i++)line(q.x+i*r*.23f,q.y-r*.35f,q.x+i*r*.23f,q.y+r*.35f,shade(color,1.2f));}
 if(checkpoint){ellipse(q.x,q.y+2,r*.38f,2,C(30,25,8));ellipse(q.x,q.y+2,r*.29f,1,C(31,30,25));}
 if(goal){float t=time*2;line(q.x,q.y-8,q.x+4+sinf(t)*2,q.y-30,C(20,18,12));ellipse(q.x+4+sinf(t)*2,q.y-35,5,7,C(30,10,16));rect((int)q.x-5,(int)q.y-8,10,7,C(23,15,7));for(int i=0;i<4;i++)line(q.x-4+i*3,q.y-8,q.x-4+i*3,q.y-2,C(29,23,13));}
}
static void character(const WorldPlayer*p){Point q=project(p->x,p->z,p->y+(p->won?fabsf(sinf(p->reward*7))*.4f:0));float s=q.s*.15f;int x=(int)q.x,y=(int)q.y;
 ellipse(q.x,q.y+2,6*s,2*s,C(17,19,16));
 rect(x-4*s,y-10*s,3*s,9*s,C(10,12,22));rect(x+1*s,y-10*s,3*s,9*s,C(10,12,22));
 rect(x-5*s,y-20*s,10*s,13*s,C(30,14,16));rect(x-3*s,y-19*s,6*s,11*s,C(31,29,23));
 rect(x-6*s,y-30*s,12*s,11*s,C(29,22,16));rect(x-6*s,y-32*s,12*s,5*s,C(7,5,6));
 rect(x-3*s,y-25*s,1*s,2*s,C(4,4,5));rect(x+3*s,y-25*s,1*s,2*s,C(4,4,5));
 rect(x-7*s,y-18*s,2*s,6*s,C(29,22,16));rect(x+5*s,y-18*s,2*s,6*s,C(29,22,16));
}
void world_render(uint16_t*fb,const GeneratedWorld*w,const WorldPlayer*p){pixels=fb;player=p;rect(0,0,256,192,C(28,30,27));
 unsigned order[WORLD_MAX_NODES];for(unsigned i=0;i<w->count;i++)order[i]=i;
 for(unsigned i=1;i<w->count;i++)for(unsigned j=i;j>0&&w->nodes[order[j]].z>w->nodes[order[j-1]].z;j--){unsigned t=order[j];order[j]=order[j-1];order[j-1]=t;}
 bool drawn=false;for(unsigned i=0;i<w->count;i++){unsigned index=order[i];const WorldNode*n=&w->nodes[index];if(!drawn&&n->z<p->z-.3f){character(p);drawn=true;}node(n,index==w->goal,(int)index==p->checkpoint,p->t);}
 if(!drawn)character(p);
 if(p->won&&p->reward<4)for(int i=0;i<45;i++){int x=(i*53+(int)(sinf(i+p->reward)*25)+256)%256,y=((int)(p->reward*45)+i*13)%192;rect(x,y,2,3,i%3==0?C(31,13,17):i%3==1?C(10,25,22):C(30,26,9));}
}
