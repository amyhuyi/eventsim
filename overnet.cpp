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

bool str2StrArr(const string& s, char delim, vector<string>& storeResult){
	storeResult.clear();
	stringstream _ss(s);
	string _element;
	while(getline(_ss,_element,delim)){
        storeResult.push_back(_element);
	}
	if (storeResult.size() == 0 )
	{
		return false;
	}
	return true;
}

/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::  This function converts decimal degrees to radians             :*/
/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
double deg2rad(double deg) {
    return (deg * pi / 180);
}

/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::  This function converts radians to decimal degrees             :*/
/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
double rad2deg(double rad) {
    return (rad * 180 / pi);
}

double distance(double lat1, double lon1, double lat2, double lon2, char unit) {
    double theta, dist;
    theta = lon1 - lon2;
    dist = sin(deg2rad(lat1)) * sin(deg2rad(lat2)) + cos(deg2rad(lat1)) * cos(deg2rad(lat2)) * cos(deg2rad(theta));
    dist = acos(dist);
    dist = rad2deg(dist);
    dist = dist * 60 * 1.1515;
    switch(unit) {
        case 'M':
            break;
        case 'K':
            dist = dist * 1.609344;
            break;
        case 'N':
            dist = dist * 0.8684;
            break;
    }
    return (dist);
}


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

void Stat::PrintQueryLatencyCDF(){
    if(Query_latency_time.size()==0){
        return;
    }
    UINT32 queryTimeCnt = Query_latency_time.size();
    cout<<"PrintQueryLatencyCDF() \n";
    string latencyFile = Settings::outFileName;
    latencyFile += ".QlatCDF";
    ofstream latencyHdlr;
    latencyHdlr.open(latencyFile.c_str(),ios::out | ios::in | ios:: trunc);
    vector<FLOAT64> total_delay_v;
    for (int i = 0; i < Query_latency_time.size(); i++) {
        for (int j = 0; j < Query_latency_time[i]._delay_v.size(); j++) {
            total_delay_v.push_back(Query_latency_time[i]._delay_v[j]);
        }
    }
    sort(total_delay_v.begin(), total_delay_v.end());
    FLOAT32 pcent = 0.0;
    FLOAT32 idx;
    int total_results = total_delay_v.size();
    for (int i=0; i<total_delay_v.size(); i++) {
        if ((i+1)< total_delay_v.size()) {
            if (total_delay_v[i]<total_delay_v[i+1]) {
                idx= i+1;
                pcent = idx/total_results;
                latencyHdlr<<pcent<<'\t'<<(total_delay_v[i])<<endl;
            }
        }
        else{
            idx= i+1;
            pcent = idx/total_results;
            latencyHdlr<<pcent<<'\t'<<total_delay_v[i]<<endl;
        }
    }

}
void Stat::PrintUpdateLatencyCDF(){
    if(Insertion_latency_time.size()==0){
        return;
    }
    UINT32 updateTimeCnt = Insertion_latency_time.size();
    cout<<"PrintUpdateLatencyCDF() \n";
    string latencyFile = Settings::outFileName;
    latencyFile += ".UlatCDF";
    ofstream latencyHdlr;
    latencyHdlr.open(latencyFile.c_str(),ios::out | ios::in | ios:: trunc);
    vector<FLOAT64> total_delay_v;
    for (int i = 0; i < Insertion_latency_time.size(); i++) {
        for (int j = 0; j < Insertion_latency_time[i]._delay_v.size(); j++) {
            total_delay_v.push_back(Insertion_latency_time[i]._delay_v[j]);
        }
    }
    sort(total_delay_v.begin(), total_delay_v.end());
    //debug
    for (int i = 1; i < total_delay_v.size(); i++) {
        cout<<(total_delay_v[i])<<endl;
        if (total_delay_v[i]<total_delay_v[i-1]) {
            cout<<"Error in Sorting total_delay_v"<<endl;
        }
    }

    FLOAT32 pcent = 0.0;
    FLOAT32 idx;
    int total_results = total_delay_v.size();
    for (int i=0; i<total_delay_v.size(); i++) {
        if ((i+1)< total_delay_v.size()) {
            if (total_delay_v[i]<total_delay_v[i+1]) {
                idx= i+1;
                pcent = idx/total_results;
                latencyHdlr<<pcent<<'\t'<<(total_delay_v[i])<<endl;
            }
        }
        else{
            idx= i+1;
            pcent = idx/total_results;
            latencyHdlr<<pcent<<'\t'<<total_delay_v[i]<<endl;
        }
    }

}
