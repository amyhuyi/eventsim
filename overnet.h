/* ============================================
   Description: global definitions
   Author: Yi Hu
   ============================================ */
   
#ifndef OVERNET_HEADER

#define OVERNET_HEADER

#include <iostream>
#include <vector>
#include <map>

#include <assert.h>

using namespace std;

typedef int INT32;
typedef unsigned int UINT32;
typedef unsigned long UINT64;
typedef float FLOAT32;
typedef double FLOAT64;
typedef unsigned int ADDRINT;

#define GNRS_HASH_RANGE 4294967295 //2^32
#define GUID_SIZE 44 //160+32*5+32 = 352 bits
#define GNRS_K 5
#define PING_SIZE 9 //8 byte UDP header, 1 byte data
#define PING_PERIOD 5000// 5 seconds 

#define SHAPE_GAMMA 0.1 //ToDo:check
#define SCALE_GAMMA (50 / SHAPE_GAMMA) //ToDo:check
#define ARRIVAL_MEAN 4 //4 requests arrivals per ms
#define DIFF(a,b) ((a)>(b)?(a)-(b):(b)-(a))

static inline void InRangeMinus (UINT32 & currValue, UINT32 range){
    if (currValue == 0)
        currValue = range -1;
    else
        currValue --;
}

static inline void InRangePlus (UINT32 & currValue, UINT32 range){
    if (currValue == range -1)
        currValue = 0;
    else
        currValue ++;
}


class Settings
{
public:
	static UINT32 NumOfAS;
	static FLOAT64 EndTime;
        static FLOAT64 TestDuration;
	static UINT32 TotalVirtualGUID;
	static UINT32 TotalActiveGUID;
        static UINT32 NeighborSize;
        static UINT32 DHTHop;
};

struct Query_Latency{
    FLOAT64 _delay;
    FLOAT64 _time;
    char _operation;//'I','Q','U'
    Query_Latency(char X, FLOAT64 time): _operation(X), _time(time) {}
    bool operator < (const Query_Latency& query) const
    {
        return (_time < query._time);
    }
};

struct Retry_Count{
    FLOAT64 _delay;
    UINT32 _retry;
    FLOAT64 _time;
    char _operation;
    Retry_Count(char X, FLOAT64 time): _operation(X), _time(time) {}
    bool operator < (const Retry_Count& retryCnt) const
    {
        return (_time < retryCnt._time);
    }
};


class Stat
{
public:
	static vector<UINT32> Storage_per_node; //GNRS storage overhead
        static vector<UINT32> Workload_per_node; //GNRS answer query overhead
	static vector<Query_Latency> Query_latency_time;
        static vector<Query_Latency> Insertion_latency_time; //count for insert and update
	static vector<Retry_Count> Retry_Cnt;//count for both insertion and query retry
        static vector<Retry_Count> DHT_Retry_time; //all K host failed, then a DHT retry
        static vector<UINT32> Migration_per_node;
        /*PING overhead to maintain node table consistency
         *only record counts of extra ping during simulation
         *final process: ping counts*ping size
         */
        static vector<UINT32> Ping_per_node; 
        static UINT32 Premature_joins;
        static UINT32 Premature_leaves;
};

#endif
