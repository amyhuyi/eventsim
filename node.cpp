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

Node::Node(UINT32 hashID, UINT32 asIdx, FLOAT64 time){
    _hashID = hashID;
    _asIdx = asIdx;
    _in_service = true;
    _in_workload = false;
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

bool Node::isInWorkload(){
    return _in_workload;
}

void Node::setInWorkload(){
    _in_workload = true;
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

FLOAT64 Node::getMaxDistance(vector<UINT32> correctHost){
    assert(correctHost.size());
    FLOAT64 maxDistance;
    for (int i = 0; i < correctHost.size(); i++) {
        if (i==0){
            maxDistance = Underlay::Inst()->getLatency(_nodeIdx, correctHost[i]);
        }  
        else {
            if (maxDistance < Underlay::Inst()->getLatency(_nodeIdx, correctHost[i])) {
                maxDistance = Underlay::Inst()->getLatency(_nodeIdx, correctHost[i]);
            }
        }
    }
    return maxDistance;
}

FLOAT64 Node::getMinDistance(vector<UINT32> correctHost, UINT32 & dstNodeIdx){
    assert(correctHost.size());
    FLOAT64 minDistance;
    minDistance = Underlay::Inst()->getLatency(_nodeIdx, correctHost[0]);
    dstNodeIdx = correctHost[0];
    //debug
    //cout<<"Candidate distance: ";
    for (int i = 1; i < correctHost.size(); i++) {
        //cout<<Underlay::Inst()->getLatency(_nodeIdx, correctHost[i]) << "w node "<<correctHost[i]<<",";
        if (minDistance > Underlay::Inst()->getLatency(_nodeIdx, correctHost[i])) {
            minDistance = Underlay::Inst()->getLatency(_nodeIdx, correctHost[i]);
            dstNodeIdx = correctHost[i];
        }
    }
    UINT32 cityIdx = Underlay::Inst()->global_node_table[dstNodeIdx].getCityIdx();
    //cout<<"Min Distance "<<minDistance<<" selected node "<<dstNodeIdx
    //        <<","<<Underlay::Inst()->city_list[cityIdx].getCity()<<","<<Underlay::Inst()->city_list[cityIdx].getState()
    //        <<Underlay::Inst()->city_list[cityIdx].getCountry()<<endl;
    return minDistance;
}

FLOAT64 Node::calInsertDelay(vector<UINT32> onlyInlocal, vector<UINT32> onlyInglobal, vector<UINT32> correctHost){
    FLOAT64 maxDistance;
    FLOAT64 retryDistance;
    UINT32 dstNodeIdx;
    if(onlyInglobal.size()==0){
        assert(correctHost.size());
        maxDistance = getMaxDistance(correctHost);
        return maxDistance*2;
    }
    else{
        if (correctHost.size()) {
            retryDistance = getMinDistance(correctHost,dstNodeIdx)*2;
            retryDistance += getMaxDistance(onlyInglobal)*2;
            //cout<<"retryDistance #3"<<retryDistance<<endl;
        } 
        else {
            assert(onlyInlocal.size()&&onlyInglobal.size());
            retryDistance = getMaxDistance(onlyInlocal)*2;
            retryDistance += getMaxDistance(onlyInglobal)*2;
            retryDistance += (Settings::DHTHop) * getMinDistance(onlyInglobal,dstNodeIdx);
            //cout<<"retryDistance #4"<<retryDistance<<endl;
        }
        //debug
        return retryDistance;
    }
}

FLOAT64 Node::calQueryDelayRandomSelection(vector<UINT32> correctHost){
    assert(correctHost.size());
    UINT32 randSelHost = Util::Inst()->GenInt(correctHost.size());
    Stat::Workload_per_node[correctHost[randSelHost]]++;
    return (Underlay::Inst()->getLatency(_nodeIdx, correctHost[randSelHost])*2);
}

FLOAT64 Node::calQueryDelay(vector<UINT32> onlyInlocal, vector<UINT32> onlyInglobal, vector<UINT32> correctHost){
    FLOAT64 minDistance;
    FLOAT64 retryDistance;
    UINT32 dstNodeIdx;
    assert(Stat::Workload_per_node.size()==Underlay::Inst()->global_node_table.size());
    if(onlyInlocal.size()){
        if(correctHost.size() && getMinDistance(onlyInlocal,dstNodeIdx)< getMinDistance(correctHost,dstNodeIdx)){
            retryDistance = getMinDistance(onlyInlocal,dstNodeIdx)*2;
            Stat::Workload_per_node[dstNodeIdx]++;
            retryDistance += getMinDistance(correctHost,dstNodeIdx)*2;
            Stat::Workload_per_node[dstNodeIdx]++;
            //debug
            cout<<"retryDistance #1"<<retryDistance<<endl;
            return retryDistance;
        }
        else if(correctHost.size() && getMinDistance(onlyInlocal,dstNodeIdx)>= getMinDistance(correctHost,dstNodeIdx)){
            minDistance = getMinDistance(correctHost,dstNodeIdx);
            Stat::Workload_per_node[dstNodeIdx]++;
            return minDistance*2;
        }
        else{
            retryDistance = getMaxDistance(onlyInlocal)*2;
            for (int i = 0; i < onlyInlocal.size(); i++) {
                Stat::Workload_per_node[onlyInlocal[i]] ++;
            }
            retryDistance += getMinDistance(onlyInglobal,dstNodeIdx)*2;
            retryDistance += (Settings::DHTHop) * getMinDistance(onlyInglobal,dstNodeIdx);
            Stat::Workload_per_node[dstNodeIdx]++;
            //debug
            cout<<"retryDistance #2"<<retryDistance<<endl;
            return retryDistance;
        }
    }
    else{
        minDistance = getMinDistance(correctHost,dstNodeIdx);
        Stat::Workload_per_node[dstNodeIdx]++;
        return minDistance*2;
    }
}

/*
 * 1. calculate correct host 2. updated local view from the correct host of this operation
 * 3. statistics on delay and retry no. 
 */
void Node::calCorrectHost(set<UINT32> localHostset, set<UINT32> globalHostset, char opt){
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
    UINT32 dstNodeIdx;
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
        else if(opt == 'Q' && getMinDistance(onlyInlocal,dstNodeIdx)< getMinDistance(correctHost,dstNodeIdx)){
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
            assert(Underlay::Inst()->as_v[_asIdx]._local_view_offNodes.find(onlyInlocal[i]) == Underlay::Inst()->as_v[_asIdx]._local_view_offNodes.end());
            if(Underlay::Inst()->global_node_table[onlyInlocal[i]].isInService()==false)
                Underlay::Inst()->as_v[_asIdx]._local_view_offNodes.insert(onlyInlocal[i]);
            }
        for (int i = 0; i < onlyInglobal.size(); i++) {
            assert(Underlay::Inst()->global_node_table[onlyInglobal[i]].isInService());
            if(Underlay::Inst()->as_v[_asIdx]._local_view_offNodes.find(onlyInglobal[i])!= Underlay::Inst()->as_v[_asIdx]._local_view_offNodes.end())
                Underlay::Inst()->as_v[_asIdx]._local_view_offNodes.erase(Underlay::Inst()->as_v[_asIdx]._local_view_offNodes.find(onlyInglobal[i]));
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
/*
 * receiving a query, lookup cache, update cache when necessary (LRU update, or go through update)
 * staleFlag is to simulate stale cache enforced query
 * return the hit nodeIdx 
 */
UINT32 Node::cacheLookup(UINT32 guidIdx, UINT32& myTimestamp, vector<UINT32>& remainNodePath, bool staleFlag){
    Stat::Workload_per_node[_nodeIdx]++;
    UINT32 correctTimeStamp = (UINT32) Underlay::Inst()->global_guid_list[guidIdx].getLastUpdateTime();
    if (remainNodePath.size()==0) { //hit the replica host
        myTimestamp = correctTimeStamp;
        return _nodeIdx;
    }
    UINT32 hitNodeIdx, nextHopNodeIdx;
    nextHopNodeIdx = remainNodePath[0];
    remainNodePath.erase(remainNodePath.begin());
    for (UINT32 i = 0; i < _cache.size(); i++) {//lookup my cache
        if (_cache[i]._guidIdx == guidIdx) { //cache hit
            //cout<<"in my cache nodeIdx="<<_nodeIdx<<", guidIdx="<<guidIdx<<endl;
            _cache[i]._fromLastError++;
            _cache[i]._hitCount++;
            if (_cache[i]._goThroughProb*_cache[i]._hitCount >=1) {
                //cout<<"goThr="<<_cache[i]._goThroughProb<<", hitCnt= "<<_cache[i]._hitCount<<", goThrough to next hop"
                //        <<",remainNodepath="<<remainNodePath.size()<<endl;
                if (_cache[i]._timestamp < correctTimeStamp)
                //    cout<<"Save one stale chance _cache[i]._timestamp="<<_cache[i]._timestamp<<",correctTimeStamp="<<correctTimeStamp<<endl;
                hitNodeIdx = Underlay::Inst()->global_node_table[nextHopNodeIdx].cacheLookup(guidIdx,myTimestamp,remainNodePath,staleFlag);
                _cache[i]._timestamp = myTimestamp;
                //cout<<"update my cache for guidIdx="<<_cache[i]._guidIdx<<endl;
                _cache[i]._hitCount=0;
            }else{
                if (_cache[i]._timestamp < correctTimeStamp) {
                    if (!staleFlag) {
                        Stat::Error_cnt_per_guid[guidIdx]++;
                    //    cout<<"Stale Cache first time counted \n";
                    }
                    //cout<<"Stale Cache for guid"<<_cache[i]._guidIdx<<",cachedTS="<<_cache[i]._timestamp<<",correctTS= "<<correctTimeStamp
                    //    <<",hitCnt="<<_cache[i]._hitCount<<",popularity="<<Underlay::Inst()->global_guid_list[guidIdx].getPopularity()<<endl;
                    hitNodeIdx = Underlay::Inst()->global_node_table[nextHopNodeIdx].cacheLookup(guidIdx,myTimestamp,remainNodePath, true);
                    _cache[i]._timestamp = myTimestamp;
                    //cout<<"update my cache for"<<_cache[i]._guidIdx<<endl;
                    //_cache[i]._errorRate = 1.00/(FLOAT32)_cache[i]._fromLastError;
                    _cache[i]._fromLastError =0;
                    //_cache[i]._goThroughProb = 0.7;
                } else{
                    myTimestamp = _cache[i]._timestamp;
                    hitNodeIdx = _nodeIdx;
                    Stat::CacheHit_per_guid[guidIdx]++;
                }              
            }
            assert(_cache[i]._guidIdx == guidIdx && _cache[i]._timestamp == correctTimeStamp);
            Cache_Entry currCacheEntry = _cache[i];
            //cout<<"to delete _cache[i].guid"<<_cache[i]._guidIdx<<endl;
            _cache.erase(_cache.begin()+i);
            _cache.push_back(currCacheEntry);
            //cout<<"new _cache[i].guid="<<_cache[i]._guidIdx<<",_cache end guid="<<_cache[_cache.size()-1]._guidIdx<<endl;
            return hitNodeIdx;
        }
    }
    //cache miss
    hitNodeIdx = Underlay::Inst()->global_node_table[nextHopNodeIdx].cacheLookup(guidIdx,myTimestamp,remainNodePath,staleFlag);
    Cache_Entry currCacheEntry (guidIdx, myTimestamp, 0, Settings::GoThroughProb);
    if (_cache.size() >= Settings::CachePerc*Underlay::Inst()->global_guid_list.size()) {
        //debug
        //cout<<"to kick out a cache with hit count"<<(*_cache.begin())._hitCount<<", timestamp"<<(*_cache.begin())._timestamp<<endl;
        _cache.erase(_cache.begin()); //creation time can be handled here  
        //cout<<"to pushback a cache with timestamp"<<_cache[_cache.size()-1]._timestamp<<endl;
    } 
    _cache.push_back(currCacheEntry);
    return hitNodeIdx;
}