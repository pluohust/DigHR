#include "VectorClocks.h"
#include "Init_Close.h"
#include "Output.h"
#include "Detector.h"

#define CHECK_R 2

/*please forgive my laziness.*/
extern map<UINT32, ThreadVecTime> AllThread;
extern map<THREADID, ThreadParent> ThreadMapParent;
extern map<UINT32, USIZE> MallocSize;
extern map<UINT32, USIZE> CallocSize;
extern map<OS_THREAD_ID, THREADID> ThreadIDInf;
extern PIN_LOCK lock;
extern PIN_LOCK shareaddrlock;
extern map<THREADID, RecordCond> VecWait;
extern map<THREADID, ADDRINT> AfterLockInf;
extern map<THREADID, ADDRINT> TryLockInf;
extern bool monitorendflag;
extern bool realendflag;
extern map<ADDRINT, map<UINT32, long> > SignalVecTime;
extern map<ADDRINT, map<UINT32, long> > LockVecTime;
extern map<ADDRINT, map<ADDRINT, SharedMemoryInf> > LockAllSharedMemoryRW;
extern map<UINT32, queue<map<UINT32, long> > > CreateThreadVecTime;
extern map<UINT32, map<UINT32, long> > FiniThreadVecTime;
extern map<ADDRINT, LockDetailVCInf> NewLockInf;
extern map<ADDRINT, LockDetailVCInf> OldLockInf;
extern ofstream Testfile;
extern ofstream OutPutVectorTime;
extern ofstream Imgfile;
extern ofstream DataRaceOut;
extern ofstream ErrorLog;
extern int ThreadNum;
extern vector<ShareAddreeStruct> MallocVec;
extern size_t MaxSum;
extern set<ADDRINT> SynAddress;
extern int WWNum;

void InitThreadVecTime(UINT32 threadID)  //first frame when thread start
{
    map<UINT32, ThreadVecTime>::iterator ITforallthread;
    RWVecTime tmpRWVecTime;
    (tmpRWVecTime.VecTime)[threadID]=1;
    tmpRWVecTime.SynName="FirstFrame";
    tmpRWVecTime.threadid=threadID;

    ThreadVecTime tmpThreadVecTime;
    tmpThreadVecTime.threadid=threadID;
    (tmpThreadVecTime.VecTimeList).push_back(tmpRWVecTime);

    AllThread.insert(pair<UINT32, ThreadVecTime>(threadID, tmpThreadVecTime));
    ITforallthread=AllThread.find(threadID);
    if(AllThread.end()==ITforallthread)
    {
        ErrorLog<<"Cannot find the thread "<<dec<<threadID<<endl;
        return;
    }
    
    (ITforallthread->second).ListAddress=((ITforallthread->second).VecTimeList).begin();
    (ITforallthread->second).BlockFrame=((ITforallthread->second).VecTimeList).begin(); //Remonber the pointer cannot intelligent replication
}

void CreateNewFrame(THREADID threadid, ThreadVecTime &TargetThread, string SynName)
{
    RWVecTime NewFrame;
    NewFrame.VecTime=(TargetThread.ListAddress)->VecTime;
    NewFrame.LockAcquired=TargetThread.LockAcquired;
    NewFrame.SynName=SynName;
    NewFrame.threadid=threadid;
    map<UINT32, long>::iterator mapvectime; //to find the thread's vec time
    mapvectime=(NewFrame.VecTime).find(threadid);
    if(mapvectime==(NewFrame.VecTime).end())
    {
        ErrorLog<<"CreateNewFrame: Cannot find the thread "<<dec<<threadid<<endl;
        exit(-1);
    }
    mapvectime->second+=1;
    (TargetThread.VecTimeList).push_back(NewFrame);
    
    TargetThread.ListAddress=(TargetThread.VecTimeList).end();
    (TargetThread.ListAddress)--;
    if(TargetThread.ListAddress==(TargetThread.VecTimeList).end())
    {
        ErrorLog<<"CreateNewFrame: ListAddress error!"<<endl;
        exit(-1);
    }
    list<struct RWVecTime>::iterator TestTheLastIterator;
    TestTheLastIterator=TargetThread.ListAddress;
    TestTheLastIterator++;
    if(TestTheLastIterator!=(TargetThread.VecTimeList).end())
    {
        ErrorLog<<"CreateNewFrame: Frame ListAddress error in thread: "<<dec<<threadid<<endl;
        exit(-1);
    }
    if((NewFrame.LockAcquired).empty())
    {
        TargetThread.BlockFrame=TargetThread.ListAddress;
    }
}

bool ComparedAcquiredLockVC(ADDRINT currentlock, const ThreadVecTime &TargetThread)
{
    map<ADDRINT, LockDetailVCInf>::iterator findlockinOld;
    findlockinOld=OldLockInf.find(currentlock);
    if(findlockinOld==OldLockInf.end())
    {
        return false;
    }
    else
    {
        map<UINT32, long>::iterator oldlockVC;
        map<UINT32, long>::iterator newlockVC;
        oldlockVC=(((findlockinOld->second).FirstFrame)->VecTime).find((findlockinOld->second).threadid);
        if(oldlockVC==(((findlockinOld->second).FirstFrame)->VecTime).end())
        {
            ErrorLog<<"ComparedAcquiredLockVC: Connot find the lock: "<<hex<<currentlock<<dec<<endl;
            exit(-1);
        }
        newlockVC=((TargetThread.ListAddress)->VecTime).find((findlockinOld->second).threadid);
        if(newlockVC==((TargetThread.ListAddress)->VecTime).end())
        {
            return false;
        }
        else if(oldlockVC->second < newlockVC->second)
        {
            return true;
        }
        else
        {
            return false;
        }
    }
}

