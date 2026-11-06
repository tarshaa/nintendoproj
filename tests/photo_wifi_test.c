#include "../source/photo_wifi.h"
#include <dswifi9.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

static bool dsi=true,init_ok=true,initialized;
static int assoc=ASSOCSTATUS_SEARCHING,init_calls,connect_calls,status_calls,disconnect_calls,disable_calls;
static time_t seconds=100;
bool isDSiMode(void){return dsi;}
int Wifi_CheckInit(void){return initialized;}
bool Wifi_InitDefault(unsigned flags){assert(flags==(INIT_ONLY|WIFI_ATTEMPT_DSI_MODE));init_calls++;initialized=init_ok;return init_ok;}
void Wifi_AutoConnect(void){connect_calls++;}
int Wifi_AssocStatus(void){status_calls++;return assoc;}
int Wifi_DisconnectAP(void){disconnect_calls++;return 0;}
void Wifi_DisableWifi(void){disable_calls++;}
time_t wifi_test_time(time_t *out){if(out)*out=seconds;return seconds;}
static void check_state(PhotoWifiState want){assert(photo_wifi_state()==want);assert(strlen(photo_wifi_message())<=38);}
static void poll_without_connecting(void){int before=connect_calls;for(int i=0;i<120;i++)photo_wifi_tick();assert(connect_calls==before);}
int main(void){
 // Drawing/browsing and repeated idle ticks cannot initialize any networking.
 check_state(PHOTO_WIFI_OFF);poll_without_connecting();
 assert(init_calls==0&&status_calls==0&&connect_calls==0);
 dsi=false;assert(!photo_wifi_connect());check_state(PHOTO_WIFI_FAILED);assert(init_calls==0);
 dsi=true;init_ok=false;assert(!photo_wifi_connect());check_state(PHOTO_WIFI_FAILED);assert(connect_calls==0);
 init_ok=true;assert(photo_wifi_connect());check_state(PHOTO_WIFI_CONNECTING);assert(connect_calls==1);
 photo_wifi_tick();check_state(PHOTO_WIFI_CONNECTING);
 assoc=ASSOCSTATUS_ASSOCIATING;photo_wifi_tick();check_state(PHOTO_WIFI_CONNECTING);
 assoc=ASSOCSTATUS_ACQUIRINGDHCP;photo_wifi_tick();check_state(PHOTO_WIFI_CONNECTING);
 assoc=ASSOCSTATUS_ASSOCIATED;photo_wifi_tick();check_state(PHOTO_WIFI_CONNECTED);
 int before_init=init_calls;
 assert(photo_wifi_connect());assert(connect_calls==1&&init_calls==before_init); // generation reuses Wi-Fi
 photo_wifi_disconnect();check_state(PHOTO_WIFI_OFF);assert(disable_calls==1&&disconnect_calls==1);
 poll_without_connecting();assert(connect_calls==1);
 assoc=ASSOCSTATUS_SEARCHING;assert(photo_wifi_connect());assert(init_calls==before_init); // retry after radio disabled
 photo_wifi_disconnect();check_state(PHOTO_WIFI_OFF);poll_without_connecting(); // Back/Cancel
 assert(photo_wifi_connect());seconds+=36;photo_wifi_tick();check_state(PHOTO_WIFI_FAILED);poll_without_connecting();
 assert(photo_wifi_connect());assoc=ASSOCSTATUS_ACQUIRINGDHCP;seconds+=36;photo_wifi_tick();check_state(PHOTO_WIFI_FAILED);
 assert(photo_wifi_connect());assoc=ASSOCSTATUS_ASSOCIATING;seconds+=36;photo_wifi_tick();check_state(PHOTO_WIFI_FAILED);
 assert(photo_wifi_connect());assoc=ASSOCSTATUS_CANNOTCONNECT;photo_wifi_tick();check_state(PHOTO_WIFI_FAILED);
 assoc=ASSOCSTATUS_ASSOCIATED;assert(photo_wifi_connect());photo_wifi_tick();check_state(PHOTO_WIFI_CONNECTED);
 assoc=ASSOCSTATUS_SEARCHING;photo_wifi_tick();check_state(PHOTO_WIFI_FAILED);poll_without_connecting(); // no silent reconnect
 photo_wifi_disconnect();check_state(PHOTO_WIFI_OFF);
 puts("WiFi states: idle isolation, connect, reuse, cancel, timeout, loss and explicit retry passed");
 return 0;
}
