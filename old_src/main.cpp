//This is the start of the control
#include <list>
#include <pthread.h>
#include "pin.H"
#include "main_basictype.h"
#include <vector>
#include <set>
#include <queue>
#include <map>
#include <algorithm>
#include <string.h>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
//#include "VecDetect.h"
#define NUMBEROFVEC 5
using namespace std;

set<ADDRINT> ShareVarAddress; //Record the address
vector<ThreadParent> ThreadVarParent; //Record the relationship between child and father
map<UINT32, USIZE> MallocSize, CallocSize; //Record the malloc and calloc information
map<OS_THREAD_ID, THREADID> ThreadIDInf; //Record the thread ID information
bool startmallocmonitor=false;
PIN_LOCK lock, shareaddrlock;
//queue<RecordType> CacheRecord; //The record of monitor
vector<RecordType> VecWait;
bool monitorendflag=false, realendflag=false;

map<UINT32, ThreadVecTime> AllThread;
map<ADDRINT, map<UINT32, long> > SignalVecTime; //Condition vec time
map<UINT32, queue<map<UINT32, long> > > CreateThreadVecTime; //Record the father vec time
map<UINT32, map<UINT32, long> > FiniThreadVecTime; //record the finish vec time

ofstream Testfile,Monitorfile;
ofstream OutPutVectorTime;
ofstream Imgfile;
ofstream DataRaceOut;

int ThreadNum=0;

vector<ShareAddreeStruct> MallocVec; //存储Malloc地址
size_t MaxSum=0;

void OutPutVecorTime(UINT32 threadID) //输出Vector Time
{
	map<UINT32, ThreadVecTime>::iterator ITforallthread;
	map<UINT32, long>::iterator tmp1;
	ITforallthread=AllThread.find(threadID);
	OutPutVectorTime<<"*"<<dec<<threadID<<"*   ";
	for(tmp1=(((ITforallthread->second).ListAddress)->VecTime).begin();tmp1!=(((ITforallthread->second).ListAddress)->VecTime).end();tmp1++)
	{
		OutPutVectorTime<<tmp1->first<<":"<<tmp1->second<<"$";
	}
	OutPutVectorTime<<endl;
}

void GetMaxMemory()
{
	size_t mysum=0;
//	mysum+=CacheRecord.size() * sizeof(RecordType);
	map<UINT32, ThreadVecTime>::iterator AllIT;
	for(AllIT=AllThread.begin();AllIT!=AllThread.end();AllIT++)
	{
		mysum+=sizeof(UINT32);
		mysum+=sizeof((AllIT->second).ListAddress);
		list<struct RWVecTime>::iterator listIT;
		for(listIT=((AllIT->second).VecTimeList).begin();listIT!=((AllIT->second).VecTimeList).end();listIT++)
		{
			mysum+=sizeof(bool);
			mysum+=(listIT->VecTime).size() * (sizeof(UINT32) + sizeof(long));
			map<ADDRINT, SharedMemoryInf>::iterator ASIT;
			for(ASIT=(listIT->SharedMemory).begin(); ASIT!=(listIT->SharedMemory).end(); ASIT++)
			{
				mysum+=sizeof(ADDRINT);
				mysum+=sizeof(bool) * 2;
				mysum+=(((ASIT->second).RLockAcquired).size() + ((ASIT->second).WLockAcquired).size()) * sizeof(ADDRINT);
			}
		}
//		mysum+=((AllIT->second).LockAcquired).size() * sizeof(ADDRINT);
	}
	if(mysum > MaxSum)
		MaxSum=mysum;
}

void InitThreadVecTime(UINT32 threadID)
{
	map<UINT32, ThreadVecTime>::iterator ITforallthread;
	RWVecTime tmpRWVecTime;
	tmpRWVecTime.liveflag=false;
	ThreadVecTime tmpThreadVecTime;
	for(int i=0;i<NUMBEROFVEC;i++)
		(tmpThreadVecTime.VecTimeList).push_back(tmpRWVecTime);
	AllThread.insert(pair<UINT32, ThreadVecTime>(threadID, tmpThreadVecTime));
	ITforallthread=AllThread.find(threadID);
	(ITforallthread->second).ListAddress=((ITforallthread->second).VecTimeList).begin();
	(((ITforallthread->second).VecTimeList).begin())->liveflag=true;
}

bool CheckLock(const set<ADDRINT> &lockone, const set<ADDRINT> &locktwo)
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

void DetectSharedMemory(const map<ADDRINT, SharedMemoryInf> &MemoryOne, const map<ADDRINT, SharedMemoryInf> &MemoryTwo, UINT32 threadIDOne, UINT32 threadIDTwo)
{
	map<ADDRINT, SharedMemoryInf>::const_iterator MapforOne;
	map<ADDRINT, SharedMemoryInf>::const_iterator MapforTwo;
	for(MapforOne=MemoryOne.begin();MapforOne!=MemoryOne.end();MapforOne++)
	{
		MapforTwo=MemoryTwo.find(MapforOne->first);
		if(MapforTwo!=MemoryTwo.end())
		{
			if((MapforOne->second).Wstatus && (MapforTwo->second).Wstatus)
			{
				if(CheckLock((MapforOne->second).WLockAcquired, (MapforTwo->second).WLockAcquired))
					DataRaceOut<<"+++++++++++++++++++++"<<endl<<"Thread: "<<threadIDOne<<" Thread: "<<threadIDTwo<<" Address: "<<hex<<MapforOne->first<<dec<<" W-W"<<endl<<endl;
			}
			else if((MapforOne->second).Wstatus && (MapforTwo->second).Rstatus)
			{
				if(CheckLock((MapforOne->second).WLockAcquired, (MapforTwo->second).RLockAcquired))
					DataRaceOut<<"+++++++++++++++++++++"<<endl<<"Thread: "<<threadIDOne<<" Thread: "<<threadIDTwo<<" Address: "<<hex<<MapforOne->first<<dec<<" W-R"<<endl<<endl;
			}
			else if((MapforOne->second).Rstatus && (MapforTwo->second).Wstatus)
			{
				if(CheckLock((MapforOne->second).RLockAcquired, (MapforTwo->second).WLockAcquired))
					DataRaceOut<<"+++++++++++++++++++++"<<endl<<"Thread: "<<threadIDOne<<" Thread: "<<threadIDTwo<<" Address: "<<hex<<MapforOne->first<<dec<<" R-W"<<endl<<endl;
			}
		}
	}
}

