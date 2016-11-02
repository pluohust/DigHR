#include "Detector.h"
#include "Output.h"

extern map<UINT32, ThreadVecTime> AllThread;
extern ofstream DataRaceOut;
extern ofstream Imgfile;
extern ofstream ErrorLog;
extern int ThreadNum;
extern size_t MaxSum;
extern int Warnings;
extern UINT64 ConNumFrames;
extern UINT64 NumAnalysis;
extern set<ADDRINT> Race_address;

extern size_t MaxSum;
extern bool monitorendflag;

extern ofstream Testfile;
extern ofstream OutPutVectorTime;

bool CheckLock(const set<ADDRINT> &lockone, const set<ADDRINT> &locktwo) //check critical sections
{
    set<ADDRINT>::const_iterator setformain, setforanother;
    if(lockone.empty() || locktwo.empty())
        return true;
    for(setformain=lockone.begin(); setformain!=lockone.end(); setformain++)
    {
        setforanother=locktwo.find(*setformain);
        if(setforanother!=locktwo.end())
            return false;
    }
    return true;
}

void AddOneConFrame()
{
    ConNumFrames=ConNumFrames+1;
}

void AddOneAnalysis()
{
    NumAnalysis=NumAnalysis+1;
}

bool CompareVC(THREADID threadid, map<UINT32, long> &OldVC, map<UINT32, long> &NewVC)
{
    map<UINT32, long>::iterator OldItVC;
    map<UINT32, long>::iterator NewItVC;
    
    OldItVC=OldVC.find(threadid);
    NewItVC=NewVC.find(threadid);
    if((OldItVC!=OldVC.end())&&(NewItVC!=NewVC.end())&&(OldItVC->second<NewItVC->second))
    {
        return true;
    }
    else
    {
        return false;
    }
}

void DetectSharedMemory(const map<ADDRINT, SharedMemoryAccessInf> &MemoryOne, const map<ADDRINT, SharedMemoryAccessInf> &MemoryTwo, UINT32 threadIDOne, UINT32 threadIDTwo)
{
    map<ADDRINT, SharedMemoryAccessInf>::const_iterator MapforOne;
    map<ADDRINT, SharedMemoryAccessInf>::const_iterator MapforTwo;
    for(MapforOne=MemoryOne.begin();MapforOne!=MemoryOne.end();MapforOne++)
    {
        MapforTwo=MemoryTwo.find(MapforOne->first);
        if(MapforTwo!=MemoryTwo.end())
        {
            if((MapforOne->second).Wstatus && (MapforTwo->second).Wstatus)
            {
                DataRaceOut<<"+++++++++++++++++++++"<<endl<<"Thread: "<<threadIDOne<<" Thread: "<<threadIDTwo<<" Address: "<<hex<<MapforOne->first<<dec<<" W-W  "<<(MapforOne->second).Wcount<<"*"<<(MapforTwo->second).Wcount<<endl<<endl;
                Warnings++;
                Race_address.insert(MapforOne->first);
            }
            else if((MapforOne->second).Wstatus && (MapforTwo->second).Rstatus)
            {
                DataRaceOut<<"+++++++++++++++++++++"<<endl<<"Thread: "<<threadIDOne<<" Thread: "<<threadIDTwo<<" Address: "<<hex<<MapforOne->first<<dec<<" W-R  "<<(MapforOne->second).Wcount<<"*"<<(MapforTwo->second).Rcount<<endl<<endl;
                Warnings++;
                Race_address.insert(MapforOne->first);
            }
            else if((MapforOne->second).Rstatus && (MapforTwo->second).Wstatus)
            {
                DataRaceOut<<"+++++++++++++++++++++"<<endl<<"Thread: "<<threadIDOne<<" Thread: "<<threadIDTwo<<" Address: "<<hex<<MapforOne->first<<dec<<" R-W  "<<(MapforOne->second).Rcount<<"*"<<(MapforTwo->second).Wcount<<endl<<endl;
                Warnings++;
                Race_address.insert(MapforOne->first);
            }
        }
    }
}

/* each detection analysis for one thread */
void VectorDetect(THREADID threadID, ThreadVecTime &TargetThread)
{
    GetMaxMemory();
    Imgfile<<"Memory Size: "<<MaxSum<<endl;
    Imgfile<<"Thread Number: "<<ThreadNum<<endl;
    Imgfile<<"Warnings: "<<dec<<Warnings<<endl;
    Imgfile<<"Count of Analysis: "<<NumAnalysis<<endl;
    Imgfile<<"Concurrent Frames: "<<ConNumFrames<<endl;
    
    map<UINT32, ThreadVecTime>::iterator LoopItDetect;
    list<struct RWVecTime>::iterator MainItListDetect;
    list<struct RWVecTime>::iterator EndMainItListDetect;
    list<struct RWVecTime>::iterator AnotherItListBeDetected;
    
    if((TargetThread.VecTimeList).empty())
    {
        return;
    }
    
    EndMainItListDetect=TargetThread.BlockFrame;
    EndMainItListDetect--;
    
    for(LoopItDetect=AllThread.begin();LoopItDetect!=AllThread.end();LoopItDetect++)
    {
        if(LoopItDetect->first!=threadID)
        {
            for(MainItListDetect=TargetThread.ListAddress;MainItListDetect!=EndMainItListDetect;MainItListDetect--)
            {
                AnotherItListBeDetected=(LoopItDetect->second).BlockFrame;
                AnotherItListBeDetected--;
                for(;AnotherItListBeDetected!=((LoopItDetect->second).VecTimeList).end();AnotherItListBeDetected--)
                {
                    AddOneAnalysis();
                    if(CompareVC(LoopItDetect->first,AnotherItListBeDetected->VecTime,MainItListDetect->VecTime))
                    {
                        break;
                    }
                    if(CheckLock(MainItListDetect->LockAcquired,AnotherItListBeDetected->LockAcquired))
                    {
                        AddOneConFrame();
                        DetectSharedMemory(AnotherItListBeDetected->SharedLocation,MainItListDetect->SharedLocation,LoopItDetect->first,threadID);
                    }
                }
            }
        }
    }
}
