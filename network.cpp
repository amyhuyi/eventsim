/* ===============================================================
   Description: Handle the network/transport layers, calculate the 
   transportation time based on bandwidth and latency
   Author: Yi Hu
   =============================================================== */

#include <fstream>
#include <iostream>
#include <cmath>
#include <cstring>
#include <algorithm>
#include "network.h"
#include "event_scheduler.h"
#include "util.h"
#include "genran/str.h"
//#include "util.h"
GUID::GUID (UINT32 id, UINT32 asIdx, FLOAT64 time){
    _guid = id;
    _address_asIdx = asIdx;
    _vphostIdx = Underlay::Inst()->getvpHostIdx(_guid,0,Underlay::Inst()->global_node_table.size()-1);
    _last_update_time = time;
}
GUID::~GUID(){
    
}

UINT32 GUID::getGUID(){
    return _guid;
}
        
UINT32 GUID::getvphostIdx(){
    return _vphostIdx;
}

FLOAT64 GUID::getLastUpdateTime (){
    return _last_update_time;
}
        
void GUID::updateAddrASIdx(UINT32 newASIdx, FLOAT64 time){
    _address_asIdx = newASIdx;
    _last_update_time = time;
}
        
UINT32 GUID::getAddrASIdx(){
    return _address_asIdx;
}

Node::Node(UINT32 hashID, UINT32 asIdx, FLOAT64 time){
    _hashID = hashID;
    _asIdx = asIdx;
    _in_service = true;
    _last_update_time = time;
}

Node::~Node(){}

bool NodeSortPredicate( const Node d1, const Node d2)
{
  return d1._hashID < d2._hashID;
}

bool Node::isInService(){
    return _in_service;
}
void Node::setInService(FLOAT64 time){
    _in_service = true;
    _last_update_time = time;
}
void Node::setOffService(FLOAT64 time){
    _in_service = false;
    _last_update_time = time;
}
UINT32 Node::getASIdx(){
    return _asIdx;
}
UINT32 Node::getHashID(){
    return _hashID;
}

UINT32 Node::getNodeIdx(){
    return _nodeIdx;
}

void Node::setNodeIdx(UINT32 index){
    _nodeIdx = index;
}
AS::AS(UINT32 asindex, UINT32 tier, UINT32 capacity, UINT32 asNum, string asCountry){
    _asIdx = asindex;
    _tier = tier;
    _capacity = capacity;
    _asNum = asNum;
    _asCntry = asCountry;
}

 AS::~AS(){}
 
 UINT32 AS::getASIndex(){
     return _asIdx;
 }
 
 UINT32 AS::getCapacity(){
     return _capacity;
 }
 
 UINT32 AS::getTier(){
     return _tier;
 }
 
 UINT32 AS::getASNum(){
     return _asNum;
 }
 
 string AS::getASCntry(){
     return _asCntry;
 }
 
void AS::beNotifedAjoin(UINT32 nodeIdx){
    if(_local_view_offNodes.find(nodeIdx) != _local_view_offNodes.end()){
        _local_view_offNodes.erase(_local_view_offNodes.find(nodeIdx));
    }
}
void AS::beNotifedAleave(UINT32 nodeIdx){
    _local_view_offNodes.insert(nodeIdx);
}
void AS::joinGNRS(FLOAT64 lifetime){
    set <UINT32>::iterator it;
    for (it=_myNodes.begin(); it!=_myNodes.end(); ++it){
        if(!Underlay::Inst()->global_node_table[(*it)].isInService())
            upgradeGNRS((*it), 0, lifetime,0);
    }
 }   //initialize all nodes in service
 
void AS::leaveGNRS(FLOAT64 offtime){
    set <UINT32>::iterator it;
    for (it=_myNodes.begin(); it!=_myNodes.end(); ++it){
        if(Underlay::Inst()->global_node_table[(*it)].isInService())
            downgradeGNRS((*it), 0, offtime,0);
    }
}   //remove all nodes from service
void AS::upgradeGNRS(UINT32 nodeIdx, FLOAT64 arrival_time,FLOAT64 lifetime, UINT32 churnRounds){
    assert(_myNodes.find(nodeIdx) != _myNodes.end());
    assert(!Underlay::Inst()->global_node_table[nodeIdx].isInService());
    PreJoinMessage * aJoinMsg = new PreJoinMessage (nodeIdx, arrival_time,lifetime, churnRounds);
    EventScheduler::Inst()->AddEvent(aJoinMsg);
}// upgrade GNRS membership: add one of my nodes in service
    
void AS::downgradeGNRS(UINT32 nodeIdx, FLOAT64 arrival_time,FLOAT64 offtime, UINT32 churnRounds){
    assert(_myNodes.find(nodeIdx) != _myNodes.end());
    assert(Underlay::Inst()->global_node_table[nodeIdx].isInService());
    PreLeaveMessage * aLeaveMsg = new PreLeaveMessage (nodeIdx, arrival_time,offtime, churnRounds);
    EventScheduler::Inst()->AddEvent(aLeaveMsg);
} //downgrade GNRS membership: remove one of my nodes from service