void printList(list<struct RWVecTime>::iterator ListforPrint, UINT32 threadID)
{
/*	map<UINT32, long>::iterator mapforvec;
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

void VectorDetect(UINT32 threadID)
{
	map<UINT32, ThreadVecTime>::iterator ITforDetect;
	map<UINT32, ThreadVecTime>::iterator ITforBeDetected;
	map<UINT32, long>::iterator ITforMainVec;
	map<UINT32, long>::iterator ITforAnotherVec;
	list<struct RWVecTime>::iterator ListforAnother,ListforAnotherOld;
//	int i;
	ITforDetect=AllThread.find(threadID);
	printList((ITforDetect->second).ListAddress, threadID);
	for(ITforBeDetected=AllThread.begin();ITforBeDetected!=AllThread.end();ITforBeDetected++)
	{
		if(ITforBeDetected->first!=threadID)
		{
			printList((ITforBeDetected->second).ListAddress, ITforBeDetected->first);
			ITforMainVec=(((ITforDetect->second).ListAddress)->VecTime).find(ITforBeDetected->first);
			ListforAnother=(ITforBeDetected->second).ListAddress;
/*			for(i=0;i<NUMBEROFVEC;i++)
			{
				if(ListforAnother==((ITforBeDetected->second).VecTimeList).end())
					ListforAnother--;
				ITforAnotherVec=(ListforAnother->VecTime).find(ITforBeDetected->first);
				if(ListforAnother->liveflag && (ITforMainVec->second <= ITforAnotherVec->second))
					DetectSharedMemory(((ITforDetect->second).ListAddress)->SharedMemory, ListforAnother->SharedMemory, threadID, ITforBeDetected->first);
				else
					break;
				ListforAnother--;
			}*/
			if(ListforAnother==((ITforBeDetected->second).VecTimeList).end())
				ListforAnother--;
			ListforAnotherOld=ListforAnother;
			ITforAnotherVec=(ListforAnother->VecTime).find(ITforBeDetected->first);
			if(ListforAnother->liveflag && (ITforMainVec->second <= ITforAnotherVec->second))
			{
				DetectSharedMemory(((ITforDetect->second).ListAddress)->SharedMemory, ListforAnother->SharedMemory, threadID, ITforBeDetected->first);
				ListforAnother--;
				if(ListforAnother==((ITforBeDetected->second).VecTimeList).end())
					ListforAnother--;
				while(ListforAnother!=ListforAnotherOld)
				{
					ITforAnotherVec=(ListforAnother->VecTime).find(ITforBeDetected->first);
					if(ListforAnother->liveflag && (ITforMainVec->second <= ITforAnotherVec->second))
						DetectSharedMemory(((ITforDetect->second).ListAddress)->SharedMemory, ListforAnother->SharedMemory, threadID, ITforBeDetected->first);
					else
						break;
					ListforAnother--;
					if(ListforAnother==((ITforBeDetected->second).VecTimeList).end())
						ListforAnother--;
				}
			}
		}
	}
}