void AcquiredLockUpdateVC(THREADID threadid, ADDRINT currentlock, ThreadVecTime &TargetThread)
{
    map<ADDRINT, LockDetailVCInf>::iterator findlockinNew;
    findlockinNew=NewLockInf.find(currentlock);
    if(findlockinNew==NewLockInf.end()) //第一次获取锁
    {
        LockDetailVCInf ANewLock;
        ANewLock.ComparedFlag=false;
        ANewLock.threadid=threadid;
        ANewLock.FirstFrame=TargetThread.ListAddress;
        ANewLock.LastFrame=TargetThread.ListAddress;
        NewLockInf[currentlock]=ANewLock;
    }
    else
    {
//        if(((findlockinNew->second).threadid==threadid)&&(((TargetThread.ListAddress)->LockAcquired).size()>1)&&CompareVC(threadid,(TargetThread.BlockFrame)->VecTime,((findlockinNew->second).FirstFrame)->VecTime)) //two criticl sections in the same block
        if((findlockinNew->second).threadid==threadid) //在同一个线程里获取锁
        {
            (findlockinNew->second).ComparedFlag=false;
            (findlockinNew->second).threadid=threadid; //Update the new lock information
            (findlockinNew->second).FirstFrame=TargetThread.ListAddress;
            (findlockinNew->second).LastFrame=TargetThread.ListAddress;
            ((findlockinNew->second).SharedMemoryAccess).clear();
            return;
        }
        OldLockInf[currentlock]=findlockinNew->second; //将锁信息更新到旧的里面，下面都是新的锁信息

        (findlockinNew->second).ComparedFlag=false;
        (findlockinNew->second).threadid=threadid; //Update the new lock information
        (findlockinNew->second).FirstFrame=TargetThread.ListAddress;
        (findlockinNew->second).LastFrame=TargetThread.ListAddress;
        ((findlockinNew->second).SharedMemoryAccess).clear();
        
        map<ADDRINT, LockDetailVCInf>::iterator findlockinOld;
        map<UINT32, long>::iterator OldLockVC;
        map<UINT32, long>::iterator NewLockVC;
        findlockinOld=OldLockInf.find(currentlock);
        if(findlockinOld==OldLockInf.end()) //同一个线程，或者第一次
        {
            ErrorLog<<"Connot find the old lock: "<<hex<<currentlock<<dec<<endl;
            exit(-1);
        }
        if((TargetThread.LockAcquired).size()>1) //有嵌套锁，之后需要回溯，所以需要记录
        {
            ((TargetThread.ListAddress)->NestedLock_RecordOldLock)[currentlock]=findlockinOld->second;
        }
        if(ComparedAcquiredLockVC(currentlock, TargetThread))
        {
            for(OldLockVC=(((findlockinOld->second).LastFrame)->VecTime).begin();OldLockVC!=(((findlockinOld->second).LastFrame)->VecTime).end();OldLockVC++)
            {
                NewLockVC=((TargetThread.ListAddress)->VecTime).find(OldLockVC->first);
                if(NewLockVC==((TargetThread.ListAddress)->VecTime).end())
                {
                    if((findlockinOld->second).threadid==(OldLockVC->first))
                        ((TargetThread.ListAddress)->VecTime)[OldLockVC->first]=OldLockVC->second+1;
                    else
                        ((TargetThread.ListAddress)->VecTime)[OldLockVC->first]=OldLockVC->second;
                }
                else
                {
                    if((findlockinOld->second).threadid==(OldLockVC->first))
                    {
                        NewLockVC->second=(OldLockVC->second+1) > NewLockVC->second ? (OldLockVC->second+1) : NewLockVC->second;
                    }
                    else
                    {
                        NewLockVC->second=OldLockVC->second > NewLockVC->second ? OldLockVC->second : NewLockVC->second;
                    }
                }
            }
        
            (findlockinNew->second).ComparedFlag=true;
        }
    }
}

void PerdectNewLockAfterrelease(ADDRINT currentlock, ThreadVecTime &TargetThread)
{
    map<ADDRINT, LockDetailVCInf>::iterator findlockinNew;
    list<struct RWVecTime>::iterator LoopFrame;
    list<struct RWVecTime>::iterator EndFrame;
    findlockinNew=NewLockInf.find(currentlock);
    if(findlockinNew==NewLockInf.end())
    {
        ErrorLog<<"ReleasedLockUpdateVC: Connot find the lock: "<<hex<<currentlock<<dec<<endl;
        exit(-1);
    }
    (findlockinNew->second).LastFrame=TargetThread.ListAddress;
    EndFrame=(TargetThread.VecTimeList).end();

//验证
    if(((findlockinNew->second).FirstFrame)->threadid != TargetThread.threadid)
    {
        ErrorLog<<"ReleasedLockUpdateVC: the lock() and unlock() are not in the same thread, the lock is "<<hex<<currentlock<<dec<<endl;
        exit(-1);
    }

    for(LoopFrame=(findlockinNew->second).FirstFrame;LoopFrame!=EndFrame;LoopFrame++)
    {
//      cout<<LoopFrame->SynName<<endl;
        map<ADDRINT, SharedMemoryAccessInf>::iterator LoopEachSharedLocation;
        WriteAccessInf TmpSharedLocationAccess;
        for(LoopEachSharedLocation=(LoopFrame->SharedLocation).begin();LoopEachSharedLocation!=(LoopFrame->SharedLocation).end();LoopEachSharedLocation++)
        {
            TmpSharedLocationAccess.Read=false; //初始化
            TmpSharedLocationAccess.Write=false;
        
            map<ADDRINT, WriteAccessInf>::iterator FindSharedLocationInLockInf;
            FindSharedLocationInLockInf=((findlockinNew->second).SharedMemoryAccess).find(LoopEachSharedLocation->first);
            if(FindSharedLocationInLockInf==((findlockinNew->second).SharedMemoryAccess).end())
            {
                TmpSharedLocationAccess.Access=true;
//              cout<<"1"<<endl;
                try
                {
                    if((LoopEachSharedLocation->second).Wstatus)
                    {
                        TmpSharedLocationAccess.Write=true;
                    }
                    else
                    {
                        TmpSharedLocationAccess.Write=false;
                    }
                }
                catch(...)
                {
                    TmpSharedLocationAccess.Write=true;
                }
                try
                {
                    if((LoopEachSharedLocation->second).Rstatus)
                    {
                        TmpSharedLocationAccess.Read=true;
                    }
                    else
                    {
                        TmpSharedLocationAccess.Read=false;
                    }
                }
                catch(...)
                {
                    TmpSharedLocationAccess.Read=true;
                }
//              cout<<"2"<<endl;
                ((findlockinNew->second).SharedMemoryAccess)[LoopEachSharedLocation->first]=TmpSharedLocationAccess;
//              cout<<"3"<<endl;
            }
            else
            {
                if((LoopEachSharedLocation->second).Wstatus)
                    (FindSharedLocationInLockInf->second).Write=true;
                if((LoopEachSharedLocation->second).Rstatus)
                    (FindSharedLocationInLockInf->second).Read=true;
            }
        }
    }
}