FLOAT64 AS::getMaxDistance(vector<UINT32> correctHost){
    assert(correctHost.size());
    FLOAT64 maxDistance;
    for (int i = 0; i < correctHost.size(); i++) {
        if (i==0)
            maxDistance = Underlay::Inst()->getLatency(_asIdx, Underlay::Inst()->global_node_table[correctHost[i]].getASIdx());
        else {
            if(maxDistance < Underlay::Inst()->getLatency(_asIdx, Underlay::Inst()->global_node_table[correctHost[i]].getASIdx()))
            maxDistance = Underlay::Inst()->getLatency(_asIdx, Underlay::Inst()->global_node_table[correctHost[i]].getASIdx());
        }
    }
    return maxDistance;
}

FLOAT64 AS::getMinDistance(vector<UINT32> correctHost){
    assert(correctHost.size());
    FLOAT64 minDistance;
    for (int i = 0; i < correctHost.size(); i++) {
        if (i==0)
            minDistance = Underlay::Inst()->getLatency(_asIdx, Underlay::Inst()->global_node_table[correctHost[i]].getASIdx());
        else {
            if(minDistance > Underlay::Inst()->getLatency(_asIdx, Underlay::Inst()->global_node_table[correctHost[i]].getASIdx()))
            minDistance = Underlay::Inst()->getLatency(_asIdx, Underlay::Inst()->global_node_table[correctHost[i]].getASIdx());
        }
        //debug
        //cout<<_asIdx<<","<<Underlay::Inst()->global_node_table[correctHost[i]].getASIdx()<<","<<Underlay::Inst()->getLatency(_asIdx, Underlay::Inst()->global_node_table[correctHost[i]].getASIdx())<<"**";
    }
    //cout<<minDistance<<endl;
    return minDistance;
}

FLOAT64 AS::calInsertDelay(vector<UINT32> onlyInlocal, vector<UINT32> onlyInglobal, vector<UINT32> correctHost){
    FLOAT64 maxDistance;
    FLOAT64 retryDistance;
    if(onlyInglobal.size()==0){
        assert(correctHost.size());
        maxDistance = getMaxDistance(correctHost);
        return maxDistance*2;
    }
    else{
        if (correctHost.size()) {
            retryDistance = getMinDistance(correctHost)*2;
            retryDistance += getMaxDistance(onlyInglobal)*2;
            //cout<<"retryDistance #3"<<retryDistance<<endl;
        } 
        else {
            assert(onlyInlocal.size()&&onlyInglobal.size());
            retryDistance = getMaxDistance(onlyInlocal)*2;
            retryDistance += getMaxDistance(onlyInglobal)*2;
            retryDistance += (Settings::DHTHop) * getMinDistance(onlyInglobal);
            //cout<<"retryDistance #4"<<retryDistance<<endl;
        }
        //debug
        return retryDistance;
    }
}


FLOAT64 AS::calQueryDelay(vector<UINT32> onlyInlocal, vector<UINT32> onlyInglobal, vector<UINT32> correctHost){
    FLOAT64 minDistance;
    FLOAT64 retryDistance;
    if(onlyInlocal.size()){
        if(correctHost.size() && getMinDistance(onlyInlocal)< getMinDistance(correctHost)){
            retryDistance = getMinDistance(onlyInlocal)*2;
            retryDistance += getMinDistance(correctHost)*2;
            //debug
           // cout<<"retryDistance #1"<<retryDistance<<endl;
            return retryDistance;
        }
        else if(correctHost.size() && getMinDistance(onlyInlocal)>= getMinDistance(correctHost)){
            minDistance = getMinDistance(correctHost);
            return minDistance*2;
        }
        else{
            retryDistance = getMaxDistance(onlyInlocal)*2;
            retryDistance += getMinDistance(onlyInglobal)*2;
            retryDistance += (Settings::DHTHop) * getMinDistance(onlyInglobal);
            //debug
            //cout<<"retryDistance #2"<<retryDistance<<endl;
            return retryDistance;
        }
    }
    else{
        minDistance = getMinDistance(correctHost);
        return minDistance*2;
    }
}
UINT32 Underlay::getIdxQueryLatency(FLOAT64 currTime, bool isInsertion){
    if(isInsertion){
        for (int i = 0; i < Stat::Insertion_latency_time.size(); i++) {
            if (Stat::Insertion_latency_time[i]._time == currTime)
                return i;
        }
    }
    else{
        for (int i = 0; i < Stat::Query_latency_time.size(); i++) {
            if(Stat::Query_latency_time[i]._time == currTime)
                return i;
        }
    }
    Query_Latency aQueryLatency;
    aQueryLatency._time = currTime;
    aQueryLatency._delay_v.clear();
    if(isInsertion){
        Stat::Insertion_latency_time.push_back(aQueryLatency);
        return (Stat::Insertion_latency_time.size()-1);
    }
    else{
        Stat::Query_latency_time.push_back(aQueryLatency);
        return (Stat::Query_latency_time.size()-1);
    }
}

