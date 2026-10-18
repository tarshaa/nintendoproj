#include "game.h"
#include <string.h>
static const int pickups[3] = {110, 330, 535};
bool game_ground(float x) { return !((x>174 && x<210)||(x>404 && x<445)); }
void game_reset(Game *g) { memset(g,0,sizeof(*g)); g->x=20; g->y=140; g->grounded=true; }
void game_step(Game *g,int direction,bool jump,bool duck) {
 if(g->won||g->lost)return;
 if(++g->ticks>=90*60){g->lost=true;return;}
 g->duck=duck && g->grounded;
 float oldx=g->x, oldy=g->y;
 g->vx=direction*(g->duck?1.1f:2.0f);
 if(jump&&g->grounded&&!g->duck){g->vy=-5.8f;g->grounded=false;}
 g->x+=g->vx; if(g->x<8)g->x=8; if(g->x>705)g->x=705;
 // Low canopy: crouch to fit. Airborne players can land on top.
 int h=g->duck?8:18;
 if(g->x+6>275 && g->x-6<315 && g->y>107 && g->y-h<129) g->x=oldx;
 g->vy+=0.28f; g->y+=g->vy; g->grounded=false;
 if(g->vy>=0 && oldy<=107 && g->y>=107 && g->x+5>275 && g->x-5<315){g->y=107;g->vy=0;g->grounded=true;}
 if(g->vy>=0 && oldy<=140 && g->y>=140 && game_ground(g->x)){g->y=140;g->vy=0;g->grounded=true;}
 if(g->x>230&&g->x<270&&g->grounded)g->checkpoint=240;
 if(g->x>465&&g->grounded)g->checkpoint=470;
 if(g->y>200){g->x=g->checkpoint?g->checkpoint:20;g->y=140;g->vy=0;g->grounded=true;}
 for(int i=0;i<3;i++) if(g->x>pickups[i]-12 && g->x<pickups[i]+12 && g->y>105 && g->y<151)g->collected|=1<<i;
 if(g->x>680 && g->collected==7)g->won=true;
}
