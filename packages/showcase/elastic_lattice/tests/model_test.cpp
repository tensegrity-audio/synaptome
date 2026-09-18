#include "../source/ElasticLatticeModel.h"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
using Model = synaptome::showcase::ElasticLatticeModel;
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
    const auto low=Controls{8,.4,.3,0,.1},high=Controls{90,8,6,15,2};
    const auto oldSteps = a.steps();
    a.update(1e9, high);
    require(a.steps()-oldSteps <= 48, "long host stall exceeded step budget");
    const auto beforeSweep = a.steps();
    for(int i=0;i<120;++i) a.update(1.0/60, i%2 ? low : high);
    require(a.steps()>beforeSweep, "control sweeps unexpectedly reset model history");
    for(int i=0;i<6000;++i) {
        a.update(1.0/60, ((i/120)%2) ? low : high);
        if(i%60==0) require(a.finite(),"extreme control sweep produced unbounded state");
    }
    require(a.finite(),"long run produced invalid state");
    for(int mask=0;mask<32;++mask) {
        Controls corner{mask&1 ? 90.0 : 8.0, mask&2 ? 8.0 : .4,
                        mask&4 ? 6.0 : .3, mask&8 ? 15.0 : 0.0, mask&16 ? 2.0 : .1};
        a.reset(1001,corner);
        for(int i=0;i<600;++i) a.update(1.0/60,corner);
        require(a.finite(),"spring parameter corner became unbounded");
        for(int y=0;y<=Model::rows;++y) for(int x=0;x<=Model::columns;++x)
            if(x==0||y==0||x==Model::columns||y==Model::rows)
                require(a.heights()[y*(Model::columns+1)+x]==0,
                        "clamped membrane boundary moved");
    }
    a.reset(1001);b.reset(1001);
    require(a.stateSignature()==b.stateSignature(),"explicit reset is not reproducible");
    std::cout << "ElasticLattice: determinism, explicit seeds, pause, bounded work and long control sweeps passed\n";
}