bool IsAWriteARead(map<ADDRINT, WriteAccessInf> &AccessFrameOne, map<ADDRINT, WriteAccessInf> &AccessFrameTwo)
{
    map<ADDRINT, WriteAccessInf>::iterator ItOne;
    map<ADDRINT, WriteAccessInf>::iterator ItTwo;
    
    for(ItOne=AccessFrameOne.begin();ItOne!=AccessFrameOne.end();ItOne++)
    {
        ItTwo=AccessFrameTwo.find(ItOne->first);
        if(ItTwo!=AccessFrameTwo.end())
        {
            if(((ItOne->second).Write&&(ItTwo->second).Read)||((ItOne->second).Read&&(ItTwo->second).Write))
            {
                return true;
            }
        }
    }
    return false;
}

bool IsTwoWrite(map<ADDRINT, WriteAccessInf> &AccessFrameOne, map<ADDRINT, WriteAccessInf> &AccessFrameTwo)
{
    map<ADDRINT, WriteAccessInf>::iterator ItOne;
    map<ADDRINT, WriteAccessInf>::iterator ItTwo;
    
    for(ItOne=AccessFrameOne.begin();ItOne!=AccessFrameOne.end();ItOne++)
    {
        ItTwo=AccessFrameTwo.find(ItOne->first);
        if(ItTwo!=AccessFrameTwo.end())
        {
            if((ItOne->second).Write&&(ItTwo->second).Write)
            {
                return true;
            }
        }
    }
    return false;
}

void UpdateFrameVC_basedLock_NoThread(map<UINT32, long> &SourceVC, list<struct RWVecTime>::iterator &FirstFrame, list<struct RWVecTime>::iterator &LastFrame)
{
    list<struct RWVecTime>::iterator LoopItFrame;
    list<struct RWVecTime>::iterator EndItFrame;
    map<UINT32, long>::iterator SourceItVC;
    map<UINT32, long>::iterator TargetItVC;
    
    EndItFrame=LastFrame;
    EndItFrame++;
    
    for(SourceItVC=SourceVC.begin();SourceItVC!=SourceVC.end();SourceItVC++)
    {
        for(LoopItFrame=FirstFrame;LoopItFrame!=EndItFrame;LoopItFrame++)
        {
            TargetItVC=(LoopItFrame->VecTime).find(SourceItVC->first);
            if(TargetItVC==(LoopItFrame->VecTime).end())
            {
                (LoopItFrame->VecTime)[SourceItVC->first]=SourceItVC->second;
            }
            else
            {
                TargetItVC->second=SourceItVC->second > TargetItVC->second ? SourceItVC->second : TargetItVC->second;
            }
        }
    }
}

void UpdateFrameVC_basedLock(THREADID threadid, map<UINT32, long> &SourceVC, list<struct RWVecTime>::iterator &FirstFrame, list<struct RWVecTime>::iterator &LastFrame)
{
    list<struct RWVecTime>::iterator LoopItFrame;
    list<struct RWVecTime>::iterator EndItFrame;
    map<UINT32, long>::iterator SourceItVC;
    map<UINT32, long>::iterator TargetItVC;
    
    EndItFrame=LastFrame;
    EndItFrame++;
    
    for(SourceItVC=SourceVC.begin();SourceItVC!=SourceVC.end();SourceItVC++)
    {
        for(LoopItFrame=FirstFrame;LoopItFrame!=EndItFrame;LoopItFrame++)
        {
            TargetItVC=(LoopItFrame->VecTime).find(SourceItVC->first);
            if(TargetItVC==(LoopItFrame->VecTime).end())
            {
                if(SourceItVC->first==threadid)
                {
                    (LoopItFrame->VecTime)[SourceItVC->first]=SourceItVC->second+1;
                }
                else
                {
                    (LoopItFrame->VecTime)[SourceItVC->first]=SourceItVC->second;
                }
            }
            else
            {
                if(SourceItVC->first==threadid)
                {
                    TargetItVC->second=(SourceItVC->second+1) > TargetItVC->second ? (SourceItVC->second+1) : TargetItVC->second;
                }
                else
                {
                    TargetItVC->second=SourceItVC->second > TargetItVC->second ? SourceItVC->second : TargetItVC->second;
                }
            }
        }
    }
}

void UpdateNestedLockInf(THREADID threadid, list<struct RWVecTime>::iterator &FirstFrame, list<struct RWVecTime>::iterator &LastFrame)
{
    list<struct RWVecTime>::iterator LoopItFrame;
    list<struct RWVecTime>::iterator EndItFrame;
    list<struct RWVecTime>::iterator NextItFrame;
    
    EndItFrame=LastFrame;
    EndItFrame++;
    for(LoopItFrame=FirstFrame;LoopItFrame!=EndItFrame;LoopItFrame++)
    {
        if((LoopItFrame->NestedLock_RecordOldLock).size()>0)
        {
            map<ADDRINT, LockDetailVCInf>::iterator NestedLock;
            NestedLock=(LoopItFrame->NestedLock_RecordOldLock).begin();
            if(CompareVC(threadid,((NestedLock->second).FirstFrame)->VecTime,LoopItFrame->VecTime))
            {
                UpdateFrameVC_basedLock(threadid,((NestedLock->second).LastFrame)->VecTime,LoopItFrame,LoopItFrame);
            }
        }
        NextItFrame=LoopItFrame;
        NextItFrame++;
        if(NextItFrame!=EndItFrame)
        {
            UpdateFrameVC_basedLock_NoThread(LoopItFrame->VecTime,NextItFrame,NextItFrame);
        }
    }
}

void StoreWAddress(set<ADDRINT> &W_address, map<ADDRINT, WriteAccessInf> &AccessFrameOne, map<ADDRINT, WriteAccessInf> &AccessFrameTwo)
{
    map<ADDRINT, WriteAccessInf>::iterator ItOne;
    map<ADDRINT, WriteAccessInf>::iterator ItTwo;
    
    for(ItOne=AccessFrameOne.begin();ItOne!=AccessFrameOne.end();ItOne++)
    {
        ItTwo=AccessFrameTwo.find(ItOne->first);
        if(ItTwo!=AccessFrameTwo.end())
        {
            if((ItOne->second).Write&&(ItTwo->second).Write)
            {
                W_address.insert(ItOne->first);
            }
        }
    }
}

