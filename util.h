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
};
