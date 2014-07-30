#include "util.h"

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