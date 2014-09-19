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


bool AS::isGNRSMember(){
    set<UINT32>::iterator it;
    for(it=_myNodes.begin();it!=_myNodes.end(); it++)
        if(Underlay::Inst()->global_node_table[(*it)].isInService())
            return true;
    return false;
}
