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

GUID::GUID (UINT32 id, UINT32 nodeIdx, FLOAT64 time, char mobilityDegree, UINT64 popularity){
    _guid = id;
    _popularity = popularity;
    _vphostIdx = Underlay::Inst()->getvpHostIdx(_guid,0,Underlay::Inst()->global_node_table.size()-1);
    _mobility_degree = mobilityDegree;
    _address_q.clear();
    _address_q.push_back(nodeIdx);
    _updateTime_q.clear();
    _updateTime_q.push_back(time);
    _updateRate = Settings::UpdateFrqGUID;
    _issuedQueryCnt =0;
    _cacheHits =0;
    _incacheCnt =0;
}
GUID::~GUID(){
    
}

FLOAT64 GUID::getUpdateRate(){
    return _updateRate;
}

void GUID::setUpdateRate(FLOAT64 rate){
    assert(rate >= 0);
    _updateRate=rate;
}

UINT64 GUID::getPopularity(){
    return _popularity;
}
UINT32 GUID::getGUID(){
    return _guid;
}

char GUID::getMobility(){
    return _mobility_degree;
}

void GUID::setMobility(char newMobilityDegree){
    _mobility_degree = newMobilityDegree;
}

UINT32 GUID::getvphostIdx(){
    return _vphostIdx;
}

FLOAT64 GUID::getLastUpdateTime (){
    assert(_updateTime_q.size());
    return _updateTime_q[0];
}
 /*
  ensure _address_q[0] is current address node, _address_q[1] is the next, round robin
  */      
void GUID::updateAddrNodeIdx(UINT32 newNodeIdx, FLOAT64 time){
    assert(_address_q.size() && _updateTime_q.size());
    if (_address_q[0] != newNodeIdx) { //allow a node update an GUID multiple times in one time-frame
        UINT32 curr = *_address_q.begin();
        FLOAT64 currTime = *_updateTime_q.begin();
        _address_q.erase(_address_q.begin());
        _address_q.push_back(curr);
        _updateTime_q.erase(_updateTime_q.begin());
        _updateTime_q.push_back(currTime);
        assert(_address_q[0] == newNodeIdx);
    }
    _updateTime_q[0] = time;
}

UINT32 GUID::getNextAddrNodeIdx(){
    assert(_address_q.size());   
    if (_address_q.size()>1) {
        return _address_q[1];
    } else {
        return _address_q[0];
    }
}

UINT32 GUID::getCurrAddrNodeIdx(){
    assert(_address_q.size());
    return _address_q[0];
}

UINT32 GUID::getAddrASIdx(){
    assert(_address_q.size());
    return Underlay::Inst()->global_node_table[_address_q[0]].getASIdx();
}

bool GUID::increaseQeueryCnt(){
    _issuedQueryCnt ++;
    /*
    if ((_issuedQueryCnt*_updateRate) >= 1) {
        simulateAnUpdate();
        _issuedQueryCnt =0;
        return true;
    }
    return false;*/
    return true;

}

UINT32 GUID::getQueryCnt(){
    return _issuedQueryCnt;
}

void GUID::simulateAnUpdate(){
    FLOAT64 currUpdateTime = getLastUpdateTime();
    currUpdateTime ++;
    updateAddrNodeIdx(getNextAddrNodeIdx(),currUpdateTime);
    assert(getLastUpdateTime() == currUpdateTime);
    //cout<<"guid="<<_guid<<"issued an update w query cnt ="<<_issuedQueryCnt<<endl;
    _issuedQueryCnt=0;
}

void GUID::increaseCacheHits(){
    _cacheHits++;
}

void GUID::increaseInCacheCnt(){
    _incacheCnt++;
}

UINT32 GUID::getCacheHits(){
    return _cacheHits;
}


UINT32 GUID::getInCacheCnt(){
    return _incacheCnt;
}

void GUID::resetCacheHits(){
    _cacheHits=0;
}

void GUID::resetInCacheCnt(){
    _incacheCnt=0;
}