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
    Underlay::Inst()->determineHost(_guidIdx, globalHostset,localHostset,_nodeIdx);
    if (_type == MT_GUID_INSERTION || _type == MT_GUID_UPDATE) {
        Underlay::Inst()->global_node_table[_nodeIdx].calCorrectHost(localHostset,globalHostset,'I');
    } else {
        Underlay::Inst()->global_node_table[_nodeIdx].calCorrectHost(localHostset,globalHostset,'Q');
    }
    return true;
}



Underlay* Underlay::_underlay_ptr = NULL;

Underlay* Underlay::Inst() {
	assert (_underlay_ptr != NULL);
	return _underlay_ptr;
}

void Underlay::CreateInst(const char* cityFile, const char* routeFile, const char* asFile, const char* predFile) {
	assert( _underlay_ptr == NULL );
	_underlay_ptr = new Underlay(cityFile, routeFile, asFile, predFile);
}

UINT32 Underlay::GetNumOfAS() {
    return _num_of_as;
}
UINT32 Underlay::GetNumOfNode() {
    return _num_of_node;
}

Underlay::Underlay(const char* cityFile, const char* routeFile, const char* asFile, const char* predFile) {
    ReadInCityInfo(cityFile);
    ReadInRoutingTable(routeFile);
    ReadInASInfo(asFile);
    ReadInPredicate(predFile);
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
}
//First line: total no. of as
//from second line for each i to all j: predicate from asIdx i to asIdx j
void Underlay::ReadInPredicate(const char* predFile){
    ifstream predFileHdlr(predFile);
    assert(predFileHdlr.is_open());
    UINT32 nn; 
    predFileHdlr >> nn;
    cout<<"Total no. of AS from predicate file "<<nn<<endl;
    assert(nn == _num_of_as);
    as_pre_matx = (int*) malloc(sizeof(int) * nn * nn );
    memset(as_pre_matx, 0, sizeof(int) * nn * nn);
    cout << "reading Predicate table" << endl; 
    int pred_curr;
    for (UINT32 i=0; i < nn; i++) { //Read Predicate matrix  
	for (UINT32 j=0; j < nn; j++){
            predFileHdlr >>  pred_curr; 
            as_pre_matx[i*nn + j] =  pred_curr; 	
	}
    }
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
    deployed_cities.clear();
    //vector<UINT32> pop_per_gw, pop_per_city;
    for (int i = 0; i < city_list.size(); i++) {
        if (city_list[i]._nodeIdx_v.size()) {
            deployed_cities.push_back(i);
            //pop_per_city.push_back(city_list[i]._nodeIdx_v.size());
            if (city_list[i].isGW()) {
                gw_cities.push_back(i);
            //    pop_per_gw.push_back(city_list[i]._nodeIdx_v.size());
            }
        }
    }
}
/*
 * initialize all statistic vector (all zero element)
 */
void Underlay::InitializeStat(){
    if (Settings::DeployOnlyGW) {
        workload_cities = gw_cities;
    } else {
        workload_cities = deployed_cities;
    }
    cout<<"gw cities "<<gw_cities.size()<<",deployed cities "<<deployed_cities.size()<<", workload cities"<<workload_cities.size()<<endl;
    UINT32 currNodeIdx;
    for (UINT32 i = 0;  i < workload_cities.size(); i++) {
        for (UINT32 j = 0; j < city_list[workload_cities[i]]._nodeIdx_v.size(); j++) {
            currNodeIdx = city_list[workload_cities[i]]._nodeIdx_v[j];
            global_node_table[currNodeIdx].setInWorkload();
        }
    }
    Wrkld_Count thisWrkld;
    thisWrkld._cacheWrkld=0;
    thisWrkld._replicaWrkld=0;
    for (UINT32 i = 0; i < global_node_table.size(); i++) {
        Stat::Storage_per_node.push_back(0);
        Stat::Workload_per_node.push_back(thisWrkld);
    }
}
/* insert ActiveGUIDperPoP no. of guid on workload_cities
 * select PoPs in workload_cities for GUID insertion
 */
void Underlay::InitializeWorkload(){
    UINT32 currNodeIdx;
    set<UINT32> assignedGUID;
    UINT32 currGUID;
    UINT32 currCityIdx=0;
    char mobilityDegree;
    assert(workload_cities.size());
    vector<UINT32> workload_cities_guidquota;
    UINT64 totalActiveGUIDs=0, randNum;
    for (int i = 0; i < workload_cities.size(); i++) {
        workload_cities_guidquota.push_back(city_list[workload_cities[i]]._nodeIdx_v.size()*Settings::ActiveGUIDperPoP);
        totalActiveGUIDs += city_list[workload_cities[i]]._nodeIdx_v.size()*Settings::ActiveGUIDperPoP;
    }
    assert(workload_cities_guidquota.size() == workload_cities.size());
    //debug
    cout<<"Initialized pareto"<<Settings::ParetoParameter<<endl;
    Util::Inst()->ResetPareto(Settings::ParetoParameter);
    for(UINT64 i=0; i<totalActiveGUIDs; i++){
        if (workload_cities_guidquota[currCityIdx]==0) {
            currCityIdx ++;
        }
        assert(currCityIdx<workload_cities.size());
        randNum = Util::Inst()->GenInt(city_list[workload_cities[currCityIdx]]._nodeIdx_v.size());
        currNodeIdx = city_list[workload_cities[currCityIdx]]._nodeIdx_v[randNum];
        currGUID = Util::Inst()->GenInt(GNRS_HASH_RANGE);
        while (assignedGUID.find(currGUID) != assignedGUID.end()) {
            currGUID = Util::Inst()->GenInt(GNRS_HASH_RANGE);
        }
        randNum = Util::Inst()->GenInt(totalActiveGUIDs);
        if (randNum <= totalActiveGUIDs * Settings::LocalMobilityPerc) {
            mobilityDegree = 'L';
        } 
        else if(randNum <= totalActiveGUIDs * (Settings::LocalMobilityPerc+Settings::RegionalMobilityPerc)){
            mobilityDegree = 'R';
        }
        else {
            mobilityDegree = 'G';
        }

        GUID aGuid(currGUID,currNodeIdx,EventScheduler::Inst()->GetCurrentTime(),
                mobilityDegree,Util::Inst()->GenPareto());
        global_guid_list.push_back(aGuid);
        assignedGUID.insert(currGUID);
        workload_cities_guidquota[currCityIdx]--;
    }
    updateGUIDs_v.clear();
    for (UINT64 i = 0; i < global_guid_list.size(); i++) {
        initializeMobility(i);
        //assign guid based on city popularity
        //select a percentage of guids to be mobile naturally proportional to city population
        if (global_guid_list[i].getMobility()== 'L') {
            updateGUIDs_v.push_back(i);
        }
    }
    //cout<<"total guid"<<global_guid_list.size()<<",updatePercentageg="<<Settings::LocalMobilityPerc<<",update guids= "<<updateGUIDs_v.size()<<endl;
    genOutFileName();
    /*sort(popularity_stat.begin(), popularity_stat.end());
    string outfilename = Settings::outFileName;
    outfilename += "_guidPopularity_cdf";
    Util::Inst()->genCDF(outfilename.c_str(), popularity_stat);*/
}

