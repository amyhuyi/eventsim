#include "overnet.h"
#include "event.h"
#include "event_scheduler.h"
#include "network.h"
#include "util.h"
#include <iostream>
#include <string>
#include <sstream>
#include <cmath>

//default setting
vector<UINT32> Stat::Ping_per_node;
vector<UINT32> Stat::Storage_per_node; //GNRS storage overhead
vector<UINT32> Stat::Workload_per_node; //GNRS answer query overhead
vector<Query_Latency> Stat::Query_latency_time;
vector<Query_Latency> Stat::Insertion_latency_time;
vector<Retry_Count> Stat::Retry_Cnt;
vector<Retry_Count> Stat::DHT_RetryCnt;
vector<UINT32> Stat::Migration_per_node;
UINT32 Stat::Premature_joins=0;
UINT32 Stat::Premature_leaves=0;

FLOAT64 Settings::EndTime = 5000;
FLOAT64 Settings::TestThreshold = 0.1;
UINT32 Settings::TotalVirtualGUID = 1000000000;
UINT32 Settings::TotalActiveGUID = 10000;	// 
UINT32 Settings::NeighborSize =0; //full range neighbor size if undefined in command line, use default 2*ceil(K/2)
UINT32 Settings::DHTHop = 5;//estimated hops for a DHT path
UINT32 Settings::GNRS_K =5;
FLOAT64 Settings::OnOffSession =10;//session length for churn
UINT32 Settings::OnOffRounds=0;//0: leave, 1: leave+join, 2: leave+join+leave...
UINT32 Settings::ChurnHours =0;//# of consecutive hours of churn generation
UINT32 Settings::QueryHours =0;//# of hours query generation
UINT32 Settings::UpdateHours =0;//# of hours update generation
FLOAT64 Settings::QueryPerNode = 10000;
FLOAT64 Settings::UpdatePerNode = 1000;
string Settings::outFileName;
FLOAT64 Settings::ChurnPerNode=0.01;
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


void ParseArg(const char * argv)
{
    string arg(argv);

    if (arg.find("gnrs_k=") != string::npos)
    {
	stringstream ss (stringstream::in | stringstream::out);
	ss <<arg.substr(8);
	ss >>Settings::GNRS_K;
    }
    else if (arg.find("endtime=") != string::npos)
    {
	stringstream ss (stringstream::in | stringstream::out);
	ss <<arg.substr(8);
	ss >>Settings::EndTime;
    }
    else if (arg.find("testthreshold=") != string::npos)
    {
	stringstream ss (stringstream::in | stringstream::out);
	ss <<arg.substr(14);
	ss >>Settings::TestThreshold;
    }
    else if (arg.find("onoffsession=") != string::npos)
    {
	stringstream ss (stringstream::in | stringstream::out);
	ss <<arg.substr(13);
	ss >>Settings::OnOffSession;
    }
    else if (arg.find("onoffrounds=") != string::npos)
    {
	stringstream ss (stringstream::in | stringstream::out);
	ss <<arg.substr(12);
	ss >>Settings::OnOffRounds;
    }
    else if (arg.find("churnhours=") != string::npos)
    {
	stringstream ss (stringstream::in | stringstream::out);
	ss <<arg.substr(11);
	ss >>Settings::ChurnHours;
    }
    else if (arg.find("queryhours=") != string::npos)
    {
	stringstream ss (stringstream::in | stringstream::out);
	ss <<arg.substr(11);
	ss >>Settings::QueryHours;
    }
    else if (arg.find("updatehours=") != string::npos)
    {
	stringstream ss (stringstream::in | stringstream::out);
	ss <<arg.substr(12);
	ss >>Settings::UpdateHours;
    }
    else if (arg.find("querypernode=") != string::npos)
    {
	stringstream ss (stringstream::in | stringstream::out);
	ss <<arg.substr(13);
	ss >>Settings::QueryPerNode;
    }
    else if (arg.find("updatepernode=") != string::npos)
    {
	stringstream ss (stringstream::in | stringstream::out);
	ss <<arg.substr(14);
	ss >>Settings::UpdatePerNode;
    }
    else if (arg.find("totalactiveguid=") != string::npos)
    {
	stringstream ss (stringstream::in | stringstream::out);
	ss <<arg.substr(16);
	ss >>Settings::TotalActiveGUID;
    }
    else if (arg.find("neighborsize=") != string::npos)
    {
	stringstream ss (stringstream::in | stringstream::out);
	ss <<arg.substr(13);
	ss >>Settings::NeighborSize;
    }
    else if (arg.find("outfilename=") != string::npos)
    {
	stringstream ss (stringstream::in | stringstream::out);
	ss <<arg.substr(12);
	ss >>Settings::outFileName;
    }
    else if (arg.find("churnpernode=") != string::npos)
    {
	stringstream ss (stringstream::in | stringstream::out);
	ss <<arg.substr(13);
	ss >>Settings::ChurnPerNode;
    }
}