void StoreWWInf(ADDRINT currentlock, ThreadVecTime &TargetThread, LockDetailVCInf &NewLock, LockDetailVCInf &OldLock) //存储W-W情况信息
{
    struct WWRecord WW_Inf_tmp;
    WW_Inf_tmp.lock=currentlock;
    StoreWAddress(WW_Inf_tmp.W_address, NewLock.SharedMemoryAccess, OldLock.SharedMemoryAccess);
    WW_Inf_tmp.Unlock_Frame=OldLock.LastFrame;
    WW_Inf_tmp.Lock_Frame=NewLock.FirstFrame;
    WW_Inf_tmp.Last_Frame=NewLock.LastFrame;
    
    (TargetThread.WW_Inf).push_back(WW_Inf_tmp);
}

void UpdateLockInf_ifNested(THREADID threadid, ADDRINT currentlock, ThreadVecTime &TargetThread)
{
    map<ADDRINT, LockDetailVCInf>::iterator findlockinOld;
    findlockinOld=OldLockInf.find(currentlock);
    map<ADDRINT, LockDetailVCInf>::iterator findlockinNew;
    findlockinNew=NewLockInf.find(currentlock);
    if(findlockinNew==NewLockInf.end())
    {
        ErrorLog<<"ReleasedLockUpdateVC: Connot find the lock: "<<hex<<currentlock<<dec<<endl;
        exit(-1);
    }
    if(findlockinOld==OldLockInf.end())
    {
        return;
    }
    else
    {
        if((findlockinOld->second).threadid==threadid)
        {
/*          map<UINT32, long>::iterator blockVC;
            map<UINT32, long>::iterator oldlockVC;
            blockVC=((TargetThread.BlockFrame)->VecTime).find(threadid);
            if(blockVC==((TargetThread.BlockFrame)->VecTime).end())
            {
                ErrorLog<<"ReleasedLockUpdateVC1: Connot find the thread: "<<dec<<threadid<<endl;
                exit(-1);
            }
            oldlockVC=(((findlockinOld->second).FirstFrame)->VecTime).find(threadid);
            if(oldlockVC==(((findlockinOld->second).FirstFrame)->VecTime).end())
            {
                ErrorLog<<"ReleasedLockUpdateVC2: Connot find the thread: "<<dec<<threadid<<endl;
                exit(-1);
            }
            if(blockVC->second < oldlockVC->second)
            {
                (findlockinOld->second).LastFrame=(findlockinNew->second).LastFrame;
                map<ADDRINT, WriteAccessInf>::iterator AccessinOld;
                map<ADDRINT, WriteAccessInf>::iterator AccessinNew;
                for(AccessinNew=((findlockinOld->second).SharedMemoryAccess).begin();AccessinNew!=((findlockinOld->second).SharedMemoryAccess).end();AccessinNew++)
                {
                    AccessinOld=((findlockinOld->second).SharedMemoryAccess).find(AccessinNew->first);
                    if(AccessinOld==((findlockinOld->second).SharedMemoryAccess).end())
                    {
                        ((findlockinOld->second).SharedMemoryAccess)[AccessinNew->first]=AccessinNew->second;
                    }
                    else
                    {
                        if((AccessinNew->second).Write)
                        {
                            (AccessinOld->second).Write=true;
                        }
                    }
                }
                return true;
            }
            else
            {
                OldLockInf[currentlock]=findlockinNew->second;
                return false;
            }*/
            return;
        }
        else
        {
            if(!(findlockinNew->second).ComparedFlag) //两个critical sections暂时没有因果关系
            {
                if(IsAWriteARead((findlockinNew->second).SharedMemoryAccess,(findlockinOld->second).SharedMemoryAccess))
                {
                    UpdateFrameVC_basedLock((findlockinOld->second).threadid,((findlockinOld->second).LastFrame)->VecTime,(findlockinNew->second).FirstFrame,(findlockinNew->second).LastFrame);
                    UpdateNestedLockInf((findlockinOld->second).threadid,(findlockinNew->second).FirstFrame,(findlockinNew->second).LastFrame);
                }
                else if(IsTwoWrite((findlockinNew->second).SharedMemoryAccess,(findlockinOld->second).SharedMemoryAccess)) //两个W-W，做好记录
                {
                    WWNum++;
                    StoreWWInf(currentlock, TargetThread, findlockinNew->second, findlockinOld->second);
                }
            }
        }
    }
}

void ReleasedLockUpdateVC(THREADID threadid, ADDRINT currentlock, ThreadVecTime &TargetThread)
{
    PerdectNewLockAfterrelease(currentlock,TargetThread);

    UpdateLockInf_ifNested(threadid,currentlock,TargetThread);
}

bool Judege_R_In(set<ADDRINT> &W_address, map<ADDRINT, SharedMemoryAccessInf> &SharedLocation)
{
    set<ADDRINT>::iterator It_set;
    map<ADDRINT, SharedMemoryAccessInf>::iterator It_map;
    
    for(It_set=W_address.begin(); It_set!=W_address.end() ;It_set++)
    {
        It_map=SharedLocation.find(*It_set);
        if((It_map!=SharedLocation.end()) && (It_map->second).Rstatus)
        {
            return true;
        }
    }
    return false;
}

bool Judge_WFrame_VC(THREADID threadid, map<UINT32, long> &oldtime, map<UINT32, long> &lasttime)
{
    map<UINT32, long>::iterator old, last;
    
    old=oldtime.find(threadid);
    last=lasttime.find(threadid);
    
    if((old==oldtime.end()) || (last==lasttime.end()))
    {
        ErrorLog<<"Judge_WFrame_VC: VC error!"<<endl;
        return true;
    }
    if((last->second - old->second)>CHECK_R)
    {
        return true;
    }
    else
    {
        return false;
    }
}

