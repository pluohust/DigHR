#include "monitor_shared_memory.h"

extern set<ADDRINT> ShareVarAddress;
extern map<UINT32, ThreadVecTime> AllThread;
extern ofstream Testfile;
extern bool startmallocmonitor;
extern vector<ShareAddreeStruct> MallocVec;
extern PIN_LOCK lock;
extern PIN_LOCK shareaddrlock;
extern map<UINT32, USIZE> MallocSize;
extern map<UINT32, USIZE> CallocSize;
extern ofstream Imgfile;
extern ofstream ErrorLog;
extern set<ADDRINT> SynAddress;

VOID GetShareAddress() //if you know the address of global variables, please store them in address file.
{
    FILE *faddress;
    char oneline[50];
    ADDRINT oneaddress;
    int i;
    if((faddress=fopen("./address","rt"))==NULL)
    {
        ErrorLog<<"Can't open address"<<endl;
        return;
    }
    while(!feof(faddress))
    {
        oneaddress=0;
        fgets(oneline,50,faddress);
        if('\0'==oneline[0])
            break;
        for(i=0;' '!=oneline[i];i++);
        i=i+3;
        while(('\0'!=oneline[i])&&(i<50))
        {
            if((oneline[i]>='0')&&(oneline[i]<='9'))
            {
                oneaddress=oneaddress*16;
                oneaddress=oneaddress+oneline[i]-'0';
            }
            else if((oneline[i]>='a')&&(oneline[i]<='f'))
            {
                oneaddress=oneaddress*16;
                oneaddress=oneaddress+10+oneline[i]-'a';
            }
            else if((oneline[i]>='A')&&(oneline[i]<='F'))
            {
                oneaddress=oneaddress*16;
                oneaddress=oneaddress+10+oneline[i]-'A';
            }
            i++;
        }
        ShareVarAddress.insert(oneaddress);
    }
    fclose(faddress);
}

BOOL IsGlobalVariable(ADDRINT address) //identify if the address is a shared-memory location
{
    if(SynAddress.find(address)!=SynAddress.end())
        return false;
    else if(ShareVarAddress.find(address)!=ShareVarAddress.end()) //if it is in ShareVarAddress
        return true;
    else if(!MallocVec.empty()) //else if it is a heap location and use binary chop algorithm
    {
        int low=0;
        int high=MallocVec.size() - 1;
        int mid=(low+high)/2;
        if((MallocVec[0]).address_name > address)
            return false;
        else if((MallocVec[high]).address_name < address)
        {
            if((MallocVec[high]).address_name+(MallocVec[high]).address_size > address)
            {
                ShareVarAddress.insert(address);
                return true;
            }
            else
                return false;
        }
        while (low < high)
        {
            mid=(low+high)/2;
            if((MallocVec[mid]).address_name > address)
                high=mid-1;
            else if((MallocVec[mid]).address_name < address)
                low=mid+1;
            else
                break;
        }
        if((MallocVec[mid]).address_name > address)
        {
            if(((MallocVec[mid-1]).address_name <= address)&&((MallocVec[mid-1]).address_name + (MallocVec[mid-1]).address_size) > address)
            {
                ShareVarAddress.insert(address);
                return true;
            }
            else
                return false;
        }
        else if((MallocVec[mid]).address_name < address)
        {
            if((MallocVec[mid]).address_name + (MallocVec[mid]).address_size> address)
            {
                ShareVarAddress.insert(address);
                return true;
            }
            else
                return false;
        }
        else
        {
            ShareVarAddress.insert(address);
            return true;
        }
    }
    return false;
}

VOID RecordMemRead(VOID * ip, VOID * addr, UINT32 size, THREADID threadid)
{
    RecordType GetOne;
    PIN_GetLock(&lock, threadid+1);
    if(IsGlobalVariable((ADDRINT)addr))
    {
        GetOne=(RecordType) {10, threadid, (ADDRINT)addr}; //10 is a magic number to represent a write operation
        Testfile<<"10"<<" "<<threadid<<" "<<hex<<(ADDRINT)addr<<dec<<endl;
        
        map<ADDRINT, SharedMemoryAccessInf>::iterator mapforsharedmemory;
        map<UINT32, ThreadVecTime>::iterator ITforAllThread;
        ITforAllThread=AllThread.find(GetOne.threadID);
        if(AllThread.end()==ITforAllThread)
        {
            ErrorLog<<"Cannot find the thread "<<dec<<threadid<<endl;
            PIN_ReleaseLock(&lock);
            return;
        }
        mapforsharedmemory=(((ITforAllThread->second).ListAddress)->SharedLocation).find(GetOne.object);
        if(mapforsharedmemory==(((ITforAllThread->second).ListAddress)->SharedLocation).end()) // a new address
        {
            SharedMemoryAccessInf tmpSharedMemoryAccessInf;
            tmpSharedMemoryAccessInf.Rstatus=true;
            tmpSharedMemoryAccessInf.Wstatus=false;
            tmpSharedMemoryAccessInf.Rcount=1;
            tmpSharedMemoryAccessInf.Wcount=0;
            (((ITforAllThread->second).ListAddress)->SharedLocation).insert(pair<ADDRINT, SharedMemoryAccessInf>(GetOne.object,tmpSharedMemoryAccessInf));
        }
        else // there is access to this address before
        {
            if(!(mapforsharedmemory->second).Rstatus)
            {
                (mapforsharedmemory->second).Rstatus=true;
                (mapforsharedmemory->second).Rcount=1;
            }
            else
            {
                (mapforsharedmemory->second).Rcount++;
            }
        }
    }
    PIN_ReleaseLock(&lock);
}

