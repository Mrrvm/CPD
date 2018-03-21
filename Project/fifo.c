#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <omp.h>

struct node {
	struct node *next;
	int data;
};

struct queue {
	omp_lock_t lock;
	struct node *head;
	struct node *tail;
};

/* append data to q->tail */
void enqueue(struct queue *q, int data) {
	struct node *n;

	/* prepare node */
	n = malloc(sizeof(*n));
	n->data = data;
	n->next = NULL;

	omp_set_lock(&q->lock);

	/* enqueue node */
	if (q->tail)
		q->tail->next = n;
	q->tail = n;

	/* let head point to node if queue was empty */
	if (!q->head)
		q->head = q->tail;

	omp_unset_lock(&q->lock);
}

/* return q->head->data */
int dequeue(struct queue *q) {
	int data;
	struct node *n;

	omp_set_lock(&q->lock);

	/* check for empty queue */
	if (!q->head) {
		omp_unset_lock(&q->lock);
		return 0;
	}

	/* dequeue head node */
	n = q->head;
	q->head = q->head->next;

	/* don't let tail point to the dequeued node */
	if (!q->head)
		q->tail = NULL;

	omp_unset_lock(&q->lock);

	/* extract data and free node */
	data = n->data;
	free(n);
	assert(data);

	return data;
}



int main( int argc, char **argv ) {
	struct queue q = { .head = NULL, .tail = NULL };
	omp_init_lock(&q.lock);

	#pragma omp parallel
	#pragma omp single nowait
	{
		int i;
		for(i=1;i<5;++i) {
			#pragma omp task
			{
				int j;
				for(j = 0; j < 10; ++j) {
					enqueue(&q, i*10+j);
					printf("enqueue %d\n", i*10+j);
				}
			}
			#pragma omp task
			{
				int d, j;
				for(j = 0; j < 10; ++j) {
					d = dequeue(&q);
					if (d)
						printf("dequeue %d\n", d);
				}
			}
		}
	}

	int d;
	while(1) {
		d = dequeue(&q);
		if (!d)
			break;
		printf("dequeue %d\n", d);
	}
	assert(!q.head && !q.tail);

	omp_destroy_lock(&q.lock);
	return 0;
}
