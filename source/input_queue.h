#pragma once
#include <stdint.h>
#define INPUT_CAPACITY 32
typedef struct {uint32_t pressed,held;} InputSample;
typedef struct {InputSample samples[INPUT_CAPACITY];unsigned start,count;} InputQueue;
// Caller holds a critical section when consuming a queue written by VBlank.
void input_push(InputQueue *q,InputSample input);
unsigned input_drain(InputQueue *q,InputSample out[INPUT_CAPACITY]);
