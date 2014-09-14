/* 
 * File:   city.h
 * Author: Yi Hu <yihu@winlab.rutgers.edu>
 *
 * Created on September 13, 2014, 10:52 PM
 */

#ifndef CITY_H
#define	CITY_H
#include <string>
#include <vector>
#include <set>
#include "overnet.h"
#include "event.h"

class CITY {
private:
    string _city;
    string _state;
    string _country;
    FLOAT32 _latitude;
    FLOAT32 _longitude;
    FLOAT32 _population;
    UINT32 _ixp_weight;
public:
    vector<UINT32> _nodeIdx_v;
    CITY(string city, string state, string country, FLOAT32 lat, FLOAT32 lon, FLOAT32 pop, UINT32 ixp_weight);
    ~CITY();
    string getCity();
    string getState();
    string getCountry();
    UINT32 getIXPWeight();
    FLOAT32 getLat();
    FLOAT32 getLon();
    FLOAT32 getPop();
    bool isGW();
};

#endif	/* CITY_H */

