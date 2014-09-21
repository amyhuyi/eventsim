/* ===============================================================
   Description: (1)GNRS member AS network routing table/latency, (2)GNRS
                nodes join/leave from each member AS
   Author: Yi Hu
   =============================================================== */
   
#ifndef UNDERLAY_HEADER

#define UNDERLAY_HEADER 
#include <string>
#include <vector>
#include <set>
#include "overnet.h"
#include "event.h"
#include "node.h"
#include "as.h"
#include "guid.h"
#include "city.h"

class GNRSOperationMessage : public Message
{
private:
    UINT32 _nodeIdx; //_nodeIdx is the node index, _src is the as index
    UINT32 _guidIdx;
public:
	GNRSOperationMessage (MsgType type, UINT32 nodeIdx, UINT32 guidIdx, FLOAT64 time); 
	virtual bool Callback();            
};

class Underlay
{
private:
    static Underlay* _underlay_ptr;
    UINT32 _num_of_as;
    UINT32 _num_of_node;
    UINT32 *as_dist_matx;           //shortest distance matrix
    int *as_pre_matx;             //predicate matrix for path recompute: the last hop to reach destination
    Underlay(const char* cityFile, const char* routeFile, const char* asFile, const char* predFile);
    bool isAvailable(UINT32 nodeIdx, int asIdx); //check node availability from global or local AS view
    void determineLocalHost(UINT32 guidIdx, set<UINT32>& _hostNodeIdx, int asIdx);
    void determineRegionalHost(UINT32 guidIdx, set<UINT32>& _hostNodeIdx, int asIdx);
    void determineGlobalHost(UINT32 guidIdx, set<UINT32>& _hostNodeIdx, int asIdx);
    UINT32 getCityIdx(string cityName);
    void getQueryNodes(UINT32 guidIdx, vector<UINT32>& _queryNodes, vector<UINT32>& _queryQuota, FLOAT32 exponent, bool popularity);
    void getQueryNodesPerLoc(FLOAT32 lat1,FLOAT32 lon1, UINT32 totalNo, vector<UINT32>& _queryNodes, vector<UINT32>& _queryQuota, FLOAT32 exponent);
    void getQueryNodesByPoP(UINT32 guidIdx, vector<UINT32>& _queryNodes, vector<UINT32>& _queryQuota, FLOAT32 exponent); //by popularity
    UINT32 issueQueries (UINT32 guidIdx, FLOAT32 exponent);
    void initializeMobility(UINT32 guidIdx);//assign address queue for a GUID among gw cities or all deployed cities
    FLOAT64 calSingleQueryWrkld (UINT64 currGUIDIdx, UINT32 currNodeIdx); //return this query latency
    FLOAT32 genLocalityExponent();
    void genOutFileName();
    void getShortestPath(UINT32 srcAS, UINT32 destAS, vector<UINT32> &pathContainer);
    UINT32 calCacheLatOverhead(vector<UINT32> pathNodeIdx, UINT32 hitNodeIdx);
public:
    UINT32 getvpHostIdx(UINT32 guid, UINT32 start_idx, UINT32 end_idx);
    vector<Node> global_node_table; //sorted based on node ID
    vector<AS> as_v;
    vector<GUID> global_guid_list;
    vector<CITY> city_list;
    vector<UINT32> gw_cities;
    vector<UINT32> deployed_cities;
    vector<UINT32> workload_cities;
    static Underlay* Inst();
    static void CreateInst(const char* cityFile,const char* routeFile, const char* asFile, const char* predFile);
    UINT32 numericDistance(UINT32 hashID1, UINT32 hashID2);
    ~Underlay();
    UINT32 GetNumOfAS();
    UINT32 GetNumOfNode();
    void ReadInRoutingTable(const char* routeFile);
    void ReadInASInfo(const char* asFile);//as index, tier, capacity
    void ReadInCityInfo(const char* cityFile);
    void ReadInPredicate(const char* predFile);
    void InitializeNetwork(); //assign nodes to each AS based on capacity
    void InitializeStat();
    void InitializeWorkload();//create GUID insertion workload (among gw city or all deployed cities)
    void PrepareWorkloadCal();//preparation work to assist storage and query workload calculation
    UINT32 generateQueryWorkload(FLOAT64 mean_arrival);
    UINT32 generateUpdateWorkload(FLOAT64 mean_arrival);
    UINT32 generateLeaveChurn(UINT32 churnLength, FLOAT64 mean_arrival, FLOAT64 sessionTime, UINT32 onOffTimes);
    void SynchNetwork();
    FLOAT64 getLatency(UINT32 src, UINT32 dest);
    UINT32 migrationOverhead4Join(UINT32 nodeIdx);
    UINT32 migrationOverhead4Leave(UINT32 nodeIdx);
    void getNeighbors(UINT32 nodeIdx, set<UINT32> & neighborsIdx_v); // put all online neighbors into neighborsIdx_v
    void determineHost(UINT32 guidIdx, set<UINT32>& glbCalHostSet, set<UINT32>& lclCalHostSet, int nodeIdx);
    UINT32 getIdxRetryCnt(FLOAT64 currTime, bool isDHTretry);
    UINT32 getIdxQueryLatency(FLOAT64 currTime, bool isInsertion);
    void calStorageWorkload();//calculate storage workload distribution among hashIDs
    void calQueryWorkload(); //calculate query workload distribution among hashIDs
};

#endif
/*
 Time unit: latency is in ms, mean arrival is # arrivals per hour, so time in stat:: is also per hour 
 */