#include <pthread.h>
#include <stdio.h>

#include "barrier.h"

#define LENGTH	16
#define NUM_THREADS	4

void *sort_thread();


struct list_descriptor
{
	int start_list;
	int list_length;
	int start_thread;
	int end_thread;
	int numthreads;
	pthread_mutex_t sum_lock;
	int less_than;
	float pivot;
	pthread_cond_t less_scan_cond;
	pthread_cond_t more_scan_cond;
	pthread_mutex_t less_scan_lock;
	pthread_mutex_t more_scan_lock;
	struct barrier_variable barrier1, barrier2, barrier3, barrier4;
	struct list_descriptor *less_list;
	struct list_descriptor *greater_list;
	int less_scan_int, less_scan_val;
	int more_scan_int, more_scan_val;
};

struct list_descriptor *list;
	
float a[LENGTH];
float temp[LENGTH];
int position[LENGTH];

main()
{
	int i;

/*
	pthread_init();
*/

        for (i=0; i< LENGTH; i++)
		a[i] = (rand()%256) / 100.0;

        for (i=0; i< LENGTH; i++)
		printf("%5.2f ", a[i]);
	printf("\n\n");

	list = (struct list_descriptor *)
			malloc(sizeof(struct list_descriptor));

	initialize_list_descriptor(list);

	block_qsort();
        for (i=0; i< LENGTH; i++)
		printf("%5.2f ", a[i]);
	printf("\n\n");

}


init_barrier(struct barrier_variable *barrier)
{
        pthread_mutex_init(&(barrier->lock), NULL);
        pthread_cond_init(&(barrier->cond), NULL);
        barrier->count = 0;
}


barrier(struct barrier_variable *barrier, int thread_count)
{
        pthread_mutex_lock(&(barrier->lock));
        (barrier -> count) ++;
        if ((barrier -> count) == thread_count)
        {
                barrier -> count = 0;
                pthread_cond_broadcast(&(barrier->cond));
        }
        else
        while (pthread_cond_wait(&(barrier->cond), &(barrier->lock)) != 0);
        pthread_mutex_unlock(&(barrier->lock));
}



initialize_list_descriptor(struct list_descriptor *head)
{
	init_barrier(&(head -> barrier1));
	init_barrier(&(head -> barrier2));
	init_barrier(&(head -> barrier3));
	init_barrier(&(head -> barrier4));

	pthread_cond_init(&(head->less_scan_cond), NULL);
	pthread_cond_init(&(head->more_scan_cond), NULL);

	pthread_mutex_init(&(head->less_scan_lock), NULL);
	pthread_mutex_init(&(head->more_scan_lock), NULL);

	head -> less_scan_int = 0;
	head -> less_scan_val = 0;
	head -> more_scan_int = 0;
	head -> more_scan_val = 0;
	head -> less_list = NULL;
	head -> greater_list = NULL;

	head -> start_list = 0;
	head -> list_length = LENGTH;
	head -> start_thread = 0;
	head -> end_thread = NUM_THREADS - 1;
	head -> numthreads = NUM_THREADS;
	pthread_mutex_init(&(head -> sum_lock), NULL);
	head -> less_than = 0;
	head -> pivot = a[0];
}

block_qsort()
{
	int i;
	pthread_t sort_threads[NUM_THREADS];
	int workspace[NUM_THREADS];

	for (i=0; i<NUM_THREADS; i++)
	{
		workspace[i] = i;
		pthread_create(&sort_threads[i], NULL, sort_thread,
				(void *) &workspace[i]);
	}

	for (i=0; i<NUM_THREADS; i++)
		pthread_join(sort_threads[i], NULL);
}

