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
//#include "util.h"
GUID::GUID (UINT32 id, UINT32 asIdx, FLOAT64 time){
    _guid = id;
    _address_asIdx = asIdx;
    _objID = _guid%(Underlay::Inst()->GetNumOfNode()); //_objID -> nodeIdx for the virtual primary host
    _last_update_time = time;
}
GUID::~GUID(){
    
}

bool GUID::operator <(const GUID& guid1){
    return _guid < guid1._guid;
}
UINT32 GUID::GetGUID(){
    return _guid;
}
        
UINT32 GUID::GetobjID(){
    return _objID;
}

FLOAT64 GUID::GetLastUpdateTime (){
    return _last_update_time;
}
        
void GUID::UpdateAddrASIdx(UINT32 newASIdx, FLOAT64 time){
    _address_asIdx = newASIdx;
    _last_update_time = time;
}
        
UINT32 GUID::GetAddrASIdx(){
    return _address_asIdx;
}

Node::Node(UINT32 nodeIdx, UINT32 asIdx, FLOAT64 time){
    _nodeIdx = nodeIdx;
    _asIdx = asIdx;
    _in_service = true;
    _last_update_time = time;
}

Node::~Node(){}

bool Node::operator < (Node node1) {
    return _nodeIdx < node1._nodeIdx;
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

AS::AS(UINT32 asindex, UINT32 tier, UINT32 capacity){
    _asIdx = asindex;
    _tier = tier;
    _capacity = capacity;
}

 AS::~AS(){}
 
 UINT32 AS::getCapacity(){
     return _capacity;
 }
 
 UINT32 AS::getTier(){
     return _tier;
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

GNRSOperationMessage::GNRSOperationMessage(MsgType type, UINT32 asIdx, UINT32 guid, FLOAT64 time) {
    _type = type;
    _asIdx = asIdx;
    _src = asIdx;
    _guid = guid;
    _time_done = _time_created+time;
}
bool GNRSOperationMessage::Callback(){
    if (_type == MT_GUID_INSERTION) {
        GUID aGUID = GUID(_guid, _asIdx, GetTimeDone());
        Underlay::Inst()->global_guid_list.push_back(aGUID);
    }
    if (_type == MT_GUID_UPDATE) {
        Underlay::Inst()->global_guid_list[_guid].UpdateAddrASIdx(_asIdx,GetTimeDone());
    }
    set<UINT32> localHostset;
    set<UINT32> globalHostset;
    Underlay::Inst()->determineHost(_guid, globalHostset);
    Underlay::Inst()->as_v[_asIdx].determineHost(_guid, localHostset);
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
bool AS::insertGUID(UINT32 guid, FLOAT64 time){
    if(isGNRSMember()){
        GNRSOperationMessage * anInsertionMsg = new GNRSOperationMessage (MT_GUID_INSERTION, _asIdx, guid, time);
        EventScheduler::Inst()->AddEvent(anInsertionMsg);
        return true;
    }
    return false;
}

bool AS::updateGUID(UINT32 guid, FLOAT64 time){
    if(isGNRSMember()){
        GNRSOperationMessage * anUpdateMsg = new GNRSOperationMessage (MT_GUID_UPDATE, _asIdx, guid, time);
        EventScheduler::Inst()->AddEvent(anUpdateMsg);
        return true;
    } 
    return false;
}
    
bool AS::queryGUID(UINT32 guid, FLOAT64 time){
    if(isGNRSMember()){
        GNRSOperationMessage * aQueryMsg = new GNRSOperationMessage (MT_GUID_QEURY, _asIdx, guid, time);
        EventScheduler::Inst()->AddEvent(aQueryMsg);
        return true;
    }
    return false;
}
    
void AS::determineHost(UINT32 guid, set<UINT32>& localhostNodeIdx){
    localhostNodeIdx.clear();
    UINT32 vPhostIdx = Underlay::Inst()->global_guid_list[guid].GetobjID();
    if(_local_view_offNodes.find(vPhostIdx) == _local_view_offNodes.end()){
        localhostNodeIdx.insert(vPhostIdx);
    }
    UINT32 total_no_of_nodes = Underlay::Inst()->GetNumOfNode();
    UINT32 leftPtr = vPhostIdx;
    InRangeMinus(leftPtr,total_no_of_nodes);
    UINT32 rightPtr = vPhostIdx;
    InRangePlus(rightPtr, total_no_of_nodes);
    
    while(localhostNodeIdx.size()< Settings::GNRS_K && leftPtr != rightPtr){
        while (_local_view_offNodes.find(rightPtr) != _local_view_offNodes.end()){
            InRangePlus(rightPtr, total_no_of_nodes);
        }
        while (_local_view_offNodes.find(leftPtr) != _local_view_offNodes.end()) {
            InRangeMinus(leftPtr, total_no_of_nodes);
        }
        if(abs((double)leftPtr-(double)vPhostIdx)< abs((double)rightPtr-(double)vPhostIdx)){
            localhostNodeIdx.insert(leftPtr);
            InRangeMinus(leftPtr,total_no_of_nodes);
        }
        else{
            localhostNodeIdx.insert(rightPtr);
            InRangePlus(rightPtr,total_no_of_nodes);
        }
    }
    
    if(localhostNodeIdx.size()< Settings::GNRS_K && leftPtr == rightPtr){
        if(_local_view_offNodes.find(leftPtr) == _local_view_offNodes.end())
            localhostNodeIdx.insert(leftPtr);
    }
    //debug
    /*
    cout<<"local size at AS"<<_asIdx<< "for "<<vPhostIdx<<" is "<<localhostNodeIdx.size()<<":";
    set<UINT32>::iterator it;
    for (it = localhostNodeIdx.begin(); it != localhostNodeIdx.end(); it++) {
        cout<<(*it)<<" ";   
    }
    cout<<endl;
    */
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
 *first line no. of AS
 *as1tier' 'as1capacity' 'as2tier' 'as2capacity' '...  
 */
void Underlay::ReadInASInfo(const char* asFile) {
    ifstream asFileHdlr(asFile);
    assert(asFileHdlr.is_open());
    UINT32 nn;
    asFileHdlr >> nn;
    cout<<"Total no. of AS from as file "<<nn<<endl;
    assert(nn == _num_of_as);
    
    cout << "reading AS tier and capacity " <<endl; 
		
    UINT32 tier;
    UINT32 capacity;
    as_v.clear();
    for (UINT32 i=0; i < nn; i++) { 
        asFileHdlr >> tier;
        asFileHdlr >> capacity;
        AS thisAS (i, tier, capacity);
        //AS thisAS (i, tier, 1);
        as_v.push_back(thisAS);
    }
}

/*
 * assign nodes to AS fulfill capacity
 * turn on all nodes in service
 * Interleave nodes belongs to the same AS
 */
void Underlay::InitializeNetwork(){
    _num_of_node =0;
    assert(as_v.size() == _num_of_as);
    for(int i=0; i<as_v.size(); i++){
        _num_of_node += as_v[i].getCapacity();
        as_v[i]._myNodes.clear();
        as_v[i]._local_view_offNodes.clear();
    }
    global_node_table.clear();
    UINT32 currASIdx =0;
    UINT32 currNodeIdx =0;
    UINT32 loop =0;
    while(currNodeIdx < _num_of_node){
        loop=0;
        while (as_v[currASIdx]._myNodes.size() == as_v[currASIdx].getCapacity()){
            currASIdx ++;
            if(currASIdx >= _num_of_as){
                currASIdx = 0;
                loop++;
                assert(loop<=1);
            }             
        }
        Node currNode(currNodeIdx, currASIdx, EventScheduler::Inst()->GetCurrentTime());
        global_node_table.push_back(currNode);
        as_v[currASIdx]._myNodes.insert(currNodeIdx);
        currNodeIdx++;
        InRangePlus(currASIdx,_num_of_as);
    }
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
    assert(neighborsIdx_v.size()==halfNeighborSize);
    currNodeIdx = nodeIdx;
    while (neighborsIdx_v.size()< (2*halfNeighborSize)){
        InRangePlus(currNodeIdx, _num_of_node);
        if(global_node_table[currNodeIdx].isInService()){
            neighborsIdx_v.insert(currNodeIdx);
        }
    }
    assert(neighborsIdx_v.size()== (2*halfNeighborSize));
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

void Underlay::InitializeWorkload(){
    UINT32 currNodeIdx, currASIdx;
    for(UINT32 i=0; i<Settings::TotalActiveGUID; i++){
        currNodeIdx= Util::Inst()->GenInt(_num_of_node);
        currASIdx = global_node_table[currNodeIdx].getASIdx();
        //as_v[currASIdx].insertGUID(i, 0);
        GUID aGuid(i,currASIdx,EventScheduler::Inst()->GetCurrentTime());
        global_guid_list.push_back(aGuid);
    }
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
            currGUIDIdx = Util::Inst()->GenInt(Settings::TotalActiveGUID);
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

void Underlay::determineHost(UINT32 guid, set<UINT32> & _hostNodeIdx){
    UINT32 vPhostIdx = global_guid_list[guid].GetobjID();
    assert(vPhostIdx>=0 && vPhostIdx<_num_of_node);
    _hostNodeIdx.clear();
    UINT32 leftPtr = vPhostIdx;
    InRangeMinus(leftPtr, _num_of_node); 
    UINT32 rightPtr = vPhostIdx;
    InRangePlus(rightPtr, _num_of_node);
    if(global_node_table[vPhostIdx].isInService())
        _hostNodeIdx.insert(vPhostIdx);
   
    while(_hostNodeIdx.size()< Settings::GNRS_K && leftPtr != rightPtr){
        while (!global_node_table[rightPtr].isInService()){
            InRangePlus(rightPtr, _num_of_node);
        }
        while (! global_node_table[leftPtr].isInService()) {
            InRangeMinus(leftPtr, _num_of_node);
        }
        if(abs((double)leftPtr-(double)vPhostIdx)< abs((double)rightPtr-(double)vPhostIdx)){
            _hostNodeIdx.insert(leftPtr);
            InRangeMinus(leftPtr,_num_of_node);
        }
        else{
            _hostNodeIdx.insert(rightPtr);
            InRangePlus(rightPtr,_num_of_node);
        }
    }
    if(_hostNodeIdx.size()< Settings::GNRS_K && leftPtr == rightPtr){
        if(global_node_table[rightPtr].isInService())
            _hostNodeIdx.insert(rightPtr);
    }
    //debug
    /*
    cout<<"global host size for "<<vPhostIdx<<" is "<<_hostNodeIdx.size()<<":";
    set<UINT32>::iterator it;
    for (it = _hostNodeIdx.begin(); it != _hostNodeIdx.end(); it++) {
        cout<<(*it)<<" ";
    }
    cout<<endl;
    */
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