VOID RecordMemWrite(VOID * ip, VOID * addr, UINT32 size, THREADID threadid)
{
    RecordType GetOne;
    PIN_GetLock(&lock, threadid+1);
    if(IsGlobalVariable((ADDRINT)addr))
    {
        GetOne=(RecordType) {11, threadid, (ADDRINT)addr}; //10 is a magic number to represent a write operation
        Testfile<<"11"<<" "<<threadid<<" "<<hex<<(ADDRINT)addr<<dec<<endl;
        
        map<ADDRINT, SharedMemoryAccessInf>::iterator mapforsharedmemory;
        map<UINT32, ThreadVecTime>::iterator ITforAllThread;
        ITforAllThread=AllThread.find(GetOne.threadID);
        if(AllThread.end()==ITforAllThread)
        {
            ErrorLog<<"Cannot find the thread "<<dec<<threadid<<endl;
            PIN_ReleaseLock(&lock);
            return;
        }
        mapforsharedmemory=(((ITforAllThread->second).ListAddress)->SharedLocation).find(GetOne.object);
        if(mapforsharedmemory==(((ITforAllThread->second).ListAddress)->SharedLocation).end()) // a new address
        {
            SharedMemoryAccessInf tmpSharedMemoryAccessInf;
            tmpSharedMemoryAccessInf.Rstatus=false;
            tmpSharedMemoryAccessInf.Wstatus=true;
            tmpSharedMemoryAccessInf.Rcount=0;
            tmpSharedMemoryAccessInf.Wcount=1;
            (((ITforAllThread->second).ListAddress)->SharedLocation).insert(pair<ADDRINT, SharedMemoryAccessInf>(GetOne.object,tmpSharedMemoryAccessInf));
        }
        else // there is access to this address before
        {
            if(!(mapforsharedmemory->second).Wstatus)
            {
                (mapforsharedmemory->second).Wstatus=true;
                (mapforsharedmemory->second).Wcount=1;
            }
            else
            {
                (mapforsharedmemory->second).Wcount++;
            }
        }
    }
    PIN_ReleaseLock(&lock);
}

VOID Instruction(INS ins, VOID *v)
{
    UINT32 memOperands = INS_MemoryOperandCount(ins);

    for (UINT32 memOp = 0; memOp < memOperands; memOp++)
    {
        if (INS_MemoryOperandIsRead(ins, memOp))
            INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)RecordMemRead, IARG_INST_PTR, IARG_MEMORYOP_EA, memOp, IARG_MEMORYREAD_SIZE, IARG_THREAD_ID, IARG_END);
        if (INS_MemoryOperandIsWritten(ins, memOp))
            INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)RecordMemWrite, IARG_INST_PTR, IARG_MEMORYOP_EA, memOp, IARG_MEMORYWRITE_SIZE, IARG_THREAD_ID, IARG_END);
    }
}

VOID BeforeMalloc(ADDRINT size, THREADID threadid, VOID *imgname)
{
    string malloccheck=(const char *)imgname;
    PIN_GetLock(&shareaddrlock, threadid+1);
    if(startmallocmonitor)
    {
        MallocSize.insert(make_pair(threadid, size));
    }
    PIN_ReleaseLock(&shareaddrlock);
}

