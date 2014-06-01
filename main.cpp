#include "overnet.h"
#include "event.h"
#include "event_scheduler.h"
#include "network.h"
#include "util.h"
#include <iostream>
#include <string>
#include <sstream>

//default setting
vector<UINT32> Stat::Ping_per_node;
vector<UINT32> Stat::Storage_per_node; //GNRS storage overhead
vector<UINT32> Stat::Workload_per_node; //GNRS answer query overhead
vector<Query_Latency> Stat::Query_latency_time;
vector<Query_Latency> Stat::Insertion_latency_time;
vector<Retry_Count> Stat::Retry_Cnt;
vector<Retry_Count> Stat::DHT_Retry_time;
vector<UINT32> Stat::Migration_per_node;
UINT32 Stat::Premature_joins=0;
UINT32 Stat::Premature_leaves=0;

UINT32 Settings::NumOfAS =0;
FLOAT64 Settings::EndTime = 500000;
FLOAT64 Settings::TestDuration = 0.1;
UINT32 Settings::TotalVirtualGUID = 1000000000;
UINT32 Settings::TotalActiveGUID = 10000;	// 
UINT32 Settings::NeighborSize =0; //full range neighbor size if undefined in command line, use default 2*ceil(K/2)
UINT32 Settings::DHTHop = 5;//estimated hops for a DHT path

/*!
 *  @brief Computes floor(log2(n))
 *  Works by finding position of MSB set.
 *  @returns -1 if n == 0.
 */
static inline INT32 FloorLog2(UINT32 n)
{
	INT32 p = 0;

	if (n == 0) return -1;

	if (n & 0xffff0000) { p += 16; n >>= 16; }
	if (n & 0x0000ff00) { p +=  8; n >>=  8; }
	if (n & 0x000000f0) { p +=  4; n >>=  4; }
	if (n & 0x0000000c) { p +=  2; n >>=  2; }
	if (n & 0x00000002) { p +=  1; }
        return p;
}

/*!
 *  @brief Computes floor(log2(n))
 *  Works by finding position of MSB set.
 *  @returns -1 if n == 0.
 */
static inline INT32 CeilLog2(UINT32 n)
{
	return FloorLog2(n - 1) + 1;
}


/*void ParseArg(const char * argv)
{
    string arg(argv);

    if (arg.find("numofnodes=") != string::npos)
    {
	stringstream ss (stringstream::in | stringstream::out);
	ss <<arg.substr(11);
	ss >>Settings::NumOfNodes;
    }
    else if (arg.find("numofobjs=") != string::npos)
    {
	stringstream ss (stringstream::in | stringstream::out);
	ss <<arg.substr(10);
	ss >>Settings::NumOfObjs;
    }
}*/

