#pragma once
#include <stdbool.h>

typedef enum {PHOTO_WIFI_OFF,PHOTO_WIFI_CONNECTING,PHOTO_WIFI_CONNECTED,PHOTO_WIFI_FAILED} PhotoWifiState;
// Read/poll operations never initialize Wi-Fi or reconnect implicitly.
bool photo_wifi_connect(void);
void photo_wifi_tick(void);
void photo_wifi_disconnect(void);
PhotoWifiState photo_wifi_state(void);
const char *photo_wifi_message(void);
