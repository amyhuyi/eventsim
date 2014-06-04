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
//#include "network.h"
#include "event_scheduler.h"
#include "util.h"
#include "overnet.h"

Stat* Stat::_stat_ptr = NULL;

Stat::Stat(){
    
}

Stat::~Stat(){
    
}

Stat* Stat::Inst(){
    if (_stat_ptr==NULL){
        _stat_ptr = new Stat();
    }
    return _stat_ptr;
}


void Stat::PrintRetryStat()
{   
    if(!Retry_Cnt.size()){
        cout<<"Empty Stat::Retry_Count_time.size() = "<<Retry_Cnt.size()<<endl;
        return;
    }
    cout<<"print retry stat\n";
    //format time nodeCnt querymsgCnt InsertionMsgCnt totalMsgCnt
    string retryFile = Settings::outFileName;
    retryFile += ".retry";
    ofstream retryHdlr;
    retryHdlr.open(retryFile.c_str(),ios::out | ios::in | ios:: trunc);

    sort(Retry_Cnt.begin(),Retry_Cnt.end());
    for (int i = 0; i < Retry_Cnt.size(); i++) {
        //sort(Retry_Cnt[i]._Qdelay.begin(), Retry_Cnt[i]._Qdelay.end());
        //sort(Retry_Cnt[i]._Udelay.begin(), Retry_Cnt[i]._Udelay.end());
        retryHdlr<<Retry_Cnt[i]._time<<" "<<(FLOAT32)Retry_Cnt[i]._retryUpdate/(FLOAT32)Retry_Cnt[i]._issuedUpdate
                <<" "<<(FLOAT32)Retry_Cnt[i]._retryQuery/(FLOAT32)Retry_Cnt[i]._issuedQuery<<" ";
        if(Retry_Cnt[i]._retryUpdate){
            retryHdlr<<(FLOAT32)Retry_Cnt[i]._retryUMsg/(FLOAT32)Retry_Cnt[i]._retryUpdate<<" "
                    <<Retry_Cnt[i]._Udelay[0]<<" "
                    <<Retry_Cnt[i]._Udelay[0.25*Retry_Cnt[i]._Udelay.size()]<<" "
                    <<Retry_Cnt[i]._Udelay[0.5*Retry_Cnt[i]._Udelay.size()]<<" "
                    <<Retry_Cnt[i]._Udelay[0.75*Retry_Cnt[i]._Udelay.size()]<<" "
                    <<Retry_Cnt[i]._Udelay[0.95*Retry_Cnt[i]._Udelay.size()]<<" ";
        }
        else{
            retryHdlr<<0<<" "<<0<<" "<<0<<" "<<0<<" "<<0<<" "<<0<<" ";
        }
        if(Retry_Cnt[i]._retryQuery){
            retryHdlr<<(FLOAT32)Retry_Cnt[i]._retryQMsg/(FLOAT32)Retry_Cnt[i]._retryQuery<<" "
                    <<Retry_Cnt[i]._Qdelay[0]<<" "
                    <<Retry_Cnt[i]._Qdelay[0.25*Retry_Cnt[i]._Qdelay.size()]<<" "
                    <<Retry_Cnt[i]._Qdelay[0.5*Retry_Cnt[i]._Qdelay.size()]<<" "
                    <<Retry_Cnt[i]._Qdelay[0.75*Retry_Cnt[i]._Qdelay.size()]<<" "
                    <<Retry_Cnt[i]._Qdelay[0.95*Retry_Cnt[i]._Qdelay.size()]<<endl;
        }
        else{
            retryHdlr<<0<<" "<<0<<" "<<0<<" "<<0<<" "<<0<<" "<<0<<endl;
        }
    }

}

void Stat::PrintLatencyStat(){
    if(Query_latency_time.size()==0 && Insertion_latency_time.size()==0){
        cout<<"Empty both: Query_latency_time.size() = "<<Query_latency_time.size()
                <<"Insertion_latency_time.size() = "<<Insertion_latency_time.size()<<endl;
        return;
    }
    UINT32 queryTimeCnt = Query_latency_time.size();
    UINT32 insertionTimeCnt = Insertion_latency_time.size();
    cout<<"print latency stat\n";
    //format time nodeCnt querymsgCnt InsertionMsgCnt totalMsgCnt
    string latencyFile = Settings::outFileName;
    latencyFile += ".latency";
    ofstream latencyHdlr;
    latencyHdlr.open(latencyFile.c_str(),ios::out | ios::in | ios:: trunc);
    UINT32 currIdx=0;
    while (currIdx<queryTimeCnt || currIdx<insertionTimeCnt) {
        if(currIdx<queryTimeCnt && Query_latency_time[currIdx]._delay_v.size()){
            sort(Query_latency_time[currIdx]._delay_v.begin(), Query_latency_time[currIdx]._delay_v.end());
            latencyHdlr<<'Q'<<" "<<Query_latency_time[currIdx]._time<<" "<<Query_latency_time[currIdx]._delay_v[0]
                    <<" "<<Query_latency_time[currIdx]._delay_v[0.25*Query_latency_time[currIdx]._delay_v.size()]
                    <<" "<<Query_latency_time[currIdx]._delay_v[0.5*Query_latency_time[currIdx]._delay_v.size()]
                    <<" "<<Query_latency_time[currIdx]._delay_v[0.75*Query_latency_time[currIdx]._delay_v.size()]
                    <<" "<<Query_latency_time[currIdx]._delay_v[0.97*Query_latency_time[currIdx]._delay_v.size()]<<" ";
        }
        else{
            latencyHdlr<<'Q'<<" "<<0<<" "<<0<<" "<<0<<" "<<0<<" "<<0<<" ";
        }
        if(currIdx<insertionTimeCnt && Insertion_latency_time[currIdx]._delay_v.size()){
            sort(Insertion_latency_time[currIdx]._delay_v.begin(),Insertion_latency_time[currIdx]._delay_v.end());
            latencyHdlr<<'I'<<" "<<Insertion_latency_time[currIdx]._time<<" "<<Insertion_latency_time[currIdx]._delay_v[0]
                    <<" "<<Insertion_latency_time[currIdx]._delay_v[0.25*Insertion_latency_time[currIdx]._delay_v.size()]
                    <<" "<<Insertion_latency_time[currIdx]._delay_v[0.5*Insertion_latency_time[currIdx]._delay_v.size()]
                    <<" "<<Insertion_latency_time[currIdx]._delay_v[0.75*Insertion_latency_time[currIdx]._delay_v.size()]
                    <<" "<<Insertion_latency_time[currIdx]._delay_v[0.95*Insertion_latency_time[currIdx]._delay_v.size()];
        }
        else{
            latencyHdlr<<'I'<<" "<<0<<" "<<0<<" "<<0<<" "<<0<<" "<<0<<" ";
        }
        latencyHdlr<<endl;
        currIdx++;
    }

    
}