void *sort_thread(void *ptr)
{
	int thread_no, numthreads, less_threads, less, greater;
	int i, less_before_me, more_before_me;
	int my_start_index, my_length;
	struct list_descriptor *head;
	float partition;

	head = list;

	thread_no = * ((int *) ptr);

	while ((head -> numthreads > 1) && (head -> list_length > 1))
	{
	less = greater = 0;
	partition = (float)head -> list_length / (float)head -> numthreads;

	if (thread_no == head -> start_thread)
		my_start_index = head -> start_list;
	else
		my_start_index = head -> start_list +
		(int) ((float)(thread_no - head -> start_thread) * partition); 

	if (thread_no == head -> end_thread)
		my_length = head -> list_length + head -> start_list
				- my_start_index;
	else
		my_length = head -> start_list + (int) ((float)(thread_no -
		head -> start_thread + 1) * partition) - my_start_index;


	for (i=my_start_index; i<my_start_index + my_length; i++)
	{
		if (a[i] < head -> pivot)
			less ++;
		else
			greater ++;
	}

	pthread_mutex_lock(&(head->sum_lock));
	head -> less_than += less;
	pthread_mutex_unlock(&(head->sum_lock));

	barrier(&(head -> barrier2), head -> numthreads);

	less_before_me = sum_scan(head -> start_thread, head -> end_thread,
			thread_no, less, &(head -> less_scan_val),
			&(head -> less_scan_int), &(head->less_scan_cond),
			&(head->less_scan_lock));

	more_before_me = sum_scan(head -> start_thread, head -> end_thread,
                        thread_no, greater, &(head -> more_scan_val),
			&(head -> more_scan_int), &(head->more_scan_cond),
			&(head->more_scan_lock));

	for (i=my_start_index; i<my_start_index + my_length; i++)
	{
		if (a[i] < head -> pivot)
			position[i] = less_before_me ++;
		else
			position[i] = head -> less_than + more_before_me ++;
	}

	for (i=my_start_index; i<my_start_index + my_length; i++)
		temp[head -> start_list + position[i]] = a[i];

	barrier(&(head -> barrier1), head -> numthreads);

	for (i=my_start_index; i<my_start_index + my_length; i++)
		a[i] = temp[i];
	barrier(&(head -> barrier4), head -> numthreads);

	less_threads = (head -> less_than * head -> numthreads) /
			head -> list_length;
	if ((head -> less_than > 1) && (less_threads == 0))
		less_threads = 1;

	if ((head -> list_length - head -> less_than) <= 1)
		less_threads = head -> numthreads;

	if ((thread_no == head -> start_thread) &&
		(head -> numthreads > 1))
	{
		head -> less_list = (struct list_descriptor *)
                        malloc(sizeof(struct list_descriptor));

		initialize_list_descriptor(head -> less_list);

		head -> less_list -> start_list = head -> start_list;
		head -> less_list -> list_length = head -> less_than;


		head -> less_list -> pivot = a[head -> start_list];

		head -> less_list -> start_thread = head -> start_thread;

		head -> less_list -> end_thread = head -> start_thread +
							less_threads - 1;
		head -> less_list -> numthreads = less_threads;


		head -> greater_list = (struct list_descriptor *)
                        malloc(sizeof(struct list_descriptor));
		initialize_list_descriptor(head -> greater_list);

		head -> greater_list -> start_list = head -> start_list +
						head -> less_than + 1;
		head -> greater_list -> list_length = head -> list_length -
						head -> less_than - 1;
		head -> greater_list -> pivot = 
				a[head -> greater_list -> start_list];
		head -> greater_list -> start_thread = head -> start_thread +
							less_threads;
		head -> greater_list -> end_thread = head -> end_thread;
		
		head -> greater_list -> numthreads = head -> numthreads -
							less_threads;
	}

	barrier(&(head -> barrier3), head -> numthreads);

	if (thread_no < (head -> start_thread + less_threads))
		head = head -> less_list;
	else
		head = head -> greater_list;


	}
	serial_qsort(&(a[head->start_list]), head -> list_length,
		&(position[head->start_list]), &(temp[head->start_list]));
}

serial_qsort(list, length, position, temp)
float *list;
int *position;
float *temp;
int length;
{
        /* pick a pivot */
        float pivot;
        int less, i;
        int current_less_pos, current_more_pos;

        if (length <= 1)
                return;

        pivot = list[0];
        less = 0;
        for (i=0; i< length; i++)
                if (list[i] < pivot)
                        less ++;

        current_less_pos = 0;
        current_more_pos = less;

        for (i=0; i< length; i++)
                if (list[i] < pivot)
                {
                        position[i] = current_less_pos;
                        current_less_pos ++;
                }
                else
                {
                        position[i] = current_more_pos;
                        current_more_pos ++;
                }

        for (i=0; i< length; i++)
                temp[position[i]] = list[i];

        for (i=0; i< length; i++)
                list[i] = temp[i];

        serial_qsort(list, less, position, temp);
        serial_qsort(&list[less+1], (length - less - 1), position, temp);
}


	

sum_scan (start_thread, end_thread, thread_no, variable, scan_val, scan_int,
condition, condition_lock)
int start_thread, end_thread, thread_no, variable;
int *scan_val, *scan_int;
pthread_cond_t *condition;
pthread_mutex_t *condition_lock;
{
	int my_val, offset;

	offset = thread_no - start_thread;
	
	pthread_mutex_lock(condition_lock);
	if (*scan_int == offset)
		pthread_cond_broadcast(condition);
	while (*scan_int != offset)
		while (pthread_cond_wait(condition, condition_lock) != 0);
	my_val = *scan_val;
	(*scan_val) += variable;
	(*scan_int) ++;
	pthread_mutex_unlock(condition_lock);

	return(my_val);
}