UINT32 Underlay::getIdxRetryCnt(FLOAT64 currTime, bool isDHTretry){
    if(isDHTretry){
        for(int i=0; i<Stat::DHT_RetryCnt.size(); i++){
            if (Stat::DHT_RetryCnt[i]._time == currTime) {
                return i;
            }
        }
    }
    else {
        for(int i=0; i<Stat::Retry_Cnt.size(); i++){
            if (Stat::Retry_Cnt[i]._time == currTime) {
                return i;
            }
        }
    }
    Retry_Count aRetryCount;
    aRetryCount._time = currTime;
    aRetryCount._retryQMsg =0;
    aRetryCount._retryQuery=0;
    aRetryCount._retryUMsg=0;
    aRetryCount._retryUpdate=0;
    aRetryCount._issuedQuery=0;
    aRetryCount._issuedUpdate=0;
    aRetryCount._Qdelay.clear();
    aRetryCount._Udelay.clear();
    if(isDHTretry){
        Stat::DHT_RetryCnt.push_back(aRetryCount);
        return (Stat::DHT_RetryCnt.size()-1);
    }
    else {
        Stat::Retry_Cnt.push_back(aRetryCount);
        return (Stat::Retry_Cnt.size()-1);
    }
}
/*
 * 1. calculate correct host 2. updated local view from the correct host of this operation
 * 3. statistics on delay and retry no. 
 */
void AS::calCorrectHost(set<UINT32> localHostset, set<UINT32> globalHostset, char opt){
    assert(opt=='I'||opt=='Q'||opt=='U');
    //find correct host
    set<UINT32>::iterator it;
    vector<UINT32> correctHost;
    correctHost.clear();
    vector<UINT32> onlyInlocal;
    vector<UINT32> onlyInglobal;
    for(it=localHostset.begin();it!=localHostset.end();++it){
        if(globalHostset.find((*it))!=globalHostset.end())
            correctHost.push_back((*it));
        else
            onlyInlocal.push_back((*it));
    }
    for (it = globalHostset.begin(); it != globalHostset.end(); ++it) {
        if (localHostset.find((*it))== localHostset.end())
            onlyInglobal.push_back((*it));
    }
    
    FLOAT64 currDelay=-1;
    if (opt == 'I' || opt == 'U') {
        currDelay = calInsertDelay(onlyInlocal, onlyInglobal, correctHost);
    } else {
        currDelay = calQueryDelay(onlyInlocal, onlyInglobal, correctHost);
    }
    //statistics accounting on retry
    bool shouldInsertRetryCnt = false;
    UINT32 currRetryMsg=0;
    UINT32 idxRetryCnt=0;
    if(correctHost.size()==0){
        idxRetryCnt = Underlay::Inst()->getIdxRetryCnt(EventScheduler::Inst()->GetCurrentTime(), true);
        if(opt == 'I' || opt == 'U' ){
            Stat::DHT_RetryCnt[idxRetryCnt]._retryUpdate++;
            Stat::DHT_RetryCnt[idxRetryCnt]._Udelay.push_back(currDelay);
        }
        else if(opt == 'Q'){
            Stat::DHT_RetryCnt[idxRetryCnt]._retryQuery++;
            Stat::DHT_RetryCnt[idxRetryCnt]._Qdelay.push_back(currDelay);
        }    
        shouldInsertRetryCnt=true;
    }    
    if (correctHost.size() < localHostset.size()) {
        if(opt == 'I' || opt == 'U' || (opt == 'Q'&& correctHost.size()==0)){
            currRetryMsg = (localHostset.size()- correctHost.size());
            shouldInsertRetryCnt =true;
        }
        else if(opt == 'Q' && getMinDistance(onlyInlocal)< getMinDistance(correctHost)){
            currRetryMsg = localHostset.size()-1; //assume query one nearest host fail, retry all the other available host
            shouldInsertRetryCnt =true;
        }
    }
    if(shouldInsertRetryCnt){
        idxRetryCnt = Underlay::Inst()->getIdxRetryCnt(EventScheduler::Inst()->GetCurrentTime(), false);
        if(opt == 'I'|| opt == 'U'){
            Stat::Retry_Cnt[idxRetryCnt]._Udelay.push_back(currDelay);
            Stat::Retry_Cnt[idxRetryCnt]._retryUpdate++;
            Stat::Retry_Cnt[idxRetryCnt]._retryUMsg += currRetryMsg;
        }
        else if(opt == 'Q'){
            Stat::Retry_Cnt[idxRetryCnt]._Qdelay.push_back(currDelay);
            Stat::Retry_Cnt[idxRetryCnt]._retryQuery++;
            Stat::Retry_Cnt[idxRetryCnt]._retryQMsg += currRetryMsg;
        }
        //update local view of node table
        for (int i = 0; i < onlyInlocal.size(); i++) {
            assert(_local_view_offNodes.find(onlyInlocal[i]) == _local_view_offNodes.end());
            if(Underlay::Inst()->global_node_table[onlyInlocal[i]].isInService()==false)
                _local_view_offNodes.insert(onlyInlocal[i]);
            }
        for (int i = 0; i < onlyInglobal.size(); i++) {
            assert(Underlay::Inst()->global_node_table[onlyInglobal[i]].isInService());
            if(_local_view_offNodes.find(onlyInglobal[i])!= _local_view_offNodes.end())
                _local_view_offNodes.erase(_local_view_offNodes.find(onlyInglobal[i]));
            }
    }
    //latency statistics accounting
    UINT32 idxLatency=0;
    if (opt == 'I' || opt == 'U') {
        idxLatency = Underlay::Inst()->getIdxQueryLatency(EventScheduler::Inst()->GetCurrentTime(), true);
        Stat::Insertion_latency_time[idxLatency]._delay_v.push_back(currDelay);
    } 
    else {
        idxLatency = Underlay::Inst()->getIdxQueryLatency(EventScheduler::Inst()->GetCurrentTime(), false);
        Stat::Query_latency_time[idxLatency]._delay_v.push_back(currDelay);
    }
}

