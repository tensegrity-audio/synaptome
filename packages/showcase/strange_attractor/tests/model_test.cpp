#include "../source/StrangeAttractorModel.h"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
using Model = synaptome::showcase::StrangeAttractorModel;
using Controls = Model::Controls;
void require(bool condition,const char* message) { if(!condition) throw std::runtime_error(message); }
int main() {
    Model a,b;
    require(a.finite(),"default model is not finite");
    require(a.stateSignature()==b.stateSignature(),"same-seed first load differs");
    b.reset(1002);
    require(a.stateSignature()!=b.stateSignature(),"seed does not affect initial state");
    b.reset(1001);
    Controls defaults;
    const auto first=a.stateSignature();
    for(int i=0;i<1200;++i) {a.update(1.0/60,defaults);b.update(1.0/60,defaults);}
    require(a.stateSignature()==b.stateSignature(),"deterministic repeat diverged");
    require(a.stateSignature()!=first,"model does not evolve");
    const auto paused=a.stateSignature();
    for(int i=0;i<30;++i) a.update(0,defaults);
    a.update(-1,defaults);a.update(std::numeric_limits<double>::infinity(),defaults);
    a.update(std::numeric_limits<double>::quiet_NaN(),defaults);
    require(a.stateSignature()==paused,"paused/invalid delta changed state");
    const auto low=Controls{6,24,2},high=Controls{16,40,3.5};
    const auto oldSteps = a.steps();
    a.update(1e9, high);
    require(a.steps()-oldSteps <= 40, "long host stall exceeded step budget");
    const auto beforeSweep = a.steps();
    for(int i=0;i<120;++i) a.update(1.0/60, i%2 ? low : high);
    require(a.steps()>beforeSweep, "control sweeps unexpectedly reset model history");
    for(int i=0;i<6000;++i) {
        a.update(1.0/60, ((i/120)%2) ? low : high);
        if(i%60==0) require(a.finite(),"extreme control sweep produced unbounded state");
    }
    require(a.finite(),"long run produced invalid state");
    for(int mask=0;mask<8;++mask) {
        Controls corner{mask&1 ? 16.0 : 6.0, mask&2 ? 40.0 : 24.0, mask&4 ? 3.5 : 2.0};
        a.reset(1001,corner);
        for(int i=0;i<600;++i) a.update(1.0/60,corner);
        require(a.finite(),"Lorenz parameter corner became nonfinite");
    }
    a.reset(1001);b.reset(1001);
    require(a.stateSignature()==b.stateSignature(),"explicit reset is not reproducible");
    std::cout << "StrangeAttractor: determinism, explicit seeds, pause, bounded work and long control sweeps passed\n";
}
