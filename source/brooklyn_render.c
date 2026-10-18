#include "brooklyn_render.h"
#define C(r,g,b) ((r)|((g)<<5)|((b)<<10)|32768)
static void box(uint16_t*p,int x,int y,int w,int h,int c){for(int j=y<0?0:y;j<y+h&&j<192;j++)for(int i=x<0?0:x;i<x+w&&i<256;i++)p[j*256+i]=c;}
void brooklyn_render(uint16_t*p,const Brooklyn*g){
 int s=(int)g->x-65;if(s<0)s=0;if(s>690)s=690;
 box(p,0,0,256,192,C(25,26,24));
 for(int a=-1;a<10;a++){
 int x=a*120-s;box(p,x,30,116,105,C(20,12,9));
 for(int y=34;y<128;y+=9){box(p,x,y,116,1,C(26,19,14));for(int k=0;k<6;k++)box(p,x+k*22+(y%2)*10,y,1,9,C(26,19,14));}
 box(p,x+7,40,38,55,C(4,6,7));box(p,x+12,45,28,43,C(12,19,20));box(p,x+25,45,3,43,C(3,4,4));box(p,x+12,66,28,3,C(3,4,4));
 box(p,x+66,34,37,64,C(28,25,19));box(p,x+68,37,32,45,C(4,6,7));box(p,x+82,37,3,45,C(24,21,16));
 box(p,x+64,87,42,14,C(23,23,20));for(int k=0;k<7;k++)box(p,x+68+k*5,90,2,8,C(10,12,12));
 box(p,x,117,115,22,C(6,15,8));for(int k=0;k<7;k++)box(p,x+k*16,113+(k%3)*4,10,8,C(23,25,24));
 }
 for(int x=0;x<256;x++)if(brooklyn_ground(x+s)){box(p,x,150,1,27,C(27,26,23));box(p,x,150,1,3,C(17,14,12));}else box(p,x,150,1,27,C(6,9,12));
 box(p,230-s,105,55,31,C(3,12,12));for(int x=232;x<285;x+=10)box(p,x-s,105,4,30,C(12,20,18));
 box(p,350-s,70,29,18,C(25,25,22));for(int k=0;k<5;k++)box(p,353+k*5-s,73,2,11,C(8,10,10));
 for(int k=0;k<9;k++){int x=355+(k*19+g->ticks*2)%150;box(p,x-s,92+(k%4)*11,8,1,C(22,29,29));}
 int n=889-s,hop=g->won?(g->ticks/8%2)*3:0;
 box(p,n-12,144,35,7,C(19,10,7));box(p,n,115-hop,10,10,C(24,17,12));box(p,n-1,117-hop,12,3,C(2,3,4));box(p,n-3,125-hop,16,13,C(3,4,5));box(p,n-3,138-hop,19,6,C(9,16,25));box(p,n+9,144-hop,6,6,C(24,24,23));
 int x=(int)g->x-s,y=(int)g->y,h=g->duck?9:20;
 box(p,x-5,y-h,10,h,C(28,12,15));box(p,x-4,y-h,8,6,C(25,18,13));box(p,x-5,y-2,5,3,C(3,4,6));box(p,x+2,y-2,5,3,C(3,4,6));
 if(!g->won){int by=g->gliding?y-h-12:y-10;box(p,x+6,by,10,11,C(30,29,26));box(p,x+8,by+3,6,4,C(25,5,6));box(p,x+8,by-3,6,1,C(30,29,26));box(p,x+7,by-2,1,3,C(30,29,26));box(p,x+14,by-2,1,3,C(30,29,26));}
 else for(int k=0;k<3;k++){box(p,n-8+k*6,132,1,10,C(7,20,8));box(p,n-10+k*6,127-(k%2)*3,5,5,C(31,12+k*4,20));}
}