VOID GetShareAddress()
{
	FILE *faddress;
	char oneline[50];
	ADDRINT oneaddress;
	int i;
	if((faddress=fopen("./address","rt"))==NULL)
	{
		cout<<"Can't open address"<<endl;
		exit(-1);
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

BOOL IsGlobalVariable(ADDRINT address)
{
	if(ShareVarAddress.find(address)!=ShareVarAddress.end())
		return true;
	else if((startmallocmonitor)&&(!MallocVec.empty()))
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

VOID ThreadStart(THREADID threadid, CONTEXT *ctxt, INT32 flags, VOID *v)
{
	RecordType GetOne;
	OS_THREAD_ID tmp;
	map<OS_THREAD_ID, THREADID>::iterator maptmp;
	struct ThreadVecTime OneThreadVecInf;
	set<ADDRINT>::iterator ITSharedVar;
	PIN_GetLock(&lock, threadid+1);
	ThreadNum++; //计算线程数量
	cout<<"Thread Number: "<<ThreadNum<<endl;
	if(INVALID_OS_THREAD_ID!=(tmp=PIN_GetParentTid()))
	{
		maptmp=ThreadIDInf.find(tmp);
		if(maptmp!=ThreadIDInf.end())
		{
//			CacheRecord.push((RecordType){1, threadid, (ADDRINT)maptmp->second});
			GetOne=(RecordType){1, threadid, (ADDRINT)maptmp->second};
			Testfile<<"1"<<" "<<threadid<<" "<<hex<<(ADDRINT)maptmp->second<<dec<<endl;
			OutPutVectorTime<<"1"<<" "<<threadid<<" "<<hex<<(ADDRINT)maptmp->second<<dec<<endl;
			ThreadVarParent.push_back((ThreadParent) {false, maptmp->second, threadid}); //Just for thread join
		}
		else
		{
//			CacheRecord.push((RecordType){1, threadid, 0});
			GetOne=(RecordType){1, threadid, 0};
			Testfile<<"1"<<" "<<threadid<<" "<<hex<<"0"<<dec<<endl;
			OutPutVectorTime<<"1"<<" "<<threadid<<" "<<hex<<"0"<<dec<<endl;
		}
	}
	else
	{
//		CacheRecord.push((RecordType){1, threadid, 0});
		GetOne=(RecordType){1, threadid, 0};
		Testfile<<"1"<<" "<<threadid<<" "<<hex<<"0"<<dec<<endl;
		OutPutVectorTime<<"1"<<" "<<threadid<<" "<<hex<<"0"<<dec<<endl;
	}
	ThreadIDInf.insert(make_pair(PIN_GetTid(), threadid));
	
	map<UINT32, ThreadVecTime>::iterator ITforAllThread;
	if(AllThread.empty())
	{
		InitThreadVecTime(GetOne.threadID);
		ITforAllThread=AllThread.find(GetOne.threadID);
		(((ITforAllThread->second).ListAddress)->VecTime).insert(pair<UINT32, long>(GetOne.threadID,1));
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
			(((ITforAllThread->second).ListAddress)->VecTime).insert(pair<UINT32, long>(GetOne.threadID,1));
		}
	}
	OutPutVecorTime(threadid);
	
	PIN_ReleaseLock(&lock);
}

VOID ThreadFini(THREADID threadid, const CONTEXT *ctxt, INT32 code, VOID *v)
{
	RecordType GetOne;
	PIN_GetLock(&lock, threadid+1);
//	CacheRecord.push((RecordType) {2, threadid, 0});
	GetOne=(RecordType) {2, threadid, 0};
	Testfile<<"2"<<" "<<threadid<<" "<<hex<<"0"<<dec<<endl;
	OutPutVectorTime<<"2"<<" "<<threadid<<" "<<hex<<"0"<<dec<<endl;
	vector<ThreadParent>::iterator itertmp;
	itertmp=ThreadVarParent.end();
	for(itertmp--;(!ThreadVarParent.empty()) && (itertmp!=ThreadVarParent.begin()); itertmp--)
		if(threadid==itertmp->childthreadid)
			break;
	if((!ThreadVarParent.empty()) && (threadid==itertmp->childthreadid))
		itertmp->liveflag=true; //Just for thread join
	
	VectorDetect(GetOne.threadID);
	map<UINT32, ThreadVecTime>::iterator ITforAllThread;
	list<RWVecTime>::iterator ListNext;
	ITforAllThread=AllThread.find(GetOne.threadID);
	ListNext=(ITforAllThread->second).ListAddress;
	list<RWVecTime>::iterator ListOld;
	ListOld=ListNext;
	ListNext++;
	if(ListNext==((ITforAllThread->second).VecTimeList).end())
		ListNext++;
	if(ListNext->liveflag) //链表满了
	{
		map<UINT32, ThreadVecTime>::iterator ITforBeDetected;
		for(ITforBeDetected=AllThread.begin();ITforBeDetected!=AllThread.end();ITforBeDetected++)
		{
			if(ITforBeDetected->first!=GetOne.threadID)
			{
				map<UINT32, long>::iterator thefirst;
				map<UINT32, long>::iterator theother;
				thefirst=(ListNext->VecTime).find(GetOne.threadID);
				theother=(((ITforBeDetected->second).ListAddress)->VecTime).find(GetOne.threadID);
				if(thefirst->second<=theother->second)
					break;
			}
		}
		if(ITforBeDetected!=AllThread.end())
		{
			RWVecTime tmpRWVecTime;
			tmpRWVecTime.liveflag=false;
			((ITforAllThread->second).VecTimeList).insert(ListNext,tmpRWVecTime);
			ListNext=ListOld;
			ListNext++;
			if(ListNext==((ITforAllThread->second).VecTimeList).end())
				ListNext++;
		}
	}
	(ITforAllThread->second).ListAddress=ListNext; //更新指向最新项
	ListNext->VecTime=ListOld->VecTime;
	map<UINT32, long>::iterator mapvectime; //用于查找对应的本线程的vec time
	mapvectime=(ListNext->VecTime).find(GetOne.threadID);
	mapvectime->second+=1; //对应vec time加1
	(ListNext->SharedMemory).clear();
	ListNext->liveflag=true;
	FiniThreadVecTime.insert(pair<UINT32, map<UINT32, long> >(GetOne.threadID, ListNext->VecTime));
	(ITforAllThread->second).ListAddress=ListNext;
	OutPutVecorTime(threadid);
	PIN_ReleaseLock(&lock);
}

VOID BeforePthread_create(THREADID threadid)
{
	RecordType GetOne;
	PIN_GetLock(&lock, threadid+1);
//	CacheRecord.push((RecordType) {13, threadid, 0});
	GetOne=(RecordType) {13, threadid, 0};
	Testfile<<"13"<<" "<<threadid<<" "<<hex<<"0"<<dec<<endl;
	OutPutVectorTime<<"13"<<" "<<threadid<<" "<<hex<<"0"<<dec<<endl;
	VectorDetect(GetOne.threadID);
	map<UINT32, ThreadVecTime>::iterator ITforAllThread;
	list<RWVecTime>::iterator ListNext;
	ITforAllThread=AllThread.find(GetOne.threadID);
	ListNext=(ITforAllThread->second).ListAddress;
	list<RWVecTime>::iterator ListOld;
	ListOld=ListNext;
	ListNext++;
	if(ListNext==((ITforAllThread->second).VecTimeList).end())
		ListNext++;
	if(ListNext->liveflag) //链表满了
	{
		map<UINT32, ThreadVecTime>::iterator ITforBeDetected;
		for(ITforBeDetected=AllThread.begin();ITforBeDetected!=AllThread.end();ITforBeDetected++)
		{
			if(ITforBeDetected->first!=GetOne.threadID)
			{
				map<UINT32, long>::iterator thefirst;
				map<UINT32, long>::iterator theother;
				thefirst=(ListNext->VecTime).find(GetOne.threadID);
				theother=(((ITforBeDetected->second).ListAddress)->VecTime).find(GetOne.threadID);
				if(thefirst->second<=theother->second)
					break;
			}
		}
		if(ITforBeDetected!=AllThread.end())
		{
			RWVecTime tmpRWVecTime;
			tmpRWVecTime.liveflag=false;
			((ITforAllThread->second).VecTimeList).insert(ListNext,tmpRWVecTime);
			ListNext=ListOld;
			ListNext++;
			if(ListNext==((ITforAllThread->second).VecTimeList).end())
				ListNext++;
		}
	}
	(ITforAllThread->second).ListAddress=ListNext; //更新指向最新项
	ListNext->VecTime=ListOld->VecTime;
	map<UINT32, long>::iterator mapvectime; //用于查找对应的本线程的vec time
	mapvectime=(ListNext->VecTime).find(GetOne.threadID);
	mapvectime->second+=1; //对应vec time加1
	(ListNext->SharedMemory).clear();
	ListNext->liveflag=true;
	map<UINT32, queue<map<UINT32, long> > >::iterator mapforqueue; //用于记录父亲新建线程时的信息
	mapforqueue=CreateThreadVecTime.find(GetOne.threadID);
	if(mapforqueue==CreateThreadVecTime.end()) //第一次新建子线程
	{
		queue<map<UINT32, long> > queuetmp;
		queuetmp.push(ListNext->VecTime);
		CreateThreadVecTime.insert(pair<UINT32, queue<map<UINT32, long> > >(GetOne.threadID, queuetmp));
	}
	else
	{
		(mapforqueue->second).push(ListNext->VecTime);
	}
	(ITforAllThread->second).ListAddress=ListNext;
	OutPutVecorTime(threadid);
	PIN_ReleaseLock(&lock);
}

VOID AfterPthread_join(THREADID threadid)
{
	RecordType GetOne;
	PIN_GetLock(&lock, threadid+1);
	vector<ThreadParent>::iterator itertmp;
	itertmp=ThreadVarParent.end();
	for(itertmp--; itertmp!=ThreadVarParent.begin(); itertmp--)
		if((threadid==itertmp->fatherthreadid)&&(itertmp->liveflag))
			break;
	if((threadid==itertmp->fatherthreadid)&&(itertmp->liveflag))
	{
//		CacheRecord.push((RecordType) {3, threadid, (ADDRINT)(itertmp->childthreadid)});
		GetOne=(RecordType) {3, threadid, (ADDRINT)(itertmp->childthreadid)};
		Testfile<<"3"<<" "<<threadid<<" "<<hex<<(ADDRINT)(itertmp->childthreadid)<<dec<<endl;
		OutPutVectorTime<<"3"<<" "<<threadid<<" "<<hex<<(ADDRINT)(itertmp->childthreadid)<<dec<<endl;
		
		ThreadVarParent.erase(itertmp);
	}
	else
	{
//		CacheRecord.push((RecordType) {3, threadid, 0});
		GetOne=(RecordType) {3, threadid, 0};
		Testfile<<"3"<<" "<<threadid<<" "<<hex<<"0"<<dec<<endl;
		OutPutVectorTime<<"3"<<" "<<threadid<<" "<<hex<<"0"<<dec<<endl;
	}
	VectorDetect(GetOne.threadID);
	map<UINT32, ThreadVecTime>::iterator ITforAllThread;
	ITforAllThread=AllThread.find(GetOne.threadID);
	map<UINT32, long>::iterator mapvectimemain; //用于查找对应的本线程的vec time
	
	list<RWVecTime>::iterator ListNext;
	ListNext=(ITforAllThread->second).ListAddress;
	list<RWVecTime>::iterator ListOld;
	ListOld=ListNext;
	ListNext++;
	if(ListNext==((ITforAllThread->second).VecTimeList).end())
		ListNext++;
	if(ListNext->liveflag) //链表满了
	{
		map<UINT32, ThreadVecTime>::iterator ITforBeDetected;
		for(ITforBeDetected=AllThread.begin();ITforBeDetected!=AllThread.end();ITforBeDetected++)
		{
			if(ITforBeDetected->first!=GetOne.threadID)
			{
				map<UINT32, long>::iterator thefirst;
				map<UINT32, long>::iterator theother;
				thefirst=(ListNext->VecTime).find(GetOne.threadID);
				theother=(((ITforBeDetected->second).ListAddress)->VecTime).find(GetOne.threadID);
				if(thefirst->second<=theother->second)
					break;
			}
		}
		if(ITforBeDetected!=AllThread.end())
		{
			RWVecTime tmpRWVecTime;
			tmpRWVecTime.liveflag=false;
			((ITforAllThread->second).VecTimeList).insert(ListNext,tmpRWVecTime);
			ListNext=ListOld;
			ListNext++;
			if(ListNext==((ITforAllThread->second).VecTimeList).end())
				ListNext++;
		}
	}
	(ITforAllThread->second).ListAddress=ListNext; //更新指向最新项
	ListNext->VecTime=ListOld->VecTime;
	mapvectimemain=(ListNext->VecTime).find(GetOne.threadID);
//	mapvectimemain->second+=1; //对应vec time加1
	(ListNext->SharedMemory).clear();
	ListNext->liveflag=true;
	
	map<UINT32, long>::iterator mapvectimechild; //用于查找对应的子线程的vec time
	map<UINT32, map<UINT32, long> >::iterator mapfini; //用于查找fini
	mapfini=FiniThreadVecTime.find((THREADID)(GetOne.object));
	for(mapvectimemain=(((ITforAllThread->second).ListAddress)->VecTime).begin();mapvectimemain!=(((ITforAllThread->second).ListAddress)->VecTime).end();mapvectimemain++)
	{
		mapvectimechild=(mapfini->second).find(mapvectimemain->first);
		mapvectimemain->second=mapvectimechild->second > mapvectimemain->second ? mapvectimechild->second : mapvectimemain->second;
	}
	FiniThreadVecTime.erase((THREADID)(GetOne.object));
	OutPutVecorTime(threadid);
	PIN_ReleaseLock(&lock);
}

VOID BeforePthread_mutex_lock(ADDRINT currentlock, THREADID threadid)
{
	RecordType GetOne;
	PIN_GetLock(&lock, threadid+1);
//	CacheRecord.push((RecordType) {4, threadid, currentlock});
	GetOne=(RecordType) {4, threadid, currentlock};
	Testfile<<"4"<<" "<<threadid<<" "<<hex<<currentlock<<dec<<endl;
	
	map<UINT32, ThreadVecTime>::iterator ITforAllThread;
	ITforAllThread=AllThread.find(GetOne.threadID);
	((ITforAllThread->second).LockAcquired).insert(GetOne.object);
	
	PIN_ReleaseLock(&lock);
}

VOID BeforePthread_mutex_unlock(ADDRINT currentlock, THREADID threadid)
{
	RecordType GetOne;
	PIN_GetLock(&lock, threadid+1);
//	CacheRecord.push((RecordType) {5, threadid, currentlock});
	GetOne=(RecordType) {5, threadid, currentlock};
	Testfile<<"5"<<" "<<threadid<<" "<<hex<<currentlock<<dec<<endl;
	
	map<UINT32, ThreadVecTime>::iterator ITforAllThread;
	ITforAllThread=AllThread.find(GetOne.threadID);
	((ITforAllThread->second).LockAcquired).insert(currentlock);
	((ITforAllThread->second).LockAcquired).erase(GetOne.object);
	
	PIN_ReleaseLock(&lock);
}

VOID BeforePthread_cond_wait(ADDRINT cond, ADDRINT mutex, THREADID threadid)
{
	PIN_GetLock(&lock, threadid+1);
	VecWait.push_back((RecordType) {6, threadid, cond});
	PIN_ReleaseLock(&lock);
}

VOID AfterPthread_cond_wait(THREADID threadid)
{
	RecordType GetOne;
	vector<RecordType>::iterator itwait;
	PIN_GetLock(&lock, threadid+1);
	for(itwait=VecWait.begin(); VecWait.end()!=itwait; itwait++)
	{
		if(threadid==itwait->threadID)
			break;
//		else
//			return;
	}
//	CacheRecord.push((RecordType) {itwait->style, itwait->threadID, itwait->object});
	GetOne=(RecordType) {itwait->style, itwait->threadID, itwait->object};
	Testfile<<itwait->style<<" "<<itwait->threadID<<" "<<hex<<itwait->object<<dec<<endl;
	OutPutVectorTime<<itwait->style<<" "<<itwait->threadID<<" "<<hex<<itwait->object<<dec<<endl;
	VecWait.erase(itwait);
	Testfile<<"1"<<endl;
	VectorDetect(GetOne.threadID);
	Testfile<<"2"<<endl;
	map<UINT32, ThreadVecTime>::iterator ITforAllThread;
	ITforAllThread=AllThread.find(GetOne.threadID);
	map<UINT32, long>::iterator mapvectimemain; //用于查找对应的本线程的vec time
	
	list<RWVecTime>::iterator ListNext;
	ListNext=(ITforAllThread->second).ListAddress;
	list<RWVecTime>::iterator ListOld;
	ListOld=ListNext;
	ListNext++;
	if(ListNext==((ITforAllThread->second).VecTimeList).end())
		ListNext++;
	if(ListNext->liveflag) //链表满了
	{
		map<UINT32, ThreadVecTime>::iterator ITforBeDetected;
		for(ITforBeDetected=AllThread.begin();ITforBeDetected!=AllThread.end();ITforBeDetected++)
		{
			if(ITforBeDetected->first!=GetOne.threadID)
			{
				map<UINT32, long>::iterator thefirst;
				map<UINT32, long>::iterator theother;
				thefirst=(ListNext->VecTime).find(GetOne.threadID);
				theother=(((ITforBeDetected->second).ListAddress)->VecTime).find(GetOne.threadID);
				if(thefirst->second<=theother->second)
					break;
			}
		}
		if(ITforBeDetected!=AllThread.end())
		{
			RWVecTime tmpRWVecTime;
			tmpRWVecTime.liveflag=false;
			((ITforAllThread->second).VecTimeList).insert(ListNext,tmpRWVecTime);
			ListNext=ListOld;
			ListNext++;
			if(ListNext==((ITforAllThread->second).VecTimeList).end())
				ListNext++;
		}
	}
	(ITforAllThread->second).ListAddress=ListNext; //更新指向最新项
	ListNext->VecTime=ListOld->VecTime;
	mapvectimemain=(ListNext->VecTime).find(GetOne.threadID);
//	mapvectimemain->second+=1; //对应vec time加1
	(ListNext->SharedMemory).clear();
	ListNext->liveflag=true;
	
	map<UINT32, long>::iterator mapvectimechild; //用于查找对应的子线程的vec time
	map<ADDRINT, map<UINT32, long> >::iterator mapsignal; //用于查找signal
	mapsignal=SignalVecTime.find(GetOne.object);
	for(mapvectimechild=(mapsignal->second).begin();mapvectimechild!=(mapsignal->second).end();mapvectimechild++)
	{
		mapvectimemain=(((ITforAllThread->second).ListAddress)->VecTime).find(mapvectimechild->first);
//		if(mapvectimemain!=(((ITforAllThread->second).ListAddress)->VecTime).end())
			mapvectimemain->second=mapvectimechild->second > mapvectimemain->second ? mapvectimechild->second : mapvectimemain->second;
	}
/*	for(mapvectimemain=(((ITforAllThread->second).ListAddress)->VecTime).begin();mapvectimemain!=(((ITforAllThread->second).ListAddress)->VecTime).end();mapvectimemain++)
	{
		
		mapvectimechild=(mapsignal->second).find(mapvectimemain->first);
		if(mapvectimechild==(mapsignal->second).end())
			cout<<"fuck!"<<endl;
		
		mapvectimemain->second=mapvectimechild->second > mapvectimemain->second ? mapvectimechild->second : mapvectimemain->second;
	}*/
	SignalVecTime.erase(GetOne.object);
	OutPutVecorTime(threadid);
	PIN_ReleaseLock(&lock);
}

VOID BeforePthread_cond_timedwait(ADDRINT cond, ADDRINT mutex, THREADID threadid)
{
	PIN_GetLock(&lock, threadid+1);
	VecWait.push_back((RecordType) {7, threadid, cond});
	PIN_ReleaseLock(&lock);
}

VOID AfterPthread_cond_timedwait(THREADID threadid)
{
	RecordType GetOne;
	vector<RecordType>::iterator itwait;
	PIN_GetLock(&lock, threadid+1);
	for(itwait=VecWait.begin(); VecWait.end()!=itwait; itwait++)
		if(threadid==itwait->threadID)
			break;
//	CacheRecord.push((RecordType) {itwait->style, itwait->threadID, itwait->object});
	GetOne=(RecordType) {itwait->style, itwait->threadID, itwait->object};
	Testfile<<itwait->style<<" "<<itwait->threadID<<" "<<hex<<itwait->object<<dec<<endl;
	OutPutVectorTime<<itwait->style<<" "<<itwait->threadID<<" "<<hex<<itwait->object<<dec<<endl;
	VecWait.erase(itwait);
	
	VectorDetect(GetOne.threadID);
	map<UINT32, ThreadVecTime>::iterator ITforAllThread;
	ITforAllThread=AllThread.find(GetOne.threadID);
	map<UINT32, long>::iterator mapvectimemain; //用于查找对应的本线程的vec time
	
	list<RWVecTime>::iterator ListNext;
	ListNext=(ITforAllThread->second).ListAddress;
	list<RWVecTime>::iterator ListOld;
	ListOld=ListNext;
	ListNext++;
	if(ListNext==((ITforAllThread->second).VecTimeList).end())
		ListNext++;
	if(ListNext->liveflag) //链表满了
	{
		map<UINT32, ThreadVecTime>::iterator ITforBeDetected;
		for(ITforBeDetected=AllThread.begin();ITforBeDetected!=AllThread.end();ITforBeDetected++)
		{
			if(ITforBeDetected->first!=GetOne.threadID)
			{
				map<UINT32, long>::iterator thefirst;
				map<UINT32, long>::iterator theother;
				thefirst=(ListNext->VecTime).find(GetOne.threadID);
				theother=(((ITforBeDetected->second).ListAddress)->VecTime).find(GetOne.threadID);
				if(thefirst->second<=theother->second)
					break;
			}
		}
		if(ITforBeDetected!=AllThread.end())
		{
			RWVecTime tmpRWVecTime;
			tmpRWVecTime.liveflag=false;
			((ITforAllThread->second).VecTimeList).insert(ListNext,tmpRWVecTime);
			ListNext=ListOld;
			ListNext++;
			if(ListNext==((ITforAllThread->second).VecTimeList).end())
				ListNext++;
		}
	}
	(ITforAllThread->second).ListAddress=ListNext; //更新指向最新项
	ListNext->VecTime=ListOld->VecTime;
	mapvectimemain=(ListNext->VecTime).find(GetOne.threadID);
//	mapvectimemain->second+=1; //对应vec time加1
	(ListNext->SharedMemory).clear();
	ListNext->liveflag=true;
	
	map<UINT32, long>::iterator mapvectimechild; //用于查找对应的子线程的vec time
	map<ADDRINT, map<UINT32, long> >::iterator mapsignal; //用于查找signal
	mapsignal=SignalVecTime.find(GetOne.object);
	for(mapvectimechild=(mapsignal->second).begin();mapvectimechild!=(mapsignal->second).end();mapvectimechild++)
	{
		mapvectimemain=(((ITforAllThread->second).ListAddress)->VecTime).find(mapvectimechild->first);
		mapvectimemain->second=mapvectimechild->second > mapvectimemain->second ? mapvectimechild->second : mapvectimemain->second;
	}
/*	for(mapvectimemain=(((ITforAllThread->second).ListAddress)->VecTime).begin();mapvectimemain!=(((ITforAllThread->second).ListAddress)->VecTime).end();mapvectimemain++)
	{
		
		mapvectimechild=(mapsignal->second).find(mapvectimemain->first);
		if(mapvectimechild==(mapsignal->second).end())
			cout<<"fuck!"<<endl;
		
		mapvectimemain->second=mapvectimechild->second > mapvectimemain->second ? mapvectimechild->second : mapvectimemain->second;
	}*/
	SignalVecTime.erase(GetOne.object);
	OutPutVecorTime(threadid);
	PIN_ReleaseLock(&lock);
}

VOID BeforePthread_cond_signal(ADDRINT cond, THREADID threadid)
{
	RecordType GetOne;
	PIN_GetLock(&lock, threadid+1);
//	CacheRecord.push((RecordType) {8, threadid, cond});
	GetOne=(RecordType) {8, threadid, cond};
	Testfile<<"8"<<" "<<threadid<<" "<<hex<<cond<<dec<<endl;
	OutPutVectorTime<<"8"<<" "<<threadid<<" "<<hex<<cond<<dec<<endl;
	VectorDetect(GetOne.threadID);
	map<UINT32, ThreadVecTime>::iterator ITforAllThread;
	list<RWVecTime>::iterator ListNext;
	ITforAllThread=AllThread.find(GetOne.threadID);
	ListNext=(ITforAllThread->second).ListAddress;
	list<RWVecTime>::iterator ListOld;
	ListOld=ListNext;
	++ListNext;
	if(ListNext==((ITforAllThread->second).VecTimeList).end())
		ListNext++;
	if(ListNext->liveflag) //链表满了
	{
		map<UINT32, ThreadVecTime>::iterator ITforBeDetected;
		for(ITforBeDetected=AllThread.begin();ITforBeDetected!=AllThread.end();ITforBeDetected++)
		{
			if(ITforBeDetected->first!=GetOne.threadID)
			{
				map<UINT32, long>::iterator thefirst;
				map<UINT32, long>::iterator theother;
				thefirst=(ListNext->VecTime).find(GetOne.threadID);
				theother=(((ITforBeDetected->second).ListAddress)->VecTime).find(GetOne.threadID);
				if(thefirst->second<=theother->second)
					break;
			}
		}
		if(ITforBeDetected!=AllThread.end())
		{
			RWVecTime tmpRWVecTime;
			tmpRWVecTime.liveflag=false;
			((ITforAllThread->second).VecTimeList).insert(ListNext,tmpRWVecTime);
			ListNext=ListOld;
			ListNext++;
			if(ListNext==((ITforAllThread->second).VecTimeList).end())
				ListNext++;
		}
	}
	(ITforAllThread->second).ListAddress=ListNext; //更新指向最新项
	ListNext->VecTime=ListOld->VecTime;
	map<UINT32, long>::iterator mapvectime; //用于查找对应的本线程的vec time
	mapvectime=(ListNext->VecTime).find(GetOne.threadID);
	mapvectime->second+=1; //对应vec time加1
	(ListNext->SharedMemory).clear();
	ListNext->liveflag=true;
	SignalVecTime.insert(pair<ADDRINT, map<UINT32, long> >(GetOne.object, ListNext->VecTime));
	(ITforAllThread->second).ListAddress=ListNext;
	OutPutVecorTime(threadid);
	PIN_ReleaseLock(&lock);
}

VOID BeforePthread_cond_broadcast(ADDRINT cond, THREADID threadid)
{
	RecordType GetOne;
	PIN_GetLock(&lock, threadid+1);
//	CacheRecord.push((RecordType) {8, threadid, cond});
	GetOne=(RecordType) {8, threadid, cond};
	Testfile<<"8"<<" "<<threadid<<" "<<hex<<cond<<dec<<endl;
	OutPutVectorTime<<"8"<<" "<<threadid<<" "<<hex<<cond<<dec<<endl;
	VectorDetect(GetOne.threadID);
	map<UINT32, ThreadVecTime>::iterator ITforAllThread;
	list<RWVecTime>::iterator ListNext;
	ITforAllThread=AllThread.find(GetOne.threadID);
	ListNext=(ITforAllThread->second).ListAddress;
	list<RWVecTime>::iterator ListOld;
	ListOld=ListNext;
	++ListNext;
	if(ListNext==((ITforAllThread->second).VecTimeList).end())
		ListNext++;
	if(ListNext->liveflag) //链表满了
	{
		map<UINT32, ThreadVecTime>::iterator ITforBeDetected;
		for(ITforBeDetected=AllThread.begin();ITforBeDetected!=AllThread.end();ITforBeDetected++)
		{
			if(ITforBeDetected->first!=GetOne.threadID)
			{
				map<UINT32, long>::iterator thefirst;
				map<UINT32, long>::iterator theother;
				thefirst=(ListNext->VecTime).find(GetOne.threadID);
				theother=(((ITforBeDetected->second).ListAddress)->VecTime).find(GetOne.threadID);
				if(thefirst->second<=theother->second)
					break;
			}
		}
		if(ITforBeDetected!=AllThread.end())
		{
			RWVecTime tmpRWVecTime;
			tmpRWVecTime.liveflag=false;
			((ITforAllThread->second).VecTimeList).insert(ListNext,tmpRWVecTime);
			ListNext=ListOld;
			ListNext++;
			if(ListNext==((ITforAllThread->second).VecTimeList).end())
				ListNext++;
		}
	}
	(ITforAllThread->second).ListAddress=ListNext; //更新指向最新项
	ListNext->VecTime=ListOld->VecTime;
	map<UINT32, long>::iterator mapvectime; //用于查找对应的本线程的vec time
	mapvectime=(ListNext->VecTime).find(GetOne.threadID);
	mapvectime->second+=1; //对应vec time加1
	(ListNext->SharedMemory).clear();
	ListNext->liveflag=true;
	SignalVecTime.insert(pair<ADDRINT, map<UINT32, long> >(GetOne.object, ListNext->VecTime));
	(ITforAllThread->second).ListAddress=ListNext;
	OutPutVecorTime(threadid);
	PIN_ReleaseLock(&lock);
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
			cout<<"Malloc error!\n";
			return;
		}
		if((mallocfinder->second)<10240)
		{
			MallocVec.push_back((ShareAddreeStruct) {ret, mallocfinder->second});
			sort(MallocVec.begin(), MallocVec.end(), less<ShareAddreeStruct>());
			Imgfile<<"Thread "<<threadid<<" Malloc address "<<ret<<" Size "<<mallocfinder->second<<endl;
		}
		MallocSize.erase(mallocfinder);
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
			cout<<"Malloc error!\n";
			return;
		}
		MallocVec.push_back((ShareAddreeStruct) {ret, callocfinder->second});
		sort(MallocVec.begin(), MallocVec.end(), less<ShareAddreeStruct>());
		CallocSize.erase(callocfinder);
	}
	PIN_ReleaseLock(&shareaddrlock);
}

