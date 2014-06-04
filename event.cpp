/* ============================================
   Description: Define the interface of an event.
   Author: Yi Hu
   ============================================ */
   
#include "event.h"
#include "event_scheduler.h"
#include "network.h"

Event::Event()
{
    _time_created = EventScheduler::Inst()->GetCurrentTime();
    _time_done = -1;    
}

Event::~Event() {}

FLOAT64 Event::GetTimeCreated() const {
    return _time_created;
}
    
FLOAT64 Event::GetTimeDone() const {
    return _time_done;
}

void Event::SetTimeDone(FLOAT64 time_done) {
    _time_done = time_done;
}
   
void Event::PrintInfo() {
    cout <<"Event ( Created at " <<_time_created <<", done at " <<_time_done <<" )" <<endl;
}
DummyEvent::DummyEvent() : Event()
{
	_time_done = _time_created + 1;
}

bool DummyEvent::Callback()	// if return is true, this event can be deleted
{
	
        UINT32 totalNodes = Underlay::Inst()->global_node_table.size();
        if(_time_done <= Settings::QueryHours)
            Underlay::Inst()->generateWorkload(1,Settings::QueryPerNode*totalNodes,'Q');
        if(_time_done <= Settings::UpdateHours)
        Underlay::Inst()->generateWorkload(1,Settings::UpdatePerNode*totalNodes,'U');
        _time_done += 1;
	EventScheduler::Inst()->AddEvent(this);
	return false;
}

void DummyEvent::PrintInfo()
{
}

Message::Message() :
    Event(){
    _type = MT_UNDEF;
    _dst = 0;
    _src = 0;
    _size = 0;
}

 
Message::~Message() {}

UINT32 Message::GetType() {
	return _type;
}

UINT32 Message::GetSize() {
    return _size;
}

void Message::SetSize(UINT32 size) {
    _size = size;
}

UINT32 Message::GetDestination() {
	return _dst;
}

void Message::SetDestination(UINT32 dst) {
	_dst = dst;
}

UINT32 Message::GetSource() {
	return _src;
}

void Message::SetSource(UINT32 src) {
	_src = src;
}

void Message::PrintInfo() {
    //cout <<"Message ( Created at " <<_time_created <<", done at " <<_time_done <<", type=" <<_type <<", src=" <<_src <<", dst=" <<_dst <<", size=" <<_size <<" )" <<endl;
    //debug
    cout <<"Message ( Created at " <<_time_created <<", done at " <<_time_done <<", type=" <<_type <<", src=" <<_src <<endl;
        
}

PreJoinMessage::PreJoinMessage(UINT32 nodeIdx,FLOAT64 arrivalTime, FLOAT64 service_time,UINT32 churnNo){
    _nodeIdx = _src= nodeIdx;
    _service_time = service_time;
    _churnNo = churnNo;
    _type = MT_Pre_JOIN;
    if(service_time >= Settings::TestThreshold){
        _time_done = _time_created + arrivalTime + Settings::TestThreshold;
    }
    else{
        _time_done = _time_created + arrivalTime + service_time;
    } 
}
bool PreJoinMessage::Callback(){
    assert(Underlay::Inst()->global_node_table[_nodeIdx].isInService() == false);
    set <UINT32> myNeighborsIdx_v;
    Underlay::Inst()->getNeighbors(_nodeIdx, myNeighborsIdx_v);
    set <UINT32>::iterator it;
    UINT32 currNodeIdx, currASIdx;
    UINT32 pingCnt;
    
    if(_service_time >= Settings::TestThreshold){
        pingCnt = Settings::TestThreshold/PING_PERIOD;
        JoinMessage * aJoinMsg = new JoinMessage(_nodeIdx, (_service_time - Settings::TestThreshold), _churnNo);
        EventScheduler::Inst()->AddEvent(aJoinMsg);
        Underlay::Inst()->global_node_table[_nodeIdx].setInService(EventScheduler::Inst()->GetCurrentTime());
        Underlay::Inst()->as_v[Underlay::Inst()->global_node_table[_nodeIdx].getASIdx()].beNotifedAjoin(_nodeIdx);
        UINT32 migrationOverhead = Underlay::Inst()->migrationOverhead4Join(_nodeIdx);
        Stat::Migration_per_node[_nodeIdx] += migrationOverhead;
        for(it = myNeighborsIdx_v.begin(); it != myNeighborsIdx_v.end(); ++it){
            currNodeIdx = (*it);
            currASIdx = Underlay::Inst()->global_node_table[currNodeIdx].getASIdx();
            Stat::Ping_per_node[currNodeIdx]+=pingCnt;
            Stat::Migration_per_node[currNodeIdx] += migrationOverhead/Settings::GNRS_K; //ToDo:check
            Underlay::Inst()->as_v[currASIdx].beNotifedAjoin(_nodeIdx);
        }
    }
    else{
        pingCnt = _service_time/PING_PERIOD;
        for(it = myNeighborsIdx_v.begin(); it != myNeighborsIdx_v.end(); ++it){
            currNodeIdx = (*it);
            Stat::Ping_per_node[currNodeIdx]+=pingCnt;
            Stat::Premature_joins++;
        }
        if(_churnNo>0){
            PreLeaveMessage * aleaveMsg = new PreLeaveMessage(_nodeIdx, 0, _service_time, (_churnNo-1));
            EventScheduler::Inst()->AddEvent(aleaveMsg);
        }
    } 
    return true;
}

