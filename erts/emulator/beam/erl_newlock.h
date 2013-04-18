#ifndef ERL_NEWLOCK_H
#define ERL_NEWLOCK_H

#include<stdint.h>

/* values preliminary */
#define MAX_QUEUE_LENGTH 16384
#define MAX_PASSES 256

struct queue_entry {
    uint32_t ticket;
    void* value;
};

typedef void* QueueEntry;
/* might also be: */
/* typedef struct queue_entry QueueEntry; */

typedef struct {
/*    uint16_t size; // needs to be atomic */
/*    uint16_t capacity; // potentially needs to be atomic */
/*    erts_atomic32_t capacity; */
    erts_atomic32_t head;
    QueueEntry* entries;
} queue_handle;

typedef union {
    queue_handle qh;
    byte _cache_line_padding[64];
} padded_queue_handle;

/* TODO: this function should be deprecated for it is misleading */
static ERTS_INLINE int queue_is_empty(queue_handle* q) {
    return (erts_atomic32_read_mb(&q->head) == -1);
}


static ERTS_INLINE int queue_is_full(queue_handle* q) {
    return (erts_atomic32_read_mb(&q->head) == MAX_QUEUE_LENGTH-1);
}


static ERTS_INLINE uint32_t queue_size(queue_handle* q) {
    return erts_atomic32_read_mb(&q->head) + 1;
}

static ERTS_INLINE void queue_open(queue_handle* q) {
	erts_atomic32_set_nob(&q->head, -1);
}



void queue_init(queue_handle* q);
int queue_push(queue_handle* q, void* entry);
void* queue_pop(queue_handle* q, unsigned int idx);

enum lock_unlocking {
    NO_NEED_TO_UNLOCK,
    NEED_TO_UNLOCK
};

enum locknode_type {
    DX,
    R_INIT,
    R_WAIT,
    R_PROCEED,
    R_HANDOFF
};

#define EXCLUSIVE_LOCK 0x00000000
#define READ_LOCK 0x40000000
#define LOCK_MASK (~(EXCLUSIVE_LOCK | READ_LOCK))
typedef struct newlock_locknode {
    erts_atomic32_t locked;
    erts_atomic32_t readers;
    erts_atomic32_t type;
    erts_atomic_t next;
    queue_handle queue;
} newlock_node;

void acquire_newlock(erts_atomic_t* L, newlock_node* I);
enum lock_unlocking acquire_read_newlock(erts_atomic_t* L, newlock_node* I, newlock_node** T);
int try_newlock(erts_atomic_t* L, newlock_node* I);
int is_free_newlock(erts_atomic_t* L);
void read_read_newlock(newlock_node* L);
void release_read_newlock(erts_atomic_t* L, newlock_node* I);
void release_newlock(erts_atomic_t* L, newlock_node* I);

#endif // ERL_NEWLOCK_H
