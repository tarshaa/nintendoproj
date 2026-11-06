#pragma once
#include <stdbool.h>
#define INIT_ONLY 0
#define WIFI_ATTEMPT_DSI_MODE 8
enum {ASSOCSTATUS_SEARCHING,ASSOCSTATUS_ASSOCIATING,ASSOCSTATUS_ACQUIRINGDHCP,ASSOCSTATUS_ASSOCIATED,ASSOCSTATUS_CANNOTCONNECT};
int Wifi_CheckInit(void);
bool Wifi_InitDefault(unsigned flags);
void Wifi_AutoConnect(void);
int Wifi_AssocStatus(void);
int Wifi_DisconnectAP(void);
void Wifi_DisableWifi(void);