VOID BeforeFree(ADDRINT address, THREADID threadid)
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
}

VOID BeforeMain()
{
	Imgfile<<"Main"<<endl;
	startmallocmonitor=true;
}

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
		RTN_Close(rtnpthread_mutex_lock);
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
		RTN_InsertCall(rtnmain, IPOINT_BEFORE, AFUNPTR(BeforeMain), IARG_END);
		RTN_Close(rtnmain);
	}
}

VOID RecordMemRead(VOID * ip, VOID * addr, UINT32 size, THREADID threadid)
{
	RecordType GetOne;
	PIN_GetLock(&lock, threadid+1);
	if(IsGlobalVariable((ADDRINT)addr))
	{
//		CacheRecord.push((RecordType) {10, threadid, (ADDRINT)addr});
		GetOne=(RecordType) {10, threadid, (ADDRINT)addr};
		Testfile<<"10"<<" "<<threadid<<" "<<hex<<(ADDRINT)addr<<dec<<endl;
		
		map<ADDRINT, SharedMemoryInf>::iterator mapforsharedmemory;
		map<UINT32, ThreadVecTime>::iterator ITforAllThread;
		ITforAllThread=AllThread.find(GetOne.threadID);
		mapforsharedmemory=(((ITforAllThread->second).ListAddress)->SharedMemory).find(GetOne.object);
		if(mapforsharedmemory==(((ITforAllThread->second).ListAddress)->SharedMemory).end())
		{
			SharedMemoryInf tmpSharedMemoryInf;
			tmpSharedMemoryInf.Rstatus=true;
			tmpSharedMemoryInf.Wstatus=false;
			tmpSharedMemoryInf.RLockAcquired=(ITforAllThread->second).LockAcquired;
			(((ITforAllThread->second).ListAddress)->SharedMemory).insert(pair<ADDRINT, SharedMemoryInf>(GetOne.object,tmpSharedMemoryInf));
		}
		else
		{
			if(!(mapforsharedmemory->second).Rstatus)
			{
				(mapforsharedmemory->second).Rstatus=true;
				(mapforsharedmemory->second).RLockAcquired=(ITforAllThread->second).LockAcquired;
			}
			else if(!((mapforsharedmemory->second).RLockAcquired).empty())
			{
				if(((ITforAllThread->second).LockAcquired).empty())
					((mapforsharedmemory->second).RLockAcquired).clear();
				else
				{
					set<ADDRINT> settmp;
					set_intersection(((mapforsharedmemory->second).RLockAcquired).begin(), ((mapforsharedmemory->second).RLockAcquired).end(), ((ITforAllThread->second).LockAcquired).begin(), ((ITforAllThread->second).LockAcquired).end(), inserter(settmp, settmp.begin()));
					(mapforsharedmemory->second).RLockAcquired=settmp;
				}
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
//		CacheRecord.push((RecordType) {11, threadid, (ADDRINT)addr});
		GetOne=(RecordType) {11, threadid, (ADDRINT)addr};
		Testfile<<"11"<<" "<<threadid<<" "<<hex<<(ADDRINT)addr<<dec<<endl;
		
		map<ADDRINT, SharedMemoryInf>::iterator mapforsharedmemory;
		map<UINT32, ThreadVecTime>::iterator ITforAllThread;
		ITforAllThread=AllThread.find(GetOne.threadID);
		mapforsharedmemory=(((ITforAllThread->second).ListAddress)->SharedMemory).find(GetOne.object);
		if(mapforsharedmemory==(((ITforAllThread->second).ListAddress)->SharedMemory).end())
		{
			SharedMemoryInf tmpSharedMemoryInf;
			tmpSharedMemoryInf.Rstatus=false;
			tmpSharedMemoryInf.Wstatus=true;
			tmpSharedMemoryInf.WLockAcquired=(ITforAllThread->second).LockAcquired;
			(((ITforAllThread->second).ListAddress)->SharedMemory).insert(pair<ADDRINT, SharedMemoryInf>(GetOne.object,tmpSharedMemoryInf));
		}
		else
		{
			if(!(mapforsharedmemory->second).Wstatus)
			{
				(mapforsharedmemory->second).Wstatus=true;
				(mapforsharedmemory->second).WLockAcquired=(ITforAllThread->second).LockAcquired;
			}
			else if(!((mapforsharedmemory->second).WLockAcquired).empty())
			{
				if(((ITforAllThread->second).LockAcquired).empty())
					((mapforsharedmemory->second).WLockAcquired).clear();
				else
				{
					set<ADDRINT> settmp;
					set_intersection(((mapforsharedmemory->second).WLockAcquired).begin(), ((mapforsharedmemory->second).WLockAcquired).end(), ((ITforAllThread->second).LockAcquired).begin(), ((ITforAllThread->second).LockAcquired).end(), inserter(settmp, settmp.begin()));
					(mapforsharedmemory->second).WLockAcquired=settmp;
				}
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

int Usage()
{
	cerr << "This is the invocation pintool" << endl;
	cerr << endl << KNOB_BASE::StringKnobSummary() << endl;
	return -1;
}

VOID Fini(INT32 code, VOID *v)
{
	Imgfile<<"****************"<<endl;
	GetMaxMemory();
	cout<<"Memory Size: "<<MaxSum<<endl;
	Imgfile<<"Memory Size: "<<MaxSum<<endl;
	cout<<"Thread Number: "<<ThreadNum<<endl;
	Imgfile<<"Thread Number: "<<ThreadNum<<endl;
	monitorendflag=true;
}

/*void *DetectorThread(void *arg)
{
	RecordType getone;
	while(!realendflag)
	{
		if(!CacheRecord.empty())
		{
			getone=CacheRecord.front();
			CacheRecord.pop();
			Monitorfile<<getone.style<<" "<<getone.threadID<<" "<<hex<<getone.object<<dec<<endl;
			VecDetect(getone);
		}
		else
		{
			if(monitorendflag)
				realendflag=true;
		}
	}
	return NULL;
}*/

int main(int argc, char * argv[])
{
	char filename[50];
	if (PIN_Init(argc, argv))
		return Usage();
	PIN_InitLock(&lock);
	PIN_InitLock(&shareaddrlock);
	PIN_InitSymbols();
	
	sprintf(filename,"Test%d.out",getpid());
	Testfile.open(filename);
	Testfile.setf(ios::showbase);
	sprintf(filename,"Monitor%d.out",getpid());
	Monitorfile.open(filename);
	Monitorfile.setf(ios::showbase);
	sprintf(filename, "Img%d.out", getpid());
	Imgfile.open(filename);
	Imgfile.setf(ios::showbase);
	sprintf(filename, "VectorTime%d.out", getpid());
	OutPutVectorTime.open(filename);
	OutPutVectorTime.setf(ios::showbase);
//	InitOutPut();
	sprintf(filename,"DataRaceOut.out");
	DataRaceOut.open(filename);
	DataRaceOut.setf(ios::showbase);
	DataRaceOut<<"<<<<<start>>>>>"<<endl;
	GetShareAddress();
	
	IMG_AddInstrumentFunction(ImageLoad, 0);
	INS_AddInstrumentFunction(Instruction, 0);
	PIN_AddFiniFunction(Fini, 0);

	PIN_AddThreadStartFunction(ThreadStart, 0);
	PIN_AddThreadFiniFunction(ThreadFini, 0);

/*	pthread_t testthread;
	int rc;
	rc=pthread_create(&testthread, NULL, DetectorThread, 0);
	if(rc)
	{
		cout<<"Creat thread error!"<<endl;
		return -1;
	}*/

	PIN_StartProgram();
//	pthread_join(testthread, 0);
	Testfile.close();
	Monitorfile.close();
	Imgfile.close();
//	FiniOutPut();
	DataRaceOut<<"<<<<<stop>>>>>"<<endl;
	DataRaceOut.close();
	OutPutVectorTime.close();
	return 0;
}
