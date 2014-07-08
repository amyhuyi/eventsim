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

CITY::CITY(string city, string state, string country, FLOAT32 lat, FLOAT32 lon, FLOAT32 pop, UINT32 ixp_weight){
    _city = city;
    _state = state;
    _country = country;
    _latitude = lat;
    _longitude = lon;
    _population = pop;
    _ixp_weight = ixp_weight;
    _nodeIdx_v.clear();
}

CITY::~CITY(){
    
}

string CITY::getCity(){
    return _city;
}

string CITY::getState(){
    return _state;
}

string CITY::getCountry(){
    return _country;
}

FLOAT32 CITY::getLat(){
    return _latitude;
}

FLOAT32 CITY::getLon(){
    return _longitude;
}

FLOAT32 CITY::getPop(){
    return _population;
}
UINT32 CITY::getIXPWeight(){
    return _ixp_weight;
}
bool CITY::isGW(){
    return (_ixp_weight>0);
}

GUID::GUID (UINT32 id, UINT32 nodeIdx, FLOAT64 time){
    _guid = id;
    _address_nodeIdx = nodeIdx;
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
        
void GUID::updateAddrNodeIdx(UINT32 newNodeIdx, FLOAT64 time){
    _address_nodeIdx = newNodeIdx;
    _last_update_time = time;
}
        
UINT32 GUID::getAddrNodeIdx(){
    return _address_nodeIdx;
}

UINT32 GUID::getAddrASIdx(){
    return Underlay::Inst()->global_node_table[_address_nodeIdx].getASIdx();
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

UINT32 Node::getCityIdx() {
    return _cityIdx;
}

void Node::setCityIdx(UINT32 index){
    _cityIdx = index;
}

void Node::setNodeIdx(UINT32 index){
    _nodeIdx = index;
}


bool Node::insertGUID(UINT32 guidIdx, FLOAT64 time){
    if(isInService()){
        GNRSOperationMessage * anInsertionMsg = new GNRSOperationMessage (MT_GUID_INSERTION, _nodeIdx, guidIdx, time);
        EventScheduler::Inst()->AddEvent(anInsertionMsg);
        return true;
    }
    return false;
}

bool Node::updateGUID(UINT32 guidIdx, FLOAT64 time){
    if(isInService()){
        GNRSOperationMessage * anUpdateMsg = new GNRSOperationMessage (MT_GUID_UPDATE, _nodeIdx, guidIdx, time);
        EventScheduler::Inst()->AddEvent(anUpdateMsg);
        return true;
    } 
    return false;
}
    
bool Node::queryGUID(UINT32 guidIdx, FLOAT64 time){
    if(isInService()){
        GNRSOperationMessage * aQueryMsg = new GNRSOperationMessage (MT_GUID_QEURY, _nodeIdx, guidIdx, time);
        EventScheduler::Inst()->AddEvent(aQueryMsg);
        return true;
    }
    return false;
}

AS::AS(UINT32 asindex, UINT32 tier, UINT32 capacity, UINT32 asNum, string asCountry){
    _asIdx = asindex;
    _tier = tier;
    _capacity = capacity;
    _asNum = asNum;
    _asCntry = asCountry;
    _myCities.clear();
    _myNodes.clear();
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

GNRSOperationMessage::GNRSOperationMessage(MsgType type, UINT32 nodeIdx, UINT32 guidIdx, FLOAT64 time) {
    _type = type;
    _nodeIdx = nodeIdx;
    _src = Underlay::Inst()->global_node_table[_nodeIdx].getASIdx();
    _guidIdx = guidIdx;
    _time_done = _time_created+time;
}
bool GNRSOperationMessage::Callback(){
    if (_type == MT_GUID_UPDATE || _type == MT_GUID_INSERTION) {
        Underlay::Inst()->global_guid_list[_guidIdx].updateAddrNodeIdx(_nodeIdx,GetTimeDone());
    }
    set<UINT32> localHostset; // local AS determined host set
    set<UINT32> globalHostset; //global determined host set
    Underlay::Inst()->determineHost(_guidIdx, globalHostset,-1);
    Underlay::Inst()->determineHost(_guidIdx, localHostset,_src);
    if (_type == MT_GUID_INSERTION || _type == MT_GUID_UPDATE) {
        Underlay::Inst()->as_v[_src].calCorrectHost(localHostset,globalHostset,'I');
    } else {
        Underlay::Inst()->as_v[_src].calCorrectHost(localHostset,globalHostset,'Q');
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


Underlay* Underlay::_underlay_ptr = NULL;

Underlay* Underlay::Inst() {
	assert (_underlay_ptr != NULL);
	return _underlay_ptr;
}

void Underlay::CreateInst(const char* cityFile, const char* routeFile, const char* asFile) {
	assert( _underlay_ptr == NULL );
	_underlay_ptr = new Underlay(cityFile, routeFile, asFile);
}

UINT32 Underlay::GetNumOfAS() {
    return _num_of_as;
}
UINT32 Underlay::GetNumOfNode() {
    return _num_of_node;
}

Underlay::Underlay(const char* cityFile, const char* routeFile, const char* asFile) {
    ReadInCityInfo(cityFile);
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
    string temp_line;
    vector<string> _vstrTemp;
    as_v.clear();
    for (UINT32 i=0; i < nofullAS; i++) {
        asFileHdlr >> tier;
        asFileHdlr >> currSubASno;
        for (int j = 0; j < currSubASno; j++) {
            temp_line.clear();
            getline(asFileHdlr,temp_line);
            if (temp_line.length()==0) {
                getline(asFileHdlr,temp_line);
            }
            str2StrArr(temp_line, ';', _vstrTemp);
            capacity = atoi(_vstrTemp[0].c_str());
            country = _vstrTemp[1];
            assert(_vstrTemp.size() == (capacity+2));
            AS thisAS(as_v.size(), tier, capacity, i, country);
            for (int k = 0; k < capacity; k++) {
                thisAS._myCities.insert(getCityIdx(_vstrTemp[2+k]));
            }
            assert(thisAS._myCities.size() == thisAS.getCapacity());
            as_v.push_back(thisAS);
        }
    }
}
UINT32 Underlay::getCityIdx(string cityName){
    string temp_line;
    vector<string> _vstrTemp;
    temp_line = cityName;
    str2StrArr(temp_line, ',', _vstrTemp);
    assert(_vstrTemp.size()==3);
    assert(city_list.size());
    for (int i = 0; i < city_list.size(); i++) {
        if (city_list[i].getCity() == _vstrTemp[0] && city_list[i].getState() == _vstrTemp[1]
                && city_list[i].getCountry() == _vstrTemp[2]) {
            return i;
        }
    }
    cerr<<"Can't locate cityNAme "<<cityName<<endl;
    return 0;
}

void Underlay::ReadInCityInfo(const char* cityFile){
    ifstream cityFileHdlr(cityFile);
    assert(cityFileHdlr.is_open());
    string temp_line;
    vector<string> _vstrTemp;
    while (cityFileHdlr.good()){
        temp_line.clear();
	getline(cityFileHdlr,temp_line);
        if (cityFileHdlr.eof()) {
            break;
        }
	str2StrArr(temp_line, ',', _vstrTemp);
        if (_vstrTemp.size()==8 && _vstrTemp[0] != "#city") {
            CITY thisCity(_vstrTemp[0], _vstrTemp[1], _vstrTemp[2], atof(_vstrTemp[4].c_str()), atof(_vstrTemp[5].c_str()),
                  atof(_vstrTemp[3].c_str()), atoi(_vstrTemp[7].c_str()));
            city_list.push_back(thisCity);
        }
        else {
            if (_vstrTemp[0] != "#city") {
                cerr<<"wrong city name "<< temp_line <<endl;
            }
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
            currNode.setCityIdx(*(as_v[i]._myCities.begin()));
            as_v[i]._myCities.erase(as_v[i]._myCities.begin());
            global_node_table.push_back(currNode);
            assignedHashID.insert(currHashID);
        }

    }
    sort(global_node_table.begin(), global_node_table.end(),NodeSortPredicate);
    for (int i = 0; i < global_node_table.size(); i++) {
        global_node_table[i].setNodeIdx(i);
        as_v[global_node_table[i].getASIdx()]._myNodes.insert(i);
        city_list[global_node_table[i].getCityIdx()]._nodeIdx_v.push_back(i);
    }
    gw_cities.clear();
    for (int i = 0; i < city_list.size(); i++) {
        if (city_list[i].isGW() && city_list[i]._nodeIdx_v.size()) {
            gw_cities.push_back(i);
        }
    }
}
/* insert activeGUIDno. of guid
 * select PoPs in gw cities for GUID insertion
 */
void Underlay::InitializeWorkload(){
    cout<<"Settings::TotalActiveGUID"<<Settings::TotalActiveGUID<<endl;
    UINT32 currNodeIdx;
    set<UINT32> assignedGUID;
    UINT32 currGUID;
    UINT32 currCityIdx;
    assert(gw_cities.size());
    for(UINT32 i=0; i<Settings::TotalActiveGUID; i++){
        //currNodeIdx= Util::Inst()->GenInt(_num_of_node);        
        currCityIdx = Util::Inst()->GenInt(gw_cities.size());
        assert(city_list[gw_cities[currCityIdx]]._nodeIdx_v.size());
        currNodeIdx = city_list[gw_cities[currCityIdx]]._nodeIdx_v[0];
        currGUID = Util::Inst()->GenInt(GNRS_HASH_RANGE);
        while (assignedGUID.find(currGUID) != assignedGUID.end()) {
            currGUID = Util::Inst()->GenInt(GNRS_HASH_RANGE);
        }
        GUID aGuid(currGUID,currNodeIdx,EventScheduler::Inst()->GetCurrentTime());
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

void Underlay::getQueryNodes(UINT32 guidIdx, vector<UINT32>& _queryNodes, FLOAT32 exponent){
    _queryNodes.clear();
    FLOAT32 totalWeight=0;
    FLOAT32 lat1, lon1, lat2, lon2;
    FLOAT32 currDistance;
    lat1 = city_list[global_node_table[global_guid_list[guidIdx].getAddrNodeIdx()].getCityIdx()].getLat();
    lon1 = city_list[global_node_table[global_guid_list[guidIdx].getAddrNodeIdx()].getCityIdx()].getLon();
    for (int i = 0; i < gw_cities.size(); i++) {
        lat2 = city_list[gw_cities[i]].getLat();
        lon2 = city_list[gw_cities[i]].getLon();
        currDistance = distance(lat1, lon1, lat2, lon2, 'M');
        if (currDistance ==0) {
            currDistance = 0.1;
        }
        totalWeight += pow(currDistance, exponent);
    }
    FLOAT32 guidPerWeight = (FLOAT32)(200)/totalWeight;
    UINT32 quota;
    for (int i = 0; i < gw_cities.size(); i++) {
        lat2 = city_list[gw_cities[i]].getLat();
        lon2 = city_list[gw_cities[i]].getLon();
        currDistance = distance(lat1, lon1, lat2, lon2, 'M');
        if (currDistance ==0) {
            currDistance = 0.1;
        }
        if (pow(currDistance, exponent) * guidPerWeight > 0.5) {
            quota = (UINT32)pow(currDistance, exponent) * guidPerWeight +1;
            for (int j = 0; j < quota; j++) {
                _queryNodes.push_back(city_list[gw_cities[i]]._nodeIdx_v[0]);
            }
        } 
    }
    //assert(_queryNodes.size());
    //debug
    cout<< city_list[global_node_table[global_guid_list[guidIdx].getAddrNodeIdx()].getCityIdx()].getCity()<<","
            <<city_list[global_node_table[global_guid_list[guidIdx].getAddrNodeIdx()].getCityIdx()].getCountry()
            <<" GUID birth city"<<endl;
    if (_queryNodes.size()) {
        for (int i = 0; i < _queryNodes.size(); i++) {
             cout<<city_list[global_node_table[_queryNodes[i]].getCityIdx()].getCity()<<","
                <<city_list[global_node_table[_queryNodes[i]].getCityIdx()].getCountry()<<" a query city"<<endl;
        }
    } else {
        cout<<"Empty query nodes \n";
    }

    

}
/*
 generate update or query workload, insert is done at network initialization
 */
UINT32 Underlay::generateWorkload(FLOAT64 mean_arrival, char opt){
    assert(opt == 'U' || opt == 'Q');
    UINT32 currGUIDIdx, currNodeIdx, currASIdx;
    UINT32 currRound =0, generatedCnt =0;
    Util::Inst() ->ResetPoisson(mean_arrival);
    vector<UINT32> queryNodes;
    //currRound = (UINT32)Util::Inst()->GenPoisson();
    currRound = (UINT32)(mean_arrival/200);//assume each guid has 200 queries, total query no. in this round is mean_arrival
    int i=0;
    while(i<currRound){
        currGUIDIdx = Util::Inst()->GenInt(global_guid_list.size());
        //to do exponent parameter setting
        getQueryNodes(currGUIDIdx, queryNodes, Settings::Locality_Exponent);
        if (queryNodes.size()) {
            for (int j = 0; j < queryNodes.size(); j++) {
                currNodeIdx = queryNodes[j];
                if (opt == 'Q'&& global_node_table[currNodeIdx].queryGUID(currGUIDIdx, 0)) {
                    generatedCnt++;
                } else if (opt == 'U' && global_node_table[currNodeIdx].updateGUID(currGUIDIdx, 0)){
                    generatedCnt++;
                }
            }
            i++;
        } 
    }
    //statistic keeping
    UINT32 idxDHTCnt = Underlay::Inst()->getIdxRetryCnt(EventScheduler::Inst()->GetCurrentTime(), true);
    UINT32 idxRetryCnt = Underlay::Inst()->getIdxRetryCnt(EventScheduler::Inst()->GetCurrentTime(), false);
    if(opt == 'I' || opt == 'U' ){
        Stat::DHT_RetryCnt[idxDHTCnt]._issuedUpdate = generatedCnt;
        Stat::Retry_Cnt[idxRetryCnt]._issuedUpdate = generatedCnt;
    }
    else if(opt == 'Q'){
        Stat::DHT_RetryCnt[idxDHTCnt]._issuedQuery = generatedCnt;
        Stat::Retry_Cnt[idxRetryCnt]._issuedQuery = generatedCnt;
    }
    //debug
    cout<<"suppose to generate "<<mean_arrival<<" "<<opt<<" actual generated"<<generatedCnt<<endl;
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
//find the local replica host -- numerically nearest nodes online and deployed in the same city
void Underlay::determineLocalHost(UINT32 guidIdx, set<UINT32>& _hostNodeIdx, int asIdx){
    //set<UINT32> localNodes = as_v[global_guid_list[guidIdx].getAddrASIdx()]._myNodes;
    //set<UINT32>::iterator it;
    UINT32 currNodeIdx = global_guid_list[guidIdx].getAddrNodeIdx();
    vector<UINT32> localNodes = city_list[global_node_table[currNodeIdx].getCityIdx()]._nodeIdx_v;
    vector<UINT32>:: iterator it;
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
    cout<<"guid= "<<global_guid_list[guidIdx].getGUID()<<"from city "<<city_list[global_node_table[currNodeIdx].getCityIdx()].getCity()
            <<","<<city_list[global_node_table[currNodeIdx].getCityIdx()].getCountry()<<endl;
    cout<<"localhost "<<localhostIdx<<"from city"<<city_list[global_node_table[localhostIdx].getCityIdx()].getCity()
            <<city_list[global_node_table[localhostIdx].getCityIdx()].getCountry()<<endl;
}
void Underlay::determineGlobalHost(UINT32 guidIdx, set<UINT32>& _hostNodeIdx, int asIdx){
    UINT32 currGUID = global_guid_list[guidIdx].getGUID();
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
    cout<<"Global Host"<<asIdx<<"'s view determined host size for "<<currGUID<<" is "<<_hostNodeIdx.size()<<":";
    set<UINT32>::iterator it;
    for (it = _hostNodeIdx.begin(); it != _hostNodeIdx.end(); it++) {
        cout<<global_node_table[(*it)].getHashID()<<" "<<as_v[global_node_table[(*it)].getASIdx()].getASCntry()<<" ";
    }
    cout<<endl;
}


void Underlay::determineRegionalHost(UINT32 guidIdx, set<UINT32>& _hostNodeIdx, int asIdx){
    string currCntry = as_v[global_guid_list[guidIdx].getAddrASIdx()].getASCntry();
    UINT32 currGUID = global_guid_list[guidIdx].getGUID();
    UINT32 assingedRegional =0;
    UINT32 vPhostIdx = global_guid_list[guidIdx].getvphostIdx();
    assert(vPhostIdx>=0 && vPhostIdx<_num_of_node);
    UINT32 leftPtr = vPhostIdx;
    InRangeMinus(leftPtr, _num_of_node); 
    UINT32 rightPtr = vPhostIdx;
    InRangePlus(rightPtr, _num_of_node);
    if(isAvailable(vPhostIdx, asIdx) && as_v[global_node_table[vPhostIdx].getASIdx()].getASCntry() == currCntry
            && assingedRegional<Settings::Regional_K && _hostNodeIdx.insert(vPhostIdx).second){
        assingedRegional++;
    }
    UINT32 currCandidateIdx;
    UINT32 scanned=1;
    while(assingedRegional<Settings::Regional_K && scanned<=_num_of_node){
        if(!isAvailable(rightPtr, asIdx) ||
                as_v[global_node_table[rightPtr].getASIdx()].getASCntry() != currCntry){
            InRangePlus(rightPtr, _num_of_node);
            scanned++;
            continue;
        }
        if(!isAvailable(leftPtr, asIdx) ||
                as_v[global_node_table[leftPtr].getASIdx()].getASCntry() != currCntry){
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
        if(_hostNodeIdx.insert(currCandidateIdx).second) {
            assingedRegional++; 
        }
    }
    //debug 
    cout<<"from AS"<<asIdx<<"'s view determined global + regional host size for "<<currGUID<<" is "<<_hostNodeIdx.size()<<":";
    set<UINT32>::iterator it;
    for (it = _hostNodeIdx.begin(); it != _hostNodeIdx.end(); it++) {
        cout<<global_node_table[(*it)].getHashID()<<" "<<as_v[global_node_table[(*it)].getASIdx()].getASCntry()<<" ";
    }
    cout<<endl;
    cout<<currCntry<<endl;
}

/*
 first determine GNRS_K global replica
 then determine Regional_K regional replica
 finally determine Local_K local replica
 the final result is the union of the three set, an overlapping replica accounts for all replicas it qualifies
 */
void Underlay::determineHost(UINT32 guidIdx, set<UINT32> & _hostNodeIdx, int asIdx){
    _hostNodeIdx.clear();
    if(Settings::GNRS_K){
        determineGlobalHost(guidIdx,_hostNodeIdx,asIdx);
    }
    if(Settings::Regional_K){
        determineRegionalHost(guidIdx,_hostNodeIdx, asIdx);
    }
    if(Settings::Local_K){
        determineLocalHost(guidIdx,_hostNodeIdx, asIdx);
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