int main(int argc, const char* argv[])
{
    
    EventScheduler::Inst()->AddEvent(new DummyEvent());
    cout <<"Initializing the network ..." <<endl;
    //Underlay::CreateInst("2220IXP_Prov4_IXP2_m_route.txt", "2220IXP_Prov4_IXP2_m_ASInfo.txt");
    Underlay::CreateInst("2220SQ_Prov4_m_route.txt", "2220SQ_Prov4_m_ASInfo.txt");
    
    /*for (int i = 0; i < Underlay::Inst()->as_v.size(); i++) {
        cout<<"for as "<<i<<" tier = "<<Underlay::Inst()->as_v[i]._asIdx<<endl;
        for (int j = 0; j < Underlay::Inst()->as_v.size(); j++) {
            cout<<"latency "<<i<<" to "<<j<<" = "<<Underlay::Inst()->getLatency(i,j)<<endl;
        }
    }*/
    
    Underlay::Inst()->InitializeWorkload();
    cout<<"total # nodes "<<Underlay::Inst()->global_node_table.size()<<endl;
    /*
    for (int i = 0; i < Underlay::Inst()->global_node_table.size(); i++) {
        cout<<"for node "<<i <<" "<<Underlay::Inst()->global_node_table[i].getASIdx()<<endl;
    }*/
    for (UINT32 i = 0; i < Underlay::Inst()->GetNumOfNode(); i++) {
        Stat::Migration_per_node.push_back(0);
        Stat::Ping_per_node.push_back(0);
        Stat::Storage_per_node.push_back(0);
        Stat::Workload_per_node.push_back(0);
    }
    
    Underlay::Inst()->generateWorkload(1,100,'Q');
    Underlay::Inst()->generateWorkload(1,100,'U');
    while ( ! EventScheduler::Inst()->Empty() && EventScheduler::Inst()->GetCurrentTime() <= Settings::EndTime
            && EventScheduler::Inst()->GetSize()>1){
	Event * pevent = EventScheduler::Inst()->CurrentEvent();
	//cout <<"queued jobs: " <<EventScheduler::Inst()->GetSize() <<" info: ";
	//pevent->PrintInfo();
	if (pevent->Callback()){
            delete pevent;
	}
	EventScheduler::Inst()->NextEvent();
    }
    //cout<<"AS 0 leave..."<<endl;
    //Underlay::Inst()->as_v[0].leaveGNRS(EventScheduler::Inst()->GetCurrentTime());
    while ( ! EventScheduler::Inst()->Empty() && EventScheduler::Inst()->GetCurrentTime() <= Settings::EndTime
            && EventScheduler::Inst()->GetSize()>1){
	Event * pevent = EventScheduler::Inst()->CurrentEvent();
	//cout <<"queued jobs: " <<EventScheduler::Inst()->GetSize() <<" info: ";
	pevent->PrintInfo();
	if (pevent->Callback()){
            delete pevent;
	}
	EventScheduler::Inst()->NextEvent();
    }
    Underlay::Inst()->generateLeaveChurn(5, 0.01*Underlay::Inst()->global_node_table.size(), 5, 0);
    Underlay::Inst()->generateWorkload(1000,Underlay::Inst()->global_node_table.size(),'Q');
    //Underlay::Inst()->generateWorkload(1000,Underlay::Inst()->global_node_table.size(),'U');
    
    while ( ! EventScheduler::Inst()->Empty() && EventScheduler::Inst()->GetCurrentTime() <= Settings::EndTime
            && EventScheduler::Inst()->GetSize()>1){
	Event * pevent = EventScheduler::Inst()->CurrentEvent();
	//cout <<"queued jobs: " <<EventScheduler::Inst()->GetSize() <<" info: ";
	//pevent->PrintInfo();
	if (pevent->Callback()){
            delete pevent;
	}
	EventScheduler::Inst()->NextEvent();
    }
    
    //Underlay::Inst()->SynchNetwork();
    //Underlay::Inst()->generateWorkload(100,100,'Q');
    //Underlay::Inst()->generateWorkload(1,100,'U');
    
    while ( ! EventScheduler::Inst()->Empty() && EventScheduler::Inst()->GetCurrentTime() <= Settings::EndTime
            && EventScheduler::Inst()->GetSize()>1){
	Event * pevent = EventScheduler::Inst()->CurrentEvent();
	//cout <<"queued jobs: " <<EventScheduler::Inst()->GetSize() <<" info: ";
	//pevent->PrintInfo();
	if (pevent->Callback()){
            delete pevent;
	}
	EventScheduler::Inst()->NextEvent();
    }
    cout<<"Stat::Query_latency_time.size() "<<Stat::Query_latency_time.size()<<endl;
    for (int i = 0; i < Stat::Query_latency_time.size(); i++) {
       cout<<Stat::Query_latency_time[i]._delay<<" "<<Stat::Query_latency_time[i]._operation<<" "<<Stat::Query_latency_time[i]._time<<endl;
    }
    cout<<"Stat::Insertion_latency_time.size() "<<Stat::Insertion_latency_time.size()<<endl;
    for (int i = 0; i < Stat::Insertion_latency_time.size(); i++) {
       cout<<Stat::Insertion_latency_time[i]._delay<<" "<<Stat::Insertion_latency_time[i]._operation<<" "<<Stat::Insertion_latency_time[i]._time<<endl;
    }
    cout<<"Stat::Retry_Cnt.size()"<<Stat::Retry_Cnt.size()<<endl;
    for (int i = 0; i < Stat::Retry_Cnt.size();i++) {
       cout<<Stat::Retry_Cnt[i]._retry<<" "<<Stat::Retry_Cnt[i]._operation<<" "<<Stat::Retry_Cnt[i]._time<<" "<<Stat::Retry_Cnt[i]._delay<<endl;
    }
    for (int i = 0; i < Stat::Stat::DHT_Retry_time.size(); i++) {
       cout<<Stat::Stat::DHT_Retry_time[i]._retry<<" "<<Stat::Stat::DHT_Retry_time[i]._operation<<" "<<Stat::Stat::DHT_Retry_time[i]._time<<endl;
    }
    Underlay::Inst()->PrintRetryStat();
    /*for (int i = 0; i < 10; i++) {
        for(int j=0; j<10; j++)
            cout<<i+j+1<<" ";
    }*/
    
}
