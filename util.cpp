#include "util.h"
#include "network.h"

Util* Util::_inst = NULL;

Util::Util() {
    Random::Set(_urng);
    _poisson = new Poisson(ARRIVAL_MEAN);
    _uniform = new Uniform();
    _normal = new Normal();
    _gamma = new Gamma(SHAPE_GAMMA);
    _pareto = new Pareto(SHAPE_PARETO);
}

Util::~Util() {
    delete _uniform;
    delete _normal;
    delete _gamma;
    delete _poisson;
    delete _pareto;
}

Util* Util::Inst() {
    if (_inst == NULL)
        _inst = new Util();
    return _inst;
}

UINT32 Util::GenInt(UINT32 bound) {
    return (UINT32) (_uniform->Next() * bound);
}

FLOAT64 Util::GenUniform()
{
    return (FLOAT64) (_uniform->Next());
}
    
FLOAT64 Util::GenGamma() {
    return (FLOAT64) (_gamma->Next() * SCALE_GAMMA);
}
    
FLOAT64 Util::GenNormal() {
    return (FLOAT64) (_normal->Next());
}

FLOAT64 Util::GenPoisson(){
    return (FLOAT64) (_poisson->Next());
}

void Util::ResetPoisson(FLOAT64 thisMean){
    delete _poisson;
    _poisson = new Poisson(thisMean);
}

FLOAT64 Util::GenPareto(){
    return (FLOAT64) (_pareto->Next());
}

void Util::ResetPareto(FLOAT64 thisShape){
    delete _pareto;
    _pareto = new Pareto(thisShape);
}
UINT32 Util::genPareto(FLOAT64 thisShape, FLOAT64 maxValue){
    ResetPareto(thisShape);
    vector<UINT32> capacity_v;
    UINT32 currRand;
    unsigned long long total_value=0;
    for (int i = 0; i < 4000000; i++) {
        currRand = GenPareto();
        if (currRand <= maxValue) {
            capacity_v.push_back(currRand);
            total_value += currRand;
        }
    }
    total_value = total_value / capacity_v.size();
    return total_value;
}

void Util::getParetoVec(FLOAT64 thisShape, long int totalNo, vector<UINT32>& results_v){
    ResetPareto(thisShape);
    results_v.clear();
    for (int i = 0; i < totalNo; i++) {
        results_v.push_back(GenPareto());
    }
}

UINT32 Util::calParetoMean(){
    FLOAT64 currShape;
    FLOAT64 currMax;
    UINT32 currMean;
    for (int i = 0; i < 10; i++) {
        currShape = i*0.1 + 0.78;
        for (int j = 0; j < 10; j++) {
            currMax = (j+1)*1000;
            currMean = genPareto(currShape,currMax);
            cout<<"mean = "<<currMean<<" with shape "<<currShape<<" max value"<<currMax<<endl;
        }
    }
}

