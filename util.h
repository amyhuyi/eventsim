#include "genran/newran.h"
#include "overnet.h"

class Util
{
private:
    static Util * _inst;
    MotherOfAll _urng;
    Uniform* _uniform;
    Normal* _normal;
    Gamma* _gamma;
    Poisson* _poisson;
    Pareto* _pareto;
    Util();
public:
    ~Util();
    static Util* Inst();
    UINT32 GenInt(UINT32 bound);
    FLOAT64 GenUniform();
    FLOAT64 GenGamma();
    FLOAT64 GenNormal();
    FLOAT64 GenPoisson();
    void ResetPoisson(FLOAT64 mean); //reset poisson distribution starting time 0 and mean as specifed
    FLOAT64 GenPareto();
    void ResetPareto(FLOAT64 thisShape);
    void matchPareto(const char* outfilename, FLOAT64 scale, FLOAT64 thisShape);
    UINT32 genPareto(FLOAT64 thisShape, FLOAT64 maxValue); // return mean
    UINT32 calHashIDCapacity();
    UINT32 calParetoMean();
    void genCDF(const char* outfilename, vector<UINT32>& results_v);
    void genCDF(const char* outfilename, vector<FLOAT64>& results_v);
    void genPDF(const char* outfilename, vector<UINT32>& results_v);
    void getParetoVec(FLOAT64 thisShape, long int totalNo, vector<UINT32>& results_v);
};
