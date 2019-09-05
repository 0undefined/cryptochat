#ifndef COMMANDS_H
#define COMMANDS_H
#include <pthread.h>

typedef unsigned char queuesize_t;

#define QUEUE_ERR_FULL      1
#define QUEUE_ERR_EMPTY     2
#define QUEUE_ERR_MUTEX     3
#define QUEUE_OK            0

#define QUEUE_SIZE          (queuesize_t)-1

enum command_enum {
    COMMAND_ECHO,
    COMMAND_HELP,
    COMMAND_QUIT,
    COMMAND_CONNECT,
    COMMAND_MSG,
    COMMAND_WHISPER,
};

struct command {
    enum command_enum id;
    //struct command *arg;
    char shorthand;
    char *longhand;
    char *help;
    int (*fun)(char *argv);
};

// Queue, FIFO aka. push onto head, pop from tail
struct command_queue {
    queuesize_t size; // = 255;
    queuesize_t head; // = 0;
    queuesize_t tail; // = 0;

    struct command queue[QUEUE_SIZE];

    pthread_mutex_t *rw_lock;
};


struct command_queue *queue_init(struct command_queue *cq) {
    cq = (struct command_queue*)malloc(sizeof(struct command_queue));

    cq->size = QUEUE_SIZE;
    cq->head = 0;
    cq->tail = 0;

    pthread_mutex_init(cq->rw_lock, NULL);

    return cq;
}

queuesize_t queue_get_len(struct command_queue *cq) { // !t-unsafe
    queuesize_t len = 0;
    if(cq->head >= cq->tail) {
        len = cq->head - cq->tail;
    } else {
        len = (cq->size - cq->tail) + cq->head;
    }
    return len;
}

struct command *peek(struct command_queue *cq) {
    if(!pthread_mutex_lock(cq->rw_lock)) return NULL;

    // ret nil on empty queue
    if(!queue_get_len(cq)) {
        pthread_mutex_unlock(cq->rw_lock);
        return NULL;
    }
    struct command *ret = &(cq->queue[cq->tail]);

    pthread_mutex_unlock(cq->rw_lock);
    return ret;
}

struct command *pop(struct command_queue *cq) {  // ret first element if non-empty, nil otherwise
    if(!pthread_mutex_lock(cq->rw_lock)) return NULL;

    if(queue_get_len(cq) == 0) return NULL; // queue is empty

    struct command *ret = &(cq->queue[cq->tail++]);

    pthread_mutex_unlock(cq->rw_lock);
    return ret;
}

int push(struct command_queue *cq, struct command c) {  // add element
    if(!pthread_mutex_lock(cq->rw_lock)) return QUEUE_ERR_MUTEX;

    if(queue_get_len(cq) == cq->size) return QUEUE_ERR_FULL; // queue is full
    else if(cq->head == cq->size - 1) cq->head = 0;
    else                              cq->head++;

    cq->queue[cq->head] = c;

    pthread_mutex_unlock(cq->rw_lock);
    return QUEUE_OK;
}

#endif
