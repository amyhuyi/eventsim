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

class GUID
{
private:
	UINT32 _guid;   //unique for each GUID
	UINT32 _objID;  //hash value of GUID into GNRS space, an objID may corresponds to multiple GUID
	FLOAT64 _last_update_time;
        UINT32 _address_asIdx;      // current AS Idx the GUID is attached to, which is the AS inserted this GUID
public:
        GUID (UINT32 id, UINT32 asIdx, FLOAT64 time); // compute objID from GUID and GNRS space range
	~GUID();
        bool operator < (const GUID & guid1);
	UINT32 GetGUID();
        UINT32 GetobjID();
        FLOAT64 GetLastUpdateTime ();
        UINT32 GetAddrASIdx();
        void UpdateAddrASIdx(UINT32 newASIdx, FLOAT64 time);
};

class Node{
private:
    UINT32 _nodeIdx;
    bool _in_service;
    UINT32 _asIdx;
    FLOAT64 _last_update_time;
public:
    Node (UINT32 nodeIdx, UINT32 asIdx, FLOAT64 time);
    ~Node();
    bool operator < (Node node1);
    bool isInService();
    void setInService(FLOAT64 time);
    void setOffService(FLOAT64 time);
    UINT32 getASIdx();
};
   

class AS
{
private:
    UINT32 _tier;
    UINT32 _capacity;
    UINT32 _asIdx;
    FLOAT64 getMaxDistance(vector<UINT32> correctHost);
    FLOAT64 getMinDistance(vector<UINT32> correctHost);
    FLOAT64 calInsertDelay(vector<UINT32> onlyInlocal, vector<UINT32> onlyInglobal, vector<UINT32> correctHost);
    FLOAT64 calQueryDelay(vector<UINT32> onlyInlocal, vector<UINT32> onlyInglobal, vector<UINT32> correctHost);
public:
    
    AS(UINT32 asindex, UINT32 tier, UINT32 capacity);
    set <UINT32> _myNodes; //index of my nodes in the global_node_table
    set <UINT32> _local_view_offNodes; // local view of offline nodes
    ~AS();
    UINT32 getCapacity();
    UINT32 getTier();
    bool initMyNodes();
    bool isGNRSMember();
    void joinGNRS(FLOAT64 lifetime);    //initialize all nodes in service
    void leaveGNRS(FLOAT64 offtime);   //remove all nodes from service
    void upgradeGNRS(UINT32 nodeIdx, FLOAT64 arrival_time,FLOAT64 lifetime,UINT32 churnRounds); //add one node in service
    void downgradeGNRS(UINT32 nodeIdx, FLOAT64 arrival_time,FLOAT64 offtime,UINT32 churnRounds); //remove one nodes from service
    bool insertGUID(UINT32 guid, FLOAT64 time);//time is the absolute finish time
    bool queryGUID(UINT32 guid, FLOAT64 time);
    bool updateGUID(UINT32 guid, FLOAT64 time);
    void determineHost(UINT32 guid, set<UINT32> & _hostNodeIdx);
    void beNotifedAjoin(UINT32 nodeIdx);
    void beNotifedAleave(UINT32 nodeIdx);
    void calCorrectHost(set<UINT32> localHostset, set<UINT32> globalHostset, char opt);
};

class GNRSOperationMessage : public Message
{
private:
    UINT32 _asIdx;
    UINT32 _guid;
public:
	GNRSOperationMessage (MsgType type, UINT32 asIdx, UINT32 guid, FLOAT64 time); 
	virtual bool Callback();            
};

class Underlay
{
private:
    static Underlay* _underlay_ptr;
    UINT32 _num_of_as;
    UINT32 _num_of_node;
    UINT32 *as_dist_matx;           //shortest distance matrix
    UINT32 *as_pre_matx;             //predicate matrix for path recompute: the last hop to reach destination
    Underlay(const char* routeFile, const char* asFile);
public:
    
    vector<Node> global_node_table; //sorted based on node ID
    vector<AS> as_v;
    vector<GUID> global_guid_list;
    static Underlay* Inst();
    static void CreateInst(const char* routeFile, const char* asFile);
    
    ~Underlay();
    UINT32 GetNumOfAS();
    UINT32 GetNumOfNode();
    void ReadInRoutingTable(const char* routeFile);
    void ReadInASInfo(const char* asFile);//as index, tier, capacity
    void InitializeNetwork(); //assign nodes to each AS based on capacity
    void InitializeWorkload();//create GUID insertion workload
    UINT32 generateWorkload(UINT32 workloadLength, FLOAT64 mean_arrival, char opt);//opt = 'U'/'Q'
    UINT32 generateLeaveChurn(UINT32 churnLength, FLOAT64 mean_arrival, FLOAT64 sessionTime, UINT32 onOffTimes);
    void SynchNetwork();
    FLOAT64 getLatency(UINT32 src, UINT32 dest);
    UINT32 migrationOverhead4Join(UINT32 nodeIdx);
    UINT32 migrationOverhead4Leave(UINT32 nodeIdx);
    void getNeighbors(UINT32 nodeIdx, set<UINT32> & neighborsIdx_v); // put all online neighbors into neighborsIdx_v
    void determineHost(UINT32 guid, set<UINT32>& _hostNodeIdx);
    UINT32 getIdxRetryCnt(FLOAT64 currTime, bool isDHTretry);
    UINT32 getIdxQueryLatency(FLOAT64 currTime, bool isInsertion);
};

#endif
/*
 Time unit: latency is in ms, mean arrival is # arrivals per hour, so time in stat:: is also per hour 
 */