GNRSOperationMessage::GNRSOperationMessage(MsgType type, UINT32 asIdx, UINT32 guidIdx, FLOAT64 time) {
    _type = type;
    _asIdx = asIdx;
    _src = asIdx;
    _guidIdx = guidIdx;
    _time_done = _time_created+time;
}
bool GNRSOperationMessage::Callback(){
    if (_type == MT_GUID_UPDATE || _type == MT_GUID_INSERTION) {
        Underlay::Inst()->global_guid_list[_guidIdx].updateAddrASIdx(_asIdx,GetTimeDone());
    }
    set<UINT32> localHostset; // local AS determined host set
    set<UINT32> globalHostset; //global determined host set
    Underlay::Inst()->determineHost(_guidIdx, globalHostset,-1);
    Underlay::Inst()->determineHost(_guidIdx, localHostset,_asIdx);
    if (_type == MT_GUID_INSERTION || _type == MT_GUID_UPDATE) {
        Underlay::Inst()->as_v[_asIdx].calCorrectHost(localHostset,globalHostset,'I');
    } else {
        Underlay::Inst()->as_v[_asIdx].calCorrectHost(localHostset,globalHostset,'Q');
    }
    return true;
}
bool AS::isGNRSMember(){
    set<UINT32>::iterator it;
    for(it=_myNodes.begin();it!=_myNodes.end(); it++)
        if(Underlay::Inst()->global_node_table[(*it)].isInService())
            return true;
    return false;
}
bool AS::insertGUID(UINT32 guidIdx, FLOAT64 time){
    if(isGNRSMember()){
        GNRSOperationMessage * anInsertionMsg = new GNRSOperationMessage (MT_GUID_INSERTION, _asIdx, guidIdx, time);
        EventScheduler::Inst()->AddEvent(anInsertionMsg);
        return true;
    }
    return false;
}

bool AS::updateGUID(UINT32 guidIdx, FLOAT64 time){
    if(isGNRSMember()){
        GNRSOperationMessage * anUpdateMsg = new GNRSOperationMessage (MT_GUID_UPDATE, _asIdx, guidIdx, time);
        EventScheduler::Inst()->AddEvent(anUpdateMsg);
        return true;
    } 
    return false;
}
    
bool AS::queryGUID(UINT32 guidIdx, FLOAT64 time){
    if(isGNRSMember()){
        GNRSOperationMessage * aQueryMsg = new GNRSOperationMessage (MT_GUID_QEURY, _asIdx, guidIdx, time);
        EventScheduler::Inst()->AddEvent(aQueryMsg);
        return true;
    }
    return false;
}

Underlay* Underlay::_underlay_ptr = NULL;

Underlay* Underlay::Inst() {
	assert (_underlay_ptr != NULL);
	return _underlay_ptr;
}

void Underlay::CreateInst(const char* routeFile, const char* asFile) {
	assert( _underlay_ptr == NULL );
	_underlay_ptr = new Underlay(routeFile, asFile);
}

UINT32 Underlay::GetNumOfAS() {
    return _num_of_as;
}
UINT32 Underlay::GetNumOfNode() {
    return _num_of_node;
}

Underlay::Underlay(const char* routeFile, const char* asFile) {
    ReadInRoutingTable(routeFile);
    ReadInASInfo(asFile);
    InitializeNetwork();
}

Underlay::~Underlay() {}

/*
 use ExportRouteFile format from DJ.cpp
 */
void Underlay::ReadInRoutingTable(const char* routeFile) {
    ifstream routeFileHdlr(routeFile);
    assert(routeFileHdlr.is_open());
    
    routeFileHdlr >> _num_of_as;
    cout<<"Total no. of AS from route file "<<_num_of_as<<endl;
    UINT32 nn = _num_of_as;
    as_dist_matx = (UINT32*)malloc(sizeof(UINT32) * nn* nn); 
    memset(as_dist_matx, 0, sizeof(UINT32) * nn * nn);		
    cout << "reading Routing table - Distance " <<endl; 	
    UINT32 dist_curr; 
    for (int i=0; i < nn; i++) { //Read distance matrix 
	for (int j=0; j < nn; j++){
            routeFileHdlr >> dist_curr; 
            as_dist_matx[i*nn + j] = dist_curr;
            //debug
            //cout<<"as "<<i<<" to as "<<j<<" delay: "<<(float)as_dist_matx[i*nn + j]/(float)1000<<endl;
	}
    }
    //cout<<endl;
    /*
    as_pre_matx = (UINT32*) malloc(sizeof(UINT32) * nn * nn );
    memset(as_pre_matx, 0, sizeof(UINT32) * nn * nn);
    cout << "reading Routing table - Predicate " << endl; 
    UINT32 pred_curr;
    for (int i=0; i < nn; i++) { //Read Predicate matrix  
	for (int j=0; j < nn; j++){
            routeFileHdlr >>  pred_curr; 
            as_pre_matx[i*nn + j] =  pred_curr; 	
	}
    }
    */
}

