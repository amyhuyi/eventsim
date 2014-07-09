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

class CITY {
private:
    string _city;
    string _state;
    string _country;
    FLOAT32 _latitude;
    FLOAT32 _longitude;
    FLOAT32 _population;
    UINT32 _ixp_weight;
public:
    vector<UINT32> _nodeIdx_v;
    CITY(string city, string state, string country, FLOAT32 lat, FLOAT32 lon, FLOAT32 pop, UINT32 ixp_weight);
    ~CITY();
    string getCity();
    string getState();
    string getCountry();
    UINT32 getIXPWeight();
    FLOAT32 getLat();
    FLOAT32 getLon();
    FLOAT32 getPop();
    bool isGW();
};


class GUID
{
private:
	
	UINT32 _vphostIdx;  //index of the virtual primary host into global node table
	FLOAT64 _last_update_time;
        UINT32 _address_nodeIdx;      // current Node (PoP) Idx the GUID is attached to, which is the node inserted this GUID
public:
        UINT32 _guid;   //unique for each GUID, a hash value in GNRS space
        GUID (UINT32 id, UINT32 nodeIdx, FLOAT64 time); // compute objID from GUID and GNRS space range
	~GUID();
	UINT32 getGUID();
        UINT32 getvphostIdx();
        FLOAT64 getLastUpdateTime ();
        UINT32 getAddrNodeIdx();
        UINT32 getAddrASIdx();
        void updateAddrNodeIdx(UINT32 newNodeIdx, FLOAT64 time);
};

class Node{
private:
    UINT32 _cityIdx;
    UINT32 _nodeIdx; //index in the global node table
    bool _in_service;
    UINT32 _asIdx;
    FLOAT64 _last_update_time;
    FLOAT64 getMaxDistance(vector<UINT32> correctHost);
    FLOAT64 getMinDistance(vector<UINT32> correctHost);
    FLOAT64 calInsertDelay(vector<UINT32> onlyInlocal, vector<UINT32> onlyInglobal, vector<UINT32> correctHost);
    FLOAT64 calQueryDelay(vector<UINT32> onlyInlocal, vector<UINT32> onlyInglobal, vector<UINT32> correctHost);
public:
    UINT32 _hashID; //hash value of the node in GNRS space, uniquely identify a node
    Node (UINT32 hashID, UINT32 asIdx, FLOAT64 time);
    ~Node();
    bool isInService();
    void setInService(FLOAT64 time);
    void setOffService(FLOAT64 time);
    UINT32 getASIdx();
    UINT32 getNodeIdx();
    UINT32 getHashID();
    void setNodeIdx( UINT32 index);
    void setCityIdx( UINT32 index);
    UINT32 getCityIdx();
    bool insertGUID(UINT32 guid, FLOAT64 time);//time is the absolute finish time
    bool queryGUID(UINT32 guid, FLOAT64 time);
    bool updateGUID(UINT32 guid, FLOAT64 time);
    void calCorrectHost(set<UINT32> localHostset, set<UINT32> globalHostset, char opt);
};
bool NodeSortPredicate( const Node d1, const Node d2);

class AS
{
private:
    UINT32 _tier;
    UINT32 _capacity;
    UINT32 _asIdx;
    UINT32 _asNum;
    string _asCntry; 
public:
    AS(UINT32 asindex, UINT32 tier, UINT32 capacity, UINT32 asNum, string asCountry);
    set<UINT32> _myCities;
    set <UINT32> _myNodes; //index of my nodes in the global_node_table
    set <UINT32> _local_view_offNodes; // local view of offline nodes
    ~AS();
    UINT32 getCapacity();
    UINT32 getTier();
    UINT32 getASNum();
    UINT32 getASIndex();
    string getASCntry();
    bool isGNRSMember();
    void joinGNRS(FLOAT64 lifetime);    //initialize all nodes in service
    void leaveGNRS(FLOAT64 offtime);   //remove all nodes from service
    void upgradeGNRS(UINT32 nodeIdx, FLOAT64 arrival_time,FLOAT64 lifetime,UINT32 churnRounds); //add one node in service
    void downgradeGNRS(UINT32 nodeIdx, FLOAT64 arrival_time,FLOAT64 offtime,UINT32 churnRounds); //remove one nodes from service
    void beNotifedAjoin(UINT32 nodeIdx);
    void beNotifedAleave(UINT32 nodeIdx);
};

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
    UINT32 *as_pre_matx;             //predicate matrix for path recompute: the last hop to reach destination
    Underlay(const char* cityFile, const char* routeFile, const char* asFile);
    bool isAvailable(UINT32 nodeIdx, int asIdx); //check node availability from global or local AS view
    void determineLocalHost(UINT32 guidIdx, set<UINT32>& _hostNodeIdx, int asIdx);
    void determineRegionalHost(UINT32 guidIdx, set<UINT32>& _hostNodeIdx, int asIdx);
    void determineGlobalHost(UINT32 guidIdx, set<UINT32>& _hostNodeIdx, int asIdx);
    UINT32 getCityIdx(string cityName);
    void getQueryNodes(UINT32 guidIdx, vector<UINT32>& _queryNodes, FLOAT32 exponent);
public:
    UINT32 getvpHostIdx(UINT32 guid, UINT32 start_idx, UINT32 end_idx);
    vector<Node> global_node_table; //sorted based on node ID
    vector<AS> as_v;
    vector<GUID> global_guid_list;
    vector<CITY> city_list;
    vector<UINT32> gw_cities;
    static Underlay* Inst();
    static void CreateInst(const char* cityFile,const char* routeFile, const char* asFile);
    UINT32 numericDistance(UINT32 hashID1, UINT32 hashID2);
    ~Underlay();
    UINT32 GetNumOfAS();
    UINT32 GetNumOfNode();
    void ReadInRoutingTable(const char* routeFile);
    void ReadInASInfo(const char* asFile);//as index, tier, capacity
    void ReadInCityInfo(const char* cityFile);
    void InitializeNetwork(); //assign nodes to each AS based on capacity
    void InitializeWorkload();//create GUID insertion workload
    UINT32 generateWorkload(FLOAT64 mean_arrival, char opt);//opt = 'U'/'Q'
    UINT32 generateLeaveChurn(UINT32 churnLength, FLOAT64 mean_arrival, FLOAT64 sessionTime, UINT32 onOffTimes);
    void SynchNetwork();
    FLOAT64 getLatency(UINT32 src, UINT32 dest);
    UINT32 migrationOverhead4Join(UINT32 nodeIdx);
    UINT32 migrationOverhead4Leave(UINT32 nodeIdx);
    void getNeighbors(UINT32 nodeIdx, set<UINT32> & neighborsIdx_v); // put all online neighbors into neighborsIdx_v
    void determineHost(UINT32 guidIdx, set<UINT32>& _hostNodeIdx, int asIdx);
    UINT32 getIdxRetryCnt(FLOAT64 currTime, bool isDHTretry);
    UINT32 getIdxQueryLatency(FLOAT64 currTime, bool isInsertion);
};

#endif
/*
 Time unit: latency is in ms, mean arrival is # arrivals per hour, so time in stat:: is also per hour 
 */