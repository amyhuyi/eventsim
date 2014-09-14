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

CITY::CITY(string city, string state, string country, FLOAT32 lat, FLOAT32 lon, FLOAT32 pop, UINT32 ixp_weight){
    _city = city;
    _state = state;
    _country = country;
    _latitude = lat;
    _longitude = lon;
    _population = pop;
    _ixp_weight = ixp_weight;
    _nodeIdx_v.clear();
}

CITY::~CITY(){
    
}

string CITY::getCity(){
    return _city;
}

string CITY::getState(){
    return _state;
}

string CITY::getCountry(){
    return _country;
}

FLOAT32 CITY::getLat(){
    return _latitude;
}

FLOAT32 CITY::getLon(){
    return _longitude;
}

FLOAT32 CITY::getPop(){
    return _population;
}
UINT32 CITY::getIXPWeight(){
    return _ixp_weight;
}
bool CITY::isGW(){
    return (_ixp_weight>0);
}

