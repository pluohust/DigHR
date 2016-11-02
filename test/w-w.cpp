#include <pthread.h>
#include <malloc.h>
#include <iostream>
#include <sys/time.h>
#include <unistd.h>

using namespace std;

pthread_mutex_t L1, L2;

int *p1, *p2, *p3, *p4, *p5, *p6, *p7, *p8;

void *TestThread(void *arg)
{
    sleep(1);
    pthread_mutex_lock(&L1);
    *p1=2;
    pthread_mutex_unlock(&L1);
    cout<<"*p1: "<<*p1<<endl;
    pthread_mutex_lock(&L2);
    *p2=2;
    pthread_mutex_unlock(&L2);
    return NULL;
}

int main()
{
    pthread_t testthread;
    int rc;

    p1=(int *)malloc(sizeof(int));
    char luopeng_malloc_get1[1];
    sprintf(luopeng_malloc_get1,"",8808,p1,sizeof(int));
    p2=(int *)malloc(sizeof(int));
    sprintf(luopeng_malloc_get1,"",8808,p2,sizeof(int));

    rc=pthread_create(&testthread, NULL, TestThread, 0);
    pthread_mutex_lock(&L1);
    *p1=1;
    *p2=2;
    pthread_mutex_unlock(&L1);
    pthread_join(testthread, 0);
    cout<<"p1: "<<hex<<p1<<dec<<endl;
    cout<<"p2: "<<hex<<p2<<dec<<endl;
    free(p1);
    free(p2);
    return 0;
}