VOID AfterMalloc(ADDRINT ret,THREADID threadid, VOID *imgname)
{
    string malloccheck=(const char *)imgname;
    PIN_GetLock(&shareaddrlock, threadid+1);
    Imgfile<<"MALLOC->"<<(const char *)imgname<<endl;
    if(startmallocmonitor)
    {
        map<UINT32, USIZE>::iterator mallocfinder;
        mallocfinder=MallocSize.find(threadid);
        if(mallocfinder==MallocSize.end())
        {
            ErrorLog<<"Malloc error!\n";
            PIN_ReleaseLock(&shareaddrlock);
            return;
        }
        if((mallocfinder->second)<10240)
        {
            MallocVec.push_back((ShareAddreeStruct) {ret, mallocfinder->second}); //store the malloc information to identify shared-memory locations
            sort(MallocVec.begin(), MallocVec.end(), less<ShareAddreeStruct>());
            Imgfile<<"Thread "<<threadid<<" Malloc address "<<ret<<" Size "<<mallocfinder->second<<endl;
        }
        MallocSize.erase(mallocfinder);
    }
    PIN_ReleaseLock(&shareaddrlock);
}

VOID BeforeSprintf(ADDRINT funcflag, ADDRINT dest, ADDRINT source, ADDRINT length, const CHAR * filename, THREADID threadid)
{
    PIN_GetLock(&shareaddrlock, threadid+1);
    switch(funcflag)
    {
        case 8808:
//          cout<<"flag: "<<dec<<funcflag<<" address: "<<hex<<dest<<" length: "<<dec<<source<<endl;
            MallocVec.push_back((ShareAddreeStruct) {dest, (USIZE)source});
            sort(MallocVec.begin(), MallocVec.end(), less<ShareAddreeStruct>());
            break;
    }
    PIN_ReleaseLock(&shareaddrlock);
}

VOID BeforeCalloc(ADDRINT num, ADDRINT size, THREADID threadid)
{
    PIN_GetLock(&shareaddrlock, threadid+1);
    if(startmallocmonitor)
    {
        CallocSize.insert(make_pair(threadid, num*size));
    }
    PIN_ReleaseLock(&shareaddrlock);
}

VOID AfterCalloc(ADDRINT ret,THREADID threadid)
{
    PIN_GetLock(&shareaddrlock, threadid+1);
    if(startmallocmonitor)
    {
        map<UINT32, USIZE>::iterator callocfinder;
        callocfinder=CallocSize.find(threadid);
        if(callocfinder==CallocSize.end())
        {
            ErrorLog<<"Malloc error!\n";
            PIN_ReleaseLock(&shareaddrlock);
            return;
        }
        MallocVec.push_back((ShareAddreeStruct) {ret, callocfinder->second}); //store the calloc information to identify shared-memory locations
        sort(MallocVec.begin(), MallocVec.end(), less<ShareAddreeStruct>());
        CallocSize.erase(callocfinder);
    }
    PIN_ReleaseLock(&shareaddrlock);
}

/*VOID BeforeFree(ADDRINT address, THREADID threadid)
{
    PIN_GetLock(&shareaddrlock, threadid+1);
    vector<ShareAddreeStruct>::iterator p;
    for(p=MallocVec.begin(); p!=MallocVec.end(); p++)
    {
        if(address==p->address_name)
        {
            MallocVec.erase(p);
            break;
        }
    }
    PIN_ReleaseLock(&shareaddrlock);
}*/

VOID BeforeFree(ADDRINT address, THREADID threadid)
{
    PIN_GetLock(&shareaddrlock, threadid+1);
    vector<ShareAddreeStruct>::iterator p;
    set<ADDRINT>::iterator shareaddress_it;
    for(p=MallocVec.begin(); p!=MallocVec.end(); p++)
    {
        if(address==p->address_name)
        {
            for(shareaddress_it=ShareVarAddress.begin();shareaddress_it!=ShareVarAddress.end();shareaddress_it++)
            {
                if((*shareaddress_it>=p->address_name)&&(*shareaddress_it<(p->address_name+p->address_size)))
                {
                    ShareVarAddress.erase(shareaddress_it); //remove the old shared-memory locations in the free heap memory
                }
            }
            MallocVec.erase(p);
            break;
        }
    }
    
/*  map<ADDRINT, bool>::iterator mapforfreememory;
    map<UINT32, ThreadVecTime>::iterator ITforAllThread;
    ITforAllThread=AllThread.find(threadid);
    if (ITforAllThread!=AllThread.end())
    {
        ((ITforAllThread->second).ListAddress)->Freeflag=true;
        mapforfreememory=(((ITforAllThread->second).ListAddress)->FreeMemory).find(address);
        if(mapforfreememory!=(((ITforAllThread->second).ListAddress)->FreeMemory).end())
            (((ITforAllThread->second).ListAddress)->FreeMemory).insert(pair<ADDRINT, bool>(address,true));
    }*/
//  Testfile<<"Free thread id: "<<threadid<<" address: "<<hex<<address<<dec<<endl;
    
    PIN_ReleaseLock(&shareaddrlock);
}

VOID Beforemain()
{
    Imgfile<<"main"<<endl;
    startmallocmonitor=true;
}
