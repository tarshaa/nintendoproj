// Camera transfer sequence follows the CC0 BlocksDS camera example.
#include <nds.h>
#include <fat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "game.h"
#define W 256
#define H 192
#define PIXELS (W*H)
#define CHANNEL 1
#define INK RGB15(3,4,7)
#define WHITE RGB15(31,31,29)
static u16 *top,*bottom;
static u16 photo[PIXELS] __attribute__((aligned(32)));
static u16 frame[PIXELS] __attribute__((aligned(32)));
static bool storage, camera_ready,transfer,has_frame;
static CameraDevice camera=CAMERA_OUTER;
static int pending_camera,frame_wait,selection,save_id,ids[999],count,chosen;
static char message[64];
static Game game;
typedef enum {HOME,CAMERA,REVIEW,LIBRARY,PLAY,PAUSE,WIN,LOSE,ERROR_SCREEN} State;
static State state=HOME;
static u16 sky=RGB15(12,22,31),grass=RGB15(12,25,8);
static const unsigned char glyphs[][7]={
{14,17,17,31,17,17,17},{30,17,17,30,17,17,30},{14,17,16,16,16,17,14},{30,17,17,17,17,17,30},
{31,16,16,30,16,16,31},{31,16,16,30,16,16,16},{14,17,16,23,17,17,15},{17,17,17,31,17,17,17},
{14,4,4,4,4,4,14},{7,2,2,2,18,18,12},{17,18,20,24,20,18,17},{16,16,16,16,16,16,31},
{17,27,21,21,17,17,17},{17,25,21,19,17,17,17},{14,17,17,17,17,17,14},{30,17,17,30,16,16,16},
{14,17,17,17,21,18,13},{30,17,17,30,20,18,17},{15,16,16,14,1,1,30},{31,4,4,4,4,4,4},
{17,17,17,17,17,17,14},{17,17,17,17,17,10,4},{17,17,17,21,21,21,10},{17,17,10,4,10,17,17},
{17,17,10,4,4,4,4},{31,1,2,4,8,16,31},
{14,17,19,21,25,17,14},{4,12,4,4,4,4,14},{14,17,1,2,4,8,31},{30,1,1,14,1,1,30},
{2,6,10,18,31,2,2},{31,16,16,30,1,1,30},{14,16,16,30,17,17,14},{31,1,2,4,8,8,8},
{14,17,17,14,17,17,14},{14,17,17,15,1,1,14}};
static void rect(u16 *p,int x,int y,int w,int h,u16 c){
 for(int yy=y<0?0:y;yy<y+h&&yy<H;yy++)for(int xx=x<0?0:x;xx<x+w&&xx<W;xx++)p[yy*W+xx]=c|BIT(15);
}
static void text(u16 *p,int x,int y,const char*s,u16 c){
 for(;*s;s++,x+=6){int n=*s>='A'&&*s<='Z'?*s-'A':*s>='0'&&*s<='9'?26+*s-'0':-1;
 if(n>=0)for(int j=0;j<7;j++)for(int i=0;i<5;i++)if(glyphs[n][j]&(16>>i))rect(p,x+i,y+j,1,1,c);

 if(*s=='-')rect(p,x+1,y+3,3,1,c);
 if(*s==':'){rect(p,x+2,y+2,1,1,c);rect(p,x+2,y+5,1,1,c);}
 }
}
static void title(const char*s){rect(frame,0,0,W,H,WHITE);text(frame,14,14,s,INK);}
static void button(int y,const char*s,bool active){rect(frame,12,y,232,30,active?INK:RGB15(25,27,27));text(frame,22,y+11,s,active?WHITE:INK);}
static void path_for(char *p,int id,const char*ext){snprintf(p,96,"sd:/photo-worlds/world-%03d.%s",id,ext);}
static unsigned checksum(const u16*p){unsigned h=2166136261u;for(int i=0;i<PIXELS;i++){h^=p[i];h*=16777619u;}return h;}
static void palette(void){
 unsigned r=0,g=0,b=0,n=0;for(int i=0;i<PIXELS/2;i+=19){r+=photo[i]&31;g+=(photo[i]>>5)&31;b+=(photo[i]>>10)&31;n++;}
 sky=RGB15(8+r/n*2/3,8+g/n*2/3,8+b/n*2/3);
 r=g=b=n=0;for(int i=PIXELS/2;i<PIXELS;i+=19){r+=photo[i]&31;g+=(photo[i]>>5)&31;b+=(photo[i]>>10)&31;n++;}
 grass=RGB15(r/n,g/n,b/n);
}
static bool load_world(int id){
 char p[96];path_for(p,id,"pwg");FILE*f=fopen(p,"rb");if(!f)return false;
 unsigned head[3];bool ok=fread(head,sizeof(head),1,f)==1&&head[0]==0x50574731&&head[1]==PIXELS;
 if(ok)ok=fread(photo,sizeof(photo),1,f)==1;
 fclose(f);if(ok)ok=checksum(photo)==head[2];
 if(ok){save_id=id;palette();memcpy(top,photo,sizeof(photo));}return ok;
}
static void scan_worlds(void){count=0;if(!storage)return;for(int i=1;i<=999;i++){char p[96];struct stat st;path_for(p,i,"pwg");if(stat(p,&st)==0)ids[count++]=i;}}
static bool save_world(void){
 if(!storage){strcpy(message,"SD UNAVAILABLE - NOT SAVED");return false;}
 int id;for(id=1;id<=999;id++){char p[96];struct stat st;path_for(p,id,"pwg");if(stat(p,&st)!=0)break;}
 if(id>999){strcpy(message,"LIBRARY FULL - NOT SAVED");return false;}
 char tmp[96],dest[96];path_for(tmp,id,"tmp");path_for(dest,id,"pwg");
 FILE*f=fopen(tmp,"wb");if(!f){strcpy(message,"CANNOT WRITE TO SD");return false;}
 unsigned head[3]={0x50574731,PIXELS,checksum(photo)};
 bool ok=fwrite(head,sizeof(head),1,f)==1&&fwrite(photo,sizeof(photo),1,f)==1;
 if(fclose(f)!=0)ok=false;
 if(ok)ok=rename(tmp,dest)==0;
 if(!ok){remove(tmp);strcpy(message,"SAVE FAILED - TRY AGAIN");return false;}
 save_id=id;strcpy(message,"SAVED ON SD");return true;
}
static void stop_camera(void){
 if(!camera_ready)return;
 // Do not wait forever on a failed camera transfer.
 for(int i=0;i<120&&ndmaBusy(CHANNEL);i++)swiWaitForVBlank();
 cameraStopTransfer();REG_NDMA_CR(CHANNEL)=0;cameraDeinit();camera_ready=false;transfer=false;
}
static void start_camera(void){
 if(!isDSiMode()){strcpy(message,"NEEDS DSI MODE - USE UNLAUNCH");state=ERROR_SCREEN;return;}
 has_frame=false;transfer=false;frame_wait=0;pending_camera=0;
 if(!cameraInit()){strcpy(message,"CAMERA INIT FAILED");state=ERROR_SCREEN;return;}
 camera_ready=true;
 if(!cameraSelect(camera)){stop_camera();strcpy(message,"CAMERA SELECT FAILED");state=ERROR_SCREEN;return;}
 state=CAMERA;
}
static void camera_tick(int pressed){
 pending_camera |= pressed & (KEY_A|KEY_L|KEY_R|KEY_Y);
 if(transfer && !ndmaBusy(CHANNEL)&&!cameraTransferActive()){transfer=false;has_frame=true;frame_wait=0;}
 if(pressed&KEY_B){stop_camera();state=HOME;return;}
 if(transfer){if(++frame_wait>180){stop_camera();strcpy(message,"CAMERA TIMEOUT - B TO RETURN");state=ERROR_SCREEN;}return;}
 pressed |= pending_camera; pending_camera=0;
 if((pressed&(KEY_A|KEY_L|KEY_R))&&has_frame){memcpy(photo,top,sizeof(photo));stop_camera();palette();state=REVIEW;message[0]=0;return;}
 if(pressed&KEY_Y){camera=camera==CAMERA_OUTER?CAMERA_INNER:CAMERA_OUTER;if(!cameraSelect(camera)){stop_camera();strcpy(message,"CAMERA SWITCH FAILED");state=ERROR_SCREEN;return;}has_frame=false;}
 transfer=cameraStartTransfer(top,MCUREG_APT_SEQ_CMD_PREVIEW,CHANNEL);
 if(!transfer){stop_camera();strcpy(message,"CAMERA TRANSFER FAILED");state=ERROR_SCREEN;}
}
static void draw_game(void){
 rect(frame,0,0,W,H,sky);int scroll=(int)game.x-70;if(scroll<0)scroll=0;if(scroll>464)scroll=464;
 for(int x=0;x<W;x++)if(game_ground(x+scroll))rect(frame,x,140,1,35,grass);
 rect(frame,275-scroll,107,40,22,INK);
 text(frame,273-scroll,95,"DUCK",WHITE);
 int px=(int)game.x-scroll,py=(int)game.y,h=game.duck?8:18;
 rect(frame,px-6,py-h,12,h,RGB15(29,9,16));rect(frame,px+1,py-h+3,3,3,WHITE);
 const int loc[3]={110,330,535};for(int i=0;i<3;i++)if(!(game.collected&(1<<i)){
 int x=loc[i]-scroll;rect(frame,x-2,117,5,13,RGB15(31,28,4));rect(frame,x-6,121,13,5,RGB15(31,28,4));}
 rect(frame,690-scroll,103,3,37,INK);rect(frame,693-scroll,103,16,12,RGB15(30,12,20));
 rect(frame,0,0,256,29,WHITE);int n=(game.collected&1)+((game.collected>>1)&1)+((game.collected>>2)&1);
 char line[42];snprintf(line,sizeof(line),"STARS %d OF 3    TIME %d",n,90-game.ticks/60);text(frame,8,5,line,INK);
 text(frame,8,17,"COLLECT 3 STARS - REACH FLAG",INK);
 rect(frame,0,176,256,16,WHITE);text(frame,5,180,"A JUMP  HOLD B DUCK  START PAUSE",INK);
 if(game.ticks>75*60)text(frame,8,36,"HURRY",WHITE);
}
static void render(void){
 switch(state){
 case HOME:title("PHOTO WORLDS - TEST BUILD");button(55,"TAKE A PHOTO",selection==0);button(100,"SAVED GAMES",selection==1);text(frame,14,154,"UP DOWN SELECT - A OPEN",INK);text(frame,14,170,storage?"SD READY":"SD UNAVAILABLE",INK);break;
 case CAMERA:title("CAMERA");text(frame,14,55,"A OR L OR R - TAKE PHOTO",INK);text(frame,14,80,"Y - SWITCH CAMERA",INK);text(frame,14,105,"B - HOME",INK);text(frame,14,153,"PREVIEW CAPTURE 256 X 192",INK);break;
 case REVIEW:title("PHOTO READY");text(frame,12,45,"OFFLINE TEST: PHOTO COLOURS",INK);text(frame,12,61,"SHAPE THE LEVEL PALETTE",INK);text(frame,12,94,"A - SAVE AND PLAY",INK);text(frame,12,113,"B - RETAKE",INK);text(frame,12,150,message,INK);break;
 case LIBRARY:title("SAVED GAMES");if(count){char s[40];snprintf(s,sizeof(s),"WORLD %03d - %d OF %d",ids[chosen],chosen+1,count);text(frame,12,60,s,INK);text(frame,12,95,"LEFT RIGHT - BROWSE",INK);text(frame,12,115,"A - PLAY   B - HOME",INK);}else{text(frame,12,70,"NO SAVED GAMES YET",INK);text(frame,12,100,"B - HOME TO TAKE A PHOTO",INK);}text(frame,12,157,message,INK);break;
 case PLAY:draw_game();break;
 case PAUSE:title("PAUSED");text(frame,12,65,"START - RESUME",INK);text(frame,12,90,"A - RESTART",INK);text(frame,12,115,"B - HOME",INK);break;
 case WIN:title("YOU BROUGHT THEM HOME");for(int i=0;i<3;i++){int x=83+i*40;rect(frame,x,90,2,35,grass);rect(frame,x-6,80,14,14,RGB15(30,12+i*4,20));rect(frame,x-2,84,6,6,RGB15(31,27,4));}text(frame,40,143,"A LITTLE GARDEN FOR YOU",INK);text(frame,20,171,"A REPLAY  B HOME",INK);break;
 case LOSE:title("TIME IS UP");text(frame,20,80,"YOUR WORLD IS STILL SAVED",INK);text(frame,20,115,"A RETRY  B HOME",INK);break;
 case ERROR_SCREEN:title("COULD NOT CONTINUE");text(frame,8,65,message,INK);text(frame,12,125,"B - HOME",INK);break;
 }
 memcpy(bottom,frame,sizeof(frame));
}
int main(void){
 videoSetMode(MODE_5_2D);videoSetModeSub(MODE_5_2D);lcdMainOnTop();
 vramSetBankA(VRAM_A_MAIN_BG);vramSetBankC(VRAM_C_SUB_BG);
 int t=bgInit(3,BgType_Bmp16,BgSize_B16_256x256,0,0);
 int b=bgInitSub(3,BgType_Bmp16,BgSize_B16_256x256,0,0);
 top=bgGetGfxPtr(t);bottom=bgGetGfxPtr(b);rect(top,0,0,W,H,RGB15(16,23,29));text(top,65,88,"PHOTO WORLDS",WHITE);
 storage=fatInitDefault();if(storage){struct stat st;if(stat("sd:/photo-worlds",&st)!=0)storage=mkdir("sd:/photo-worlds",0777)==0;else storage=S_ISDIR(st.st_mode);}
 while(1){swiWaitForVBlank();scanKeys();int p=keysDown(),held=keysHeld();
 switch(state){
 case HOME:
 if(p&(KEY_UP|KEY_DOWN))selection^=1;
 if(p&KEY_TOUCH){touchPosition touch;touchRead(&touch);if(touch.px>=12&&touch.px<244){if(touch.py>=55&&touch.py<85){selection=0;p|=KEY_A;}if(touch.py>=100&&touch.py<130){selection=1;p|=KEY_A;}}}
 if(p&KEY_A){message[0]=0;if(!selection)start_camera();else{scan_worlds();chosen=count?count-1:0;state=LIBRARY;if(count&&!load_world(ids[chosen]))strcpy(message,"FILE DAMAGED - CHOOSE ANOTHER");}}break;
 case CAMERA:camera_tick(p);break;
 case REVIEW:if(p&KEY_B)start_camera();else if(p&KEY_A){if(save_world()){game_reset(&game);state=PLAY;}}break;
 case LIBRARY:
 if(p&KEY_B)state=HOME;else if(count){bool change=false;if(p&KEY_LEFT){chosen=(chosen+count-1)%count;change=true;}if(p&KEY_RIGHT){chosen=(chosen+1)%count;change=true;}if(change){message[0]=0;if(!load_world(ids[chosen]))strcpy(message,"FILE DAMAGED - CHOOSE ANOTHER");}if((p&KEY_A)&&load_world(ids[chosen])){game_reset(&game);state=PLAY;}}break;
 case PLAY:if(p&KEY_START)state=PAUSE;else{game_step(&game,!!(held&KEY_RIGHT)-!!(held&KEY_LEFT),p&KEY_A,held&KEY_B);if(game.won)state=WIN;if(game.lost)state=LOSE;}break;
 case PAUSE:if(p&KEY_START)state=PLAY;else if(p&KEY_A){game_reset(&game);state=PLAY;}else if(p&KEY_B)state=HOME;break;
 case WIN:case LOSE:if(p&KEY_A){game_reset(&game);state=PLAY;}else if(p&KEY_B)state=HOME;break;
 case ERROR_SCREEN:if(p&KEY_B)state=HOME;break;
 }
 render();
 }
}
