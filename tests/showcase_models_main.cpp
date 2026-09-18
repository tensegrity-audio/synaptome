// Pure algorithm contracts. No openFrameworks headers, runtime or GPU.
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include "../packages/showcase/wave_tank/source/WaveTankModel.h"
#include "../packages/showcase/phase_lattice/source/PhaseLatticeModel.h"
#include "../packages/showcase/chladni_plate/source/ChladniPlateModel.h"
#include "../packages/showcase/differential_growth/source/DifferentialGrowthModel.h"
#include "../packages/showcase/dendritic_crystal/source/DendriticCrystalModel.h"
#include "../packages/showcase/magnetic_dipoles/source/MagneticDipolesModel.h"
#include "../packages/showcase/vortex_advection/source/VortexAdvectionModel.h"
#include "../packages/showcase/sand_ripples/source/SandRipplesModel.h"
#include "../packages/showcase/voronoi_foam/source/VoronoiFoamModel.h"
#include "../packages/showcase/strange_attractor/source/StrangeAttractorModel.h"
#include "../packages/showcase/gerstner_ocean/source/GerstnerOceanModel.h"
#include "../packages/showcase/elastic_lattice/source/ElasticLatticeModel.h"

namespace {
void require(bool ok,const std::string& message){if(!ok)throw std::runtime_error(message);}
template<int N>struct Priority:Priority<N-1>{};template<>struct Priority<0>{};
template<class M>auto tick(M&m,double dt,const typename M::Controls&c,Priority<2>)->decltype(m.update(dt,c),void()){m.update(dt,c);}
template<class M>auto tick(M&m,double dt,const typename M::Controls&c,Priority<1>)->decltype(m.step(dt,c),void()){m.step(dt,c);}
template<class M>auto tick(M&m,double dt,const typename M::Controls&c,Priority<0>)->decltype(m.advance(dt,c),void()){m.advance(dt,c);}
template<class M>auto signature(const M&m,int)->decltype(m.stateSignature()){return m.stateSignature();}
template<class M>auto signature(const M&m,long)->decltype(m.signature()){return m.signature();}
template<class M>auto finite(const M&m,Priority<1>)->decltype(m.boundedState()){return m.boundedState();}
template<class M>auto finite(const M&m,Priority<0>)->decltype(m.finite()){return m.finite();}
bool finite(const synaptome_show_differential::DifferentialGrowthModel&m,Priority<1>){if(m.points().empty()||m.points().size()>m.maxNodes)return false;for(const auto&p:m.points())if(!std::isfinite(p.x)||!std::isfinite(p.y)||std::abs(p.x)>1.1f||std::abs(p.y)>1.1f)return false;return true;}
bool finite(const synaptome_show_dendritic::DendriticCrystalModel&m,Priority<1>){if(m.cells().size()!=m.width*m.height||!std::isfinite(m.time()))return false;std::size_t count=0;for(const auto&c:m.cells()){if(!std::isfinite(c.born))return false;if(c.occupied)++count;}return count==m.occupiedCount()&&count>0;}
bool finite(const synaptome_show_magnetic::MagneticDipolesModel&m,Priority<1>){if(m.segments().empty()||m.segments().size()>m.maxSegments)return false;for(const auto&s:m.segments())for(float v:{s.a.x,s.a.y,s.b.x,s.b.y,s.strength})if(!std::isfinite(v)||std::abs(v)>1e6f)return false;return std::isfinite(m.time());}
template<class M>void test(const std::string&name,typename M::Controls alternate){
    typename M::Controls controls;M a,b,other;a.reset(12345);b.reset(12345);other.reset(54321);
    require(signature(a,0)==signature(b,0),name+" seeded reset mismatch");require(signature(a,0)!=signature(other,0),name+" seed has no effect");
    for(int i=0;i<180;++i){tick(a,1.0/60,controls,Priority<2>{});tick(b,1.0/60,controls,Priority<2>{});tick(other,1.0/60,controls,Priority<2>{});}
    require(signature(a,0)==signature(b,0),name+" deterministic replay mismatch");
    const auto paused=signature(a,0);
    for(double dt:{0.,-.1,std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity()})tick(a,dt,controls,Priority<2>{});
    require(signature(a,0)==paused,name+" invalid/paused dt advanced state");
    b.reset(12345);other.reset(12345);
    for(int i=0;i<180;++i){tick(b,1.0/60,controls,Priority<2>{});tick(other,1.0/60,alternate,Priority<2>{});}
    require(signature(b,0)!=signature(other,0),name+" model control does not affect evolution");
    for(int i=0;i<3600;++i){tick(a,1.0/60,(i/300)%2?alternate:controls,Priority<2>{});if(i%60==0)require(finite(a,Priority<1>{}),name+" unbounded long-run state");}
    tick(a,3600,alternate,Priority<2>{});require(finite(a,Priority<1>{}),name+" unbounded long dt state");
    std::cout<<"PASS model "<<name<<" deterministic=true pause=true controls=true longRunFrames=3600 bounded=true\n";
}
}
int main(){try{
    using Wave=synaptome_show_wave_tank::WaveTankModel;Wave::Controls wave;wave.waveSpeed=28;wave.damping=.05f;wave.driveStrength=12;test<Wave>("Wave Tank",wave);
    using Phase=synaptome_show_phase_lattice::PhaseLatticeModel;Phase::Controls phase;phase.coupling=12;phase.dispersion=2;phase.phaseLagDeg=90;test<Phase>("Phase Lattice",phase);
    using Chladni=synaptome_show_chladni_plate::ChladniPlateModel;Chladni::Controls ch;ch.modeX=9;ch.modeY=8;ch.agitation=1;ch.attraction=6;test<Chladni>("Chladni Plate",ch);
    using Differential=synaptome_show_differential::DifferentialGrowthModel;Differential::Controls dif;dif.growthRate=1;dif.repulsion=2;dif.stiffness=2;test<Differential>("Differential Growth",dif);
    using Dendritic=synaptome_show_dendritic::DendriticCrystalModel;Dendritic::Controls den;den.growthRate=15000;den.adhesion=1;den.dissolutionRate=.5f;test<Dendritic>("Dendritic Crystal",den);
    using Magnetic=synaptome_show_magnetic::MagneticDipolesModel;Magnetic::Controls mag;mag.separation=.7f;mag.twist=1;mag.orbitRate=1;test<Magnetic>("Magnetic Dipoles",mag);
    VortexAdvectionModel::Controls vortex;vortex.circulation=3;vortex.coreRadius=.02f;vortex.strain=1;test<VortexAdvectionModel>("Vortex Advection",vortex);
    SandRipplesModel::Controls sand;sand.transportRate=3;sand.windDirectionDeg=170;sand.relaxationRate=3;test<SandRipplesModel>("Sand Ripples",sand);
    VoronoiFoamModel::Controls foam;foam.driftSpeed=1;foam.relaxationRate=2;foam.shear=1;test<VoronoiFoamModel>("Voronoi Foam",foam);
    using Attractor=synaptome::showcase::StrangeAttractorModel;Attractor::Controls att;att.rho=40;att.sigma=14;test<Attractor>("Strange Attractor",att);
    using Ocean=synaptome::showcase::GerstnerOceanModel;Ocean::Controls ocean;ocean.waveHeight=1.1;test<Ocean>("Gerstner Ocean",ocean);
    using Elastic=synaptome::showcase::ElasticLatticeModel;Elastic::Controls elastic;elastic.springStiffness=90;test<Elastic>("Elastic Lattice",elastic);
    SandRipplesModel conservation;conservation.reset(1702);double massBefore=0;for(float h:conservation.heights())massBefore+=h;for(int i=0;i<600;++i)conservation.advance(1.f/60,sand);double massAfter=0;for(float h:conservation.heights())massAfter+=h;require(std::abs(massAfter-massBefore)/massBefore<1e-4,"Sand Ripples lost material mass");
    std::cout<<"PASS conservative sand transport relative mass error="<<std::abs(massAfter-massBefore)/massBefore<<"\n";
}catch(const std::exception&e){std::cerr<<"FAIL "<<e.what()<<"\n";return 1;}return 0;}
