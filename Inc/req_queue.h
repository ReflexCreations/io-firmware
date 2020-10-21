#ifndef __REQ_QUEUE_H
#define __REQ_QUEUE_H

#include "request.h"

#define MAX_REQ_QUEUE_LENGTH (16U)

typedef struct {
    Request items[MAX_REQ_QUEUE_LENGTH];
    int8_t front;
    int8_t rear;
    uint8_t count;
} RequestQueue;

void req_queue_add(RequestQueue *, Request);
Request req_queue_take(RequestQueue *);
void req_queue_init(RequestQueue *);

#endif