#include "../source/GerstnerOceanModel.h"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
using Model = synaptome::showcase::GerstnerOceanModel;
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
    const auto low=Controls{.01,.7,0,-180},high=Controls{.32,3,1,180};
    const auto beforePhase = a.phases();
    a.update(1e9, high);
    require(a.finite(), "large host stall produced invalid geometry");
    require(a.phases() != beforePhase, "phase did not advance");
    for(double phase : a.phases()) require(phase>=0 && phase<6.283185308, "phase escaped finite interval");
    for(int i=0;i<6000;++i) {
        a.update(1.0/60, ((i/120)%2) ? low : high);
        if(i%60==0) require(a.finite(),"extreme control sweep produced unbounded state");
    }
    require(a.finite(),"long run produced invalid state");
    for(int mask=0;mask<16;++mask) {
        Controls corner{mask&1 ? .32 : .01, mask&2 ? 3.0 : .7,
                        mask&4 ? 1.0 : 0.0, mask&8 ? 180.0 : -180.0};
        a.reset(1001,corner);
        for(int i=0;i<60;++i) a.update(1.0/60,corner);
        require(a.finite(),"wave parameter corner produced invalid geometry");
        for(const auto& p:a.surface()) require(std::abs(p.y)<=corner.waveHeight+1e-12,
                                              "wave exceeded declared height envelope");
    }
    a.reset(1001);b.reset(1001);
    require(a.stateSignature()==b.stateSignature(),"explicit reset is not reproducible");
    std::cout << "GerstnerOcean: determinism, explicit seeds, pause, bounded work and long control sweeps passed\n";
}
