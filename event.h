/* ============================================
   Description: Define the interface of an event.
   Author: Yi Hu
   ============================================ */
   
#ifndef EVENT_HEADER
#define EVENT_HEADER

#include "overnet.h"
   
class Event {
protected:
    FLOAT64 _time_created;
    FLOAT64 _time_done;          
public:
    Event();
    virtual ~Event();
    FLOAT64 GetTimeCreated() const;    
    FLOAT64 GetTimeDone() const;    
    void SetTimeDone(FLOAT64 time_done);
    virtual bool Callback() = 0;	// if return is true, this event can be deleted
    virtual void PrintInfo();
};

class DummyEvent : public Event {
public:
	DummyEvent();
	virtual bool Callback();	// if return is true, this event can be deleted
	virtual void PrintInfo();
};
enum MsgType {
	MT_UNDEF = 0,			// 0
	MT_Pre_JOIN,			// 1
	MT_JOIN,			// 2
	MT_Pre_LEAVE,			// 3
	MT_LEAVE,                       // 4
	MT_GUID_INSERTION,              // 5
        MT_GUID_QEURY,                  // 6
        MT_GUID_UPDATE,                 // 7
	MT_PING,			// 8
	MT_END				// 9
};

class Message : public Event
{
protected:
    UINT32 _size;       // bits
    UINT32 _dst, _src; 	// destination and source AS index
    MsgType _type;		// 
public:
    Message();
    ~Message();
    
    UINT32 GetType();
    UINT32 GetSize();
    void SetSize(UINT32 size);
    UINT32 GetDestination();
    void SetDestination(UINT32 dst);
    UINT32 GetSource();
    void SetSource(UINT32 src);
    
    virtual bool Callback()=0;
    virtual void PrintInfo();
};

/*
 * inform neighbors left K-1 and right K-1 nodes, increase their extra PING overhead
 */
class PreJoinMessage : public Message
{
private:
    UINT32 _nodeIdx;
    UINT32 _service_time;
    UINT32 _churnNo;
public:
	PreJoinMessage(UINT32 node, FLOAT64 arrival_time, FLOAT64 service_time, UINT32 churnNo); 
        virtual bool Callback();
};

/*
 * inform neighbors left K-1 and right K-1 nodes, transfer workload, notify other AS through query reply
 * will cause GUID query timeout
 * Update global node table
 */
class JoinMessage : public Message
{
private:
    UINT32 _nodeIdx;
    UINT32 _service_time;
    UINT32 _churnNo;
public:
	JoinMessage(UINT32 node, FLOAT64 remaining_service_time,UINT32 churnNo);
        virtual bool Callback();
};

/*
 * inform neighbors left K-1 and right K-1 nodes
 */
class PreLeaveMessage : public Message
{
private:
    UINT32 _nodeIdx;
    UINT32 _off_time;
    UINT32 _churnNo;
public:
	PreLeaveMessage(UINT32 node, FLOAT64 arrival_time, FLOAT64 service_time,UINT32 churnNo);
        virtual bool Callback();
};

/*
 * inform neighbors left K-1 and right K-1 nodes, transfer workload, notify other AS through query reply
 * will cause GUID query timeout
 * Update global node table
 */
class LeaveMessage : public Message
{
private:
    UINT32 _nodeIdx;
    UINT32 _off_time;
    UINT32 _churnNo;
public:
	LeaveMessage(UINT32 node, FLOAT64 remaining_off_time,UINT32 churnNo);
        virtual bool Callback();
};

#endif