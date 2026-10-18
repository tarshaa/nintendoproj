#include "input_queue.h"
void input_push(InputQueue*q,InputSample in){
 if(q->count==INPUT_CAPACITY){q->start=(q->start+1)%INPUT_CAPACITY;q->count--;}
 q->samples[(q->start+q->count)%INPUT_CAPACITY]=in;q->count++;
}
unsigned input_drain(InputQueue*q,InputSample out[INPUT_CAPACITY]){
 unsigned n=q->count;for(unsigned i=0;i<n;i++)out[i]=q->samples[(q->start+i)%INPUT_CAPACITY];q->start=q->count=0;return n;
}
