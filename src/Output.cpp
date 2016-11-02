#include "Output.h"

extern map<UINT32, ThreadVecTime> AllThread;
extern ofstream OutPutVectorTime;
extern size_t MaxSum;
extern ofstream ErrorLog;

void OutPutVecorTime(UINT32 threadID) //output Vector Time
{
    map<UINT32, ThreadVecTime>::iterator ITforallthread;
    map<UINT32, long>::iterator tmp1;
    ITforallthread=AllThread.find(threadID);
    if(AllThread.end()==ITforallthread)
    {
        ErrorLog<<"Cannot find the thread "<<dec<<threadID<<endl;
        return;
    }
    OutPutVectorTime<<"*"<<dec<<threadID<<"*   ";
    for(tmp1=(((ITforallthread->second).ListAddress)->VecTime).begin();tmp1!=(((ITforallthread->second).ListAddress)->VecTime).end();tmp1++)
    {
        OutPutVectorTime<<tmp1->first<<":"<<tmp1->second<<"$";
    }
    OutPutVectorTime<<endl;
}

void OutPutAllVecorTime(string SynName) //output all Vector Time
{
    map<UINT32, ThreadVecTime>::iterator ITforallthread;
    list<struct RWVecTime>::iterator ITforlist;
    map<UINT32, long>::iterator tmp1;
    
    OutPutVectorTime<<endl<<"********************"<<SynName<<"********************"<<endl;
    
    for(ITforallthread=AllThread.begin();ITforallthread!=AllThread.end();ITforallthread++)
    {
        OutPutVectorTime<<endl<<"------------ "<<ITforallthread->first<<" ------------"<<endl;
        for(ITforlist=((ITforallthread->second).VecTimeList).begin();ITforlist!=((ITforallthread->second).VecTimeList).end();ITforlist++)
        {
            for(tmp1=(ITforlist->VecTime).begin();tmp1!=(ITforlist->VecTime).end();tmp1++)
            {
                OutPutVectorTime<<tmp1->first<<":"<<tmp1->second<<"$";
            }
            OutPutVectorTime<<"  >>> "<<ITforlist->SynName;
            OutPutVectorTime<<endl;
        }
    }
    OutPutVectorTime<<endl;
}

void GetMaxMemory() //count memory used
{
    size_t mysum=0;
    map<UINT32, ThreadVecTime>::iterator AllIT;
    for(AllIT=AllThread.begin();AllIT!=AllThread.end();AllIT++)
    {
        mysum+=sizeof(UINT32);
        mysum+=sizeof((AllIT->second).ListAddress);
        mysum+=sizeof((AllIT->second).BlockFrame);
        mysum+=((AllIT->second).LockAcquired).size() * sizeof(ADDRINT);
        list<struct RWVecTime>::iterator listIT;
        for(listIT=((AllIT->second).VecTimeList).begin();listIT!=((AllIT->second).VecTimeList).end();listIT++)
        {
            mysum+=sizeof(listIT->SynName);
            mysum+=(listIT->VecTime).size() * (sizeof(UINT32) + sizeof(long));
            mysum+=(listIT->LockAcquired).size() * sizeof(ADDRINT);
            mysum+=(listIT->SharedLocation).size() * (sizeof(ADDRINT) + sizeof(SharedMemoryAccessInf));
            map<ADDRINT, LockDetailVCInf>::iterator NestIT;
            for(NestIT=(listIT->NestedLock_RecordOldLock).begin();NestIT!=(listIT->NestedLock_RecordOldLock).end();NestIT++)
            {
                mysum+=sizeof(ADDRINT)+sizeof(bool)+sizeof(THREADID);
                mysum+=2 * sizeof((NestIT->second).FirstFrame);
                mysum+=((NestIT->second).SharedMemoryAccess).size() * (sizeof(ADDRINT) + sizeof(WriteAccessInf));
            }
        }
    }
    if(mysum > MaxSum)
        MaxSum=mysum;
}

void printList(list<struct RWVecTime>::iterator ListforPrint, UINT32 threadID)
{
/*  map<UINT32, long>::iterator mapforvec;
    map<ADDRINT, SharedMemoryInf>::iterator mapforsharedmemory;
    set<ADDRINT>::iterator setforlock;
    if((ListforPrint->SharedMemory).empty())
        return;
    DataRaceOut<<"*****"<<endl;
    DataRaceOut<<"Thread ID: "<<threadID<<endl;
    DataRaceOut<<"VecTime:"<<endl;
    for(mapforvec=(ListforPrint->VecTime).begin(); mapforvec!=(ListforPrint->VecTime).end(); mapforvec++)
        DataRaceOut<<"Thread ID: "<<mapforvec->first<<" Vec: "<<mapforvec->second<<endl;
    for(mapforsharedmemory=(ListforPrint->SharedMemory).begin(); mapforsharedmemory!=(ListforPrint->SharedMemory).end(); mapforsharedmemory++)
    {
        DataRaceOut<<"Address: "<<hex<<mapforsharedmemory->first<<dec<<endl;
        if((mapforsharedmemory->second).Rstatus)
        {
            DataRaceOut<<"R: "<<((mapforsharedmemory->second).RLockAcquired).size()<<endl<<hex;
            DataRaceOut<<dec<<endl;
        }
        if((mapforsharedmemory->second).Wstatus)
        {
            DataRaceOut<<"W: "<<((mapforsharedmemory->second).RLockAcquired).size()<<endl<<hex;
            DataRaceOut<<dec<<endl;
        }
    }*/
}
