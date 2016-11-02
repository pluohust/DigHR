#include "monitor_function.h"
#include "monitor_shared_memory.h"
#include "VectorClocks.h"

extern ofstream Imgfile;

VOID ImageLoad(IMG img, VOID *v)
{
    Imgfile<< "IMG name : "<< IMG_Name(img) <<" Address: "<<IMG_StartAddress(img)<<endl;
    RTN rtnmalloc = RTN_FindByName(img, "malloc");
    if ( RTN_Valid( rtnmalloc ))
    {
        RTN_Open(rtnmalloc);
        RTN_InsertCall(rtnmalloc, IPOINT_BEFORE, AFUNPTR(BeforeMalloc), IARG_FUNCARG_ENTRYPOINT_VALUE, 0, IARG_THREAD_ID, IARG_PTR , (VOID *)(IMG_Name(img)).c_str(), IARG_END);
        RTN_InsertCall(rtnmalloc, IPOINT_AFTER, AFUNPTR(AfterMalloc), IARG_FUNCRET_EXITPOINT_VALUE, IARG_THREAD_ID, IARG_PTR, (VOID *)(IMG_Name(img)).c_str(), IARG_END);
        RTN_Close(rtnmalloc);
    }
    
    RTN rtncalloc = RTN_FindByName(img, "calloc");
    if ( RTN_Valid( rtncalloc ))
    {
        RTN_Open(rtncalloc);
        RTN_InsertCall(rtncalloc, IPOINT_BEFORE, AFUNPTR(BeforeCalloc), IARG_FUNCARG_ENTRYPOINT_VALUE, 0, IARG_FUNCARG_ENTRYPOINT_VALUE, 1, IARG_THREAD_ID, IARG_END);
        RTN_InsertCall(rtncalloc, IPOINT_AFTER, AFUNPTR(AfterCalloc), IARG_FUNCRET_EXITPOINT_VALUE, IARG_THREAD_ID, IARG_END);
        RTN_Close(rtncalloc);
    }
    
    RTN rtnfree = RTN_FindByName(img, "free");
    if ( RTN_Valid( rtnfree ))
    {
        RTN_Open(rtnfree);
        RTN_InsertCall(rtnfree, IPOINT_BEFORE, AFUNPTR(BeforeFree), IARG_FUNCARG_ENTRYPOINT_VALUE, 0, IARG_THREAD_ID, IARG_END);
        RTN_Close(rtnfree);
    }
    
    RTN rtnpthread_create = RTN_FindByName(img, "pthread_create");
    if ( RTN_Valid( rtnpthread_create ))
    {
        RTN_Open(rtnpthread_create);
        RTN_InsertCall(rtnpthread_create, IPOINT_BEFORE, AFUNPTR(BeforePthread_create), IARG_THREAD_ID, IARG_END);
        RTN_Close(rtnpthread_create);
    }
    
    RTN rtnpthread_join = RTN_FindByName(img, "pthread_join");
    if ( RTN_Valid( rtnpthread_join ))
    {
        RTN_Open(rtnpthread_join);
        RTN_InsertCall(rtnpthread_join, IPOINT_AFTER, (AFUNPTR)AfterPthread_join, IARG_THREAD_ID, IARG_END);
        RTN_Close(rtnpthread_join);
    }
    
    RTN rtnpthread_mutex_lock = RTN_FindByName(img, "pthread_mutex_lock");
    if ( RTN_Valid( rtnpthread_mutex_lock ))
    {
        RTN_Open(rtnpthread_mutex_lock);
        RTN_InsertCall(rtnpthread_mutex_lock, IPOINT_BEFORE, AFUNPTR(BeforePthread_mutex_lock), IARG_FUNCARG_ENTRYPOINT_VALUE, 0, IARG_THREAD_ID, IARG_END);
        RTN_InsertCall(rtnpthread_mutex_lock, IPOINT_AFTER, AFUNPTR(AfterPthread_mutex_lock), IARG_THREAD_ID, IARG_END);
        RTN_Close(rtnpthread_mutex_lock);
    }
    
    RTN rtnpthread_mutex_trylock = RTN_FindByName(img, "pthread_mutex_trylock");
    if ( RTN_Valid( rtnpthread_mutex_trylock ))
    {
        RTN_Open(rtnpthread_mutex_trylock);
        RTN_InsertCall(rtnpthread_mutex_trylock, IPOINT_BEFORE, AFUNPTR(BeforePthread_mutex_trylock), IARG_FUNCARG_ENTRYPOINT_VALUE, 0, IARG_THREAD_ID, IARG_END);
        RTN_InsertCall(rtnpthread_mutex_trylock, IPOINT_AFTER, AFUNPTR(AfterPthread_mutex_trylock), IARG_FUNCRET_EXITPOINT_VALUE, IARG_THREAD_ID, IARG_END);
        RTN_Close(rtnpthread_mutex_trylock);
    }
    
    RTN rtnpthread_mutex_unlock = RTN_FindByName(img, "pthread_mutex_unlock");
    if ( RTN_Valid( rtnpthread_mutex_unlock ))
    {
        RTN_Open(rtnpthread_mutex_unlock);
        RTN_InsertCall(rtnpthread_mutex_unlock, IPOINT_BEFORE, AFUNPTR(BeforePthread_mutex_unlock), IARG_FUNCARG_ENTRYPOINT_VALUE, 0, IARG_THREAD_ID, IARG_END);
        RTN_Close(rtnpthread_mutex_unlock);
    }
    
    RTN rtnpthread_cond_signal = RTN_FindByName(img, "pthread_cond_signal");
    if ( RTN_Valid( rtnpthread_cond_signal ))
    {
        RTN_Open(rtnpthread_cond_signal);
        RTN_InsertCall(rtnpthread_cond_signal, IPOINT_BEFORE, AFUNPTR(BeforePthread_cond_signal), IARG_FUNCARG_ENTRYPOINT_VALUE, 0, IARG_THREAD_ID, IARG_END);
        RTN_Close(rtnpthread_cond_signal);
    }
    
    RTN rtnpthread_cond_broadcast = RTN_FindByName(img, "pthread_cond_broadcast");
    if ( RTN_Valid( rtnpthread_cond_broadcast ))
    {
        RTN_Open(rtnpthread_cond_broadcast);
        RTN_InsertCall(rtnpthread_cond_broadcast, IPOINT_BEFORE, AFUNPTR(BeforePthread_cond_broadcast), IARG_FUNCARG_ENTRYPOINT_VALUE, 0, IARG_THREAD_ID, IARG_END);
        RTN_Close(rtnpthread_cond_broadcast);
    }
    
    RTN rtnpthread_cond_wait = RTN_FindByName(img, "pthread_cond_wait");
    if ( RTN_Valid( rtnpthread_cond_wait ))
    {
        RTN_Open(rtnpthread_cond_wait);
        RTN_InsertCall(rtnpthread_cond_wait, IPOINT_BEFORE, AFUNPTR(BeforePthread_cond_wait), IARG_FUNCARG_ENTRYPOINT_VALUE, 0, IARG_FUNCARG_ENTRYPOINT_VALUE, 1, IARG_THREAD_ID, IARG_END);
        RTN_InsertCall(rtnpthread_cond_wait, IPOINT_AFTER, AFUNPTR(AfterPthread_cond_wait), IARG_THREAD_ID, IARG_END);
        RTN_Close(rtnpthread_cond_wait);
    }
    
    RTN rtnpthread_cond_timedwait = RTN_FindByName(img, "pthread_cond_timedwait");
    if ( RTN_Valid( rtnpthread_cond_timedwait ))
    {
        RTN_Open(rtnpthread_cond_timedwait);
        RTN_InsertCall(rtnpthread_cond_timedwait, IPOINT_BEFORE, AFUNPTR(BeforePthread_cond_timedwait), IARG_FUNCARG_ENTRYPOINT_VALUE, 0, IARG_FUNCARG_ENTRYPOINT_VALUE, 1, IARG_THREAD_ID, IARG_END);
        RTN_InsertCall(rtnpthread_cond_timedwait, IPOINT_AFTER, AFUNPTR(AfterPthread_cond_timedwait), IARG_THREAD_ID, IARG_END);
        RTN_Close(rtnpthread_cond_timedwait);
    }
    
    RTN rtnmain = RTN_FindByName(img, "main");
    if ( RTN_Valid( rtnmain ))
    {
        RTN_Open(rtnmain);
        RTN_InsertCall(rtnmain, IPOINT_BEFORE, AFUNPTR(Beforemain), IARG_END);
        RTN_Close(rtnmain);
    }
    RTN rtnsprintf = RTN_FindByName(img, "sprintf");
    if ( RTN_Valid( rtnsprintf ))
    {
        RTN_Open(rtnsprintf);
        RTN_InsertCall(rtnsprintf, IPOINT_BEFORE, AFUNPTR(BeforeSprintf), IARG_FUNCARG_ENTRYPOINT_VALUE, 2, IARG_FUNCARG_ENTRYPOINT_VALUE, 3, IARG_FUNCARG_ENTRYPOINT_VALUE, 4, IARG_FUNCARG_ENTRYPOINT_VALUE, 5, IARG_FUNCARG_ENTRYPOINT_VALUE, 6, IARG_THREAD_ID, IARG_END);
        RTN_Close(rtnsprintf);
    }
}
