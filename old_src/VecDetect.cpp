#include <list>
#include <map>
#include <fstream>
#include <iostream>
#include <queue>
#include <vector>
#include <algorithm>
#include "VecDetect.h"
#include "main_basictype.h"

#define NUMBEROFVEC 5

using namespace std;

ofstream DataRaceOut;

map<UINT32, ThreadVecTime> AllThread;
map<ADDRINT, map<UINT32, long> > SignalVecTime; //Condition vec time
map<UINT32, queue<map<UINT32, long> > > CreateThreadVecTime; //Record the father vec time
map<UINT32, map<UINT32, long> > FiniThreadVecTime; //record the finish vec time

void InitOutPut()
{
	char filename[50];
	sprintf(filename,"DataRaceOut.out");
	DataRaceOut.open(filename);
	DataRaceOut.setf(ios::showbase);
	DataRaceOut<<"<<<<<start>>>>>"<<endl;
}

void FiniOutPut()
{
	DataRaceOut<<"<<<<<stop>>>>>"<<endl;
	DataRaceOut.close();
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
					cout<<"+++++++++++++++++++++"<<endl<<"Thread:"<<threadIDOne<<" Thread:"<<threadIDTwo<<" Address:"<<hex<<MapforOne->first<<dec<<" W-W"<<endl<<endl;
			}
			else if((MapforOne->second).Wstatus && (MapforTwo->second).Rstatus)
			{
				if(CheckLock((MapforOne->second).WLockAcquired, (MapforTwo->second).RLockAcquired))
					cout<<"+++++++++++++++++++++"<<endl<<"Thread:"<<threadIDOne<<" Thread:"<<threadIDTwo<<" Address:"<<hex<<MapforOne->first<<dec<<" W-R"<<endl<<endl;
			}
			else if((MapforOne->second).Rstatus && (MapforTwo->second).Wstatus)
			{
				if(CheckLock((MapforOne->second).RLockAcquired, (MapforTwo->second).WLockAcquired))
					cout<<"+++++++++++++++++++++"<<endl<<"Thread:"<<threadIDOne<<" Thread:"<<threadIDTwo<<" Address:"<<hex<<MapforOne->first<<dec<<" R-W"<<endl<<endl;
			}
		}
	}
}

void printList(list<struct RWVecTime>::iterator ListforPrint, UINT32 threadID)
{
	map<UINT32, long>::iterator mapforvec;
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
	}
}

void VectorDetect(UINT32 threadID)
{
	map<UINT32, ThreadVecTime>::iterator ITforDetect;
	map<UINT32, ThreadVecTime>::iterator ITforBeDetected;
	map<UINT32, long>::iterator ITforMainVec;
	map<UINT32, long>::iterator ITforAnotherVec;
	list<struct RWVecTime>::iterator ListforAnother;
	int i;
	ITforDetect=AllThread.find(threadID);
	printList((ITforDetect->second).ListAddress, threadID);
	for(ITforBeDetected=AllThread.begin();ITforBeDetected!=AllThread.end();ITforBeDetected++)
	{
		if(ITforBeDetected->first!=threadID)
		{
			printList((ITforBeDetected->second).ListAddress, ITforBeDetected->first);
			ITforMainVec=(((ITforDetect->second).ListAddress)->VecTime).find(ITforBeDetected->first);
			ListforAnother=(ITforBeDetected->second).ListAddress;
			for(i=0;i<NUMBEROFVEC;i++)
			{
				if(ListforAnother==((ITforBeDetected->second).VecTimeList).end())
					ListforAnother--;
				ITforAnotherVec=(ListforAnother->VecTime).find(ITforBeDetected->first);
				if(ListforAnother->liveflag && (ITforMainVec->second <= ITforAnotherVec->second))
					DetectSharedMemory(((ITforDetect->second).ListAddress)->SharedMemory, ListforAnother->SharedMemory, threadID, ITforBeDetected->first);
				else
					break;
				ListforAnother--;
			}
		}
	}
}

void ThreadCreate(RecordType GetOne)
{
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
	ListNext->VecTime=((ITforAllThread->second).ListAddress)->VecTime;
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
}

void ThreadStart(RecordType GetOne) //一个线程的开始
{
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
}

void ThreadFini(RecordType GetOne) //一个线程的结束
{
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
	ListNext->VecTime=((ITforAllThread->second).ListAddress)->VecTime;
	map<UINT32, long>::iterator mapvectime; //用于查找对应的本线程的vec time
	mapvectime=(ListNext->VecTime).find(GetOne.threadID);
	mapvectime->second+=1; //对应vec time加1
	(ListNext->SharedMemory).clear();
	ListNext->liveflag=true;
	FiniThreadVecTime.insert(pair<UINT32, map<UINT32, long> >(GetOne.threadID, ListNext->VecTime));
	(ITforAllThread->second).ListAddress=ListNext;
}

void ThreadJoin(RecordType GetOne) //Join一个子线程
{
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
	ListNext->VecTime=((ITforAllThread->second).ListAddress)->VecTime;
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
}

void Lock(RecordType GetOne)
{
	map<UINT32, ThreadVecTime>::iterator ITforAllThread;
	ITforAllThread=AllThread.find(GetOne.threadID);
	((ITforAllThread->second).LockAcquired).insert(GetOne.object);
}

void UnLock(RecordType GetOne)
{
	map<UINT32, ThreadVecTime>::iterator ITforAllThread;
	ITforAllThread=AllThread.find(GetOne.threadID);
	((ITforAllThread->second).LockAcquired).erase(GetOne.object);
}

void Wait(RecordType GetOne)
{
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
	ListNext->VecTime=((ITforAllThread->second).ListAddress)->VecTime;
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
}

void Time_Wait(RecordType GetOne)
{
	Wait(GetOne);
}

void Signal(RecordType GetOne)
{
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
	ListNext->VecTime=((ITforAllThread->second).ListAddress)->VecTime;
	map<UINT32, long>::iterator mapvectime; //用于查找对应的本线程的vec time
	mapvectime=(ListNext->VecTime).find(GetOne.threadID);
	mapvectime->second+=1; //对应vec time加1
	(ListNext->SharedMemory).clear();
	ListNext->liveflag=true;
	SignalVecTime.insert(pair<ADDRINT, map<UINT32, long> >(GetOne.object, ListNext->VecTime));
	(ITforAllThread->second).ListAddress=ListNext;
}

void Broadcast(RecordType GetOne)
{
	Signal(GetOne);
}

void Read(RecordType GetOne)
{
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

void Write(RecordType GetOne)
{
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

void VecDetect(RecordType GetOne)
{
	switch (GetOne.style)
	{
		case 1:
			ThreadStart(GetOne);
			break;
		case 2:
			ThreadFini(GetOne);
			break;
		case 3:
			ThreadJoin(GetOne);
			break;
		case 4:
			Lock(GetOne);
			break;
		case 5:
			UnLock(GetOne);
			break;
		case 6:
			Wait(GetOne);
			break;
		case 7:
			Time_Wait(GetOne);
			break;
		case 8:
			Signal(GetOne);
			break;
		case 9:
			Broadcast(GetOne);
			break;
		case 10:
			Read(GetOne);
			break;
		case 11:
			Write(GetOne);
			break;
		case 13:
			ThreadCreate(GetOne);
			break;
	}
}