void Underlay::genOutFileName(){
    string strgOutName = Settings::outFileName;
    strgOutName += "_";
    stringstream ss (stringstream::in | stringstream::out);
    ss<<global_guid_list.size();
    strgOutName += ss.str();
    strgOutName += "_";
    ss.str("");
    ss<<Settings::GNRS_K;
    strgOutName += ss.str();
    strgOutName += "_";
    ss.str("");
    ss <<Settings::Regional_K;
    strgOutName += ss.str();
    strgOutName += "_";
    ss.str("");
    ss <<Settings::Local_K;
    strgOutName += ss.str();
    ss.str("");
    if (Settings::ParetoParameter != 1.04) {
        strgOutName += "_Pareto";
        ss <<Settings::ParetoParameter;
        strgOutName += ss.str();
        ss.str("");
    }
    strgOutName += "_QOrigBal";
    ss <<Settings::QueryOriginBalance;
    strgOutName += ss.str();
    ss.str("");
    if (Settings::DeployOnlyGW) {
        strgOutName += "_DplyGW";  
    } /*else {
        strgOutName += "_DplyAll";
    }*///omit deply all as default, show deploy gw 
    if (Settings::CacheOn) {
        strgOutName += "_CacheOn";
        ss <<Settings::CacheOn;
        strgOutName += ss.str();
        ss.str("");
        if (Settings::AdaptGo) {
            strgOutName += "_aptGo";
        } 
        strgOutName += "_GoThru";
        ss <<Settings::GoThroughProb;
        strgOutName += ss.str();
        ss.str("");
        strgOutName +="_CachePerc";
        ss <<Settings::CachePerc;
        strgOutName += ss.str();
        ss.str("");
        strgOutName +="_StrtUpdFrq";
        ss <<Settings::UpdateFrqGUID;
        strgOutName += ss.str();
        ss.str("");
        strgOutName +="_Rnd";
        ss <<Settings::QWrkldRounds;
        strgOutName += ss.str();
        ss.str("");
        strgOutName +="_UpdPerc";
        ss <<Settings::LocalMobilityPerc;
        strgOutName += ss.str();
        ss.str("");
        strgOutName +="_TTL";
        ss <<Settings::TTL;
        strgOutName += ss.str();
        ss.str("");
        
    } else {
        strgOutName += "_CacheOff";
    }
    if (Settings::balanceBase) {
        strgOutName += "_RndSlct";
    } /*else {
        strgOutName += "_ShrtSlct";
    }*/
    if (Settings::ChurnHours){
        strgOutName +="_ChnRt";
        ss <<Settings::ChurnPerNode;
        strgOutName += ss.str();
        ss.str("");
        strgOutName +="_ChnSes";
        ss <<Settings::OnOffSession;
        strgOutName += ss.str();
        ss.str("");
        strgOutName +="_ChnRnd";
        ss <<Settings::OnOffRounds;
        strgOutName += ss.str();
        ss.str("");
    }
    if (Settings::QueryHours){
        strgOutName +="_QHrs";
        ss <<Settings::QueryHours;
        strgOutName += ss.str();
        ss.str("");
    }
    if (Settings::UpdateHours){
        strgOutName +="_UHrs";
        ss <<Settings::UpdateHours;
        strgOutName += ss.str();
        ss.str("");
    }
    Settings::outFileName = strgOutName;
}

