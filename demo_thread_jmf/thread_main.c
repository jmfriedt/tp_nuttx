#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define NTHREADS 10

void *thread_function(void *);
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
int counter=0;  // cette init a 0 ne suffit pas, il FAUT faire dans main

#ifdef CONFIG_BUILD_KERNEL
int main(int argc, char *argv[])
// int main(int argc, FAR char *argv[])
#else
int threadjmf_main(int argc, char *argv[])
#endif
{int d[10];
 pthread_t thread_id[NTHREADS];
 int i,n;
 counter=0;
 if (argc>1) n=strtol(argv[1],NULL,10); else n=NTHREADS;
 for (i=0; i < n; i++)   
     {d[i]=i;
      printf("pthread_create%d: %d\n",i,pthread_create( &thread_id[i], NULL, thread_function, &d[i] ));
     } // CREATION DES 10 THREADS
 for(i=0; i < n; i++) {pthread_join( thread_id[i], NULL);}     // ATTENTE DE LA MORT DE CES THREADS
 printf("Final counter value: %d\n", counter);
 return(0);
}

void *thread_function(void *d)
{int lcount;
 pthread_mutex_lock( &mutex1 );
 lcount=counter;
 lcount++;
 printf("Thread number %d: %lx\n", *(int *)d, pthread_self());
 usleep(100000); // 100 ms
 counter=lcount;
 pthread_mutex_unlock( &mutex1 );
 return(NULL);
}