int main(int argc, const char* argv[])
{
    EventScheduler::Inst()->AddEvent(new DummyEvent());
    cout <<"Initializing the network ..." <<endl;
    //Underlay::CreateInst("2220IXP_Prov4_IXP2_m_route.txt", "2220IXP_Prov4_IXP2_m_ASInfo.txt");
    //Underlay::CreateInst("2220SQ_Prov4_m_route.txt", "2220SQ_Prov4_m_ASInfo.txt");
    //argv[1]: routeFileName, argv[2]:asInfoFileName
    Underlay::CreateInst(argv[1], argv[2]);
    if(argc>3){
        for (int i = 3; i < argc; i++) {
            ParseArg(argv[i]);
        }
    }
    /*for (int i = 0; i < Underlay::Inst()->as_v.size(); i++) {
        cout<<"for as "<<i<<" tier = "<<Underlay::Inst()->as_v[i]._asIdx<<endl;
        for (int j = 0; j < Underlay::Inst()->as_v.size(); j++) {
            cout<<"latency "<<i<<" to "<<j<<" = "<<Underlay::Inst()->getLatency(i,j)<<endl;
        }
    }*/
    
    Underlay::Inst()->InitializeWorkload();
    cout<<"total # nodes "<<Underlay::Inst()->global_node_table.size()<<endl;
    //Settings::DHTHop = log10((FLOAT64)Underlay::Inst()->global_node_table.size());
    /*
    if(Settings::QueryHours > Settings::UpdateHours)
        Settings::EndTime = Settings::QueryHours;
    else
        Settings::EndTime = Settings::UpdateHours;
    */
    cout<<"Settings::"<<endl;
    cout<<"Settings::EndTime="<<Settings::EndTime<<endl;
    cout<<"Settings::TestThreshold="<<Settings::TestThreshold<<endl;
    cout<<"Settings::TotalVirtualGUID="<<Settings::TotalVirtualGUID<<endl;
    cout<<"Settings::TotalActiveGUID="<<Settings::TotalActiveGUID<<endl;	// 
    cout<<"Settings::NeighborSize="<<Settings::NeighborSize<<endl; //full range neighbor size if undefined in command line, use default 2*ceil(K/2)
    cout<<"Settings::DHTHop="<<Settings::DHTHop<<endl;//estimated hops for a DHT path
    cout<<"Settings::GNRS_K="<<Settings::GNRS_K<<endl;
    cout<<"Settings::OnOffSession="<<Settings::OnOffSession<<endl;//session length for churn
    cout<<"Settings::OnOffRounds="<<Settings::OnOffRounds<<endl;//0: leave, 1: leave+join, 2: leave+join+leave...
    cout<<"Settings::ChurnHours="<<Settings::ChurnHours<<endl;//# of consecutive hours of churn generation
    cout<<"Settings::QueryHours="<<Settings::QueryHours<<endl;//# of hours query generation
    cout<<"Settings::UpdateHours="<<Settings::UpdateHours<<endl;//# of hours update generation
    cout<<"Settings::QueryPerNode="<<Settings::QueryPerNode<<endl;
    cout<<"Settings::UpdatePerNode="<<Settings::UpdatePerNode<<endl;
     cout<<"Settings::ChurnPerNode="<<Settings::ChurnPerNode<<endl;
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
    UINT32 totalNodes = Underlay::Inst()->global_node_table.size();
    if(Settings::ChurnHours)
        Underlay::Inst()->generateLeaveChurn(Settings::ChurnHours, Settings::ChurnPerNode*totalNodes, 
                Settings::OnOffSession, Settings::OnOffRounds);
    /*
    if(Settings::QueryHours)
        Underlay::Inst()->generateWorkload(Settings::QueryHours,Settings::QueryPerNode*totalNodes,'Q');
    if(Settings::UpdateHours)
        Underlay::Inst()->generateWorkload(Settings::UpdateHours,Settings::UpdatePerNode*totalNodes,'U');
     * */
    cout <<"total queued jobs: " <<EventScheduler::Inst()->GetSize() <<" info: ";
    while ( EventScheduler::Inst()->GetCurrentTime() <= Settings::EndTime
            && EventScheduler::Inst()->GetSize()>1){
	Event * pevent = EventScheduler::Inst()->CurrentEvent();
	//cout <<"queued jobs: " <<EventScheduler::Inst()->GetSize() <<" info: ";
	pevent->PrintInfo();
	if (pevent->Callback()){
            delete pevent;
	}
	EventScheduler::Inst()->NextEvent();
    }
    /*
    for (int i = 0; i < Stat::Query_latency_time.size(); i++) {
       cout<<Stat::Query_latency_time[i]._delay<<" "<<Stat::Query_latency_time[i]._time<<endl;
    }
    cout<<"Stat::Insertion_latency_time.size() "<<Stat::Insertion_latency_time.size()<<endl;
    for (int i = 0; i < Stat::Insertion_latency_time.size(); i++) {
       cout<<Stat::Insertion_latency_time[i]._delay<<" "<<Stat::Insertion_latency_time[i]._time<<endl;
    }
    cout<<"Stat::Retry_Cnt.size()"<<Stat::Retry_Cnt.size()<<endl;
    for (int i = 0; i < Stat::Retry_Cnt.size();i++) {
       cout<<Stat::Retry_Cnt[i]._retry<<" "<<Stat::Retry_Cnt[i]._operation<<" "<<Stat::Retry_Cnt[i]._time<<" "<<Stat::Retry_Cnt[i]._delay<<endl;
    }
    
    for (int i = 0; i < Stat::Stat::DHT_Retry_time.size(); i++) {
       cout<<Stat::Stat::DHT_Retry_time[i]._retry<<" "<<Stat::Stat::DHT_Retry_time[i]._operation<<" "<<Stat::Stat::DHT_Retry_time[i]._time<<endl;
    }
    */
    
    cout<<"Stat::Retry_Cnt.size()"<<Stat::Retry_Cnt.size()<<endl;
    for (int i = 0; i < Stat::Retry_Cnt.size();i++) {
       cout<<Stat::Retry_Cnt[i]._time<<" "<<Stat::Retry_Cnt[i]._retryUpdate<<" "<<Stat::Retry_Cnt[i]._retryQuery<<endl;
    }
    
    for (int i = 0; i < Stat::DHT_RetryCnt.size(); i++) {
       cout<<Stat::DHT_RetryCnt[i]._time<<" "<<Stat::DHT_RetryCnt[i]._retryUpdate<<" "<<Stat::DHT_RetryCnt[i]._retryQuery<<endl;
    }
    Stat::Inst()->PrintRetryStat();
    Stat::Inst()->PrintLatencyStat();
    /*for (int i = 0; i < 10; i++) {
        for(int j=0; j<10; j++)
            cout<<i+j+1<<" ";
    }*/
    
}
