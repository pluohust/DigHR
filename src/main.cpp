#include "main_basictype.h"
#include "monitor_shared_memory.h"
#include "Detector.h"
#include "Output.h"
#include "VectorClocks.h"
#include "Init_Close.h"
#include "monitor_function.h"

using namespace std;

set<ADDRINT> ShareVarAddress; //Record the address
set<ADDRINT> SynAddress; //locks and conditional variables
map<THREADID, ThreadParent> ThreadMapParent; //Record the relationship between child and father
map<UINT32, USIZE> MallocSize, CallocSize; //Record the malloc and calloc information
map<OS_THREAD_ID, THREADID> ThreadIDInf; //Record the thread ID information
bool startmallocmonitor=false; //The flag to start malloc fuction and recod heap memory locations
PIN_LOCK lock, shareaddrlock;
map<THREADID, RecordCond> VecWait; //Record conditional variables
map<THREADID, ADDRINT> AfterLockInf; //Record lock
bool monitorendflag=false, realendflag=false; //the end flags

map<THREADID, ADDRINT> TryLockInf; //Record trylock information

map<UINT32, ThreadVecTime> AllThread; //The whole information
map<ADDRINT, map<UINT32, long> > SignalVecTime; //Condition vec time
map<ADDRINT, map<UINT32, long> > LockVecTime; //Lock vec time
map<ADDRINT, map<ADDRINT, SharedMemoryInf> > LockAllSharedMemoryRW; //Store the shared-memory access information in each lock
map<UINT32, queue<map<UINT32, long> > > CreateThreadVecTime; //Record the father vec time
map<UINT32, map<UINT32, long> > FiniThreadVecTime; //record the finish vec time

map<ADDRINT, LockDetailVCInf> NewLockInf; //Record the new lock
map<ADDRINT, LockDetailVCInf> OldLockInf; //record the old lock

ofstream Testfile; //output all information
ofstream OutPutVectorTime; //output vec time
ofstream Imgfile; //output the img access information
ofstream DataRaceOut; //output race
ofstream ErrorLog; //output error

int ThreadNum=0; //thread count
int Warnings=0;
UINT64 ConNumFrames=0; //the count of concurrent frames
UINT64 NumAnalysis=0; //the count of analysis
set<ADDRINT> Race_address; //store the race address
int WWNum=0; //W-W Frame count

vector<ShareAddreeStruct> MallocVec; //store Malloc address
size_t MaxSum=0; //memory used

int main(int argc, char * argv[])
{
    if (PIN_Init(argc, argv))
        return Usage();
    PIN_InitLock(&lock);
    PIN_InitLock(&shareaddrlock);
    PIN_InitSymbols();
    
    InitFileOutput();
    GetShareAddress();
    
    IMG_AddInstrumentFunction(ImageLoad, 0);
    INS_AddInstrumentFunction(Instruction, 0);
    PIN_AddFiniFunction(Fini, 0);

    PIN_AddThreadStartFunction(ThreadStart, 0);
    PIN_AddThreadFiniFunction(ThreadFini, 0);

    PIN_StartProgram();

    CloseFileOutput();
    return 0;
}