/*
 *first line no.ofFullAS' 'no.ofSubAS
 *as1tier' 'as1SubASno' 'pop#as1Subas1' 'pop#as1Subas2' '...
 *as2tier' 'as2SubASno' 'pop#as2Subas1' 'pop#as2Subas2' '...
 */
void Underlay::ReadInASInfo(const char* asFile) {
    ifstream asFileHdlr(asFile);
    assert(asFileHdlr.is_open());
    UINT32 nofullAS, noAS;
    asFileHdlr >> nofullAS;
    asFileHdlr >> noAS;
    cout<<"Total no. of AS from as file "<<noAS<<" Total no. of full AS"<<nofullAS<<endl;
    assert(noAS == _num_of_as);
    cout << "reading AS tier and capacity " <<endl; 
		
    UINT32 tier;
    UINT32 capacity;
    string country;
    UINT32 currSubASno;
    as_v.clear();
    for (UINT32 i=0; i < nofullAS; i++) { 
        asFileHdlr >> tier;
        asFileHdlr >> currSubASno;
        for (int j = 0; j < currSubASno; j++) {
            asFileHdlr>>capacity;
            asFileHdlr>>country;
            AS thisAS(as_v.size(), tier, capacity, i, country);
            as_v.push_back(thisAS);
        }
    }
}

/*
 * assign nodes to AS fulfill capacity
 * turn on all nodes in service
 * assign node hashID from gnrs hash space
 * sort global node table on node's hashID
 */
void Underlay::InitializeNetwork(){
    _num_of_node =0;
    global_node_table.clear();
    assert(as_v.size() == _num_of_as);
    UINT32 currHashID;
    set<UINT32> assignedHashID;
    
    for(int i=0; i<as_v.size(); i++){
        _num_of_node += as_v[i].getCapacity();
        as_v[i]._myNodes.clear();
        as_v[i]._local_view_offNodes.clear();
        for (int j = 0; j < as_v[i].getCapacity(); j++) {
            currHashID = Util::Inst()->GenInt(GNRS_HASH_RANGE);
            while (assignedHashID.find(currHashID) != assignedHashID.end()) {
                currHashID = Util::Inst()->GenInt(GNRS_HASH_RANGE);
            }
            Node currNode(currHashID, i, EventScheduler::Inst()->GetCurrentTime());
            global_node_table.push_back(currNode);
            assignedHashID.insert(currHashID);
        }

    }
    
    sort(global_node_table.begin(), global_node_table.end(),NodeSortPredicate);
    for (int i = 0; i < global_node_table.size(); i++) {
        global_node_table[i].setNodeIdx(i);
        as_v[global_node_table[i].getASIdx()]._myNodes.insert(i);
    }
}
/* insert activeGUIDno. of guid
 */
void Underlay::InitializeWorkload(){
    UINT32 currNodeIdx, currASIdx;
    set<UINT32> assignedGUID;
    UINT32 currGUID;
    for(UINT32 i=0; i<Settings::TotalActiveGUID; i++){
        currNodeIdx= Util::Inst()->GenInt(_num_of_node);
        currASIdx = global_node_table[currNodeIdx].getASIdx();
        currGUID = Util::Inst()->GenInt(GNRS_HASH_RANGE);
        while (assignedGUID.find(currGUID) != assignedGUID.end()) {
            currGUID = Util::Inst()->GenInt(GNRS_HASH_RANGE);
        }
        GUID aGuid(currGUID,currASIdx,EventScheduler::Inst()->GetCurrentTime());
        global_guid_list.push_back(aGuid);
        assignedGUID.insert(currGUID);
    }
}
UINT32 Underlay::numericDistance(UINT32 hashID1, UINT32 hashID2){
    if (hashID1 > hashID2)
        return (hashID1 - hashID2);
    else
        return (hashID2 - hashID1);
}

/*
 * default neighbor size is the minimal neighbor size K+1
 * larger neighbor size is set through Settings:NeighborSize
 */
void Underlay::getNeighbors(UINT32 nodeIdx, set<UINT32> & neighborsIdx_v){
    UINT32 currNodeIdx = nodeIdx;
    neighborsIdx_v.clear();
    UINT32 halfNeighborSize = ceil((double)(Settings::GNRS_K/2));
    if(halfNeighborSize < (Settings::NeighborSize/2))
        halfNeighborSize = Settings::NeighborSize/2;
    while (neighborsIdx_v.size()< halfNeighborSize){
        InRangeMinus(currNodeIdx, _num_of_node);
        if(global_node_table[currNodeIdx].isInService()){
            neighborsIdx_v.insert(currNodeIdx);
        }
    }
    //assert(neighborsIdx_v.size()==halfNeighborSize);
    currNodeIdx = nodeIdx;
    while (neighborsIdx_v.size()< (2*halfNeighborSize)){
        InRangePlus(currNodeIdx, _num_of_node);
        if(global_node_table[currNodeIdx].isInService()){
            neighborsIdx_v.insert(currNodeIdx);
        }
    }
    //assert(neighborsIdx_v.size()== (2*halfNeighborSize));
}

/*
 */