void WW_R_UpdateVC(ThreadVecTime &TargetThread)
{
    list<struct WWRecord>::iterator EachWWInf;
    bool frame_loop=true;

    for(EachWWInf=(TargetThread.WW_Inf).begin(); EachWWInf!=(TargetThread.WW_Inf).end();)
    {
        if((EachWWInf->Last_Frame)==(TargetThread.ListAddress)) //都指向最后一个Frame
        {
            return;
        }
        if(Judege_R_In(EachWWInf->W_address, (TargetThread.ListAddress)->SharedLocation)) //后面的Frame有对应的R
        {
            if((EachWWInf->Lock_Frame)->threadid != TargetThread.threadid) //测试
            {
                ErrorLog<<"WW_R_UpdateVC: they are not in the same thread!"<<endl;
                exit(-1);
            }
            UpdateFrameVC_basedLock((EachWWInf->Unlock_Frame)->threadid, (EachWWInf->Unlock_Frame)->VecTime, EachWWInf->Lock_Frame, TargetThread.ListAddress); //更新后面Frame的VC
            UpdateNestedLockInf(TargetThread.threadid, EachWWInf->Lock_Frame, TargetThread.ListAddress); //处理后面嵌套锁问题
            EachWWInf=(TargetThread.WW_Inf).erase(EachWWInf);
        }
        else if(frame_loop && Judge_WFrame_VC(TargetThread.threadid, (EachWWInf->Lock_Frame)->VecTime, (TargetThread.ListAddress)->VecTime)) //间隔距离
        {
            if((EachWWInf->Lock_Frame)->threadid != TargetThread.threadid) //测试
            {
                ErrorLog<<"WW_R_UpdateVC: they are not in the same thread!"<<endl;
                exit(-1);
            }
            UpdateFrameVC_basedLock(TargetThread.threadid, (EachWWInf->Unlock_Frame)->VecTime, EachWWInf->Lock_Frame, TargetThread.ListAddress); //更新后面Frame的VC
            UpdateNestedLockInf(TargetThread.threadid, EachWWInf->Lock_Frame, TargetThread.ListAddress); //处理后面嵌套锁问题
            EachWWInf=(TargetThread.WW_Inf).erase(EachWWInf);
        }
        else
        {
            EachWWInf++;
            frame_loop=false;
        }
    }
}

VOID ThreadStart(THREADID threadid, CONTEXT *ctxt, INT32 flags, VOID *v)
{
    RecordType GetOne;
    OS_THREAD_ID tmp;
    map<OS_THREAD_ID, THREADID>::iterator maptmp;
    struct ThreadVecTime OneThreadVecInf;
    set<ADDRINT>::iterator ITSharedVar;
    PIN_GetLock(&lock, threadid+1);
    ThreadNum++; //thread count
//  cout<<"Thread Number: "<<ThreadNum<<endl;
    if(INVALID_OS_THREAD_ID!=(tmp=PIN_GetParentTid()))
    {
        maptmp=ThreadIDInf.find(tmp);
        if(maptmp!=ThreadIDInf.end())
        {
            GetOne=(RecordType){1, threadid, (ADDRINT)maptmp->second};
            Testfile<<"1"<<" "<<threadid<<" "<<hex<<(ADDRINT)maptmp->second<<dec<<endl;
            OutPutVectorTime<<"1"<<" "<<threadid<<" "<<hex<<(ADDRINT)maptmp->second<<dec<<endl;
            ThreadMapParent[threadid]=(ThreadParent) {true, maptmp->second}; //Just for thread join
        }
        else
        {
            GetOne=(RecordType){1, threadid, 0};
            Testfile<<"1"<<" "<<threadid<<" "<<hex<<"0"<<dec<<endl;
            OutPutVectorTime<<"1"<<" "<<threadid<<" "<<hex<<"0"<<dec<<endl;
        }
    }
    else
    {
        GetOne=(RecordType){1, threadid, 0};
        Testfile<<"1"<<" "<<threadid<<" "<<hex<<"0"<<dec<<endl;
        OutPutVectorTime<<"1"<<" "<<threadid<<" "<<hex<<"0"<<dec<<endl;
//      ErrorLog<<"1"<<" "<<threadid<<" "<<hex<<"0"<<dec<<endl;
    }
    ThreadIDInf.insert(make_pair(PIN_GetTid(), threadid));
    
    map<UINT32, ThreadVecTime>::iterator ITforAllThread;
    if(AllThread.empty()) //first thread
    {
        InitThreadVecTime(GetOne.threadID);
    }
    else
    {
        list<struct RWVecTime>::iterator ITtmplist;
        for(ITforAllThread=AllThread.begin();ITforAllThread!=AllThread.end();ITforAllThread++)
        {
            for(ITtmplist=((ITforAllThread->second).VecTimeList).begin();ITtmplist!=((ITforAllThread->second).VecTimeList).end();ITtmplist++)
                (ITtmplist->VecTime).insert(pair<UINT32, long>(GetOne.threadID,0));
        }
        InitThreadVecTime(GetOne.threadID);
        ITforAllThread=AllThread.find(GetOne.threadID);
        if(AllThread.end()==ITforAllThread)
        {
            ErrorLog<<"Cannot find the thread "<<dec<<threadid<<endl;
            PIN_ReleaseLock(&lock);
            return;
        }
        map<UINT32, queue<map<UINT32, long> > >::iterator mapfindfather;
        mapfindfather=CreateThreadVecTime.find((UINT32)GetOne.object);
        if(mapfindfather!=CreateThreadVecTime.end())
        {
            ((ITforAllThread->second).ListAddress)->VecTime=(mapfindfather->second).front(); //从父线程中继承vec time
            (mapfindfather->second).pop();
            (((ITforAllThread->second).ListAddress)->VecTime).insert(pair<UINT32, long>(GetOne.threadID,1));
        }
        else
        {
            ITforAllThread=AllThread.find(GetOne.threadID);
            if(AllThread.end()==ITforAllThread)
            {
                ErrorLog<<"Cannot find the thread "<<dec<<threadid<<endl;
                PIN_ReleaseLock(&lock);
                return;
            }
            (((ITforAllThread->second).ListAddress)->VecTime).insert(pair<UINT32, long>(GetOne.threadID,1));
        }
    }
    OutPutAllVecorTime("ThreadStart");
    
    PIN_ReleaseLock(&lock);
}

