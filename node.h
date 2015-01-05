/* 
 * File:   node.h
 * Author: Yi Hu <yihu@winlab.rutgers.edu>
 *
 * Created on September 13, 2014, 10:04 PM
 */

#ifndef NODE_H
#define	NODE_H

#include <string>
#include <vector>
#include <set>
#include "overnet.h"
#include "event.h"

class Node{
private:
    UINT32 _cityIdx;
    UINT32 _nodeIdx; //index in the global node table
    bool _in_service;
    bool _in_workload; //in workload_cities or not
    UINT32 _asIdx;
    FLOAT64 _last_update_time;
    
public:
    UINT32 _hashID; //hash value of the node in GNRS space, uniquely identify a node
    vector<Query_Count> _queryWrkld_v; //query workload of this simulation setting
    vector<Query_Count> _currWrkld_v; //query workload of current round
    vector<Cache_Entry> _cache;
    Node (UINT32 hashID, UINT32 asIdx, FLOAT64 time);
    ~Node();
    bool isInService();
    void setInService(FLOAT64 time);
    void setOffService(FLOAT64 time);
    bool isInWorkload();
    void setInWorkload();
    UINT32 getASIdx();
    UINT32 getNodeIdx();
    UINT32 getHashID();
    void setNodeIdx( UINT32 index);
    void setCityIdx( UINT32 index);
    UINT32 getCityIdx();
    FLOAT64 getMaxDistance(vector<UINT32> correctHost);
    FLOAT64 getMinDistance(vector<UINT32> correctHost, UINT32 & dstNodeIdx);
    FLOAT64 calInsertDelay(vector<UINT32> onlyInlocal, vector<UINT32> onlyInglobal, vector<UINT32> correctHost);
    FLOAT64 calQueryDelay(vector<UINT32> onlyInlocal, vector<UINT32> onlyInglobal, vector<UINT32> correctHost);
    FLOAT64 calQueryDelayRandomSelection(vector<UINT32> correctHost);
    bool insertGUID(UINT32 guid, FLOAT64 time);//time is the absolute finish time
    bool queryGUID(UINT32 guid, FLOAT64 time);
    bool updateGUID(UINT32 guid, FLOAT64 time);
    void calCorrectHost(set<UINT32> localHostset, set<UINT32> globalHostset, char opt);
    UINT32 cacheLookup(UINT32 guidIdx, UINT32 & myTimestamp, vector<UINT32>& remainNodePath, bool staleFlag); //return the hit nodeIdx
    void adaptGoThrough(); // adapt goThrough probability for each cache per period
};
bool NodeSortPredicate( const Node d1, const Node d2);


#endif	/* NODE_H */

