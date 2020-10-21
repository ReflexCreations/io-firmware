#include "uart.h"
#include "bool.h"
#include "req_queue.h"

Request BlankRequest = {
    COMPORT_NONE,
    0x00,
    NULL,
    0x0000,
    NULL,
    0x0000
};

static inline uint8_t contains(RequestQueue * queue, Request req) {
    if (queue->count == 0) return false;

    for (uint8_t i = 0; i < MAX_REQ_QUEUE_LENGTH; i++) {
        if (queue->items[i].comport_id == req.comport_id 
            && queue->items[i].send_data == req.send_data) return true;
    }

    return false;
}

void req_queue_init(RequestQueue * queue) {
    queue->front = 0;
    queue->rear = -1;
    queue->count = 0;
}

void req_queue_add(RequestQueue * queue, Request request) {
    if (queue->count == MAX_REQ_QUEUE_LENGTH) return;

    if (contains(queue, request)) return;

    if (queue->rear == MAX_REQ_QUEUE_LENGTH - 1) {
        queue->rear = -1;
    }

    queue->rear++;
    queue->items[queue->rear] = request;
    queue->count++;
}

Request req_queue_take(RequestQueue * queue) {
    if (queue->count == 0) return BlankRequest;

    Request req = queue->items[queue->front];
    queue->items[queue->front] = BlankRequest;
    queue->front++;

    if (queue->front == MAX_REQ_QUEUE_LENGTH) {
        queue->front = 0;
    }

    queue->count--;
    return req;
}