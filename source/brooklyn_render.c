// Port of recovered photo-world-current/index.html: same projection and scene.
// Reference canvas is 512x384, sampled at the native 256x192 screen size.
#include "brooklyn_render.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
typedef struct {int x,y;} P;
static uint16_t *fb;static const Brooklyn *state;
#define RGB(hex) ((((hex)>>19)&31)|((((hex)>>11)&31)<<5)|((((hex)>>3)&31)<<10)|32768)
static void rect(int x,int y,int w,int h,uint16_t c){int a=(int)floorf(x*.5f),b=(int)floorf(y*.5f),ex=(x+w+1)/2,ey=(y+h+1)/2;for(int j=b<0?0:b;j<ey&&j<192;j++)for(int i=a<0?0:a;i<ex&&i<256;i++)fb[j*256+i]=c;}
static P proj(float x,float z,float y){return(P){(int)roundf(256+x*22+(z-state->cam)*5),(int)roundf(280-(z-state->cam)*13-y*21)};}
static void line(P a,P b,uint16_t c,int width){int dx=abs(b.x-a.x),sx=a.x<b.x?1:-1,dy=-abs(b.y-a.y),sy=a.y<b.y?1:-1,err=dx+dy;for(;;){rect(a.x,a.y,width,width,c);if(a.x==b.x&&a.y==b.y)break;int e=2*err;if(e>=dy){err+=dy;a.x+=sx;}if(e<=dx){err+=dx;a.y+=sy;}}}
static void poly(P *p,int n,uint16_t c,bool brick){int lo=384,hi=0;for(int i=0;i<n;i++){if(p[i].y<lo)lo=p[i].y;if(p[i].y>hi)hi=p[i].y;}if(lo<0)lo=0;if(hi>383)hi=383;
 for(int y=lo;y<=hi;y++){int xs[8],num=0;for(int i=0;i<n;i++){P a=p[i],b=p[(i+1)%n];if((a.y<=y&&b.y>y)||(b.y<=y&&a.y>y))xs[num++]=a.x+(y-a.y)*(b.x-a.x)/(b.y-a.y);}for(int i=1;i<num;i++)for(int j=i;j>0&&xs[j]<xs[j-1];j--){int t=xs[j];xs[j]=xs[j-1];xs[j-1]=t;}for(int i=0;i+1<num;i+=2){rect(xs[i],y,xs[i+1]-xs[i]+1,1,c);if(brick){if(y%7==0)rect(xs[i],y,xs[i+1]-xs[i]+1,1,RGB(0xefc7a5));else for(int x=xs[i];x<=xs[i+1];x++)if((x-(y/7%2)*8)%17==0)rect(x,y,1,1,RGB(0xefc7a5));}}}
 for(int i=0;i<n;i++)line(p[i],p[(i+1)%n],RGB(0x243026),1);
}
static void floor_tile(float a,float b,float z,float d,float h,uint16_t c){P v[]={proj(a,z,h),proj(b,z,h),proj(b,z+d,h),proj(a,z+d,h)};poly(v,4,c,false);}
static void window_at(float x,float z,float y,int w,int h){P q=proj(x,z,y);rect(q.x-2,q.y-h-2,w+4,h+4,RGB(0x242d36));rect(q.x,q.y-h,w,h,RGB(0xa6d5eb));rect(q.x+3,q.y-h+3,4,h-6,RGB(0xd2eaf1));rect(q.x+w/2-1,q.y-h,2,h,RGB(0x33404b));rect(q.x,q.y-h/2,w,2,RGB(0x33404b));rect(q.x-3,q.y,w+6,4,RGB(0xd1d5c4));}
static void building(float z){float x=-10.7f,w=4,d=7,h=7;P a[]={proj(x,z,0),proj(x+w,z,0),proj(x+w,z,h),proj(x,z,h)},b[]={proj(x+w,z,0),proj(x+w,z+d,0),proj(x+w,z+d,h),proj(x+w,z,h)};poly(a,4,RGB(0xb66051),true);poly(b,4,RGB(0x964f46),true);floor_tile(x,x+w,z,d,h,RGB(0x676979));window_at(-9.9f,z,3,22,36);window_at(-8.4f,z,3,22,36);window_at(-9.9f,z,5.5f,22,32);window_at(-8.4f,z,5.5f,22,32);line(proj(-6.8f,z,0),proj(-6.8f,z,7),RGB(0x784736),3);if((int)z%24==0){P t[]={proj(-10.4f,z,7),proj(-8.6f,z,10),proj(-6.8f,z,7)};poly(t,3,RGB(0xeee3cb),false);line(t[0],t[1],RGB(0x5a473f),4);line(t[1],t[2],RGB(0x5a473f),4);line(t[1],proj(-8.6f,z,7),RGB(0x5a473f),4);}}
static void bush(float x,float z,bool flowers){P q=proj(x,z,0);uint16_t colors[]={RGB(0x315d36),RGB(0x437b43),RGB(0x568d48)};for(int i=0;i<5;i++)rect(q.x-15+i*5,q.y-17-(i%2)*7,13,20,colors[i%3]);if(flowers)for(int i=0;i<4;i++){int xx=q.x-12+i*8,yy=q.y-20-(i%2)*8;rect(xx-3,yy-4,7,7,i%2?RGB(0xecc5c9):RGB(0xe3e6a8));rect(xx-1,yy-2,2,2,RGB(0xf6edcc));}}
static void person(float x,float z,float y,bool recipient){P q=proj(x,z,y);int sx=q.x,sy=q.y,h=!recipient&&state->duck?29:43;rect(sx-12,sy-2,29,5,RGB(0x7c8a65));rect(sx-8,sy-h,16,15,RGB(0xeecba6));rect(sx-9,sy-h-3,18,7,RGB(0x282a2c));rect(sx-10,sy-h+2,3,5,RGB(0x282a2c));if(recipient){rect(sx-8,sy-h+7,7,4,RGB(0x282a2c));rect(sx+1,sy-h+7,7,4,RGB(0x282a2c));rect(sx-1,sy-h+8,2,1,RGB(0x282a2c));}else{rect(sx-4,sy-h+8,2,2,RGB(0x282a2c));rect(sx+4,sy-h+8,2,2,RGB(0x282a2c));}rect(sx-11,sy-h+15,22,h-23,recipient?RGB(0x322c39):RGB(0xed9bd7));rect(sx-10,sy-10,8,7,recipient?RGB(0x3d7bd6):RGB(0x6363ab));rect(sx+3,sy-10,8,7,recipient?RGB(0x3d7bd6):RGB(0x6363ab));rect(sx-12,sy-3,10,4,RGB(0xe4dec8));rect(sx+3,sy-3,10,4,RGB(0xe4dec8));}
static void ac(float z){P q=proj(-6.1f,z,1.8f);rect(q.x-3,q.y-24,31,26,RGB(0x4b5555));rect(q.x,q.y-23,29,25,RGB(0xe2dfc8));rect(q.x+3,q.y-20,20,17,RGB(0x717878));for(int i=0;i<6;i++)rect(q.x+4,q.y-18+i*3,18,1,RGB(0xb8c5bf));rect(q.x+25,q.y-8,2,3,RGB(0x354945));for(int i=0;i<5;i++){int a=(int)(state->time*60+i*27)%145;line((P){q.x+32+a,q.y-8+i*3},(P){q.x+48+a,q.y-12+i*3},RGB(0xedfaff),1);}}
static void draw_object(int type,float z){
 if(type==0)building(z);
 if(type==1)bush(7+((int)z%2),z,(int)z%6==0);
 if(type==2)bush(-6.7f,z,true);
 if(type==3){for(int x=-5;x<5;x++){uint16_t c=x%2?RGB(0x216d67):RGB(0xa9d5b6);floor_tile(x,x+1,26,3,1.5f,c);P q=proj(x,26,1.5f);rect(q.x,q.y,22,6,c);rect(q.x+3,q.y+6,16,3,c);}line(proj(-5,26,0),proj(-5,26,1.5f),RGB(0x333e33),1);}
 if(type==4)ac(z);
 if(type==5){P v[]={proj(-2.5f,67,0),proj(2.5f,67,0),proj(2.5f,67,5),proj(-2.5f,67,5)};poly(v,4,RGB(0xb56755),true);P q=proj(-1,67,0);rect(q.x-3,q.y-65,51,68,RGB(0x304d41));rect(q.x,q.y-62,45,62,RGB(0x64ab74));for(int i=0;i<2;i++){int y=q.y-55+i*28;line((P){q.x+5,y},(P){q.x+40,y},RGB(0x2d5140),1);line((P){q.x+5,y+22},(P){q.x+40,y+22},RGB(0x2d5140),1);line((P){q.x+5,y},(P){q.x+5,y+22},RGB(0x2d5140),1);line((P){q.x+40,y},(P){q.x+40,y+22},RGB(0x2d5140),1);}rect(q.x+36,q.y-31,3,3,RGB(0xf2d998));}
 if(type>=6&&type<=8){P q=proj(4.5f,z,0);rect(q.x,q.y-29,2,29,RGB(0x324532));rect(q.x+2,q.y-29,13,9,state->checkpoint>=type-5?RGB(0x70e245):RGB(0xfff5b0));}
 if(type==9)person(0,65,state->won?fmaxf(0,sinf(state->reward*7))*.5f:0,true);
 if(type==10){person(state->x,state->z,state->y,false);P q=proj(state->x,state->z,state->y);rect(q.x+13,q.y-22,12,19,RGB(0xfff6dd));line((P){q.x+15,q.y-27},(P){q.x+22,q.y-27},RGB(0xfff6dd),1);line((P){q.x+15,q.y-27},(P){q.x+15,q.y-20},RGB(0xfff6dd),1);line((P){q.x+22,q.y-27},(P){q.x+22,q.y-20},RGB(0xfff6dd),1);rect(q.x+16,q.y-16,6,2,RGB(0xdd6464));if(state->gliding){P v[]={{q.x-28,q.y-52},{q.x-20,q.y-75},{q.x+20,q.y-75},{q.x+28,q.y-52}};poly(v,4,RGB(0xfff9e5),false);line(v[0],(P){q.x,q.y-25},RGB(0x243026),1);line((P){q.x,q.y-25},v[3],RGB(0x243026),1);}}
}
void brooklyn_render(uint16_t*p,const Brooklyn*g){fb=p;state=g;rect(0,0,512,384,g->wind==2?RGB(0x9ca3c3):RGB(0x7cb7ef));rect(0,125,512,259,RGB(0xd5edb3));
 for(int z=(int)fminf(75,ceilf(g->cam+22));z>fmaxf(-3,g->cam-10);z--){floor_tile(-13,13,z,1,-.3f,RGB(0x8dbb70));bool hole=(z>=13&&z<16)||(z>=36&&z<43)||(z>=49&&z<52);floor_tile(-5.2f,5.2f,z,1,0,hole?RGB(0x3c4846):z%4==0?RGB(0xbac3b8):RGB(0xd4d5bc));if(!hole){floor_tile(-5.8f,-5.2f,z,1,.25f,RGB(0xb87163));floor_tile(5.2f,5.8f,z,1,.25f,RGB(0xb87163));}floor_tile(6,11,z,1,-.1f,RGB(0x597644));}
 struct Object {float z;int type;} objects[64];int n=0;
 #define ADD(z_,t_) objects[n++]=(struct Object){z_,t_}
 for(int z=0;z<75;z+=12){ADD(z,0);}for(int z=0;z<75;z+=3){ADD(z,1);if(z%6==0){ADD(z,2);}}ADD(27,3);for(int z=36;z<45;z+=4){ADD(z,4);}ADD(67,5);ADD(20,6);ADD(32,7);ADD(48,8);ADD(65,9);ADD(g->z,10);
 for(int i=1;i<n;i++)for(int j=i;j>0&&objects[j].z>objects[j-1].z;j--){struct Object a=objects[j];objects[j]=objects[j-1];objects[j-1]=a;}
 for(int i=0;i<n;i++)if(objects[i].z>g->cam-12&&objects[i].z<g->cam+24)draw_object(objects[i].type,objects[i].z);
 if(g->wind==2)for(int i=0;i<22;i++){int x=(int)(i*39+g->time*90)%512,y=(int)(i*47+g->time*120)%384;line((P){x,y},(P){x+5,y+9},RGB(0xedf6ff),1);}
 if(g->won){P q=proj(0,65,0);for(int i=0;i<7;i++){int x=q.x+cosf(i)*22,y=q.y-25-fabsf(sinf(i))*24;line((P){q.x,q.y-8},(P){x,y},RGB(0x358d22),1);rect(x-6,y-2,12,4,i%2?RGB(0xfff7af):RGB(0xf49dde));rect(x-2,y-6,4,12,i%2?RGB(0xfff7af):RGB(0xf49dde));}}
}
