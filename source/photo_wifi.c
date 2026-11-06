#ifdef PHOTO_NETWORK
#include "photo_wifi.h"
#include <dswifi9.h>
#include <nds.h>
#include <time.h>

static PhotoWifiState state;
static bool radio_active;
static time_t started;
static const char *message="Uses your saved DSi network";

void photo_wifi_disconnect(void){
 if(radio_active){Wifi_DisconnectAP();Wifi_DisableWifi();radio_active=false;}
 state=PHOTO_WIFI_OFF;message="Uses your saved DSi network";
}
static void fail(const char *reason){photo_wifi_disconnect();state=PHOTO_WIFI_FAILED;message=reason;}
bool photo_wifi_connect(void){
 // Reuse an explicitly established connection when generation begins.
 photo_wifi_tick();
 if(state==PHOTO_WIFI_CONNECTED||state==PHOTO_WIFI_CONNECTING)return true;
 if(!isDSiMode()){fail("Open this app in DSi mode");return false;}
 if(!Wifi_CheckInit()&&!Wifi_InitDefault(INIT_ONLY|WIFI_ATTEMPT_DSI_MODE)){
  fail("WiFi could not start - try again");return false;
 }
 radio_active=true;Wifi_AutoConnect();started=time(NULL);
 state=PHOTO_WIFI_CONNECTING;message="Finding your saved network";return true;
}
void photo_wifi_tick(void){
 if(state!=PHOTO_WIFI_CONNECTING&&state!=PHOTO_WIFI_CONNECTED)return;
 int status=Wifi_AssocStatus();
 if(state==PHOTO_WIFI_CONNECTED){
  if(status!=ASSOCSTATUS_ASSOCIATED)fail("Connection lost - turn on hotspot");
  return;
 }
 if(status==ASSOCSTATUS_ASSOCIATED){state=PHOTO_WIFI_CONNECTED;message="Connected to your saved network";return;}
 if(status==ASSOCSTATUS_CANNOTCONNECT){fail("Could not join - check hotspot");return;}
 if(time(NULL)-started>35){
  fail(status==ASSOCSTATUS_SEARCHING?"Not found - turn on your hotspot":status==ASSOCSTATUS_ACQUIRINGDHCP?"No address - restart your hotspot":"Could not join - check hotspot");return;
 }
 message=status==ASSOCSTATUS_SEARCHING?"Finding your saved network":status==ASSOCSTATUS_ACQUIRINGDHCP?"Getting a network address":"Joining your saved network";
}
PhotoWifiState photo_wifi_state(void){return state;}
const char *photo_wifi_message(void){return message;}
#endif
