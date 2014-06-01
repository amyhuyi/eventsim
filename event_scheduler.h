/* ============================================
   Description: Define the interface of the event queue
   Author: Yi Hu
   ============================================ */
   
#ifndef EVENT_SCHEDULER_HEADER
#define EVENT_SCHEDULER_HEADER

#include <queue>
#include "overnet.h"
#include "event.h"

class SortTimeDone
{
public:
    bool operator() (const Event* e1, const Event* e2) const;
};

class EventScheduler
{
private:
    static EventScheduler* _scheduler_ptr;
        
    EventScheduler();
        
    priority_queue< Event*, vector<Event*>, SortTimeDone > _event_queue;
public:
    static EventScheduler* Inst();
    ~EventScheduler();
    
    bool Empty() const;
    void AddEvent(Event* event);
    Event* NextEvent();
    Event* CurrentEvent();
    void HandleCurrentEvent();
    FLOAT64 GetCurrentTime();
    UINT32 GetSize();
};

#endif
