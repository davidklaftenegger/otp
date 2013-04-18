#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "sys.h"
#include "erl_vm.h"
#include "global.h"
#include "erl_process.h"
#include "error.h"
#include "erl_newlock.h"

#define SUCCESSOR(INDEX) ((INDEX+1)%MAX_QUEUE_LENGTH)

/* TODO too many memory barriers */

void queue_init(queue_handle* q) {
    int i;
    erts_atomic32_init_nob(&q->head, -1);
    q->entries = malloc(MAX_QUEUE_LENGTH * sizeof(QueueEntry));
     // initializing the buffer is now required
    for(i = 0; i < MAX_QUEUE_LENGTH; i++) {
	q->entries[i] = NULL;
    }
    ETHR_MEMORY_BARRIER;
}

/*
void queue_init_padded(padded_queue_handle* q){
    q->qh.entries = malloc(MAX_QUEUE_LENGTH * sizeof(padded_queue_handle));
}
*/

/* return 1 on queue full */
int queue_push(queue_handle* q, void* entry) {
    erts_aint32_t myhead = erts_atomic32_inc_read_nob(&q->head);
    if(myhead >= MAX_QUEUE_LENGTH) {
	while(1) {
	erts_aint32_t curval = erts_atomic32_read_nob(&q->head);
	    if(curval == myhead) {
		/* try cmp-exchange, if success enqueuing failed, else still need to fix state */
		if(myhead != erts_atomic32_cmpxchg_mb(&q->head, myhead-1, myhead)) {
		    return 1;
		}
	    } else if(curval < 0) {
		/* queue was closed in the meantime */
		return 1;
	    }
	}
    }
    else if(myhead < 0)  {
	return 1;
    }
    q->entries[ myhead ] = entry;
    /* ETHR_MEMORY_BARRIER; // is this required? */
    return 0;
}

void* queue_pop(queue_handle* q, unsigned int idx) {
    QueueEntry entry;
    entry = q->entries[ idx ];
    while(!entry) { /* spin */
	entry = q->entries[ idx ];
	ETHR_MEMORY_BARRIER;
    }
    q->entries[ idx ] = NULL;

    return entry;
}

void acquire_newlock(erts_atomic_t* L, newlock_node* I) {
    newlock_node* pred;
    erts_atomic_set_nob(&I->next, (erts_aint_t) NULL);
    pred = (newlock_node*)erts_atomic_xchg_mb(L, (erts_aint_t) I);
    if(pred != NULL) {
	erts_atomic32_set_mb(&I->locked, 1);
	erts_atomic_set_mb(&pred->next, (erts_aint_t) I);
	while(erts_atomic32_read_mb(&I->locked)); /* spin */
    }
}

enum lock_unlocking acquire_read_newlock(erts_atomic_t* L, newlock_node* I, newlock_node** T) {
    newlock_node* pred;
    while(1) {
	erts_aint32_t readers;
	enum locknode_type type;
	pred = (newlock_node*)erts_atomic_read_mb(L);
	if(pred) {
	    type = erts_atomic32_read_mb(&pred->type);
	}
	if((!pred) || (type == DX)) {
	    /* do normal exclusive lock, but check for races */
	    erts_atomic_set_nob(&I->next, (erts_aint_t) NULL);
	    erts_atomic32_set_nob(&I->type, R_INIT);
	    if(pred == (newlock_node*)erts_atomic_cmpxchg_mb(L, (erts_aint_t) I, (erts_aint_t) pred)) { /* TODO: just always take? */
		if(pred != NULL) {
		    erts_atomic32_set_nob(&I->locked, 1);
		    erts_atomic_set_mb(&pred->next, (erts_aint_t) I);
		    /* check that no handover is useful here TODO */
		    erts_atomic32_set_mb(&I->type, R_WAIT);
		    while(erts_atomic32_read_mb(&I->locked)); /* spin */
		} else {
		    erts_atomic32_set_mb(&I->type, R_WAIT);
		}

		return NEED_TO_UNLOCK;
	    } else {
		erts_atomic32_set_mb(&I->type, DX);
	    }
	    /* this was too late, lock is already changing */
	    //while(pred == (newlock_node*) erts_atomic_read_nob(L)); /* spin until lockholder changed, TODO: measure effect*/
	} else {
	    /* already a promoted read-lock: piggyback */
	    if(type == R_INIT) while(type = erts_atomic_read_mb(&pred->type) == R_INIT); /* wait for locknode to be ready */
	    if(type == DX) continue;
	    if(type == R_HANDOFF) {
		/* TODO */
	    } else {
		/* either in R_WAIT or already in R_PROCEED */
		/* speculatively increase number of readers */
		erts_atomic32_inc_mb(&pred->readers);
		/* now need to check it is still okay to use: still proceeding? */
		*T = pred;
		if(type == R_WAIT)
		    while(type = erts_atomic32_read_mb(&pred->type) == R_WAIT); /* spin on first reader's permission */
		if(type == R_PROCEED) {
		    return NO_NEED_TO_UNLOCK;
		} else {
		    erts_atomic32_dec_mb(&pred->readers);
		}
	    }
	}
    }
}

int try_newlock(erts_atomic_t* L, newlock_node* I) {
    newlock_node* pred;
    erts_atomic_set_nob(&I->next, (erts_aint_t) NULL);
    pred = (newlock_node*)erts_atomic_cmpxchg_mb(L, (erts_aint_t) I, (erts_aint_t) NULL);

    /* no setting of next-pointers, as this only succeeds if there is no previous waiter */
    
    return pred == NULL;
}

int is_free_newlock(erts_atomic_t* L) {
    return erts_atomic_read_nob(L) == (erts_aint_t) NULL;
}

void release_newlock(erts_atomic_t* L, newlock_node* I) {
    newlock_node* next = (newlock_node*)erts_atomic_read_mb(&I->next);
    if(next == NULL) {
	if(erts_atomic_cmpxchg_mb(L, (erts_aint_t) NULL, (erts_aint_t) I) == (erts_aint_t) I) {
	    return;
	}
	do {
	    next = (newlock_node*) erts_atomic_read_mb(&I->next);
	} while(next == NULL); /* spin */
    }
    erts_atomic32_set_mb(&next->locked, 0);
}

void read_read_newlock(newlock_node* L) {
    erts_atomic32_dec_nob(&L->readers);
}

void release_read_newlock(erts_atomic_t* L, newlock_node* I) {
    erts_atomic32_set_mb(&I->type, R_PROCEED); /* unlock all readers */
    while(erts_atomic32_read_mb(&I->readers)); /* spin on readers setting pointers */
    erts_atomic32_set_mb(&I->type, DX);
    release_newlock(L, I); /* unlock next operation (likely has to wait for reader pointers to be changed) */
}

