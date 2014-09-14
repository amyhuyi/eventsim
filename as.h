/* 
 * File:   as.h
 * Author: Yi Hu <yihu@winlab.rutgers.edu>
 *
 * Created on September 13, 2014, 10:48 PM
 */

#ifndef AS_H
#define	AS_H

#include <string>
#include <vector>
#include <set>
#include "overnet.h"
#include "event.h"
class AS
{
private:
    UINT32 _tier;
    UINT32 _capacity;
    UINT32 _asIdx;
    UINT32 _asNum;
    string _asCntry; 
public:
    AS(UINT32 asindex, UINT32 tier, UINT32 capacity, UINT32 asNum, string asCountry);
    set<UINT32> _myCities;
    set <UINT32> _myNodes; //index of my nodes in the global_node_table
    set <UINT32> _local_view_offNodes; // local view of offline nodes
    ~AS();
    UINT32 getCapacity();
    UINT32 getTier();
    UINT32 getASNum();
    UINT32 getASIndex();
    string getASCntry();
    bool isGNRSMember();
    void joinGNRS(FLOAT64 lifetime);    //initialize all nodes in service
    void leaveGNRS(FLOAT64 offtime);   //remove all nodes from service
    void upgradeGNRS(UINT32 nodeIdx, FLOAT64 arrival_time,FLOAT64 lifetime,UINT32 churnRounds); //add one node in service
    void downgradeGNRS(UINT32 nodeIdx, FLOAT64 arrival_time,FLOAT64 offtime,UINT32 churnRounds); //remove one nodes from service
    void beNotifedAjoin(UINT32 nodeIdx);
    void beNotifedAleave(UINT32 nodeIdx);
};

#endif	/* AS_H */