UINT32 Util::calHashIDCapacity(){
    unsigned long long totalGUID = 1000000000;//10^9
    UINT32 capPerServer; //queries/updates 70~110
    UINT32 totalAS;//42000~100000
    UINT32 hashIDPerASMean; //2~22
    unsigned long long serverPerHashID,totalWrkld,totalSvrsRquired,totalhashIDs;
    UINT32 quPerGUID=4;
    for (hashIDPerASMean = 2; hashIDPerASMean < 23; hashIDPerASMean++) {
        for (totalAS = 40000; totalAS <= 100000; totalAS+=2000) {
            for (totalGUID = 10000000000; totalGUID < 1000000000000; totalGUID *=10) {
                for (quPerGUID = 2; quPerGUID < 6; quPerGUID++) {
                    for (capPerServer = 70000; capPerServer < 120000; capPerServer+=10000) {
                        totalWrkld = totalGUID*quPerGUID;
                        totalSvrsRquired = totalWrkld/capPerServer;
                        totalhashIDs = totalAS*hashIDPerASMean;
                        serverPerHashID = totalSvrsRquired/totalhashIDs;
                        cout<<serverPerHashID<<"\t"<<totalWrkld<<"\t"<<totalSvrsRquired<<"\t"<<totalhashIDs<<"\t"<<hashIDPerASMean
                                <<"\t"<<totalAS<<"\t"<<totalGUID<<"\t"<<quPerGUID<<"\t"<<capPerServer<<endl;
                    }                
                }
           }
        }
    }
}
void Util::matchPareto(const char* filename, FLOAT64 scale, FLOAT64 thisShape){
    ResetPareto(thisShape);
    vector<UINT32> capacity_v;
    UINT32 currRand;
    for (int i = 0; i < 4000000; i++) {
        currRand = GenPareto()*scale;
        if (currRand <= pow(10.0, 5.0)) {
            capacity_v.push_back(currRand);
        }
    }
    sort(capacity_v.begin(), capacity_v.end());
    ofstream outfHdlr;
    string outfilename= filename;
    outfilename += "_";
    stringstream ss (stringstream::in | stringstream::out);
    ss<<thisShape;
    outfilename += ss.str();
    outfilename += "_";
    ss.str("");
    ss <<scale;
    outfilename += ss.str();
    string rawoutname= outfilename;
    rawoutname += "_sorted.raw";
    outfHdlr.open(rawoutname.c_str(),ios::out | ios::in | ios:: trunc);
    for (int i=0; i<capacity_v.size(); i++) {
        outfHdlr<<capacity_v[i]<<endl;
    }
    outfHdlr.close();
    string cdfoutname= outfilename;
    cdfoutname += "_cdf.csv";
    outfHdlr.open(cdfoutname.c_str(),ios::out | ios::in | ios:: trunc);
    double pcent = 0.0;
    double idx;
    double alpha;
    int total_results = capacity_v.size();
    long int total_value=0;
    for (int i=0; i<capacity_v.size(); i++) {
        total_value += capacity_v[i];
        alpha += log ((double)capacity_v[i]);
        if ((i+1)< capacity_v.size()) {
            if (capacity_v[i]<capacity_v[i+1]) {
                idx= i+1;
                pcent = idx/total_results;
                outfHdlr<<pcent<<'\t'<<capacity_v[i]<<endl;
            }
        }
        else{
            idx= i+1;
            pcent = idx/total_results;
            outfHdlr<<pcent<<'\t'<<capacity_v[i]<<endl;
        }
    }
    outfHdlr.close();
    
    alpha = 1/alpha;
    alpha = (double)total_results* alpha;
    alpha++;
    total_value = total_value/(double)total_results;
    cout<< "Mean = "<<total_value<<" alpha = "<<alpha<<endl;
    
    string pdfoutname= outfilename;
    pdfoutname += "_pdf.csv";
    outfHdlr.open(pdfoutname.c_str(),ios::out | ios::in | ios:: trunc);
    idx =0.0;
    for (int i=0; i<capacity_v.size(); i++) {
        idx ++;
        if ((i+1)< capacity_v.size()) {
            if (capacity_v[i]<capacity_v[i+1]) {
                pcent = idx/total_results;
                outfHdlr<<pcent<<'\t'<<capacity_v[i]<<endl;
                idx = 0.0;
            }
        }
        else{
            pcent = idx/total_results;
            outfHdlr<<pcent<<'\t'<<capacity_v[i]<<endl;
        }
    }
    outfHdlr.close();
    string binoutname= outfilename;
    binoutname += "_binned.csv";
    outfHdlr.open(binoutname.c_str(),ios::out | ios::in | ios:: trunc);
    idx =0.0;
    double exponent =0.0;
    for (int i=0; i<capacity_v.size(); i++) {
        idx ++;
        if ((i+1)< capacity_v.size()) {
            if (capacity_v[i]>pow(2.0,exponent)) {
                pcent = idx/total_results;
                outfHdlr<<pcent<<'\t'<<pow(2.0,exponent)<<endl;
                idx = 0.0;
                exponent++;
            }
        }
        else{
            pcent = idx/total_results;
            outfHdlr<<pcent<<'\t'<<pow(2.0,exponent)<<endl;
        }
    }
    outfHdlr.close();

}

void Util::genCDF(const char* outfilename, vector<FLOAT64>& results_v) {
    sort(results_v.begin(), results_v.end());
    ofstream outfHdlr;
    outfHdlr.open(outfilename,ios::out | ios::in | ios:: trunc);
    double pcent = 0.0;
    double idx;
    for (int i=0; i<results_v.size(); i++) {
        if ((i+1)< results_v.size()) {
            if (results_v[i]<results_v[i+1]) {
                idx= i+1;
                pcent = idx/results_v.size();
                outfHdlr<<std::setprecision(10)<<pcent<<'\t'<<results_v[i]<<endl;
            }
        }
        else{
            idx= i+1;
            pcent = idx/results_v.size();
            outfHdlr<<std::setprecision(10)<<pcent<<'\t'<<results_v[i]<<endl;
        }
    }
    outfHdlr.close();
}

void Util::genCDF(const char* outfilename, vector<UINT32>& results_v){
    sort(results_v.begin(), results_v.end());
    ofstream outfHdlr;
    outfHdlr.open(outfilename,ios::out | ios::in | ios:: trunc);
    double pcent = 0.0;
    double idx;
    for (int i=0; i<results_v.size(); i++) {
        if ((i+1)< results_v.size()) {
            if (results_v[i]<results_v[i+1]) {
                idx= i+1;
                pcent = idx/results_v.size();
                outfHdlr<<std::setprecision(10)<<pcent<<'\t'<<results_v[i]<<endl;
            }
        }
        else{
            idx= i+1;
            pcent = idx/results_v.size();
            outfHdlr<<std::setprecision(10)<<pcent<<'\t'<<results_v[i]<<endl;
        }
    }
    outfHdlr.close();
}