VOID ThreadFini(THREADID threadid, const CONTEXT *ctxt, INT32 code, VOID *v)
{
    PIN_GetLock(&lock, threadid+1);

    RecordType GetOne;
    GetOne=(RecordType) {2, threadid, 0};
    Testfile<<"2"<<" "<<threadid<<" "<<hex<<"0"<<dec<<endl;
    OutPutVectorTime<<"2"<<" "<<threadid<<" "<<hex<<"0"<<dec<<endl;

    map<THREADID, ThreadParent>::iterator itertmp;
    itertmp=ThreadMapParent.find(threadid);
    if((0!=threadid)&&(itertmp==ThreadMapParent.end()))
    {
        ErrorLog<<"ThreadFini1: Cannot find the thread "<<dec<<threadid<<endl;
        PIN_ReleaseLock(&lock);
        return;
    }
    else
    {
        (itertmp->second).liveflag=false;
    }

    map<UINT32, ThreadVecTime>::iterator ITforAllThread;
    ITforAllThread=AllThread.find(GetOne.threadID);
    if(AllThread.end()==ITforAllThread)
    {
        ErrorLog<<"ThreadFini2: Cannot find the thread "<<dec<<threadid<<endl;
        PIN_ReleaseLock(&lock);
        return;
    }
    
    VectorDetect(GetOne.threadID,ITforAllThread->second);
//    WW_R_UpdateVC(ITforAllThread->second);

    CreateNewFrame(threadid,ITforAllThread->second,"ThreadFini");

    FiniThreadVecTime[GetOne.threadID]=((ITforAllThread->second).ListAddress)->VecTime;

    OutPutAllVecorTime("ThreadFini");

    PIN_ReleaseLock(&lock);
}

VOID BeforePthread_create(THREADID threadid)
{
    PIN_GetLock(&lock, threadid+1);

    RecordType GetOne;
    GetOne=(RecordType) {13, threadid, 0};
    Testfile<<"13"<<" "<<threadid<<" "<<hex<<"0"<<dec<<endl;
    OutPutVectorTime<<"13"<<" "<<threadid<<" "<<hex<<"0"<<dec<<endl;

    map<UINT32, ThreadVecTime>::iterator ITforAllThread;
    ITforAllThread=AllThread.find(GetOne.threadID);
    if(AllThread.end()==ITforAllThread)
    {
        ErrorLog<<"BeforePthread_create: Cannot find the thread "<<dec<<threadid<<endl;
        PIN_ReleaseLock(&lock);
        return;
    }
    
    if(((ITforAllThread->second).LockAcquired).empty())
    {
        VectorDetect(GetOne.threadID,ITforAllThread->second);
        WW_R_UpdateVC(ITforAllThread->second);
    }

    CreateNewFrame(threadid,ITforAllThread->second,"ThreadCreate");

    map<UINT32, queue<map<UINT32, long> > >::iterator mapforqueue; //to record the father thread's inf
    mapforqueue=CreateThreadVecTime.find(GetOne.threadID);
    if(mapforqueue==CreateThreadVecTime.end()) //the first time to create child thread
    {
        queue<map<UINT32, long> > queuetmp;
        queuetmp.push(((ITforAllThread->second).ListAddress)->VecTime);
        CreateThreadVecTime.insert(pair<UINT32, queue<map<UINT32, long> > >(GetOne.threadID, queuetmp));
    }
    else
    {
        (mapforqueue->second).push(((ITforAllThread->second).ListAddress)->VecTime);
    }

    OutPutAllVecorTime("ThreadCreate");

    PIN_ReleaseLock(&lock);
}

VOID AfterPthread_join(THREADID threadid)
{
    PIN_GetLock(&lock, threadid+1);

    RecordType GetOne;
    map<THREADID, ThreadParent>::iterator itertmp;
    for(itertmp=ThreadMapParent.begin();itertmp!=ThreadMapParent.end();itertmp++)
    {
        if((!(itertmp->second).liveflag)&&((itertmp->second).fatherthreadid==threadid))
            break;
    }
    if(itertmp!=ThreadMapParent.end())
    {
        GetOne=(RecordType) {3, threadid, (ADDRINT)(itertmp->first)};
        Testfile<<"3"<<" "<<threadid<<" "<<hex<<(ADDRINT)(itertmp->first)<<dec<<endl;
        OutPutVectorTime<<"3"<<" "<<threadid<<" "<<hex<<(ADDRINT)(itertmp->first)<<dec<<endl;
        
        ThreadMapParent.erase(itertmp);
    }
    else
    {
        GetOne=(RecordType) {3, threadid, 0};
        Testfile<<"3"<<" "<<threadid<<" "<<hex<<"0"<<dec<<endl;
        OutPutVectorTime<<"3"<<" "<<threadid<<" "<<hex<<"0"<<dec<<endl;
    }

    map<UINT32, ThreadVecTime>::iterator ITforAllThread;
    ITforAllThread=AllThread.find(GetOne.threadID);
    if(AllThread.end()==ITforAllThread)
    {
        ErrorLog<<"AfterPthread_join1: Cannot find the thread "<<dec<<threadid<<endl;
        PIN_ReleaseLock(&lock);
        return;
    }

    if(((ITforAllThread->second).LockAcquired).empty())
    {
        VectorDetect(GetOne.threadID,ITforAllThread->second);
        WW_R_UpdateVC(ITforAllThread->second);
    }

    CreateNewFrame(threadid,ITforAllThread->second,"ThreadJoin");

    map<UINT32, long>::iterator mapvectimemain; //to find the thread's vec time
    map<UINT32, long>::iterator mapvectimechild; //to find child's vec time
    map<UINT32, ThreadVecTime>::iterator mapfini; //to find fini
    mapfini=AllThread.find((THREADID)(GetOne.object));
    if(mapfini==AllThread.end())
    {
        ErrorLog<<"AfterPthread_join2: Cannot find the thread "<<dec<<threadid<<endl;
        exit(-1);
    }
    for(mapvectimechild=(((mapfini->second).ListAddress)->VecTime).begin();mapvectimechild!=(((mapfini->second).ListAddress)->VecTime).end();mapvectimechild++)
    {
        mapvectimemain=(((ITforAllThread->second).ListAddress)->VecTime).find(mapvectimechild->first);
        if(mapvectimemain==(((ITforAllThread->second).ListAddress)->VecTime).end())
        {
            (((ITforAllThread->second).ListAddress)->VecTime)[mapvectimechild->first]=mapvectimechild->second+1;
        }
        else
        {
            mapvectimemain->second=(mapvectimechild->second+1) > mapvectimemain->second ? (mapvectimechild->second+1) : mapvectimemain->second;
        }
    }
    FiniThreadVecTime.erase((THREADID)(GetOne.object));

    OutPutAllVecorTime("ThreadJoin");

    PIN_ReleaseLock(&lock);
}

