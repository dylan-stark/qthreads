#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <qthread/qthread.h>
#include <qthread/qloop.h>
#include <qthread/qpool.h>
#include "qtimer.h"

#define ELEMENT_COUNT 10000
#define THREAD_COUNT 128

qpool qp = NULL;
size_t **allthat;

void pool_allocator(qthread_t * me, const size_t startat,
			 const size_t stopat, void *arg)
{
    size_t i;
    qpool p = (qpool) arg;

    for (i = startat; i < stopat; i++) {
	if ((allthat[i] = qpool_alloc(me, p)) == NULL) {
	    fprintf(stderr, "qpool_alloc() failed!\n");
	    exit(-1);
	}
	allthat[i][0] = i;
    }
}

void pool_deallocator(qthread_t * me, const size_t startat,
			 const size_t stopat, void *arg)
{
    size_t i;
    qpool p = (qpool) arg;

    for (i = startat; i < stopat; i++) {
	qpool_free(p, allthat[i]);
    }
}

void malloc_allocator(qthread_t * me, const size_t startat,
			 const size_t stopat, void *arg)
{
    size_t i;
    qpool p = (qpool) arg;

    for (i = startat; i < stopat; i++) {
	if ((allthat[i] = malloc(44)) == NULL) {
	    fprintf(stderr, "qpool_alloc() failed!\n");
	    exit(-1);
	}
	allthat[i][0] = i;
    }
}

void malloc_deallocator(qthread_t * me, const size_t startat,
			 const size_t stopat, void *arg)
{
    size_t i;
    qpool p = (qpool) arg;

    for (i = startat; i < stopat; i++) {
	free(allthat[i]);
    }
}

int main(int argc, char *argv[])
{
    int threads = 1, interactive = 0;
    qthread_t *me;
    size_t i;
    unsigned long iterations = 1000;
    aligned_t *rets;
    qtimer_t timer = qtimer_new();

    if (argc >= 2) {
	threads = strtol(argv[1], NULL, 0);
	if (threads <= 0) {
	    threads = 1;
	    interactive = 0;
	} else {
	    interactive = 1;
	}
    }
    if (argc >= 3) {
	iterations = strtol(argv[2], NULL, 0);
    }

    assert(qthread_init(threads) == 0);
    me = qthread_self();

    allthat = malloc(sizeof(void*)*iterations);
    assert(allthat != NULL);
    if ((qp = qpool_create(me, 44)) == NULL) {
	fprintf(stderr, "qpool_create() failed!\n");
	exit(-1);
    }

    qtimer_start(timer);
    for (i = 0; i < iterations; i++) {
	if ((allthat[i] = qpool_alloc(me, qp)) == NULL) {
	    fprintf(stderr, "qpool_alloc() failed!\n");
	    exit(-1);
	}
	allthat[i][0] = i;
    }
    for (i = 0; i < iterations; i++) {
	assert(allthat[i][0] == i);
	qpool_free(qp, allthat[i]);
    }
    qtimer_stop(timer);
    printf("Time to set up the pool with %lu elements serially: %f\n", iterations,
	   qtimer_secs(timer));

    qtimer_start(timer);
    for (i = 0; i < iterations; i++) {
	if ((allthat[i] = qpool_alloc(me, qp)) == NULL) {
	    fprintf(stderr, "qpool_alloc() failed!\n");
	    exit(-1);
	}
	allthat[i][0] = i;
    }
    for (i = 0; i < iterations; i++) {
	assert(allthat[i][0] == i);
	qpool_free(qp, allthat[i]);
    }
    qtimer_stop(timer);
    printf("Time to alloc/free %lu pooled blocks serially: %f\n", iterations,
	   qtimer_secs(timer));

    qtimer_start(timer);
    for (i = 0; i < iterations; i++) {
	if ((allthat[i] = malloc(44)) == NULL) {
	    perror("malloc failed!");
	    exit(-1);
	}
	allthat[i][0] = i;
    }
    for (i = 0; i < iterations; i++) {
	assert(allthat[i][0] == i);
	free(allthat[i]);
    }
    qtimer_stop(timer);
    printf("Time to alloc/free %lu malloc blocks: %f\n", iterations,
	   qtimer_secs(timer));

    qtimer_start(timer);
    qt_loop_balance(0, iterations, pool_allocator, qp);
    qtimer_stop(timer);
    printf("Time to alloc %lu pooled blocks in parallel: %f\n",
	   iterations, qtimer_secs(timer));
    qtimer_start(timer);
    qt_loop_balance(0, iterations, pool_deallocator, qp);
    qtimer_stop(timer);
    printf("Time to free %lu pooled blocks in parallel: %f\n",
	   iterations, qtimer_secs(timer));

    qtimer_start(timer);
    qt_loop_balance(0, iterations, malloc_allocator, qp);
    qtimer_stop(timer);
    printf("Time to alloc %lu malloc blocks in parallel: %f\n",
	   iterations, qtimer_secs(timer));
    qtimer_start(timer);
    qt_loop_balance(0, iterations, malloc_deallocator, qp);
    qtimer_stop(timer);
    printf("Time to free %lu malloc blocks in parallel: %f\n",
	   iterations, qtimer_secs(timer));

    free(allthat);
    qpool_destroy(qp);

    qtimer_free(timer);
    qthread_finalize();
    if (interactive) {
	printf("success!\n");
    }
    return 0;
}
