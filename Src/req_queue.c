#include "uart.h"
#include "stdbool.h"
#include "req_queue.h"
#include "error_handler.h"

Request BlankRequest = {
    Comport_None,
    0x00,
    NULL,
    0x0000,
    NULL,
    0x0000
};

static inline uint8_t contains(RequestQueue * queue, Request req) {
    if (queue->count == 0) return false;

    for (uint8_t i = 0; i < MAX_REQ_QUEUE_LENGTH; i++) {
        if (request_equals(queue->items[i], req)) return true;
    }

    return false;
}

void req_queue_init(RequestQueue * queue) {
    queue->front = 0;
    queue->rear = -1;
    queue->count = 0;

    for (uint8_t i = 0; i < MAX_REQ_QUEUE_LENGTH; i++) {
        queue->items[i] = BlankRequest;
    }
}

void req_queue_add(RequestQueue * queue, Request request) {
    if (queue->count == MAX_REQ_QUEUE_LENGTH) {
        error_panic(Error_App_ReqQueue_QueueFull);
        return;
    }

    // Don't add if the request is already in the queue
    if (contains(queue, request)) return;

    if (queue->rear == MAX_REQ_QUEUE_LENGTH - 1) queue->rear = -1;

    queue->rear++;
    queue->items[queue->rear] = request;
    queue->count++;
}

Request req_queue_take(RequestQueue * queue) {
    if (queue->count == 0) return BlankRequest;

    Request req = queue->items[queue->front];
    queue->items[queue->front] = BlankRequest;
    queue->front++;

    if (queue->front == MAX_REQ_QUEUE_LENGTH) queue->front = 0;
    
    queue->count--;
    return req;
}