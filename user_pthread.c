#include <malloc.h>
#include <ucontext.h>
#include<unistd.h>
#include <stdio.h>
#include <sys/time.h>
#include<stdlib.h>
#include<signal.h>
#define THREAD_STACKSIZE 32767
#define MAXTHREADS	20

int acti=0;
typedef struct thread_t
{
	int id;
	int active;
	ucontext_t context;
	struct thread_t *next;
	struct thread_t *prev;
	
} thread_t;

thread_t* Main=new thread_t();

thread_t *readyq=NULL,*current_thread=NULL,*head=NULL,*tail=NULL;
thread_t thread_table[MAXTHREADS];



void schedule(int signal)      //signal handler for SIGPROF
{

	thread_t *t1;

	ucontext_t dummy;
	
		
		if(current_thread->active==2)
		{
			t1 = current_thread;
			printf("scheduling main\n");
			usleep(100000);
			current_thread = current_thread->next;
			swapcontext(&t1->context,&current_thread->context);
		}
		
		if(current_thread->active==0 )
		{
			printf("scheduling1\n");
			t1 = current_thread;
			if(current_thread->next==NULL)
			{
			current_thread=head;
			}
			else
			{
			current_thread = current_thread->next;
			}
			usleep(100000);
			swapcontext(&dummy,&current_thread->context);
		}
		

/*when there are no more threads in the queue... just stop the 
timer and execute the main function until it completes...
*/

}
void kill_thread()
{	
	 sigset_t a,b;
	 sigemptyset(&a);
	 sigaddset(&a, SIGPROF);   
	 sigprocmask(SIG_BLOCK, &a, &b);
	printf("killing thread");
	
	if(current_thread==tail)
	{
		
		current_thread->prev->next=NULL;
		tail=current_thread->prev;
		current_thread->prev=NULL;
		printf("bye\n");
		fflush(stdout);
		//free(current);
	}
	else
	{
		current_thread->prev->next=current_thread->next;
		current_thread->next->prev=current_thread->prev;
	}
	
	sigprocmask(SIG_SETMASK, &b, NULL);
	acti--;
	kill(getpid(),SIGPROF);


/*remove the thread from the ready queue  call the schedule function. */

}
void hello(void* arg)
{
	int i=*(int *)arg; 
	printf("Hello world .This is %d\n",i);
	
	 
}
void initfiber(void (*func)(void),void* arg)   
{
   printf("hi");
  
	hello(arg);
	printf("hi");
	current_thread->active=0;
	kill_thread();
	
    
}
void init()
{
	for ( int i = 0; i < MAXTHREADS; ++ i )
	{
		thread_table[i].active = 0;
	}
}
thread_t* create_thread( void (*func) (void*),void* arg)
{
   /*Initialize the context using getcontext(), attach a function
 	using makecontext() and keep this in the ready_queue*/
 	struct itimerval t1;
 	static int thrno = 1;
 	thread_t *thr;
 	
	for (int i=0; i<MAXTHREADS; i++)
	{
		if (thread_table[i].active == 0)
		{
			printf("%d\n",i);
			thr=&thread_table[i];
			thread_table[i].active=1;
			break;
		}
	}

 	getcontext(&thr->context);
 
	 thr->context.uc_link = 0;
	 thr->context.uc_stack.ss_sp = malloc( THREAD_STACKSIZE );
	 thr->context.uc_stack.ss_size = THREAD_STACKSIZE;
	 thr->context.uc_stack.ss_flags = 0;    
	 makecontext( &thr->context, (void (*)(void)) &initfiber, 2, func,arg );
	 
	 sigset_t a,b;
	 sigemptyset(&a);
	 sigaddset(&a, SIGPROF);   
	 sigprocmask(SIG_BLOCK, &a, &b);
 		printf("Making threads\n");
	 if(readyq==NULL)
	 {
	 	
	 	
	 	//create main thread
		
			 	Main->id = thrno++;
			 	Main->active=2;
			 	Main->next = NULL;
				Main->prev = NULL;
			 	head=tail=Main;
			 	readyq=Main;
			 	//start timer as it is first thread;
			 	signal(SIGPROF, schedule);
			 	t1.it_interval.tv_usec = 50000;
				t1.it_interval.tv_sec = 0;
				t1.it_value.tv_usec = 0;
				t1.it_value.tv_sec = 1;
				setitimer(ITIMER_PROF, &t1, NULL);
				printf("main thread created and timer started\n");
			

	 }
	 
	tail->next =thr;
	thr->prev = tail;
	thr->next=NULL;
	tail = thr;
	current_thread=head;
	acti++;
	sigprocmask(SIG_SETMASK, &b, NULL);
	
	return thr;


}

void waitforallthreads()
{
	while(acti>0){
	printf("This is main thread\n");

		for (int j=0; j<0x200000; j++) ;

	}
	
	
}

int main()
{
	init();
	
	//getcontext(&Main->context);
	thread_t *t1, *t2;

	int t1no=1,t2no=2;
	t1 = create_thread(hello, (void *)&t1no);
	t2 = create_thread(hello, (void *)&t2no);
	
	waitforallthreads();
	
}
