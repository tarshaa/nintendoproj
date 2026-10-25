#include "generated_world.h"
#include <math.h>
#include <string.h>
static uint32_t u32(const unsigned char *p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static unsigned u16(const unsigned char*p){return p[0]|(p[1]<<8);}
static float real(const unsigned char*p){uint32_t v=u32(p);float f;memcpy(&f,&v,4);return f;}
static uint32_t crc(const unsigned char*p,size_t n){uint32_t c=~0u;while(n--){c^=*p++;for(int j=0;j<8;j++)c=(c>>1)^((0u-(c&1u))&0xEDB88320u);}return ~c;}
static bool string_ok(const unsigned char*p,size_t n){size_t i=0;for(;i<n&&p[i];i++)if(p[i]<32||p[i]>126)return false;return i>0&&i<n;}
bool world_parse(GeneratedWorld*w,const void*bytes,size_t size,uint32_t checksum){
 const unsigned char*p=bytes;if(!w||!p||size<164||size>164+44*WORLD_MAX_NODES)return false;
 if(memcmp(p,"PWL1",4)||u32(p+4)!=size-12||crc(p+12,size-12)!=u32(p+8))return false;
 unsigned count=u16(p+16),goal=u16(p+18);
 if(u32(p+12)!=checksum||count<8||count>WORLD_MAX_NODES||goal==0||goal>=count||size!=164+44*count)return false;
 if(!string_ok(p+20,48)||!string_ok(p+68,96))return false;
 GeneratedWorld v={0};v.photo_checksum=checksum;v.count=count;v.goal=goal;memcpy(v.title,p+20,48);memcpy(v.objective,p+68,96);
 for(unsigned i=0;i<count;i++){
  const unsigned char*q=p+164+44*i;WorldNode*n=&v.nodes[i];n->x=real(q);n->z=real(q+4);n->y=real(q+8);n->r=real(q+12);n->color=u16(q+16);n->shape=q[18];n->flags=q[19];memcpy(n->label,q+20,24);
  if(!isfinite(n->x)||!isfinite(n->z)||!isfinite(n->y)||!isfinite(n->r)||fabsf(n->x)>80||fabsf(n->z)>80||n->y<0||n->y>40||n->r<2||n->r>4||n->color>32767||n->shape>6||n->flags>3||((n->flags&2)&&n->shape!=1)||!string_ok(q+20,24))return false;
 }
 if(!(v.nodes[0].flags&1)||!(v.nodes[goal].flags&1))return false;
 *w=v;return true;
}
void world_reset(WorldPlayer*p,const GeneratedWorld*w){memset(p,0,sizeof(*p));p->ground=0;world_retry(p,w);p->falls=0;p->cam_x=p->x;p->cam_z=p->z;p->cam_y=p->y;}
void world_retry(WorldPlayer*p,const GeneratedWorld*w){const WorldNode*n=&w->nodes[p->checkpoint];p->x=n->x;p->z=n->z;p->y=n->y;p->ground=p->checkpoint;p->vx=p->vz=p->vy=p->buffer=0;p->coyote=.1f;p->held=false;p->falls++;}
static float approach(float a,float b,float delta){return a<b?fminf(a+delta,b):fmaxf(a-delta,b);}
void world_step(WorldPlayer*p,const GeneratedWorld*w,float x,float z,bool jump,bool careful){
 const float dt=1.f/60;p->t+=dt;
 if(p->won){p->reward+=dt;return;}
 if(jump&&!p->held)p->buffer=.14f;else p->buffer=fmaxf(0,p->buffer-dt);p->held=jump;
 p->coyote=p->ground>=0?.11f:fmaxf(0,p->coyote-dt);
 float len=hypotf(x,z);if(len>1){x/=len;z/=len;}
 float speed=careful?3.2f:7.4f,accel=p->ground>=0?60:34;
 p->vx=approach(p->vx,x*speed,accel*dt);p->vz=approach(p->vz,z*speed,accel*dt);
 if(p->buffer>0&&p->coyote>0){p->vy=p->ground>=0&&(w->nodes[p->ground].flags&2)?13.8f:10.8f;p->ground=-1;p->buffer=p->coyote=0;}
 if(!jump&&p->vy>4.8f)p->vy=4.8f;
 float old_y=p->y;p->x+=p->vx*dt;p->z+=p->vz*dt;
 if(p->ground>=0){const WorldNode*n=&w->nodes[p->ground];if(hypotf(p->x-n->x,p->z-n->z)>n->r)p->ground=-1;}
 if(p->ground<0){p->vy-=24*dt;p->y+=p->vy*dt;
  int landed=-1;float best=-100;
  if(p->vy<=0)for(unsigned i=0;i<w->count;i++){const WorldNode*n=&w->nodes[i];if(old_y>=n->y-.04f&&p->y<=n->y&&n->y>best&&hypotf(p->x-n->x,p->z-n->z)<=n->r){landed=(int)i;best=n->y;}}
  if(landed>=0){p->ground=landed;p->y=best;p->vy=0;if(w->nodes[landed].flags&1)p->checkpoint=landed;if((unsigned)landed==w->goal)p->won=true;}
 }
 if(p->y<w->nodes[p->checkpoint].y-10||fabsf(p->x)>90||fabsf(p->z)>90)world_retry(p,w);
 p->cam_x+=(p->x-p->cam_x)*.12f;p->cam_z+=(p->z-p->cam_z)*.12f;p->cam_y+=(p->y-p->cam_y)*.08f;
}
