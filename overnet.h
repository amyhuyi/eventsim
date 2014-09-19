/* ============================================
   Description: global definitions
   Author: Yi Hu
   ============================================ */
   
#ifndef OVERNET_HEADER

#define OVERNET_HEADER

#include <iostream>
#include <vector>
#include <map>
#include <sstream>
#include <assert.h>
#include <iostream>
#include <string>
#include <sstream>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <algorithm>
using namespace std;

typedef int INT32;
typedef unsigned int UINT32;
typedef unsigned long long UINT64;
typedef float FLOAT32;
typedef double FLOAT64;
typedef unsigned int ADDRINT;

#define GNRS_HASH_RANGE 4294967295 //2^32
#define GUID_SIZE 44 //160+32*5+32 = 352 bits
#define PING_SIZE 9 //8 byte UDP header, 1 byte data
#define PING_PERIOD 5000// 5 seconds 
#define pi 3.14159265358979323846
#define SHAPE_GAMMA 0.1 //ToDo:check
#define SCALE_GAMMA (50 / SHAPE_GAMMA) //ToDo:check
#define SHAPE_PARETO 1.06
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

bool str2StrArr(const string& s, char delim, vector<string>& storeResult);
double deg2rad(double deg);
double rad2deg(double rad);
double distance(double lat1, double lon1, double lat2, double lon2, char unit);
class Settings
{
public:
        static string outFileName;
        static FLOAT64 EndTime;
        static FLOAT64 TestThreshold;//admission qualified threshold
        static FLOAT64 OnOffSession;//session length for churn
        static UINT32 OnOffRounds;//0: leave, 1: leave+join, 2: leave+join+leave...
        static UINT32 ChurnHours;//# of consecutive hours of churn generation
        static UINT32 QueryHours;//# of hours query generation
        static UINT32 UpdateHours;//# of hours update generation
        static FLOAT64 ChurnPerNode;
        static FLOAT64 QueryPerNode;
        static FLOAT64 UpdatePerNode;
	static UINT32 TotalVirtualGUID;
	static UINT32 ActiveGUIDperPoP;
        static UINT32 NeighborSize;
        static UINT32 DHTHop;
        static UINT32 GNRS_K; //global k
        static UINT32 Local_K; //consists of nodes in the insertion AS 
        static UINT32 Regional_K; // consists of nodes in the same country
        static bool Geo_Lat_On;
        static FLOAT32 InterLatWeight;
        static FLOAT32 IntraLatWeight;
        static FLOAT32 StrongLocalityPerc;
        static FLOAT32 MedLocalityPerc;
        static bool LocMobSync; //locality and mobility synchronize 
        static FLOAT32 LocalMobilityPerc;
        static FLOAT32 RegionalMobilityPerc;
        static UINT32  QueryPerGUID;
        static bool DeployOnlyGW; //GUID workload only from (true) gw cities or (false) all deployed cities
        static UINT32 CacheOn; //0: no cache; 1: cache lookup only at src; 2: cache lookup along the route
        static FLOAT32 CachePerc; // percentage of top popular GUID each pop locally cached
        static FLOAT32 GoThroughProb; //default go through probability for a cache entry
        static bool balanceBase; // calculate baseline of workload balance
        static UINT32 CacheLookupLat; //cache lookup latency per hop in ms
};

typedef struct _Query_Latency{
    FLOAT64 _time;
    vector<FLOAT64> _delay_v;
    bool operator < (const struct _Query_Latency& query) const
    {
        return (_time < query._time);
    }
} Query_Latency;

typedef struct _Query_Count{
    UINT32 _guidIdx; //guid to be queried
    UINT32 _queryCnt; // no. of queries to the target guid
    bool operator < (const struct _Query_Count& query) const
    {
        return (_queryCnt < query._queryCnt);
    }
} Query_Count;

typedef struct _Cache_Entry{
    UINT32 _guidIdx;
    UINT32 _timestamp; //last updated time for the guid NA mapping
    UINT32 _createtime; //creation time of this cache entry
    FLOAT32 _goThroughProb;
    UINT32 _hitCount; //to determine go through or not
    UINT32 _fromLastError; //hit count from last stale retrieval, to calculate error rate
    FLOAT32 _errorRate; // no. of stale retrieval divided by entry's total retrieval
    _Cache_Entry (UINT32 guidIdx, UINT32 timestmp=0, UINT32 creation=0, FLOAT32 gothrough=0.01):_guidIdx(guidIdx),
           _timestamp(timestmp),_createtime(creation),_goThroughProb(gothrough)
           { _hitCount=0; _errorRate=0;_fromLastError=0;}
} Cache_Entry;

typedef struct _Retry_Count{
    vector<FLOAT64> _Qdelay;
    vector<FLOAT64> _Udelay;
    UINT32 _retryQuery;
    UINT32 _retryUpdate;
    UINT32  _retryQMsg;
    UINT32 _retryUMsg;
    UINT32 _issuedQuery;
    UINT32 _issuedUpdate;
    FLOAT64 _time;
    bool operator < (const struct _Retry_Count& retryCnt) const
    {
        return (_time < retryCnt._time);
    }
} Retry_Count;


class Stat
{
public:
	static vector<UINT32> Storage_per_node; //GNRS storage overhead
        static vector<UINT32> Workload_per_node; //GNRS answer query overhead
        static vector<UINT32> CacheHit_per_guid; //GNRS queries answered by cache
        static vector<UINT32> QueryHopCnt;
        static vector<UINT32> QueryHitHopCnt;
	static vector<Query_Latency> Query_latency_time;
        static vector<Query_Latency> Insertion_latency_time; //count for insert and update
	static vector<Retry_Count> Retry_Cnt;//count for both insertion and query retry
        static vector<Retry_Count> DHT_RetryCnt; //all K host failed, then a DHT retry
        static vector<UINT32> Migration_per_node;
        static vector<FLOAT32> Error_rate_per_guid; // first record the error count, then compute the rate
        /*PING overhead to maintain node table consistency
         *only record counts of extra ping during simulation
         *final process: ping counts*ping size
         */
        static vector<UINT32> Ping_per_node; 
        static UINT32 Premature_joins;
        static UINT32 Premature_leaves;
        
        static Stat* Inst();
        ~Stat();
        
        void PrintRetryStat();
        void PrintLatencyStat();
        void PrintQueryLatencyCDF();
        void PrintUpdateLatencyCDF();
private:
        static Stat* _stat_ptr;
        Stat();
};

#endif