UINT32 Underlay::migrationOverhead4Join(UINT32 nodeIdx){
    UINT32 singleShare = Settings::TotalVirtualGUID / _num_of_node;
    UINT32 noOfoffline = 0;
    //ToDo: count no of offline neighbors spread among my life neighbors
    return singleShare;
}

UINT32 Underlay::migrationOverhead4Leave(UINT32 nodeIdx){
    UINT32 singleShare = Settings::TotalVirtualGUID / _num_of_node;
    UINT32 noOfoffline = 0;
    //ToDo: count no of offline neighbors spread among my life neighbors
    return singleShare;
}

/*
 generate update or query workload, insert is done at network initialization
 */
UINT32 Underlay::generateWorkload(UINT32 workloadLength, FLOAT64 mean_arrival, char opt){
    assert(opt == 'U' || opt == 'Q');
    UINT32 currGUIDIdx, currNodeIdx, currASIdx;
    UINT32 currRound =0, generatedCnt =0;
    
    UINT32 currBeginTime = 0;
    Util::Inst() ->ResetPoisson(mean_arrival);
    while (currBeginTime<workloadLength) {
        currRound = (UINT32)Util::Inst()->GenPoisson();
        int i=0;
        while(i<currRound){
            currGUIDIdx = Util::Inst()->GenInt(global_guid_list.size());
            currNodeIdx = Util::Inst()->GenInt(_num_of_node);
            currASIdx = global_node_table[currNodeIdx].getASIdx();
            if (opt == 'Q'&& as_v[currASIdx].queryGUID(currGUIDIdx, currBeginTime)) {
                //cout<<"AS "<< currASIdx<<" GUID"<<currGUIDIdx<<endl;
                i++;
            } else if (opt == 'U' && as_v[currASIdx].updateGUID(currGUIDIdx, currBeginTime)){
                i++;
            }
        }
        UINT32 idxDHTCnt = Underlay::Inst()->getIdxRetryCnt(EventScheduler::Inst()->GetCurrentTime(), true);
        UINT32 idxRetryCnt = Underlay::Inst()->getIdxRetryCnt(EventScheduler::Inst()->GetCurrentTime(), false);
        if(opt == 'I' || opt == 'U' ){
            Stat::DHT_RetryCnt[idxDHTCnt]._issuedUpdate = currRound;
            Stat::Retry_Cnt[idxRetryCnt]._issuedUpdate = currRound;
        }
        else if(opt == 'Q'){
            Stat::DHT_RetryCnt[idxDHTCnt]._issuedQuery = currRound;
            Stat::Retry_Cnt[idxRetryCnt]._issuedQuery = currRound;
        } 
        currBeginTime++;
        generatedCnt += currRound;
    }
    return generatedCnt;
}

/*
 *churnLength: # of hours this churn lasts; mean_arrival: # of arrival per hour
 *sessionTime: # of hours on/off; onOffTimes: # of times the node switch between
 *return the total no. of churns generated
 */
UINT32 Underlay::generateLeaveChurn(UINT32 churnLength, FLOAT64 mean_arrival, FLOAT64 sessionTime, UINT32 onOffTimes){
    UINT32 currNodeIdx, currASIdx;
    
    vector<UINT32> candidate_v;
    Util::Inst()->ResetPoisson(mean_arrival);
    for(UINT32 i=0; i<global_node_table.size();i++){
        if(global_node_table[i].isInService())
            candidate_v.push_back(i);
    }
    UINT32 currRound =0, generatedCnt =0;
    UINT32 currArrivalTime = 0;
    while(currArrivalTime<churnLength && candidate_v.size()){
        currRound = (UINT32)Util::Inst()->GenPoisson();
        int i=0;
        while(i<currRound){
            i++;
            currNodeIdx = Util::Inst()->GenInt(candidate_v.size());
            currASIdx = global_node_table[candidate_v[currNodeIdx]].getASIdx();
            as_v[currASIdx].downgradeGNRS(candidate_v[currNodeIdx],(FLOAT64)currArrivalTime,sessionTime,onOffTimes);
            candidate_v.erase(candidate_v.begin()+currNodeIdx);
        }
        currArrivalTime++;
        generatedCnt =+ currRound;
    }
    return generatedCnt;
}