JoinMessage::JoinMessage(UINT32 node, FLOAT64 remaining_service_time,UINT32 churnNo) {
    _time_done = _time_created + remaining_service_time;
    _nodeIdx = _src = node;
    _churnNo = churnNo;
    _type = MT_JOIN;
}
bool JoinMessage::Callback(){
    if(_churnNo>0){
        PreLeaveMessage * aleaveMsg = new PreLeaveMessage(_nodeIdx, 0, _service_time, (_churnNo-1));
        EventScheduler::Inst()->AddEvent(aleaveMsg);
    }
    /*else{
        PreLeaveMessage * aleaveMsg = new PreLeaveMessage(_nodeIdx, 0, Settings::EndTime,0);
        EventScheduler::Inst()->AddEvent(aleaveMsg);
    }*/
    return true;
}

PreLeaveMessage::PreLeaveMessage(UINT32 nodeIdx, FLOAT64 arrivalTime,FLOAT64 off_time,UINT32 churnNo) {
    _nodeIdx = _src = nodeIdx;
    _off_time = off_time;
    _churnNo = churnNo;
    _type = MT_Pre_LEAVE;
    if(off_time >= Settings::TestThreshold)
        _time_done = _time_created + arrivalTime + Settings::TestThreshold;
    else
        _time_done = _time_created + arrivalTime + off_time;
}
bool PreLeaveMessage::Callback(){
    assert(Underlay::Inst()->global_node_table[_nodeIdx].isInService());
    if(_off_time >= Settings::TestThreshold){
        set <UINT32> myNeighborsIdx_v;
        Underlay::Inst()->getNeighbors(_nodeIdx, myNeighborsIdx_v);
        set <UINT32>::iterator it;
        UINT32 currNodeIdx, currASIdx;
        LeaveMessage * aleaveMsg = new LeaveMessage(_nodeIdx, (_off_time - Settings::TestThreshold),_churnNo);
        EventScheduler::Inst()->AddEvent(aleaveMsg);
        Underlay::Inst()->global_node_table[_nodeIdx].setOffService(EventScheduler::Inst()->GetCurrentTime());
        Underlay::Inst()->as_v[Underlay::Inst()->global_node_table[_nodeIdx].getASIdx()].beNotifedAleave(_nodeIdx);
        UINT32 migrationOverhead = Underlay::Inst()->migrationOverhead4Leave(_nodeIdx);
        for(it = myNeighborsIdx_v.begin(); it != myNeighborsIdx_v.end(); ++it){
            currNodeIdx = (*it);
            currASIdx = Underlay::Inst()->global_node_table[currNodeIdx].getASIdx();
            Stat::Migration_per_node[currNodeIdx] += migrationOverhead/Settings::GNRS_K; //ToDo:check
            Underlay::Inst()->as_v[currASIdx].beNotifedAleave(_nodeIdx);
        }
    }
    else{
        Stat::Premature_leaves++;
        if(_churnNo>0){
            PreJoinMessage * ajoinMsg = new PreJoinMessage(_nodeIdx, 0, _off_time, (_churnNo-1));
            EventScheduler::Inst()->AddEvent(ajoinMsg);
        }
    }
        
    return true;
}

LeaveMessage::LeaveMessage(UINT32 node, FLOAT64 remaining_off_time,UINT32 churnNo) {
    _nodeIdx = _src = node;
    _off_time = remaining_off_time;
    _time_done = _time_created + remaining_off_time;
    _churnNo = churnNo;
    _type = MT_LEAVE;
}

bool LeaveMessage::Callback(){   
    if(_churnNo>0){
        PreJoinMessage * ajoinMsg = new PreJoinMessage(_nodeIdx, 0, _off_time, (_churnNo-1));
        EventScheduler::Inst()->AddEvent(ajoinMsg);
    }
    /*else{
        PreJoinMessage * ajoinMsg = new PreJoinMessage(_nodeIdx, 0, Settings::EndTime, 0);
        EventScheduler::Inst()->AddEvent(ajoinMsg);
    }*/
    return true;
}
