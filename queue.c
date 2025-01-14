/*
 * Queue handling
 * Copyright (C) 2006 Unix Solutions Ltd.
 *
 * Released under MIT license.
 * See LICENSE-MIT.txt for license terms.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <pthread.h>

#include "queue.h"

#ifndef FREE
	#define FREE(x) if(x) { free(x); x=NULL; }
#endif

QUEUE *queue_new(void) {
	QUEUE *q = calloc(1, sizeof(QUEUE));
	if (!q)
		return NULL;
	if (pthread_mutex_init(&q->mutex,NULL) != 0) {
		perror("queue_new: mutex_init");
		free(q);
		return NULL;
	}
	if (pthread_cond_init(&q->cond,NULL)!=0){
		perror("queue_new: cond_init");
		free(q);
		return NULL;
	}
	return q;
}

void queue_free(QUEUE **pq) {
	QUEUE *q = *pq;
	if (!q)
		return;
	while (q->items > 0) {
		queue_get(q);
	}
	pthread_mutex_destroy(&q->mutex);
	pthread_cond_destroy(&q->cond);
	FREE(*pq);
}

void queue_add(QUEUE *q, void *data) {
	if (!q)
		return;
	QNODE *a_msg = calloc(1, sizeof(QNODE));
	if (!a_msg) {
		perror("queue_enqueue, malloc:");
		return;
	}
	a_msg->data = data;
	a_msg->next = NULL;
	pthread_mutex_lock(&q->mutex);
	if (q->items == 0) { 			// special case - queue is empty 
		q->head = a_msg;
		q->tail = a_msg;
	} else {
		q->tail->next = a_msg;
		q->tail = a_msg;
	}
	q->items++;
	pthread_cond_signal(&q->cond);
	pthread_mutex_unlock(&q->mutex);
}

void *queue_get(QUEUE *q) {
	if (!q)
		return NULL;
	pthread_mutex_lock(&q->mutex);
	while (q->items==0) {
		pthread_cond_wait(&q->cond, &q->mutex);
		if (q->items == 0)
			return NULL;
	}
	if (!q || !q->head || q->items == 0) // Can happen in free
		return NULL;
	QNODE *a_msg = q->head;
	q->head = a_msg->next;
	if (q->head == NULL) { 		// this was last msg
	    q->tail = NULL;
	}
	q->items--;
	pthread_cond_signal(&q->cond);
	pthread_mutex_unlock(&q->mutex);
	void *data = a_msg->data;
	FREE(a_msg);
    return data;
}

void *queue_get_nowait(QUEUE* q) {
	if (!q || q->items == 0)
		return NULL;
	return queue_get(q);
}

void queue_wakeup(QUEUE *q) {
	if (q) {
		pthread_cond_signal(&q->cond);
	}
}