FLOAT64 Underlay::getLatency(UINT32 src, UINT32 dest){
    assert(src>=0 && src<_num_of_as && dest >=0 && dest < _num_of_as);
    return (FLOAT64)((FLOAT64)as_dist_matx[src*_num_of_as+dest]/(FLOAT64)1000); //change us to ms
}
bool Underlay::isAvailable(UINT32 nodeIdx, int asIdx){
    assert(nodeIdx >=0 && nodeIdx < global_node_table.size());
    if(asIdx == -1){
        return global_node_table[nodeIdx].isInService();
    }
    else{
        assert(asIdx >=0 && asIdx < as_v.size());
        if(as_v[asIdx]._local_view_offNodes.find(nodeIdx) == as_v[asIdx]._local_view_offNodes.end())
            return true;
        else
            return false;
    }
}
//find the local replica host -- numerically nearest nodes online and belong to insertion AS
void Underlay::determineLocalHost(UINT32 guidIdx, set<UINT32>& _hostNodeIdx, int asIdx){
    set<UINT32> localNodes = as_v[global_guid_list[guidIdx].getAddrASIdx()]._myNodes;
    set<UINT32>::iterator it;
    UINT32 minDistance;
    bool calculated=false;
    UINT32 localhostIdx;
    for(it= localNodes.begin(); it!=localNodes.end(); it++){
        if(!calculated  && isAvailable((*it), asIdx)){
            minDistance = numericDistance(global_node_table[(*it)].getHashID(),global_guid_list[guidIdx].getGUID());
            calculated = true;
            localhostIdx = (*it);
        }
        else if (calculated  && isAvailable((*it), asIdx)){
            if(numericDistance(global_node_table[(*it)].getHashID(),global_guid_list[guidIdx].getGUID())< minDistance){
                minDistance = numericDistance(global_node_table[(*it)].getHashID(),global_guid_list[guidIdx].getGUID());
                localhostIdx = (*it);
            }
        }
    }
    _hostNodeIdx.insert(localhostIdx);
    //debug
    cout<<"guid= "<<global_guid_list[guidIdx].getGUID()<<"from asIdx "<<asIdx<<" 's view local ASIdx = " << global_guid_list[guidIdx].getAddrASIdx()
            <<" selected local host "<<global_node_table[localhostIdx].getHashID()<<" all local nodes: "<<endl;
    for(it= localNodes.begin(); it!=localNodes.end(); it++){
        cout<<global_node_table[(*it)].getHashID()<<" " 
            << numericDistance(global_node_table[(*it)].getHashID(),global_guid_list[guidIdx].getGUID())<<",";
    }
    cout<<endl;
    cout<<"minDistance"<<minDistance<<endl;
}
void Underlay::determineHostNoRegional(UINT32 guidIdx, set<UINT32>& _hostNodeIdx, int asIdx){
    UINT32 currGUID = global_guid_list[guidIdx].getGUID();
    UINT32 currASIdx = global_guid_list[guidIdx].getAddrASIdx();
    UINT32 assingedGlobal=0;
    UINT32 vPhostIdx = global_guid_list[guidIdx].getvphostIdx();
    assert(vPhostIdx>=0 && vPhostIdx<_num_of_node);
    UINT32 leftPtr = vPhostIdx;
    InRangeMinus(leftPtr, _num_of_node); 
    UINT32 rightPtr = vPhostIdx;
    InRangePlus(rightPtr, _num_of_node);
    if(isAvailable(vPhostIdx, asIdx)){
        _hostNodeIdx.insert(vPhostIdx);
        assingedGlobal++;
    }
    UINT32 currCandidateIdx;
    UINT32 scanned=1;
    while(assingedGlobal<Settings::GNRS_K && scanned<=_num_of_node){
        if(!isAvailable(rightPtr, asIdx)){
            InRangePlus(rightPtr, _num_of_node);
            scanned++;
            continue;
        }
        if(!isAvailable(leftPtr, asIdx)){
            InRangeMinus(leftPtr, _num_of_node);
            scanned++;
            continue;
        }
        if(Settings::Local_K && global_node_table[rightPtr].getASIdx() == currASIdx){
            InRangePlus(rightPtr, _num_of_node);
            scanned++;
            continue;
        }
        if(Settings::Local_K && global_node_table[leftPtr].getASIdx() == currASIdx){
            InRangeMinus(leftPtr, _num_of_node);
            scanned++;
            continue;
        }
        if(numericDistance(global_node_table[leftPtr].getHashID(),currGUID) < 
                numericDistance(global_node_table[rightPtr].getHashID(),currGUID)){
            currCandidateIdx = leftPtr;
            InRangeMinus(leftPtr,_num_of_node);
            scanned++;
        }
        else{
            currCandidateIdx = rightPtr;
            InRangePlus(rightPtr,_num_of_node);
            scanned++;
        }
        _hostNodeIdx.insert(currCandidateIdx);
        assingedGlobal++;
    }
    //debug 
    cout<<"No-regional"<<asIdx<<"'s view determined host size for "<<currGUID<<" is "<<_hostNodeIdx.size()<<":";
    set<UINT32>::iterator it;
    for (it = _hostNodeIdx.begin(); it != _hostNodeIdx.end(); it++) {
        cout<<global_node_table[(*it)].getHashID()<<" "<<as_v[global_node_table[(*it)].getASIdx()].getASCntry()<<" ";
    }
    cout<<endl;
}
void Underlay::determineNonLocalHost(UINT32 guidIdx, set<UINT32>& _hostNodeIdx, int asIdx){
    string currCntry = as_v[global_guid_list[guidIdx].getAddrASIdx()].getASCntry();
    UINT32 currASIdx = global_guid_list[guidIdx].getAddrASIdx();
    UINT32 currGUID = global_guid_list[guidIdx].getGUID();
    UINT32 assingedRegional =0;
    UINT32 assingedGlobal=0;
    UINT32 vPhostIdx = global_guid_list[guidIdx].getvphostIdx();
    assert(vPhostIdx>=0 && vPhostIdx<_num_of_node);
    UINT32 leftPtr = vPhostIdx;
    InRangeMinus(leftPtr, _num_of_node); 
    UINT32 rightPtr = vPhostIdx;
    InRangePlus(rightPtr, _num_of_node);
    if(isAvailable(vPhostIdx, asIdx) && global_node_table[vPhostIdx].getASIdx()!= currASIdx){
        if(as_v[global_node_table[vPhostIdx].getASIdx()].getASCntry() == currCntry){
            if(assingedRegional<Settings::Regional_K){
                _hostNodeIdx.insert(vPhostIdx);
                assingedRegional++;
            }
        }
        else{
            if(assingedGlobal<Settings::GNRS_K){
                _hostNodeIdx.insert(vPhostIdx);
                assingedGlobal++;
            }
        }
    }
    UINT32 currCandidateIdx;
    UINT32 scanned=1;
    while((assingedRegional<Settings::Regional_K || assingedGlobal<Settings::GNRS_K) 
            && scanned<=_num_of_node){
        if(!isAvailable(rightPtr, asIdx) || global_node_table[rightPtr].getASIdx() == currASIdx){
            InRangePlus(rightPtr, _num_of_node);
            scanned++;
            continue;
        }
        if(!isAvailable(leftPtr, asIdx)|| global_node_table[leftPtr].getASIdx() == currASIdx){
            InRangeMinus(leftPtr, _num_of_node);
            scanned++;
            continue;
        }
        if(numericDistance(global_node_table[leftPtr].getHashID(),currGUID) < 
                numericDistance(global_node_table[rightPtr].getHashID(),currGUID)){
            currCandidateIdx = leftPtr;
            InRangeMinus(leftPtr,_num_of_node);
            scanned++;
        }
        else{
            currCandidateIdx = rightPtr;
            InRangePlus(rightPtr,_num_of_node);
            scanned++;
        }
        if(as_v[global_node_table[currCandidateIdx].getASIdx()].getASCntry() == currCntry) {
            if(assingedRegional < Settings::Regional_K){
                _hostNodeIdx.insert(currCandidateIdx);
                assingedRegional++;
            }  
        }
        else if(assingedGlobal < Settings::GNRS_K){
            _hostNodeIdx.insert(currCandidateIdx);
            assingedGlobal++;
        }
    }
    //debug 
    cout<<"from AS"<<asIdx<<"'s view determined host size for "<<currGUID<<" is "<<_hostNodeIdx.size()<<":";
    set<UINT32>::iterator it;
    for (it = _hostNodeIdx.begin(); it != _hostNodeIdx.end(); it++) {
        cout<<global_node_table[(*it)].getHashID()<<" "<<as_v[global_node_table[(*it)].getASIdx()].getASCntry()<<" ";
    }
    cout<<endl;
    cout<<currASIdx<<","<<currCntry<<endl;
}

