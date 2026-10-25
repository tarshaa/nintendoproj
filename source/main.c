// Camera transfer sequence follows the CC0 BlocksDS camera example.
#include <nds.h>
#include <fat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "game.h"
#include "input_queue.h"
#include "brooklyn_render.h"
#include "calendar.h"
#include "generated_world.h"
#ifdef PHOTO_NETWORK
#include "photo_network.h"
#endif
#include <time.h>
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
static int pending_camera,frame_wait,selection,save_id,ids[1000],count,chosen;
static char message[64];
static InputQueue inputs;
static unsigned render_ticks=1;
static void sample_input(void){scanKeys();input_push(&inputs,(InputSample){keysDown(),keysHeld()});}
static void clear_inputs(void){int irq=enterCriticalSection();inputs.start=inputs.count=0;leaveCriticalSection(irq);}
static Game game;
static Brooklyn brooklyn;
static bool active_brooklyn;
static bool active_generated;
static GeneratedWorld generated;
static WorldPlayer generated_player;
static Date dates[1000],selected_date;
static unsigned today(void){time_t t=time(NULL);struct tm *d=localtime(&t);Date v={d?d->tm_year+1900:2026,d?d->tm_mon+1:10,d?d->tm_mday:18};if(!date_valid(v))v=(Date){2026,10,18};return date_pack(v);}
static void reset_game(void){clear_inputs();game_reset(&game);brooklyn_reset(&brooklyn);if(active_generated)world_reset(&generated_player,&generated);}
typedef enum {HOME,CAMERA,REVIEW,LIBRARY,PLAY,PAUSE,WIN,LOSE,ERROR_SCREEN,GENERATE,GENERATION_ERROR} State;
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
 for(;*s;s++,x+=6){char ch=*s>='a'&&*s<='z'?*s-'a'+'A':*s;int n=ch>='A'&&ch<='Z'?ch-'A':ch>='0'&&ch<='9'?26+ch-'0':-1;
 if(n>=0)for(int j=0;j<7;j++)for(int i=0;i<5;i++)if(glyphs[n][j]&(16>>i))rect(p,x+i,y+j,1,1,c);

 if(*s=='-')rect(p,x+1,y+3,3,1,c);
 if(*s==':'){rect(p,x+2,y+2,1,1,c);rect(p,x+2,y+5,1,1,c);}
 }
}
static void title(const char*s){rect(frame,0,0,W,H,WHITE);text(frame,14,14,s,INK);}
static void button(int y,const char*s,bool active){rect(frame,12,y,232,30,active?INK:RGB15(25,27,27));text(frame,22,y+11,s,active?WHITE:INK);}
static void path_for(char *p,int id,const char*ext){if(id==0)snprintf(p,96,"sd:/photo-worlds/brooklyn.%s",ext);else snprintf(p,96,"sd:/photo-worlds/world-%03d.%s",id,ext);}
static unsigned checksum(const u16*p){unsigned h=2166136261u;for(int i=0;i<PIXELS;i++){h^=p[i];h*=16777619u;}return h;}
static bool load_generated(int id){
 char path[96];path_for(path,id,"pwl");FILE*f=fopen(path,"rb");if(!f){active_generated=false;return true;}
 unsigned char data[4096];size_t n=fread(data,1,sizeof(data),f);bool complete=feof(f);fclose(f);
 active_generated=complete&&world_parse(&generated,data,n,checksum(photo));return active_generated;
}
#ifdef PHOTO_NETWORK
static void begin_generation(void){char path[96];path_for(path,save_id,"pwg");clear_inputs();state=photo_network_begin(path)?GENERATE:GENERATION_ERROR;}
static bool save_generated(void){
 size_t n;const unsigned char*data=photo_network_result(&n);if(!world_parse(&generated,data,n,checksum(photo)))return false;
 char dest[96],temp[96];path_for(dest,save_id,"pwl");path_for(temp,save_id,"part");FILE*f=fopen(temp,"wb");if(!f)return false;
 bool ok=fwrite(data,1,n,f)==n;if(fclose(f)!=0)ok=false;if(ok)ok=rename(temp,dest)==0;if(!ok)remove(temp);
 if(ok){active_generated=true;active_brooklyn=false;}return ok;
}
#endif
static void palette(void){
 unsigned r=0,g=0,b=0,n=0;for(int i=0;i<PIXELS/2;i+=19){r+=photo[i]&31;g+=(photo[i]>>5)&31;b+=(photo[i]>>10)&31;n++;}
 sky=RGB15(8+r/n*2/3,8+g/n*2/3,8+b/n*2/3);
 r=g=b=n=0;for(int i=PIXELS/2;i<PIXELS;i+=19){r+=photo[i]&31;g+=(photo[i]>>5)&31;b+=(photo[i]>>10)&31;n++;}
 grass=RGB15(r/n,g/n,b/n);
}
static bool load_world(int id){
 char p[96];path_for(p,id,"pwg");FILE*f=fopen(p,"rb");if(!f)return false;
 unsigned head[3];bool ok=fread(head,sizeof(head),1,f)==1&&head[0]==(id==0?0x50574231u:0x50574731u)&&head[1]==PIXELS;
 if(ok)ok=fread(photo,sizeof(photo),1,f)==1;
 fclose(f);if(ok)ok=checksum(photo)==head[2];
 if(ok){active_brooklyn=id==0;save_id=id;palette();memcpy(top,photo,sizeof(photo));active_generated=false;if(id>0)ok=load_generated(id);}return ok;
}
static void scan_worlds(void){
 count=0;if(!storage)return;
 for(int i=0;i<=999;i++){char p[96];struct stat st;path_for(p,i,"pwg");if(stat(p,&st)!=0)continue;
 unsigned d=0;FILE*f=fopen(p,"rb");if(f){if(fseek(f,12+sizeof(photo),SEEK_SET)==0)fread(&d,4,1,f);fclose(f);}
 Date day=date_unpack(d);if(!date_valid(day)){struct tm*t=localtime(&st.st_mtime);day=(Date){t?t->tm_year+1900:2026,t?t->tm_mon+1:10,t?t->tm_mday:18};if(!date_valid(day))day=date_unpack(today());}
 ids[count]=i;dates[count++]=day;
 }
}
static int day_count(Date d){int n=0;for(int i=0;i<count;i++)if(date_pack(dates[i])==date_pack(d))n++;return n;}
static void calendar_preview(bool cycle){
 int first=-1,next=-1;for(int i=0;i<count;i++)if(date_pack(dates[i])==date_pack(selected_date)){if(first<0)first=i;if(i>chosen&&next<0)next=i;}
 chosen=cycle&&next>=0?next:first;message[0]=0;
 if(chosen<0||!load_world(ids[chosen])){rect(top,0,0,W,H,INK);text(top,32,90,chosen<0?"NO GAMES ON THIS DATE":"PHOTO COULD NOT LOAD",WHITE);if(chosen>=0)strcpy(message,"FILE DAMAGED");}
}
static void open_calendar(void){scan_worlds();selected_date=date_unpack(today());if(count){selected_date=dates[0];for(int i=1;i<count;i++)if(date_pack(dates[i])>date_pack(selected_date))selected_date=dates[i];}chosen=-1;calendar_preview(false);state=LIBRARY;}
static void draw_calendar(void){
 title("SAVED GAMES");char line[40];snprintf(line,sizeof(line),"%04d-%02d",selected_date.year,selected_date.month);text(frame,180,14,line,INK);const char*week[]={"SU","MO","TU","WE","TH","FR","SA"};for(int i=0;i<7;i++)text(frame,13+i*35,33,week[i],INK);
 int start=date_weekday((Date){selected_date.year,selected_date.month,1});
 for(int d=1;d<=date_days(selected_date.year,selected_date.month);d++){int k=start+d-1,x=10+k%7*35,y=45+k/7*17;bool on=d==selected_date.day;rect(frame,x,y,32,15,on?INK:RGB15(27,28,27));snprintf(line,sizeof(line),"%d",d);text(frame,x+3,y+3,line,on?WHITE:INK);Date day={selected_date.year,selected_date.month,d};if(day_count(day))rect(frame,x+26,y+10,3,3,on?WHITE:RGB15(24,7,13));}
 if(chosen>=0){snprintf(line,sizeof(line),ids[chosen]==0?"BROOKLYN - SPECIAL DELIVERY":"PHOTO WORLD %03d",ids[chosen]);text(frame,10,150,line,INK);}else text(frame,10,150,"NO GAMES ON THIS DATE",INK);
 text(frame,10,163,"A PLAY  X NEXT GAME  B HOME",INK);text(frame,10,178,"PAD DATE  L R MONTH  TAP OPEN",INK);
}
static bool save_world(void){
 if(!storage){strcpy(message,"SD UNAVAILABLE - NOT SAVED");return false;}
 int id;for(id=1;id<=999;id++){char p[96];struct stat st;path_for(p,id,"pwg");if(stat(p,&st)!=0)break;}
 if(id>999){strcpy(message,"LIBRARY FULL - NOT SAVED");return false;}
 char tmp[96],dest[96];path_for(tmp,id,"tmp");path_for(dest,id,"pwg");
 FILE*f=fopen(tmp,"wb");if(!f){strcpy(message,"CANNOT WRITE TO SD");return false;}
 unsigned head[3]={0x50574731,PIXELS,checksum(photo)};
 bool ok=fwrite(head,sizeof(head),1,f)==1&&fwrite(photo,sizeof(photo),1,f)==1;
 unsigned date=today();if(ok)ok=fwrite(&date,4,1,f)==1;
 if(fclose(f)!=0)ok=false;
 if(ok)ok=rename(tmp,dest)==0;
 if(!ok){remove(tmp);strcpy(message,"SAVE FAILED - TRY AGAIN");return false;}
 active_brooklyn=false;active_generated=false;save_id=id;strcpy(message,"SAVED ON SD");return true;
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
 if(transfer && !ndmaBusy(CHANNEL)){cameraStopTransfer();transfer=false;has_frame=true;frame_wait=0;}
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
 const int loc[3]={110,330,535};for(int i=0;i<3;i++)if(!(game.collected&(1<<i))){
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
 case HOME:title("PHOTO WORLDS - TEST BUILD");button(55,"CAPTURE GAME",selection==0);button(100,"VIEW SAVED GAMES",selection==1);text(frame,14,154,"UP DOWN SELECT - A OPEN",INK);text(frame,14,170,storage?"SD READY":"SD UNAVAILABLE",INK);break;
 case CAMERA:title("CAMERA");text(frame,14,55,"A OR L OR R - TAKE PHOTO",INK);text(frame,14,80,"Y - SWITCH CAMERA",INK);text(frame,14,105,"B - HOME",INK);text(frame,14,153,"PREVIEW CAPTURE 256 X 192",INK);break;
 case REVIEW:title("PHOTO READY");
#ifdef PHOTO_NETWORK
 text(frame,12,45,"MAKE A GAME FROM THIS PHOTO",INK);text(frame,12,61,"USES YOUR PAIRED PHOTO SERVICE",INK);text(frame,12,94,"A - SAVE AND GENERATE",INK);
#else
 text(frame,12,45,"OFFLINE TEST: PHOTO COLOURS",INK);text(frame,12,61,"SHAPE THE LEVEL PALETTE",INK);text(frame,12,94,"A - SAVE AND PLAY",INK);
#endif
 text(frame,12,113,"B - RETAKE",INK);text(frame,12,150,message,INK);break;
 case LIBRARY:draw_calendar();break;
 case PLAY:
 if(active_generated){world_render(frame,&generated,&generated_player);rect(frame,0,0,256,27,WHITE);char line[42];snprintf(line,sizeof(line),"%.40s",generated.objective);text(frame,5,4,line,INK);snprintf(line,sizeof(line),"%.40s",strlen(generated.objective)>40?generated.objective+40:"");text(frame,5,15,line,INK);rect(frame,0,177,256,15,WHITE);text(frame,4,181,"PAD MOVE A JUMP HOLD A HIGH B SLOW",INK);break;}
 if(active_brooklyn){brooklyn_render(frame,&brooklyn);rect(frame,0,0,256,28,WHITE);char line[40];snprintf(line,sizeof(line),"RETURN THE BAG    FLAGS %d OF 3",brooklyn.checkpoint);text(frame,7,4,line,INK);text(frame,7,16,brooklyn.weather_left<3?"GUST CHANGING SOON":brooklyn.wind==1?"TAILWIND":brooklyn.wind==2?"CROSSWIND - STEER LEFT":"CALM",INK);rect(frame,0,166,256,26,WHITE);text(frame,3,170,brooklyn.z<11?"UP FORWARD  LEFT RIGHT STEER":brooklyn.z<18?"HOLD UP - TAP A BEFORE GAP":brooklyn.z<30?"HOLD B AND UP TO DUCK UNDER":brooklyn.z<46?"HOLD UP AND A TO GLIDE":brooklyn.z<54?"LAST GAP - HOLD UP AND A":"RETURN BAG TO THE PERSON",INK);text(frame,3,182,"A JUMP-HOLD GLIDE B DUCK START PAUSE",INK);}else draw_game();break;
 case PAUSE:title("PAUSED");text(frame,12,65,"START - RESUME",INK);text(frame,12,90,"A - RESTART",INK);text(frame,12,115,"B - HOME",INK);break;
 case WIN:if(active_generated){for(unsigned i=0;i<render_ticks;i++)world_step(&generated_player,&generated,0,0,false,false);world_render(frame,&generated,&generated_player);if(generated_player.reward>4){rect(frame,0,175,256,17,WHITE);text(frame,35,181,"A REPLAY  B HOME",INK);}break;}if(active_brooklyn){brooklyn.reward+=render_ticks/60.f;brooklyn_render(frame,&brooklyn);rect(frame,0,0,256,31,WHITE);text(frame,44,9,brooklyn.reward>4?"IT WAS FOR YOU":"DELIVERED",INK);rect(frame,0,175,256,17,WHITE);text(frame,35,181,"A REPLAY  B HOME",INK);break;}title("YOU BROUGHT THEM HOME");for(int i=0;i<3;i++){int x=83+i*40;rect(frame,x,90,2,35,grass);rect(frame,x-6,80,14,14,RGB15(30,12+i*4,20));rect(frame,x-2,84,6,6,RGB15(31,27,4));}text(frame,40,143,"A LITTLE GARDEN FOR YOU",INK);text(frame,20,171,"A REPLAY  B HOME",INK);break;
 case LOSE:title("TIME IS UP");text(frame,20,80,"YOUR WORLD IS STILL SAVED",INK);text(frame,20,115,"A RETRY  B HOME",INK);break;
 case ERROR_SCREEN:title("COULD NOT CONTINUE");text(frame,8,65,message,INK);text(frame,12,125,"B - HOME",INK);break;
 case GENERATE:case GENERATION_ERROR:
#ifdef PHOTO_NETWORK
 title(state==GENERATE?"MAKING YOUR GAME":"PHOTO IS SAFE");
 {char line[41];const char*s=photo_network_message();for(int i=0;i<4;i++){size_t offset=(size_t)i*40;if(offset>=strlen(s))break;snprintf(line,sizeof(line),"%.40s",s+offset);text(frame,6,55+i*12,line,INK);}}
 text(frame,8,139,state==GENERATE?"B - CANCEL AND KEEP PHOTO":"A - RETRY  B - HOME",INK);
#endif
 break;
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
 irqSet(IRQ_VBLANK,sample_input);irqEnable(IRQ_VBLANK);
 while(1){swiWaitForVBlank();InputSample samples[INPUT_CAPACITY];int irq=enterCriticalSection();unsigned n=input_drain(&inputs,samples);leaveCriticalSection(irq);
 int p=0;for(unsigned i=0;i<n;i++)p|=samples[i].pressed;render_ticks=n;
 switch(state){
 case HOME:
 if(p&(KEY_UP|KEY_DOWN))selection^=1;
 if(p&KEY_TOUCH){touchPosition touch;touchRead(&touch);if(touch.px>=12&&touch.px<244){if(touch.py>=55&&touch.py<85){selection=0;p|=KEY_A;}if(touch.py>=100&&touch.py<130){selection=1;p|=KEY_A;}}}
 if(p&KEY_A){message[0]=0;if(!selection)start_camera();else open_calendar();}break;
 case CAMERA:camera_tick(p);break;
 case REVIEW:if(p&KEY_B)start_camera();else if(p&KEY_A){if(save_world()){
#ifdef PHOTO_NETWORK
 begin_generation();
#else
 reset_game();state=PLAY;
#endif
 }}break;
 case GENERATE:
#ifdef PHOTO_NETWORK
 photo_network_tick(p&KEY_B);
 if(photo_network_stage()==NET_READY){if(save_generated()){reset_game();state=PLAY;}else{strcpy(message,"GENERATED GAME INVALID OR SD FULL");state=ERROR_SCREEN;}}
 else if(photo_network_stage()==NET_FAILED)state=GENERATION_ERROR;
 else if(photo_network_stage()==NET_CANCELLED)state=HOME;
#endif
 break;
 case GENERATION_ERROR:
#ifdef PHOTO_NETWORK
 if(p&KEY_A)begin_generation();else if(p&KEY_B)state=HOME;
#endif
 break;
 case LIBRARY:{
 bool moved=false;if(p&KEY_B){state=HOME;break;}
 int delta=(!!(p&KEY_RIGHT)-!!(p&KEY_LEFT))+7*(!!(p&KEY_DOWN)-!!(p&KEY_UP));if(delta){selected_date=date_move(selected_date,delta);moved=true;}
 int month=!!(p&KEY_R)-!!(p&KEY_L);if(month){selected_date=date_month(selected_date,month);moved=true;}
 if(p&KEY_TOUCH){touchPosition t;touchRead(&t);if(t.px>=10&&t.px<255&&t.py>=45&&t.py<147){int d=(t.py-45)/17*7+(t.px-10)/35-date_weekday((Date){selected_date.year,selected_date.month,1})+1;if(d>0&&d<=date_days(selected_date.year,selected_date.month)){if(d==selected_date.day)p|=KEY_A;else{selected_date.day=d;moved=true;}}}}
 if(moved)calendar_preview(false);else if(p&KEY_X)calendar_preview(true);
 if((p&KEY_A)&&chosen>=0&&load_world(ids[chosen])){
#ifdef PHOTO_NETWORK
 if(!active_brooklyn&&!active_generated)begin_generation();else
#endif
 {reset_game();state=PLAY;}}break;}
 case PLAY:
 // Replay every display-tick input in order, even if drawing skipped frames.
 for(unsigned i=0;i<n&&state==PLAY;i++){
 int edge=samples[i].pressed,held=samples[i].held;
 if(edge&KEY_START){state=PAUSE;clear_inputs();break;}
 if(active_generated){world_step(&generated_player,&generated,!!(held&KEY_RIGHT)-!!(held&KEY_LEFT),!!(held&KEY_UP)-!!(held&KEY_DOWN),held&KEY_A,held&KEY_B);if(generated_player.won)state=WIN;}
 else if(active_brooklyn){brooklyn_step(&brooklyn,!!(held&KEY_RIGHT)-!!(held&KEY_LEFT),!!(held&KEY_UP)-!!(held&KEY_DOWN),edge&KEY_A,held&KEY_B,held&KEY_A);if(brooklyn.won)state=WIN;if(brooklyn.lost)state=LOSE;}
 else{game_step(&game,!!(held&KEY_RIGHT)-!!(held&KEY_LEFT),edge&KEY_A,held&KEY_B);if(game.won)state=WIN;if(game.lost)state=LOSE;}
 }break;
 case PAUSE:if(p&KEY_START){clear_inputs();state=PLAY;}else if(p&KEY_A){reset_game();state=PLAY;}else if(p&KEY_B)state=HOME;break;
 case WIN:case LOSE:if(p&KEY_A){reset_game();state=PLAY;}else if(p&KEY_B)state=HOME;break;
 case ERROR_SCREEN:if(p&KEY_B)state=HOME;break;
 }
 render();
 }
}