void JustBeforeLock(ADDRINT currentlock, THREADID threadid)
{
    RecordType GetOne;
    GetOne=(RecordType) {4, threadid, currentlock};
    Testfile<<"4"<<" "<<threadid<<" "<<hex<<currentlock<<dec<<endl;
    
    map<UINT32, ThreadVecTime>::iterator ITforAllThread;
    ITforAllThread=AllThread.find(GetOne.threadID);
    if(AllThread.end()==ITforAllThread)
    {
        ErrorLog<<"Cannot find the thread "<<dec<<threadid<<endl;
        PIN_ReleaseLock(&lock);
        return;
    }

    ((ITforAllThread->second).LockAcquired).insert(currentlock);
    if(!((((ITforAllThread->second).VecTimeList).empty()))&&((((ITforAllThread->second).ListAddress)->LockAcquired).empty()))
    {
        VectorDetect(GetOne.threadID,ITforAllThread->second);
        WW_R_UpdateVC(ITforAllThread->second);
    }
    CreateNewFrame(threadid,ITforAllThread->second,"MutexLock");
    AcquiredLockUpdateVC(threadid,currentlock,ITforAllThread->second);
    if(((ITforAllThread->second).LockAcquired).size()==1)
    {
        (ITforAllThread->second).BlockFrame=(ITforAllThread->second).ListAddress;
    }
    
    OutPutAllVecorTime("Lock");
}

VOID BeforePthread_mutex_lock(ADDRINT currentlock, THREADID threadid)
{
    PIN_GetLock(&lock, threadid+1);
    SynAddress.insert(currentlock); //delete the shared-memory with lock
    

    AfterLockInf[threadid]=currentlock;

    PIN_ReleaseLock(&lock);
}

VOID AfterPthread_mutex_lock(THREADID threadid)
{
    PIN_GetLock(&lock, threadid+1);

    map<THREADID, ADDRINT>::iterator findlock;
    findlock=AfterLockInf.find(threadid);
    if(findlock==AfterLockInf.end())
    {
        ErrorLog<<"AfterPthread_mutex_lock: Cannot find the thread "<<dec<<threadid<<endl;
        exit(-1);
    }

    JustBeforeLock(findlock->second, threadid);

    PIN_ReleaseLock(&lock);
}

VOID BeforePthread_mutex_trylock(ADDRINT currentlock, THREADID threadid)
{
    PIN_GetLock(&lock, threadid+1);
    SynAddress.insert(currentlock); //delete the shared-memory with lock
    

    TryLockInf[threadid]=currentlock;

    PIN_ReleaseLock(&lock);
}

VOID AfterPthread_mutex_trylock(int flag, THREADID threadid)
{
    PIN_GetLock(&lock, threadid+1);
    if(0==flag)
    {
        map<THREADID, ADDRINT>::iterator findlock;
        findlock=TryLockInf.find(threadid);
        if(findlock==TryLockInf.end())
        {
            ErrorLog<<"AfterPthread_mutex_lock: Cannot find the thread "<<dec<<threadid<<endl;
            exit(-1);
        }

        JustBeforeLock(findlock->second, threadid);
    }
    PIN_ReleaseLock(&lock);
}

/*please focus on the nesting cases*/
VOID BeforePthread_mutex_unlock(ADDRINT currentlock, THREADID threadid)
{
    PIN_GetLock(&lock, threadid+1);
    RecordType GetOne;
    GetOne=(RecordType) {5, threadid, currentlock};
    
    map<UINT32, ThreadVecTime>::iterator ITforAllThread;
    ITforAllThread=AllThread.find(GetOne.threadID);
    if(AllThread.end()==ITforAllThread)
    {
        ErrorLog<<"Cannot find the thread "<<dec<<threadid<<endl;
        PIN_ReleaseLock(&lock);
        return;
    }

    set<ADDRINT>::iterator find_lock;
    find_lock=((ITforAllThread->second).LockAcquired).find(currentlock);
    if(find_lock==((ITforAllThread->second).LockAcquired).end())
    {
/*      SynAddress.insert(currentlock);
        AfterLockInf[threadid]=currentlock;
        map<THREADID, ADDRINT>::iterator findlock;
        findlock=AfterLockInf.find(threadid);
        if(findlock==AfterLockInf.end())
        {
            ErrorLog<<"AfterPthread_mutex_lock: Cannot find the thread "<<dec<<threadid<<endl;
            exit(-1);
        }
        JustBeforeLock(findlock->second, threadid);*/
        PIN_ReleaseLock(&lock);
        return;
    }
    else
    {
        ((ITforAllThread->second).LockAcquired).erase(currentlock);
    }
    
    Testfile<<"5"<<" "<<threadid<<" "<<hex<<currentlock<<dec<<endl;

    ReleasedLockUpdateVC(threadid, currentlock, ITforAllThread->second);

    if(((ITforAllThread->second).LockAcquired).empty())
    {
        VectorDetect(GetOne.threadID,ITforAllThread->second);
        WW_R_UpdateVC(ITforAllThread->second);
    }
    
    CreateNewFrame(threadid,ITforAllThread->second,"MutexUnlock");
    
    if(((ITforAllThread->second).LockAcquired).size()==0)
    {
        (ITforAllThread->second).BlockFrame=(ITforAllThread->second).ListAddress;
    }
    
    OutPutAllVecorTime("UnLock");
    
    PIN_ReleaseLock(&lock);
}

VOID BeforePthread_cond_wait(ADDRINT cond, ADDRINT mutex, THREADID threadid)
{
    BeforePthread_mutex_unlock(mutex,threadid);
    SynAddress.insert(cond);
    SynAddress.insert(mutex);
    PIN_GetLock(&lock, threadid+1);
    VecWait[threadid]=((RecordCond) {6, threadid, cond, mutex});
    PIN_ReleaseLock(&lock);
}