void Util::genPDF(const char* outfilename, vector<UINT32>& results_v){
    sort(results_v.begin(), results_v.end());
    ofstream outfHdlr;
    outfHdlr.open(outfilename,ios::out | ios::in | ios:: trunc);
    double pcent = 0.0;
    double idx = 0.0;
    for (int i=0; i<results_v.size(); i++) {
        idx ++;
        if ((i+1)< results_v.size()) {
            if (results_v[i]<results_v[i+1]) {
                pcent = idx/results_v.size();
                outfHdlr<<std::setprecision(10)<<pcent<<'\t'<<results_v[i]<<endl;
                idx = 0.0;
            }
        }
        else{
            pcent = idx/results_v.size();
            outfHdlr<<std::setprecision(10)<<pcent<<'\t'<<results_v[i]<<endl;
        }
    }
    outfHdlr.close();
}

void Util::genHistInput(const char* outfilename, vector<UINT32>& results_v, UINT32 noOfBin, bool enableFurther){
    if (results_v.size()==0) {
        return;
    }
    ofstream outfHdlr;
    outfHdlr.open(outfilename,ios::out | ios::in | ios:: trunc);
    assert (noOfBin>0);
    sort(results_v.begin(), results_v.end());
    UINT32 currBinCnt=0;
    while (results_v.size() && results_v.front()==0) {
        currBinCnt++;
        results_v.erase(results_v.begin());
    }
    vector<Query_Count> bin_v;
    Query_Count thisBinInfor;
    UINT32 binDistance = (results_v.back()-results_v.front())/noOfBin;
    outfHdlr<<results_v.front()<<'\t'<<currBinCnt<<endl;
    thisBinInfor._guidIdx=results_v.front();
    thisBinInfor._queryCnt=currBinCnt;
    bin_v.push_back(thisBinInfor);
    currBinCnt=0;
    UINT32 currBinThreshold = results_v.front()+binDistance;
    for (int i=0; i<results_v.size(); i++) {
        while (results_v[i]>currBinThreshold) {
            outfHdlr<<currBinThreshold<<'\t'<<currBinCnt<<endl;
            thisBinInfor._guidIdx=currBinThreshold;
            thisBinInfor._queryCnt=currBinCnt;
            bin_v.push_back(thisBinInfor);
            currBinCnt =0;
            currBinThreshold += binDistance;
            if (currBinThreshold>results_v.back()) {
                currBinThreshold = results_v.back();
            }
        }
        currBinCnt++;
        if (i == results_v.size()-1) {
            outfHdlr<<currBinThreshold<<'\t'<<currBinCnt<<endl;
        }

    }
    outfHdlr.close();
    sort(bin_v.begin(),bin_v.end());
    if (bin_v.back()._guidIdx != results_v.front() && enableFurther) {
        UINT32 lower = bin_v.back()._guidIdx - binDistance;
        UINT32 upper = bin_v.back()._guidIdx;
        vector<UINT32> toFurther_v;
        for (int i = 0; i < results_v.size(); i++) {
            if (results_v[i]>= lower && results_v[i]<= upper) {
                toFurther_v.push_back(results_v[i]);
            }
        }
        string outFile = outfilename;
        outFile +="FurtherHist";
        genHistInput(outFile.c_str(),toFurther_v,noOfBin, false);
    }
    
} 

void Util::outWrkldDetail(const char* outfilename, vector<Wrkld_Count>& Wrkld_v){
    ofstream outfHdlr;
    outfHdlr.open(outfilename,ios::out | ios::in | ios:: trunc);
    vector <UINT32> cacheWrkld_v, replicaWrkld_v;
    for (int i=0; i<Wrkld_v.size(); i++) {
        outfHdlr<<Wrkld_v[i]._replicaWrkld<<'\t'<<Wrkld_v[i]._cacheWrkld<<endl;
        cacheWrkld_v.push_back(Wrkld_v[i]._cacheWrkld);
        replicaWrkld_v.push_back(Wrkld_v[i]._replicaWrkld);
    }
    outfHdlr.close();
    /*string outName = outfilename;
    outName += "cacheWrkld_hist";
    genHistInput(outName.c_str(), cacheWrkld_v, 20, true);
    outName = outfilename;
    outName += "repWrkld_hist";
    genHistInput(outName.c_str(), replicaWrkld_v, 20, true);*/
}

void Util::outErrorDetail(const char* outfilename){
    ofstream outfHdlr;
    outfHdlr.open(outfilename,ios::out | ios::in | ios:: trunc);
    for (UINT32 i = 0; i < Underlay::Inst()->global_guid_list.size(); i++) {
        if (Stat::Error_cnt_per_guid[i]) {
            outfHdlr<<Stat::Error_cnt_per_guid[i]<<"\t"<<Underlay::Inst()->global_guid_list[i].getPopularity()<<"\t";
            if (Underlay::Inst()->global_guid_list[i].getPopularity()) {
                outfHdlr<<(FLOAT32)Stat::Error_cnt_per_guid[i]/(FLOAT32)Underlay::Inst()->global_guid_list[i].getPopularity();
            }
            outfHdlr<<"\t"<<Underlay::Inst()->global_guid_list[i]._errorCacheNodes.size()<<"\t"
                    <<Underlay::Inst()->global_guid_list[i]._distinctErrCacheNodes.size()<<endl;
        }
    }
}