/* ============================================
 Description: Define the interface of the event queue
 Author: Yi Hu
 ============================================ */
#include "event_scheduler.h"

bool SortTimeDone::operator() (const Event* e1, const Event* e2) const {
        return e1->GetTimeDone() > e2->GetTimeDone();
}  

EventScheduler* EventScheduler::_scheduler_ptr = NULL;

EventScheduler::EventScheduler() {
}

EventScheduler::~EventScheduler() {
    while(!Empty())
    {
        delete NextEvent();
    }                                 
}

EventScheduler* EventScheduler::Inst() {
    if (_scheduler_ptr == NULL ) {
        _scheduler_ptr = new EventScheduler();
    }
    
    return _scheduler_ptr;
}

bool EventScheduler::Empty() const {
    return _event_queue.empty();
}

void EventScheduler::AddEvent(Event* event) {
    _event_queue.push(event);
}

Event* EventScheduler::NextEvent() {
    _event_queue.pop();
    return _event_queue.top();
}

Event* EventScheduler::CurrentEvent() {
    assert(_event_queue.top()->GetTimeDone() >= 0);
    return _event_queue.top();
}

void EventScheduler::HandleCurrentEvent() {
    assert(CurrentEvent()->GetTimeDone() >= 0);
    CurrentEvent()->Callback();
}

FLOAT64 EventScheduler::GetCurrentTime() {
	if (_event_queue.empty())
		return 0.0;
    return CurrentEvent()->GetTimeDone();
}

UINT32 EventScheduler::GetSize(){
    return _event_queue.size();
}