VOID AfterPthread_cond_wait(THREADID threadid)
{
    PIN_GetLock(&lock, threadid+1);

    RecordCond GetOne;
    map<THREADID, RecordCond>::iterator itwait;
    itwait=VecWait.find(threadid);
    if(itwait==VecWait.end())
    {
        ErrorLog<<"AfterPthread_cond_wait2: Cannot find the thread "<<dec<<threadid<<endl;
        exit(-1);
    }
    GetOne=(RecordCond) {(itwait->second).style, (itwait->second).threadID, (itwait->second).cond, (itwait->second).mutex};
    Testfile<<(itwait->second).style<<" "<<(itwait->second).threadID<<" "<<hex<<(itwait->second).cond<<dec<<endl;
    OutPutVectorTime<<(itwait->second).style<<" "<<(itwait->second).threadID<<" "<<hex<<(itwait->second).cond<<dec<<endl;
    VecWait.erase(itwait);

    map<UINT32, ThreadVecTime>::iterator ITforAllThread;
    ITforAllThread=AllThread.find(GetOne.threadID);
    if(AllThread.end()==ITforAllThread)
    {
        ErrorLog<<"AfterPthread_cond_wait1: Cannot find the thread "<<dec<<threadid<<endl;
        PIN_ReleaseLock(&lock);
        exit(-1);;
    }
    
    if(((ITforAllThread->second).LockAcquired).empty())
    {
        VectorDetect(GetOne.threadID,ITforAllThread->second);
        WW_R_UpdateVC(ITforAllThread->second);
    }

    CreateNewFrame(threadid,ITforAllThread->second,"CondWait");

    map<UINT32, long>::iterator mapvectimemain;
    map<UINT32, long>::iterator mapvectimechild; //to find thread's vec time
    map<ADDRINT, map<UINT32, long> >::iterator mapsignal; //to find signal
    mapsignal=SignalVecTime.find(GetOne.cond);
    if(mapsignal==SignalVecTime.end())
    {
        PIN_ReleaseLock(&lock);
        return;
    }
    for(mapvectimechild=(mapsignal->second).begin();mapvectimechild!=(mapsignal->second).end();mapvectimechild++)
    {
        mapvectimemain=(((ITforAllThread->second).ListAddress)->VecTime).find(mapvectimechild->first);
        if(mapvectimemain==(((ITforAllThread->second).ListAddress)->VecTime).end())
        {
            (((ITforAllThread->second).ListAddress)->VecTime)[mapvectimechild->first]=mapvectimechild->second;
        }
        else
        {
            mapvectimemain->second=mapvectimechild->second > mapvectimemain->second ? mapvectimechild->second : mapvectimemain->second;
        }
    }

//  SignalVecTime.erase(GetOne.cond);

    OutPutAllVecorTime("Wait");

    JustBeforeLock(GetOne.mutex, threadid);

    PIN_ReleaseLock(&lock);
}

VOID BeforePthread_cond_timedwait(ADDRINT cond, ADDRINT mutex, THREADID threadid)
{
    BeforePthread_mutex_unlock(mutex,threadid);
    SynAddress.insert(cond);
    SynAddress.insert(mutex);
    PIN_GetLock(&lock, threadid+1);
    VecWait[threadid]=((RecordCond) {7, threadid, cond, mutex});
    PIN_ReleaseLock(&lock);
}

VOID AfterPthread_cond_timedwait(THREADID threadid)
{
    AfterPthread_cond_wait(threadid);
}

VOID BeforePthread_cond_signal(ADDRINT cond, THREADID threadid)
{
    PIN_GetLock(&lock, threadid+1);
    SynAddress.insert(cond);

    RecordType GetOne;
    GetOne=(RecordType) {8, threadid, cond};
    Testfile<<"8"<<" "<<threadid<<" "<<hex<<cond<<dec<<endl;
    OutPutVectorTime<<"8"<<" "<<threadid<<" "<<hex<<cond<<dec<<endl;

    map<UINT32, ThreadVecTime>::iterator ITforAllThread;
    ITforAllThread=AllThread.find(GetOne.threadID);
    if(AllThread.end()==ITforAllThread)
    {
        ErrorLog<<"BeforePthread_cond_signal: Cannot find the thread "<<dec<<threadid<<endl;
        PIN_ReleaseLock(&lock);
        return;
    }
    
    if(((ITforAllThread->second).LockAcquired).empty())
    {
        VectorDetect(GetOne.threadID,ITforAllThread->second);
        WW_R_UpdateVC(ITforAllThread->second);
    }

    CreateNewFrame(threadid,ITforAllThread->second,"CondSignal");

    SignalVecTime.insert(pair<ADDRINT, map<UINT32, long> >(GetOne.object, ((ITforAllThread->second).ListAddress)->VecTime));

    OutPutAllVecorTime("Signal");

    PIN_ReleaseLock(&lock);
}

VOID BeforePthread_cond_broadcast(ADDRINT cond, THREADID threadid)
{
    PIN_GetLock(&lock, threadid+1);
    SynAddress.insert(cond);

    RecordType GetOne;
    GetOne=(RecordType) {9, threadid, cond};
    Testfile<<"9"<<" "<<threadid<<" "<<hex<<cond<<dec<<endl;
    OutPutVectorTime<<"9"<<" "<<threadid<<" "<<hex<<cond<<dec<<endl;

    map<UINT32, ThreadVecTime>::iterator ITforAllThread;
    ITforAllThread=AllThread.find(GetOne.threadID);
    if(AllThread.end()==ITforAllThread)
    {
        ErrorLog<<"BeforePthread_cond_broadcast: Cannot find the thread "<<dec<<threadid<<endl;
        PIN_ReleaseLock(&lock);
        return;
    }
    
    if(((ITforAllThread->second).LockAcquired).empty())
    {
        VectorDetect(GetOne.threadID,ITforAllThread->second);
        WW_R_UpdateVC(ITforAllThread->second);
    }

    CreateNewFrame(threadid,ITforAllThread->second,"CondBroadcast");

    SignalVecTime[GetOne.object]=((ITforAllThread->second).ListAddress)->VecTime;

    OutPutAllVecorTime("Broadcast");

    PIN_ReleaseLock(&lock);
}