void Underlay::initializeMobility(UINT32 guidIdx){
    char MobilityDegree = global_guid_list[guidIdx].getMobility();
    assert(MobilityDegree == 'L' || MobilityDegree == 'R'|| MobilityDegree == 'G');
    assert(global_guid_list[guidIdx]._address_q.size()==1);
    UINT32 currNodeIdx = global_guid_list[guidIdx].getCurrAddrNodeIdx();
    UINT32 currCityIdx = global_node_table[currNodeIdx].getCityIdx();
    UINT32 updatedCityIdx, randNum, qSize;
    bool finished = false ;
    randNum = Util::Inst()->GenInt(10000);
    if (randNum <= 5000) {
        qSize = 2;
    } else {
        qSize =3;
    }
    vector<UINT32> candidate_v;
    if (MobilityDegree == 'L') {
        if (city_list[currCityIdx]._nodeIdx_v.size()>1) {
            candidate_v = city_list[currCityIdx]._nodeIdx_v;
            for (int i = 0; i < candidate_v.size(); i++) {
                if (candidate_v[i] == currNodeIdx) {
                    candidate_v.erase(candidate_v.begin()+i);
                    break;
                }
            }
            if ((qSize-1) >= candidate_v.size()) {
                for (int i = 0; i < candidate_v.size(); i++) {
                    global_guid_list[guidIdx]._address_q.push_back(candidate_v[i]);
                }
            } else{
                for (int i = 0; i < (qSize-1); i++) {
                    randNum = Util::Inst()->GenInt(candidate_v.size());
                    global_guid_list[guidIdx]._address_q.push_back(candidate_v[randNum]);
                    candidate_v.erase(candidate_v.begin()+randNum);
                }
            }        
        }
        finished = true;
    }
    else if (MobilityDegree == 'R'){
        string currCountry = city_list[currCityIdx].getCountry();
        for (int i = 0; i < workload_cities.size(); i++) {
            if (city_list[workload_cities[i]].getCountry() == currCountry && city_list[workload_cities[i]]._nodeIdx_v.size()) {
                candidate_v.push_back(workload_cities[i]);
            }
        }
    }
    else {
        for (int i = 0; i < workload_cities.size(); i++) {
            if (city_list[workload_cities[i]]._nodeIdx_v.size()) {
                candidate_v.push_back(workload_cities[i]);
            }
        }
    }
    if (!finished && candidate_v.size()) {
        if ((qSize-1) >= candidate_v.size()) {
            for (int i = 0; i < candidate_v.size(); i++) {
                updatedCityIdx = candidate_v[i];
                assert(city_list[updatedCityIdx]._nodeIdx_v.size());
                randNum = Util::Inst()->GenInt(city_list[updatedCityIdx]._nodeIdx_v.size());
                global_guid_list[guidIdx]._address_q.push_back(city_list[updatedCityIdx]._nodeIdx_v[randNum]);
                
            }
        } else{
            for (int i = 0; i < (qSize-1); i++) {
                randNum = Util::Inst()->GenInt(candidate_v.size());
                updatedCityIdx = candidate_v[randNum];
                candidate_v.erase(candidate_v.begin()+ randNum);
                assert(city_list[updatedCityIdx]._nodeIdx_v.size());
                randNum = Util::Inst()->GenInt(city_list[updatedCityIdx]._nodeIdx_v.size());
                global_guid_list[guidIdx]._address_q.push_back(city_list[updatedCityIdx]._nodeIdx_v[randNum]);
            }
        }
    }  
    for (int i = 1; i < global_guid_list[guidIdx]._address_q.size(); i++) {
        global_guid_list[guidIdx]._updateTime_q.push_back(-1);
    }
    assert(global_guid_list[guidIdx]._address_q.size() == global_guid_list[guidIdx]._updateTime_q.size());
    //debug 
    /*
    cout<<"mobility degree "<<MobilityDegree<<" GUID address queue "<<global_guid_list[guidIdx]._address_q.size()<<" : ";
    for (int i = 0; i < global_guid_list[guidIdx]._address_q.size(); i++) {
        currNodeIdx = global_guid_list[guidIdx]._address_q[i];
        assert(currNodeIdx>=0 && currNodeIdx<global_node_table.size());
        cout<<city_list[global_node_table[currNodeIdx].getCityIdx()].getCity()<<","<<city_list[global_node_table[currNodeIdx].getCityIdx()].getState()
                <<","<<city_list[global_node_table[currNodeIdx].getCityIdx()].getCountry()<<",time"<<global_guid_list[guidIdx]._updateTime_q[i]<<";";

    }
    cout<<endl;*/
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
//ToDo: correct this function, not neighbors anymore
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
    UINT32 singleShare = Settings::ActiveGUIDperPoP;
    UINT32 noOfoffline = 0;
    //ToDo: count no of offline neighbors spread among my life neighbors
    return singleShare;
}

