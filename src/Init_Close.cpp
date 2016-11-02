#include "Init_Close.h"
#include "Output.h"

extern map<UINT32, ThreadVecTime> AllThread;
extern size_t MaxSum;
extern int ThreadNum;
extern bool monitorendflag;

extern ofstream Testfile;
extern ofstream OutPutVectorTime;
extern ofstream Imgfile;
extern ofstream DataRaceOut;
extern ofstream ErrorLog;
extern int Warnings;
extern UINT64 ConNumFrames;
extern UINT64 NumAnalysis;
extern set<ADDRINT> Race_address;
extern int WWNum;

void InitFileOutput()
{
    char filename[50];
    
    sprintf(filename,"Test%d.out",getpid());
    Testfile.open(filename);
    Testfile.setf(ios::showbase);

    sprintf(filename, "Img%d.out", getpid());
    Imgfile.open(filename);
    Imgfile.setf(ios::showbase);
    
    sprintf(filename, "VectorTime%d.out", getpid());
    OutPutVectorTime.open(filename);
    OutPutVectorTime.setf(ios::showbase);

    sprintf(filename,"DataRaceOut%d.out", getpid());
    DataRaceOut.open(filename);
    DataRaceOut.setf(ios::showbase);

    sprintf(filename,"ErrorLog%d.out", getpid());
    ErrorLog.open(filename);
    ErrorLog.setf(ios::showbase);
}

void CloseFileOutput()
{
    Testfile.close();
    Imgfile.close();
    DataRaceOut.close();
    OutPutVectorTime.close();
    ErrorLog.close();
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
    Imgfile<<"Warnings: "<<dec<<Warnings<<endl;
    cout<<"Warnings: "<<dec<<Warnings<<endl;
    Imgfile<<"Count of Analysis: "<<NumAnalysis<<endl;
    cout<<"Count of Analysis: "<<NumAnalysis<<endl;
    Imgfile<<"Concurrent Frames: "<<ConNumFrames<<endl;
    cout<<"Concurrent Frames: "<<ConNumFrames<<endl;
    Imgfile<<"Race count: "<<Race_address.size()<<endl;
    cout<<"Race count: "<<Race_address.size()<<endl;
    Imgfile<<"W-W Frames: "<<WWNum<<endl;
    cout<<"W-W Frames: "<<WWNum<<endl;
    monitorendflag=true;
    OutPutAllVecorTime("Fini");
}
