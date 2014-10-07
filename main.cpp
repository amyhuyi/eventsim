#include "overnet.h"
#include "event.h"
#include "event_scheduler.h"
#include "network.h"
#include "util.h"


//default setting
vector<UINT32> Stat::Ping_per_node;
vector<UINT32> Stat::Storage_per_node; //GNRS storage overhead
vector<Wrkld_Count> Stat::Workload_per_node;
vector<UINT32> Stat::CacheHit_per_guid;
vector<UINT32> Stat::QueryHopCnt;
vector<UINT32> Stat::QueryHitHopCnt;
vector<Query_Latency> Stat::Query_latency_time;
vector<Query_Latency> Stat::Insertion_latency_time;
vector<Retry_Count> Stat::Retry_Cnt;
vector<Retry_Count> Stat::DHT_RetryCnt;
vector<UINT32> Stat::Error_cnt_per_guid;
vector<Error_Entry> Stat::Error_stat;
vector<UINT32> Stat::Migration_per_node;
UINT32 Stat::Premature_joins=0;
UINT32 Stat::Premature_leaves=0;
UINT32 Settings::CacheLookupLat =2; //cache lookup latency at one hop in ms
FLOAT64 Settings::EndTime = 50;
FLOAT64 Settings::TestThreshold = 0.1;
UINT32 Settings::ActiveGUIDperPoP = 10;	// 
UINT32 Settings::NeighborSize =0; //full range neighbor size if undefined in command line, use default 2*ceil(K/2)
UINT32 Settings::GNRS_K =5;
UINT32 Settings::Local_K =1;
UINT32 Settings::Regional_K =1;
FLOAT64 Settings::OnOffSession =10;//session length for churn
UINT32 Settings::OnOffRounds=0;//0: leave, 1: leave+join, 2: leave+join+leave...
UINT32 Settings::ChurnHours =0;//# of consecutive hours of churn generation
UINT32 Settings::QueryHours =0;//# of hours query generation
UINT32 Settings::UpdateHours =0;//# of hours update generation
FLOAT64 Settings::QueryPerNode = 10000;
FLOAT64 Settings::UpdatePerNode = 1000;
string Settings::outFileName;
FLOAT64 Settings::ChurnPerNode=0.01;
bool Settings::Geo_Lat_On = false;
bool Settings::LocMobSync = true;
UINT32 Settings::QueryPerGUID = 200;
FLOAT32 Settings::InterLatWeight = 0.0;
FLOAT32 Settings::IntraLatWeight = 0.0;
FLOAT32 Settings::StrongLocalityPerc = 0.5;
FLOAT32 Settings::MedLocalityPerc = 0.3;
FLOAT32 Settings::LocalMobilityPerc = 0.6;
FLOAT32 Settings::RegionalMobilityPerc =0.2;
bool Settings::DeployOnlyGW = 0;
UINT32 Settings::CacheOn =0;
bool Settings::balanceBase =0;
UINT32 Settings::QueryOriginBalance =3;
FLOAT32 Settings::CachePerc = 0.1;
FLOAT32 Settings::GoThroughProb = 0.001;
FLOAT32 Settings::UpdateFrqGUID = 0.01;
UINT32 Settings::CurrentClock =0;
UINT32 Settings::QueryPerClock =10;
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
	ss <<arg.substr(7);
	ss >>Settings::GNRS_K;
    }
    else if (arg.find("local_k=") != string::npos)
    {
	stringstream ss (stringstream::in | stringstream::out);
	ss <<arg.substr(8);
	ss >>Settings::Local_K;
    }
    else if (arg.find("regional_k=") != string::npos)
    {
	stringstream ss (stringstream::in | stringstream::out);
	ss <<arg.substr(11);
	ss >>Settings::Regional_K;
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
    else if (arg.find("activeguidperpop=") != string::npos)
    {
	stringstream ss (stringstream::in | stringstream::out);
	ss <<arg.substr(17);
	ss >>Settings::ActiveGUIDperPoP;
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
    else if (arg.find("geo_lat_on=") != string::npos)
    {
	stringstream ss (stringstream::in | stringstream::out);
	ss <<arg.substr(11);
	ss >>Settings::Geo_Lat_On;
    }
    else if (arg.find("intralatweight=") != string::npos)
    {
	stringstream ss (stringstream::in | stringstream::out);
	ss <<arg.substr(15);
	ss >>Settings::IntraLatWeight;
    }
    else if (arg.find("interlatweight=") != string::npos)
    {
	stringstream ss (stringstream::in | stringstream::out);
	ss <<arg.substr(15);
	ss >>Settings::InterLatWeight;
    }
    else if (arg.find("stronglocalityperc=") != string::npos)
    {
	stringstream ss (stringstream::in | stringstream::out);
	ss <<arg.substr(19);
	ss >>Settings::StrongLocalityPerc;
    }
    else if (arg.find("medlocalityperc=") != string::npos)
    {
	stringstream ss (stringstream::in | stringstream::out);
	ss <<arg.substr(16);
	ss >>Settings::MedLocalityPerc;
    }
    else if (arg.find("locmobsync=") != string::npos)
    {
	stringstream ss (stringstream::in | stringstream::out);
	ss <<arg.substr(11);
	ss >>Settings::LocMobSync;
    }
    else if (arg.find("localmobilityperc=") != string::npos)
    {
	stringstream ss (stringstream::in | stringstream::out);
	ss <<arg.substr(18);
	ss >>Settings::LocalMobilityPerc;
    }
    else if (arg.find("regionalmobilityperc=") != string::npos)
    {
	stringstream ss (stringstream::in | stringstream::out);
	ss <<arg.substr(21);
	ss >>Settings::RegionalMobilityPerc;
    }
    else if (arg.find("queryperguid=") != string::npos)
    {
	stringstream ss (stringstream::in | stringstream::out);
	ss <<arg.substr(13);
	ss >>Settings::QueryPerGUID;
    }
    else if (arg.find("deployongw=") != string::npos)
    {
	stringstream ss (stringstream::in | stringstream::out);
	ss <<arg.substr(11);
	ss >>Settings::DeployOnlyGW;
    }
    else if (arg.find("cacheon=") != string::npos) {
        stringstream ss(stringstream::in | stringstream::out);
        ss << arg.substr(8);
        ss >> Settings::CacheOn;
    }
    else if (arg.find("cachelookuplat=") != string::npos)
    {
	stringstream ss (stringstream::in | stringstream::out);
	ss <<arg.substr(15);
	ss >>Settings::CacheLookupLat;
    }
    else if (arg.find("balancebase=") != string::npos)
    {
	stringstream ss (stringstream::in | stringstream::out);
	ss <<arg.substr(12);
	ss >>Settings::balanceBase;
    }
    else if (arg.find("queryoriginbalance=") != string::npos)
    {
	stringstream ss (stringstream::in | stringstream::out);
	ss <<arg.substr(19);
	ss >>Settings::QueryOriginBalance;
    }
    else if (arg.find("cacheperc=") != string::npos)
    {
	stringstream ss (stringstream::in | stringstream::out);
	ss <<arg.substr(10);
	ss >>Settings::CachePerc;
    }
    else if (arg.find("gothroughprob=") != string::npos)
    {
	stringstream ss (stringstream::in | stringstream::out);
	ss <<arg.substr(14);
	ss >>Settings::GoThroughProb;
    }
    else if (arg.find("updatefrqguid=") != string::npos)
    {
	stringstream ss (stringstream::in | stringstream::out);
	ss <<arg.substr(14);
	ss >>Settings::UpdateFrqGUID;
    }
    else if (arg.find("queryperclock=") != string::npos)
    {
	stringstream ss (stringstream::in | stringstream::out);
	ss <<arg.substr(14);
	ss >>Settings::QueryPerClock;
    }
}

int main(int argc, const char* argv[])
{
    EventScheduler::Inst()->AddEvent(new DummyEvent());
    cout <<"Initializing the network ..." <<endl;
    //argv[1]: cityFileName, argv[2]:routeFileName, argv[3]:asInfoFileName, argv[4]: predicateFile
    Underlay::CreateInst(argv[1], argv[2], argv[3],argv[4]);
    if(argc>=5){
        for (int i = 5; i < argc; i++) {
            ParseArg(argv[i]);
        }
    }else{
        cout<<"USAGE:: argv[1]: cityFileName, argv[2]:routeFileName, argv[3]:asInfoFileName,argv[4]: predicateFile"<<endl;
        abort();
    }
    cout<<"total # nodes "<<Underlay::Inst()->global_node_table.size()<<endl;
    cout<<"Settings::"<<endl;
    cout<<"Settings::EndTime="<<Settings::EndTime<<endl;
    cout<<"Settings::TestThreshold="<<Settings::TestThreshold<<endl;
    cout<<"Settings::ActiveGUIDperPoP="<<Settings::ActiveGUIDperPoP<<endl;	// 
    cout<<"Settings::NeighborSize="<<Settings::NeighborSize<<endl; //full range neighbor size if undefined in command line, use default 2*ceil(K/2)
    cout<<"Settings::GNRS_K="<<Settings::GNRS_K<<endl;
    cout<<"Settings::Local_K="<<Settings::Local_K<<endl;
    cout<<"Settings::Regional_K="<<Settings::Regional_K<<endl;
    cout<<"Settings::OnOffSession="<<Settings::OnOffSession<<endl;//session length for churn
    cout<<"Settings::OnOffRounds="<<Settings::OnOffRounds<<endl;//0: leave, 1: leave+join, 2: leave+join+leave...
    cout<<"Settings::ChurnHours="<<Settings::ChurnHours<<endl;//# of consecutive hours of churn generation
    cout<<"Settings::QueryHours="<<Settings::QueryHours<<endl;//# of hours query generation
    cout<<"Settings::UpdateHours="<<Settings::UpdateHours<<endl;//# of hours update generation
    cout<<"Settings::QueryPerNode="<<Settings::QueryPerNode<<endl;
    cout<<"Settings::UpdatePerNode="<<Settings::UpdatePerNode<<endl;
    cout<<"Settings::ChurnPerNode="<<Settings::ChurnPerNode<<endl;
    cout<<"Settings::MedLocalityPerc="<<Settings::MedLocalityPerc<<endl;
    cout<<"Settings::StrongLocalityPerc="<<Settings::StrongLocalityPerc<<endl;
    cout<<"Settings::LocalMobilityPerc="<<Settings::LocalMobilityPerc<<endl;
    cout<<"Settings::RegionalMobilityPerc="<<Settings::RegionalMobilityPerc<<endl;
    cout<<"Settings::QueryPerGUID="<<Settings::QueryPerGUID<<endl;
    cout<<"Settings::CachePerc="<<Settings::CachePerc<<endl;
    cout<<"Settings::CacheOn="<<Settings::CacheOn<<endl;
    cout<<"Settings::CacheLookupLat="<<Settings::CacheLookupLat<<endl;
    cout<<"Settings::GoThroughProb="<<Settings::GoThroughProb<<endl;
    cout<<"Settings::UpdateFrqGUID="<<Settings::UpdateFrqGUID<<endl;
    cout<<"Settings::QueryOriginBalance="<<Settings::QueryOriginBalance<<endl;
    cout<<"Settings::CurrentClock="<<Settings::CurrentClock<<endl;
    cout<<"Settings::QueryPerClock="<<Settings::QueryPerClock<<endl;
    if (Settings::Geo_Lat_On) {
        cout<<"Settings::Geo_Lat_On = true"<<endl;
    } else {
        cout<<"Settings::Geo_Lat_On = false"<<endl;
    }
    if (Settings::LocMobSync) {
        cout<<"Settings::LocMobSync = true"<<endl;
    } else {
        cout<<"Settings::LocMobSync = false"<<endl;
    }
    if (Settings::DeployOnlyGW) {
        cout<<"Settings::DeployOnlyGW = true"<<endl;
    } else {
        cout<<"Settings::DeployOnlyGW = false"<<endl;
    }
    if (Settings::balanceBase) {
        cout<<"Settings::balanceBase = true"<<endl;
    } else {
        cout<<"Settings::balanceBase = false"<<endl;
    }
    Underlay::Inst()->InitializeStat(); //finish node initialization, prepare node stat
    Underlay::Inst()->InitializeWorkload(); //finish guid init   
    Underlay::Inst()->PrepareWorkloadCal(); //prepare all guid stat
    
    
    UINT32 totalNodes = Underlay::Inst()->global_node_table.size();
    //Underlay::Inst()->calStorageWorkload();
    Underlay::Inst()->calQueryWorkload();
    if(Settings::ChurnHours)
        Underlay::Inst()->generateLeaveChurn(Settings::ChurnHours, Settings::ChurnPerNode*totalNodes, 
                Settings::OnOffSession, Settings::OnOffRounds);
    cout <<"EventScheduler::Inst()->GetCurrentTime() =" <<EventScheduler::Inst()->GetCurrentTime()<<endl;
    
    while ( EventScheduler::Inst()->GetCurrentTime() <= Settings::EndTime){
	Event * pevent = EventScheduler::Inst()->CurrentEvent();
	//cout <<"queued jobs: " <<EventScheduler::Inst()->GetSize() <<" info: ";
	//pevent->PrintInfo();
	if (pevent->Callback()){
            delete pevent;
	}
	EventScheduler::Inst()->NextEvent();
    }
    cout<<"Stat::Retry_Cnt.size()"<<Stat::Retry_Cnt.size()<<endl;
    for (int i = 0; i < Stat::Retry_Cnt.size();i++) {
       cout<<Stat::Retry_Cnt[i]._time<<" "<<Stat::Retry_Cnt[i]._retryUpdate<<" "<<Stat::Retry_Cnt[i]._retryQuery<<endl;
    }
    
    for (int i = 0; i < Stat::DHT_RetryCnt.size(); i++) {
       cout<<Stat::DHT_RetryCnt[i]._time<<" "<<Stat::DHT_RetryCnt[i]._retryUpdate<<" "<<Stat::DHT_RetryCnt[i]._retryQuery<<endl;
    }
    //Stat::Inst()->PrintRetryStat();
    //Stat::Inst()->PrintLatencyStat();
    //Stat::Inst()->PrintQueryLatencyCDF();
    //Stat::Inst()->PrintUpdateLatencyCDF();
    //Util::Inst()->matchPareto("/Users/yihu/Downloads/genPareto", 1, 0.78);
    //vector<UINT32> results_v;
    //Util::Inst()->getParetoVec(2.04,1000,results_v);
    //Util::Inst()->genCDF("./WkldBlc/zipf/3.04_10k_cdf.csv", results_v);
    //Util::Inst()->genPDF("./WkldBlc/zipf/3.04_10k_pdf.csv", results_v);
}