void Underlay::determineHost(UINT32 guidIdx, set<UINT32> & _hostNodeIdx, int asIdx){
    _hostNodeIdx.clear();
    if(Settings::Local_K){
        determineLocalHost(guidIdx,_hostNodeIdx, asIdx);
    }
    if(Settings::Regional_K){
        determineNonLocalHost(guidIdx,_hostNodeIdx, asIdx);
    }
    else{
        determineHostNoRegional(guidIdx,_hostNodeIdx,asIdx);
    }
}

void Underlay::SynchNetwork(){
    set <UINT32> curr_offNodes;
    for (int i = 0; i < global_node_table.size(); i++) {
        if(!global_node_table[i].isInService())
            curr_offNodes.insert(i);
    }
    for (int i = 0; i < as_v.size(); i++) {
        as_v[i]._local_view_offNodes=curr_offNodes;
    }
}
/* binary search the nearest hashID of the guid, return its index in the global node table
 */
UINT32 Underlay::getvpHostIdx(UINT32 guid, UINT32 start_idx, UINT32 end_idx){
    if(global_node_table[0].getHashID() > guid){
       return start_idx;
    }
    if(global_node_table[global_node_table.size()-1].getHashID() < guid){
       return end_idx;
    }
    assert(start_idx>= 0 && start_idx<global_node_table.size()
            && end_idx >= 0 && end_idx<global_node_table.size());
    if (start_idx > end_idx){
        if(numericDistance(global_node_table[start_idx].getHashID(), guid)<
                numericDistance(global_node_table[end_idx].getHashID(), guid)){
            return start_idx;
        }
        else
            return end_idx;
    }
    
    UINT32 middle_idx = (start_idx + end_idx)/2;
    if(global_node_table[middle_idx].getHashID() == guid){
        return middle_idx;
    }
    else if (global_node_table[middle_idx].getHashID() < guid){
        if(global_node_table[middle_idx+1].getHashID() > guid){
            return (getvpHostIdx(guid, middle_idx+1, middle_idx));
        }
        else 
            return (getvpHostIdx(guid, middle_idx+1, end_idx));
    }
    else {
        if(global_node_table[middle_idx-1].getHashID() < guid){
            return (getvpHostIdx(guid, middle_idx, middle_idx-1));
        }
        else 
            return (getvpHostIdx(guid, start_idx, middle_idx-1));
    }
    /*int minDistance;
    UINT32 vphostIdx;
    assert(global_node_table.size());
    minDistance = numericDistance(global_node_table[0].getHashID(), guid);
    vphostIdx=0;
    for(int i=1;i<global_node_table.size();i++){
        if(numericDistance(global_node_table[i].getHashID(), guid)<minDistance){
            minDistance = numericDistance(global_node_table[i].getHashID(), guid);
            vphostIdx =i;
        }
    }
    return vphostIdx;*/
}