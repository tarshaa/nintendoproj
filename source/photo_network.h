#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef enum {NET_IDLE,NET_CONNECT,NET_PROBE,NET_UPLOAD,NET_WAIT,NET_FETCH,NET_READY,NET_FAILED,NET_CANCELLED} NetStage;
bool photo_network_begin(const char *capture_path);
void photo_network_tick(bool cancel);
void photo_network_close(void);
NetStage photo_network_stage(void);
const char *photo_network_message(void);
const unsigned char *photo_network_result(size_t *size);
