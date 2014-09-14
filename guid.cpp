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
    //_address_nodeIdx = nodeIdx;
    _vphostIdx = Underlay::Inst()->getvpHostIdx(_guid,0,Underlay::Inst()->global_node_table.size()-1);
    //_last_update_time = time;
    _mobility_degree = mobilityDegree;
    _address_q.clear();
    _address_q.push_back(nodeIdx);
    _updateTime_q.clear();
    _updateTime_q.push_back(time);
}
GUID::~GUID(){
    
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