UINT32 Underlay::migrationOverhead4Leave(UINT32 nodeIdx){
    UINT32 singleShare = Settings::ActiveGUIDperPoP;
    UINT32 noOfoffline = 0;
    //ToDo: count no of offline neighbors spread among my life neighbors
    return singleShare;
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
void Underlay::getQueryNodesPerLoc(FLOAT32 lat1, FLOAT32 lon1, UINT32 totalNo, vector<UINT32>& _queryNodes, vector<UINT32>& _queryQuota, FLOAT32 exponent){
    FLOAT32 totalWeight=0;
    FLOAT32 lat2, lon2;
    FLOAT32 currDistance;
    UINT32 randNum;
    for (int i = 0; i < workload_cities.size(); i++) {
        lat2 = city_list[workload_cities[i]].getLat();
        lon2 = city_list[workload_cities[i]].getLon();
        currDistance = distance(lat1, lon1, lat2, lon2, 'M');
        if (currDistance ==0) {
            currDistance = 0.1;
        }
        totalWeight += pow(currDistance, exponent);
    }
    FLOAT32 guidPerWeight = (FLOAT32)(totalNo)/totalWeight;
    for (int i = 0; i < workload_cities.size(); i++) {
        lat2 = city_list[workload_cities[i]].getLat();
        lon2 = city_list[workload_cities[i]].getLon();
        currDistance = distance(lat1, lon1, lat2, lon2, 'M');
        if (currDistance ==0) {
            currDistance = 0.1;
        }
        if (pow(currDistance, exponent) * guidPerWeight > 0.5) {
            randNum = Util::Inst()->GenInt(city_list[workload_cities[i]]._nodeIdx_v.size());
            _queryNodes.push_back(city_list[workload_cities[i]]._nodeIdx_v[randNum]);
            _queryQuota.push_back((UINT32)pow(currDistance, exponent) * guidPerWeight +1);
        } 
    }
    assert(_queryNodes.size()==_queryQuota.size());
}

void Underlay::getQueryNodesByPoP(UINT32 guidIdx, vector<UINT32>& _queryNodes, vector<UINT32>& _queryQuota, FLOAT32 exponent){
    _queryNodes.clear();
    _queryQuota.clear();
    UINT64 totalNo = global_guid_list[guidIdx].getPopularity();
    set<UINT32> selected_cities;
    UINT32 randNum, randNum2, candidateNo;
    UINT32 cityIdx = global_node_table[global_guid_list[guidIdx].getCurrAddrNodeIdx()].getCityIdx();
    string currCountry = city_list[cityIdx].getCountry();
    UINT32 remainQuota;
    if (exponent < -0.5) {
        //debug
        if (exponent <= -10) //just for testing
            _queryNodes.push_back(city_list[cityIdx]._nodeIdx_v[0]);
        else
            _queryNodes = city_list[cityIdx]._nodeIdx_v;
    } 
    else if (exponent < -0.1){
        for (int i = 0; i < workload_cities.size(); i++) {
            if(city_list[workload_cities[i]].getCountry() == currCountry){
                randNum = Util::Inst()->GenInt(city_list[workload_cities[i]]._nodeIdx_v.size());
                _queryNodes.push_back(city_list[workload_cities[i]]._nodeIdx_v[randNum]);
            }
        }
    }
    else {
        if (totalNo < (workload_cities.size()*0.9)) {
            candidateNo = totalNo;
        } else {
            candidateNo = (workload_cities.size()*0.9);
        }

        for (UINT32 i = 0; i < candidateNo; i++) {
            randNum = Util::Inst()->GenInt(workload_cities.size());
            while (selected_cities.find(randNum) != selected_cities.end()) {
                randNum = Util::Inst()->GenInt(workload_cities.size());
            }
            selected_cities.insert(randNum);
            randNum2 = Util::Inst()->GenInt(city_list[workload_cities[randNum]]._nodeIdx_v.size());
            _queryNodes.push_back(city_list[workload_cities[randNum]]._nodeIdx_v[randNum2]);            
        }
    }
    assert(_queryNodes.size());
    Query_Count curr_queryCnt;
    curr_queryCnt._guidIdx=guidIdx;
    if (_queryNodes.size()>= totalNo) {
        UINT64 diff= _queryNodes.size() - totalNo;
        for (UINT64 i = 0; i < totalNo; i++) {
            curr_queryCnt._queryCnt=1;
            global_node_table[_queryNodes[i]]._queryWrkld_v.push_back(curr_queryCnt);
            _queryQuota.push_back(1);
        }
        for (UINT64 i = 0; i < diff; i++) {
            _queryNodes.pop_back();
        }
    } else {
        remainQuota = totalNo%_queryNodes.size();
        for (UINT64 i = 0; i < _queryNodes.size(); i++) {
            curr_queryCnt._queryCnt=totalNo/_queryNodes.size();
            if (i<remainQuota) {
                curr_queryCnt._queryCnt+= 1;
            }
            global_node_table[_queryNodes[i]]._queryWrkld_v.push_back(curr_queryCnt);
            _queryQuota.push_back(totalNo/_queryNodes.size());
        }
        _queryQuota[_queryQuota.size()-1]+= totalNo%_queryNodes.size();
    }
    assert(_queryNodes.size()==_queryQuota.size());
    //debug
    /*
    cout<<"guidIdx= " <<guidIdx<<",exponent="<<exponent<<",popularity="<<global_guid_list[guidIdx].getPopularity()<<" query nodes: "<<_queryNodes.size();
        for (int i = 0; i < _queryQuota.size(); i++) {
            cout<<","<<_queryQuota[i];
        }
    cout<<endl;
    
    cout<< city_list[global_node_table[global_guid_list[guidIdx].getCurrAddrNodeIdx()].getCityIdx()].getCity()<<","
            <<city_list[global_node_table[global_guid_list[guidIdx].getCurrAddrNodeIdx()].getCityIdx()].getCountry()
            <<" GUID birth city "<<exponent<<endl;
    for (int i = 0; i < _queryNodes.size(); i++) {
        cout<<city_list[global_node_table[_queryNodes[i]].getCityIdx()].getCity()<<","
            <<city_list[global_node_table[_queryNodes[i]].getCityIdx()].getCountry()<<" a query city with quota"
            <<_queryQuota[i]<<endl;
    }
    */

}

void Underlay::getQueryNodes(UINT32 guidIdx, vector<UINT32>& _queryNodes, vector<UINT32>& _queryQuota, FLOAT32 exponent, bool popularity){
    _queryNodes.clear();
    _queryQuota.clear();
    FLOAT32 lat1, lon1;
    FLOAT32 totalWeight=0, currWeight;
    vector<FLOAT32> weight_q;
    if (popularity) {
        lat1 = city_list[global_node_table[global_guid_list[guidIdx]._address_q[0]].getCityIdx()].getLat();
        lon1 = city_list[global_node_table[global_guid_list[guidIdx]._address_q[0]].getCityIdx()].getLon();
        getQueryNodesPerLoc(lat1, lon1, global_guid_list[guidIdx].getPopularity(), _queryNodes, _queryQuota, exponent);
    } 
    else{
        for (int i = 0; i < global_guid_list[guidIdx]._updateTime_q.size(); i++) {
            currWeight = pow(2.0, (global_guid_list[guidIdx]._updateTime_q[i]-global_guid_list[guidIdx]._updateTime_q[0]));
            weight_q.push_back(currWeight);
            totalWeight += currWeight;
        }
        assert(weight_q.size()==global_guid_list[guidIdx]._address_q.size());
        for (int i = 0; i < global_guid_list[guidIdx]._address_q.size(); i++) {
            lat1 = city_list[global_node_table[global_guid_list[guidIdx]._address_q[i]].getCityIdx()].getLat();
            lon1 = city_list[global_node_table[global_guid_list[guidIdx]._address_q[i]].getCityIdx()].getLon();
            currWeight = (weight_q[i]/totalWeight)*Settings::QueryPerGUID;
            getQueryNodesPerLoc(lat1, lon1, currWeight, _queryNodes, _queryQuota, exponent);
        }
    }   
    //debug
    /*cout<<"exponent"<<exponent<<" popularity "<<global_guid_list[guidIdx].getPopularity()<<" query nodes: "<<_queryNodes.size();
        for (int i = 0; i < _queryQuota.size(); i++) {
            cout<<","<<_queryQuota[i];
        }
    cout<<endl;
    
    cout<< city_list[global_node_table[global_guid_list[guidIdx].getCurrAddrNodeIdx()].getCityIdx()].getCity()<<","
            <<city_list[global_node_table[global_guid_list[guidIdx].getCurrAddrNodeIdx()].getCityIdx()].getCountry()
            <<" GUID birth city "<<exponent<<endl;
    for (int i = 0; i < _queryNodes.size(); i++) {
        cout<<city_list[global_node_table[_queryNodes[i]].getCityIdx()].getCity()<<","
            <<city_list[global_node_table[_queryNodes[i]].getCityIdx()].getCountry()<<" a query city with quota"
            <<_queryQuota[i]<<endl;
    }*/
}

UINT32 Underlay::issueQueries (UINT32 currGUIDIdx, FLOAT32 exponent){
    vector<UINT32> queryNodes; 
    vector<UINT32> queryQuota;
    UINT32 generatedCnt =0;
    UINT32 currNodeIdx;
    getQueryNodes(currGUIDIdx, queryNodes, queryQuota, exponent, false);
    if (queryNodes.size()) {
        assert(queryNodes.size()==queryQuota.size());
        for (int j = 0; j < queryNodes.size(); j++) {
            currNodeIdx = queryNodes[j];
            for (int k = 0; k < queryQuota[j]; k++) {
                if (global_node_table[currNodeIdx].queryGUID(currGUIDIdx, 0))
                    generatedCnt++;
            }
        }
    }
    //statistic keeping
    UINT32 idxDHTCnt = Underlay::Inst()->getIdxRetryCnt(EventScheduler::Inst()->GetCurrentTime(), true);
    UINT32 idxRetryCnt = Underlay::Inst()->getIdxRetryCnt(EventScheduler::Inst()->GetCurrentTime(), false);
    Stat::DHT_RetryCnt[idxDHTCnt]._issuedQuery += generatedCnt;
    Stat::Retry_Cnt[idxRetryCnt]._issuedQuery += generatedCnt;
    //debug
    cout<<"issued queries "<<generatedCnt<<endl;
    return generatedCnt;
}

FLOAT32 Underlay::genLocalityExponent(){
    UINT32 randNum;
    FLOAT32 exponent;
    randNum = Util::Inst()->GenInt(10000);
    if (randNum<=10000*Settings::StrongLocalityPerc) {
        exponent = -0.8;
    } else if (randNum<=10000*(Settings::StrongLocalityPerc+Settings::MedLocalityPerc)){
        exponent = -0.4;
    } else {
        exponent = 0;
    }
    return exponent;
}
/* get the path between srcAS index to destAS index*/
/* only intermediate as, not include src or dst as*/
void Underlay::getShortestPath(UINT32 srcAS, UINT32 destAS, vector<UINT32> &pathContainer) {
    pathContainer.clear();
    UINT32 as_index = as_v.size();
    int _intermediate = as_pre_matx[srcAS*as_index + destAS];
    //pathContainer.insert(pathContainer.begin(), destAS);	
    //loop until reach the destination or reach itself
    while ((_intermediate != srcAS) && (_intermediate != -1)){
	pathContainer.insert(pathContainer.begin(), (UINT32)_intermediate);
	_intermediate = as_pre_matx[srcAS*as_index + _intermediate];
    }
    //pathContainer.insert(pathContainer.begin(),srcAS);
}
/*
 calculate cache lookup latency overhead for a query
 */
UINT32 Underlay::calCacheLatOverhead(vector<UINT32> pathNodeIdx, UINT32 hitNodeIdx){
    UINT32 hopCount=102; //102 means no hit
    for (int i = 0; i < pathNodeIdx.size(); i++) {
        if (pathNodeIdx[i] == hitNodeIdx) {
            if (i!=(pathNodeIdx.size()-1)) {
                hopCount = i;
            } else {
                hopCount = 101; //101 means hit at dst
            }
            break;
        }
    }
    //Stat::QueryHitHopCnt.push_back(hopCount); //record how many queries hit cache in the middle of the route
    //debug
    if (hopCount == 102) {
        cout<<"No hit query: hit nodeIdx= "<<hitNodeIdx<<". Route info:";
        for (int i = 0; i < pathNodeIdx.size(); i++) {
            cout<<pathNodeIdx[i]<<",";
        }
        cout<<endl;
        return 0;
    }   
    if (hopCount == 101) {
        hopCount = pathNodeIdx.size()-1;
    }
    return ((++hopCount)*Settings::CacheLookupLat);
}

/*
 the query workload is updated in calQueryDelay ()
 return this query latency
 */
FLOAT64 Underlay::calSingleQueryWrkld(UINT64 currGUIDIdx, UINT32 currNodeIdx){
    vector<UINT32> queryPathAS, queryPathNode, debugQueryPathNode;
    UINT32 dstReplicahost, randNum, hitNode;
    UINT32 currTS =0, cacheLat=0;
    FLOAT64 minDistance;
    if (Settings::balanceBase) {
        randNum = Util::Inst()->GenInt(global_guid_list[currGUIDIdx]._replica_hosts.size());
        dstReplicahost = global_guid_list[currGUIDIdx]._replica_hosts[randNum];
        minDistance = getLatency(currNodeIdx,dstReplicahost);
    } else {
        minDistance = global_node_table[currNodeIdx].getMinDistance(global_guid_list[currGUIDIdx]._replica_hosts,dstReplicahost);   
    }
    if (Settings::CacheOn==0) {   //cacheOff
        Stat::Workload_per_node[dstReplicahost]._replicaWrkld++;
        return minDistance*2;
    } else if(Settings::CacheOn==1){ //cache only at src
        queryPathNode.clear();
    }
    else{ //cache along route
        getShortestPath(global_node_table[currNodeIdx].getASIdx(), global_node_table[dstReplicahost].getASIdx(),queryPathAS);
        queryPathNode.clear();
        for (int i = 0; i < queryPathAS.size(); i++) {
            if (Settings::FixPath) {
                queryPathNode.push_back((*as_v[queryPathAS[i]]._myNodes.begin()));
            } else {
                randNum = Util::Inst()->GenInt(as_v[queryPathAS[i]]._myNodes.size());
                set <UINT32>::iterator it = as_v[queryPathAS[i]]._myNodes.begin();
                std::advance(it, randNum);
                queryPathNode.push_back((*it));
            }
        }
    }
    queryPathNode.push_back(dstReplicahost);
    //Stat::QueryHopCnt.push_back(queryPathNode.size());
    debugQueryPathNode = queryPathNode;
    debugQueryPathNode.insert(debugQueryPathNode.begin(), currNodeIdx);
    hitNode= global_node_table[currNodeIdx].cacheLookup(currGUIDIdx, currTS, queryPathNode,false);
    global_guid_list[currGUIDIdx].increaseQeueryCnt();
    //cacheLat = calCacheLatOverhead(debugQueryPathNode,hitNode);
    minDistance = getLatency(currNodeIdx,hitNode);
    //return (minDistance*2 + cacheLat);
    return minDistance*2;
}

void Underlay::oneRoundQueryWrkld(vector<UINT32>& delay_results_v, FLOAT32 updateFreq){
    vector<UINT32> queryIssuer_v;
    UINT32 randNum, randNum2, currGUIDIdx, currQNodeIdx, updateGUIDIdx;
    UINT32 qCntfrmLstUpd=0, qCntfrmLstClk=0;
    /*UINT64 update_period_clk= total_query_per_round/Settings::QueryPerClock;
    update_period_clk = update_period_clk/2;
    cout<<"update_period_clk="<< update_period_clk<<endl;*/
    for (UINT32 i = 0;  i < global_guid_list.size(); i++) {
        Stat::Error_cnt_per_guid[i]=0;
    }

    for (UINT32 i = 0;  i<global_node_table.size(); i++) {
        if (global_node_table[i]._queryWrkld_v.size()) {
            global_node_table[i]._currWrkld_v = global_node_table[i]._queryWrkld_v;
            queryIssuer_v.push_back(i);
        }
    }    
    while (queryIssuer_v.size()) {
        randNum = Util::Inst()->GenInt(queryIssuer_v.size());
        currQNodeIdx = queryIssuer_v[randNum];
        randNum2 = Util::Inst()->GenInt(global_node_table[currQNodeIdx]._currWrkld_v.size());
        currGUIDIdx = global_node_table[currQNodeIdx]._currWrkld_v[randNum2]._guidIdx;
        delay_results_v.push_back((UINT32)calSingleQueryWrkld(currGUIDIdx, currQNodeIdx));
        if (++qCntfrmLstClk >= Settings::QueryPerClock) {
            Settings::CurrentClock ++;
            qCntfrmLstClk =0;
            /*if (Settings::CurrentClock % update_period_clk == 0) {
                cout<<"adaptive update!!!"<<endl;
                for (int j = 0; j < global_node_table.size(); j++) {
                    global_node_table[j].adaptGoThrough();
                }
            }*/
        }
        if (++qCntfrmLstUpd * updateFreq >=1) {
            //get a GUID to simulate update
            updateGUIDIdx = Util::Inst()->GenInt(updateGUIDs_v.size());
            updateGUIDIdx = updateGUIDs_v[updateGUIDIdx];
            global_guid_list[updateGUIDIdx].simulateAnUpdate();
            qCntfrmLstUpd=0;
        }
        if ((-- global_node_table[currQNodeIdx]._currWrkld_v[randNum2]._queryCnt)==0) {
            global_node_table[currQNodeIdx]._currWrkld_v.erase(global_node_table[currQNodeIdx]._currWrkld_v.begin()+randNum2);
        }
        if (global_node_table[currQNodeIdx]._currWrkld_v.size()==0) {
            queryIssuer_v.erase(queryIssuer_v.begin()+randNum);
        }     
    }
}

void Underlay::calQueryWorkload(){
    assert(Stat::Workload_per_node.size() == global_node_table.size());
    for (UINT32 i = 0; i < global_node_table.size(); i++) {
        global_node_table[i]._queryWrkld_v.clear();     
    }
    vector<UINT32> queryNodes;
    vector<UINT32> queryQuota;
    vector<UINT32> delay_results_v;
    UINT32 currGUIDIdx;
    UINT64 queryUntlLstRnd=0;
    for (currGUIDIdx = 0; currGUIDIdx < global_guid_list.size(); currGUIDIdx++) {
        //getQueryNodesByPoP(currGUIDIdx,queryNodes,queryQuota,-0.4);
        if (Settings::QueryOriginBalance>0) {
            getQueryNodesByPoP(currGUIDIdx,queryNodes,queryQuota,0);
        } else {
            getQueryNodesByPoP(currGUIDIdx,queryNodes,queryQuota,-11.0);
        }
    }
    /*total_query_per_round=0;
    for (UINT32 i = 0; i < global_node_table.size(); i++) {
        for (int j = 0; j < global_node_table[i]._queryWrkld_v.size(); j++) {
            total_query_per_round += global_node_table[i]._queryWrkld_v[j]._queryCnt;
        }
    }
    cout<<"total_query_per_round="<<total_query_per_round<<endl;*/
    string strgOutName = Settings::outFileName;
    
    // single round
    if (Settings::CacheOn) {
        strgOutName = Settings::outFileName + "_TotalErrPerRound";
        ofstream outfHdlr;
        outfHdlr.open(strgOutName.c_str(),ios::out | ios::in | ios:: trunc);
        for (int i = 1; i <= Settings::QWrkldRounds; i++) {
            Settings::totalErrorCnt=0;
            oneRoundQueryWrkld(delay_results_v,Settings::UpdateFrqGUID);
            outfHdlr<<Settings::totalErrorCnt<<"\t"<<(delay_results_v.size()-queryUntlLstRnd)<<"\t"<<Settings::UpdateFrqGUID<<endl;
            queryUntlLstRnd = delay_results_v.size();
            /*while (Stat::Error_stat.size()) {
                outfHdlr<<Stat::Error_stat[0]._popularity<<"\t"<<Stat::Error_stat[0]._TTL<<"\t"<<Stat::Error_stat[0]._goThrough<<endl;
                Stat::Error_stat.erase(Stat::Error_stat.begin());
            };*/
            if (i%10 == 0) {
                Settings::UpdateFrqGUID+=0.1;
            }
            if (Settings::AdaptGo) {
                for (int j = 0; j < global_node_table.size(); j++) {
                    global_node_table[j].adaptGoThrough();
                }
            }
        }
        outfHdlr.close();
        /*strgOutName = Settings::outFileName + "_ErrorStat";
        outfHdlr.open(strgOutName.c_str(),ios::out | ios::in | ios:: trunc);
        for (UINT32 i = 0;  i < Stat::Error_stat.size(); i++) {
            outfHdlr<<Stat::Error_stat[i]._popularity<<"\t"<<Stat::Error_stat[i]._TTL<<"\t"<<Stat::Error_stat[i]._goThrough<<endl;
        }
        outfHdlr.close();*/
        strgOutName = Settings::outFileName + "_ErrorScatter";
        Util::Inst()->outErrorDetail(strgOutName.c_str());
    }
    else {
        oneRoundQueryWrkld(delay_results_v,Settings::UpdateFrqGUID);
    }
    
    sort(Stat::Workload_per_node.begin(),Stat::Workload_per_node.end());
    while (Stat::Workload_per_node.size() && (Stat::Workload_per_node[0]._cacheWrkld+Stat::Workload_per_node[0]._replicaWrkld)==0) {
        Stat::Workload_per_node.erase(Stat::Workload_per_node.begin());
    }
    strgOutName = Settings::outFileName + "_QWrkld_scatter";
    Util::Inst()->outWrkldDetail(strgOutName.c_str(),Stat::Workload_per_node);
    strgOutName = Settings::outFileName + "_qLatency_cdf";
    Util::Inst()->genCDF(strgOutName.c_str(),delay_results_v);    
}
/*
 generate query workload: independent of update
 */
UINT32 Underlay::generateQueryWorkload(FLOAT64 mean_arrival){
    UINT32 currGUIDIdx;
    UINT32 currRound =0, generatedCnt =0, totalGenerated =0;
    //Util::Inst() ->ResetPoisson(mean_arrival);
    //currRound = (UINT32)Util::Inst()->GenPoisson();
    currRound = (UINT32)(mean_arrival/Settings::QueryPerGUID);//total query no. in this round is mean_arrival
    int i=0;
    while(i<currRound){
        currGUIDIdx = Util::Inst()->GenInt(global_guid_list.size());
        generatedCnt = issueQueries(currGUIDIdx,genLocalityExponent());
        if (generatedCnt) {
            totalGenerated += generatedCnt;
            i++;
        } 
    }
    //debug
    cout<<"suppose to generate "<<mean_arrival<<" Queries, actual generated"<<totalGenerated<<endl;
    return totalGenerated;
}
/*
 generate update workload: mobility local, regional, global
 * and associated query workload
 */
UINT32 Underlay::generateUpdateWorkload(FLOAT64 mean_arrival){
    UINT32 currGUIDIdx, currNodeIdx;
    UINT32 currRound;
    FLOAT32 qLocality_exp;
    currRound = (UINT32)(mean_arrival);// total update no. in this round is mean_arrival
    int i=0;
    while(i<currRound){
        currGUIDIdx = Util::Inst()->GenInt(global_guid_list.size());
        currNodeIdx = global_guid_list[currGUIDIdx].getNextAddrNodeIdx();
        if (global_node_table[currNodeIdx].updateGUID(currGUIDIdx, 0)){
            if (!Settings::LocMobSync) {
                qLocality_exp = genLocalityExponent();
            } else{
                if (global_guid_list[currGUIDIdx].getMobility() == 'L') {
                    qLocality_exp = -0.8;
                } 
                else if (global_guid_list[currGUIDIdx].getMobility() == 'R') {
                    qLocality_exp = -0.4;
                }
                else {
                    qLocality_exp = -0;
                }
            }
            cout<<"Generate an update with mobility "<<global_guid_list[currGUIDIdx].getMobility()<<" query exp "<<qLocality_exp<<endl;
            issueQueries(currGUIDIdx,qLocality_exp);
            i++;
        }
    }
    //statistic keeping
    UINT32 idxDHTCnt = Underlay::Inst()->getIdxRetryCnt(EventScheduler::Inst()->GetCurrentTime(), true);
    UINT32 idxRetryCnt = Underlay::Inst()->getIdxRetryCnt(EventScheduler::Inst()->GetCurrentTime(), false);
    Stat::DHT_RetryCnt[idxDHTCnt]._issuedUpdate = i;
    Stat::Retry_Cnt[idxRetryCnt]._issuedUpdate = i;
    //debug
    cout<<"suppose to generate "<<mean_arrival<<" Updates, actual generated"<<i<<endl;
    return i;
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
    //debug
    cout<<"generate leave cnt"<<generatedCnt<<endl;
    return generatedCnt;
}
/*
 if src and dest are nodeIdx, Geo_Lat_On is true, calculate geo latency
 otherwise, calculate AS(sub-as here) level topo latency
 */
FLOAT64 Underlay::getLatency(UINT32 src, UINT32 dest){
    assert(src>=0 && src < global_node_table.size() && dest >=0 && dest < global_node_table.size());
    if (Settings::Geo_Lat_On) {
        FLOAT32 lat1, lat2, lon1, lon2;
        lat1 = city_list[global_node_table[src].getCityIdx()].getLat();
        lon1 = city_list[global_node_table[src].getCityIdx()].getLon();
        lat2 = city_list[global_node_table[dest].getCityIdx()].getLat();
        lon2 = city_list[global_node_table[dest].getCityIdx()].getLon();
        FLOAT64 currDist = distance(lat1, lon1, lat2, lon2, 'K');
        if (currDist ==0) {
            currDist = Util::Inst()->GenInt(40);
        }
        FLOAT64 currDelay = currDist * 5.41 / ((FLOAT64)1000);
        if (global_node_table[src].getASIdx() == global_node_table[dest].getASIdx()) {
            return currDelay*(1+Settings::IntraLatWeight);
        } else {
            return currDelay*(1+Settings::InterLatWeight);
        }
    } else {
        return ((FLOAT64)as_dist_matx[global_node_table[src].getASIdx()*_num_of_as+global_node_table[dest].getASIdx()]/(FLOAT64)1000); //change us to ms
    }
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
//only the query node is within the same city, then local host can be correctly located
void Underlay::determineLocalHost(UINT32 guidIdx, set<UINT32>& _hostNodeIdx, int asIdx){
    //set<UINT32> localNodes = as_v[global_guid_list[guidIdx].getAddrASIdx()]._myNodes;
    //set<UINT32>::iterator it;
    UINT32 currNodeIdx = global_guid_list[guidIdx].getCurrAddrNodeIdx();
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
    //cout<<"guid= "<<global_guid_list[guidIdx].getGUID()<<"from city "<<city_list[global_node_table[currNodeIdx].getCityIdx()].getCity()
    //        <<","<<city_list[global_node_table[currNodeIdx].getCityIdx()].getCountry()<<endl;
    //cout<<"localhost "<<localhostIdx<<"from city"<<city_list[global_node_table[localhostIdx].getCityIdx()].getCity()
    //        <<city_list[global_node_table[localhostIdx].getCityIdx()].getCountry()<<endl;
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
    //cout<<"Global Host"<<asIdx<<"'s view determined host size for "<<currGUID<<" is "<<_hostNodeIdx.size()<<":";
    //set<UINT32>::iterator it;
    //for (it = _hostNodeIdx.begin(); it != _hostNodeIdx.end(); it++) {
    //    cout<<global_node_table[(*it)].getHashID()<<" "<<as_v[global_node_table[(*it)].getASIdx()].getASCntry()<<" ";
    //}
    //cout<<endl;
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
    //cout<<"from AS"<<asIdx<<"'s view determined global + regional host size for "<<currGUID<<" is "<<_hostNodeIdx.size()<<":";
    //set<UINT32>::iterator it;
    //for (it = _hostNodeIdx.begin(); it != _hostNodeIdx.end(); it++) {
        //cout<<global_node_table[(*it)].getHashID()<<" "<<as_v[global_node_table[(*it)].getASIdx()].getASCntry()<<" ";
    //}
    //cout<<endl;
    //cout<<currCntry<<endl;
}

/*
 first determine GNRS_K global replica
 then determine Regional_K regional replica if the request node in the same region as the GUID
 finally determine Local_K local replica if the request node in the same city as the GUID 
 the final result is the union of the three set
 */
void Underlay::determineHost(UINT32 guidIdx, set<UINT32>& glbCalHostSet, set<UINT32>& lclCalHostSet, int nodeIdx){
    assert(nodeIdx>=0 && nodeIdx<global_node_table.size());
    int asIdx = global_node_table[nodeIdx].getASIdx();
    string currCntry = as_v[global_guid_list[guidIdx].getAddrASIdx()].getASCntry();
    UINT32 currNodeIdx = global_guid_list[guidIdx].getCurrAddrNodeIdx();
    if(Settings::GNRS_K){
        determineGlobalHost(guidIdx,glbCalHostSet,-1);
        determineGlobalHost(guidIdx,lclCalHostSet,asIdx);
    }
    if(Settings::Regional_K && as_v[asIdx].getASCntry() == currCntry){
        determineRegionalHost(guidIdx,glbCalHostSet,-1);
        determineRegionalHost(guidIdx,lclCalHostSet,asIdx);
    }
    if(Settings::Local_K && global_node_table[currNodeIdx].getCityIdx() == global_node_table[nodeIdx].getCityIdx()){
        determineLocalHost(guidIdx,glbCalHostSet,-1);
        determineLocalHost(guidIdx,lclCalHostSet,asIdx);
    }
}
//calculate global replica host
//calculate storage workload per node
void Underlay::PrepareWorkloadCal(){
    assert(Stat::Storage_per_node.size() == global_node_table.size());
    set<UINT32> glbCalHostSet;
    for (UINT64 guidIdx = 0; guidIdx < global_guid_list.size(); guidIdx++) {
        Stat::Error_cnt_per_guid.push_back(0);
        //Stat::CacheHit_per_guid.push_back(0);
        //Stat::QueryHopCnt.push_back(0);//debug used for recording updates per guid
        glbCalHostSet.clear();
        global_guid_list[guidIdx]._replica_hosts.clear();
        if(Settings::GNRS_K){
            determineGlobalHost(guidIdx,glbCalHostSet,-1);
        }
        if(Settings::Regional_K){
            determineRegionalHost(guidIdx,glbCalHostSet,-1);
        }
        if(Settings::Local_K){
            determineLocalHost(guidIdx,glbCalHostSet,-1);
        }
        for (set<UINT32>::iterator it=glbCalHostSet.begin(); it!=glbCalHostSet.end(); ++it){
            global_guid_list[guidIdx]._replica_hosts.push_back(*it);
            Stat::Storage_per_node[*it]++;
        }
    }
}

void Underlay::calStorageWorkload(){
    assert(Stat::Storage_per_node.size() == global_node_table.size());
    sort(Stat::Storage_per_node.begin(),Stat::Storage_per_node.end());
    string strgOutName = Settings::outFileName;
    //debug
    //cout<<"storage per node \n";
    UINT32 unitStrWrkld = Stat::Storage_per_node[Stat::Storage_per_node.size()/2];
    if (unitStrWrkld==0) {
        unitStrWrkld =1;
    }
    cout<<"unitStrWrkld ="<<unitStrWrkld<<endl;
    for (int i = 0; i < Stat::Storage_per_node.size(); i++) {
        //cout<<Stat::Storage_per_node[i]<<endl;
        Stat::Storage_per_node[i] = Stat::Storage_per_node[i]/unitStrWrkld;
    }
    strgOutName +="_normStrg";
    Util::Inst()->genCDF(strgOutName.c_str(),Stat::Storage_per_node);
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
    /*
    int minDistance;
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