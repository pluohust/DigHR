//There are some types that all call use.
#include <vector>
#include <map>
#include <set>
#include <stdint.h>
#include <list>
#include "pin.H"

#ifndef MY_BASICTYPE_CGCL
#define MY_BASICTYPE_CGCL

struct RecordType //Record the necessary information to the detector
{
	int style;
	UINT32 threadID;
	ADDRINT object;
};

struct ShareAddreeStruct //This is the share memory space address
{
	ADDRINT address_name;
	USIZE address_size;
	
	bool operator <(const ShareAddreeStruct& rhs) const
	{
		return address_name < rhs.address_name;
	}
	
	bool operator >(const ShareAddreeStruct& rhs) const
	{
		return address_name > rhs.address_name;
	}
};

struct LockInf //Each share address' lock information
{
	int R;
	int W;
	std::vector<ADDRINT> lockID;
};

struct EachAddressInf //Each share address details
{
	bool RWflag;
	int sumR, sumW;
	int unlockR, unlockW;
	UINT32 threadID;
	ADDRINT address;
	std::vector<LockInf> lock;
	struct EachAddressInf *next;
};

struct MemoryData //Each memory address information
{
	bool SignalStatus;
	ADDRINT SignalAddress;
	std::vector<ADDRINT> veclock;
	struct EachAddressInf *EachShareAddress;
	struct MemoryData *next;
	std::vector<MemoryData *> Prior, Following;
};

struct ThreadInf //Each thread information
{
	int status; //ThreadStart is 1;
	UINT32 threadID;
	UINT32 fartherthreadID;
	struct MemoryData *data;
	struct ThreadInf *next;
};

struct ThreadParent //The relationship between the farther and child
{
	bool liveflag;
	THREADID fatherthreadid;
	THREADID childthreadid;
};

struct CreateThreadInf
{
	UINT32 threadID;
	struct MemoryData *data;
};

struct SharedMemoryInf
{
	bool Rstatus, Wstatus;
	set<ADDRINT> RLockAcquired;
	set<ADDRINT> WLockAcquired;
};

struct RWVecTime
{
	bool liveflag;
	map<UINT32, long> VecTime;
	map<ADDRINT, SharedMemoryInf> SharedMemory;
};

struct ThreadVecTime
{
	list<struct RWVecTime>::iterator ListAddress;
	list<struct RWVecTime> VecTimeList;
	set<ADDRINT> LockAcquired;
};
#endif
