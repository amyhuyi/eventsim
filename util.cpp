#include "util.h"

Util* Util::_inst = NULL;

Util::Util() {
    Random::Set(_urng);
    _poisson = new Poisson(ARRIVAL_MEAN);
    _uniform = new Uniform();
    _normal = new Normal();
    _gamma = new Gamma(SHAPE_GAMMA);
}

Util::~Util() {
    delete _uniform;
    delete _normal;
    delete _gamma;
    delete _poisson;
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
