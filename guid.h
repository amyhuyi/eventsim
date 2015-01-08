/* 
 * File:   guid.h
 * Author: Yi Hu <yihu@winlab.rutgers.edu>
 *
 * Created on September 13, 2014, 10:50 PM
 */

#ifndef GUID_H
#define	GUID_H
#include <string>
#include <vector>
#include <set>
#include "overnet.h"
#include "event.h"
class GUID
{
private:
        UINT32 _guid;   //unique for each GUID, a hash value in GNRS space
	UINT32 _vphostIdx;  //index of the virtual primary host into global node table
	FLOAT64 _updateRate; //how long to issue an update in terms of query frequency       
        char _mobility_degree;//'L' ||  'R'||  'G'
        UINT64 _popularity; //query no. following zipf
        UINT32 _issuedQueryCnt;
        UINT32 _cacheHits;//count the number of cache hits per round
public:
        vector<UINT32> _address_q; //queue of the Node (PoP) Idx this GUID traverses among
        vector<FLOAT64> _updateTime_q;
        vector<UINT32> _replica_hosts;
        set<UINT32> _distinctErrCacheNodes;
        GUID (UINT32 id, UINT32 nodeIdx, FLOAT64 time, char mobilityDegree, UINT64 popularity); // compute objID from GUID and GNRS space range
	~GUID();
	UINT32 getGUID();
        char getMobility();
        void setMobility(char newMobilityDegree);
        FLOAT64 getUpdateRate();
        void setUpdateRate(FLOAT64 rate);
        UINT32 getvphostIdx();
        FLOAT64 getLastUpdateTime ();
        UINT32 getCurrAddrNodeIdx();
        UINT32 getNextAddrNodeIdx();
        UINT32 getAddrASIdx();
        UINT64 getPopularity();
        void updateAddrNodeIdx(UINT32 newNodeIdx, FLOAT64 time);
        bool increaseQeueryCnt(); //return whether an update has been issued
        void increaseCacheHits();
        void resetCacheHits();
        UINT32 getCacheHits();
        UINT32 getQueryCnt();
        void simulateAnUpdate();
};


#endif	/* GUID_H */

