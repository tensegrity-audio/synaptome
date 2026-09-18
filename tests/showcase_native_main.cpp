// Portable native authoring checks. Records actual adapter CPU geometry.
// It does not create an OpenGL context or establish GPU/performance acceptance.
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include "showcase_support/GraphicsRecorder.h"
#include "element_confidence/LifecycleChecks.h"
#include "runtime/GeneratedElementPackageRegistrations.h"

namespace {
using synaptome::tests::element_confidence::require;
using synaptome::tests::element_confidence::prepareDeclaredElement;
using synaptome::tests::element_confidence::parameterValueMatchesJson;
using Prepared=synaptome::tests::element_confidence::PreparedDeclaredElement;
constexpr const char* prefix="console.layer1";
ofJson values(const ParameterRegistry& r) {
    ofJson j=ofJson::object();
    for(const auto& p:r.floats())j[p.meta.id]=*p.value;
    for(const auto& p:r.bools())j[p.meta.id]=*p.value;
    for(const auto& p:r.strings())j[p.meta.id]=*p.value;
    return j;
}
void assign(ParameterRegistry& r,const std::string& local,const ofJson& v) {
    const std::string id=std::string(prefix)+"."+local;
    if(r.findFloat(id))r.setFloatBase(id,v.get<float>(),true);
    else if(r.findBool(id))r.setBoolBase(id,v.get<bool>(),true);
    else if(r.findString(id))r.setStringBase(id,v.get<std::string>(),true);
    else throw std::runtime_error("unknown mapping/scene target: "+id);
}
Prepared prepare(LayerFactory& factory,const ofJson& p,const ofJson& defaults=ofJson::object()) {
    return prepareDeclaredElement(factory,p["asset"]["type"].get<std::string>(),prefix,"portable.showcase",{{"defaults",defaults}});
}
void advance(Layer& layer,int frames,float speed=1,float dt=1.f/60,float bpm=128) {
    for(int f=0;f<frames;++f)layer.update({dt,float(f)*dt,bpm,speed});
}
std::uint64_t draw(Layer& layer,float opacity=1,int width=640,int height=360) {
    auto& r=showcase_test::recorder;r.reset();
    // Draw must restore a non-default caller state, not merely stack depths.
    r.depth=true;r.cull=true;
    r.style.lineWidth=3.25f;r.style.filled=false;r.style.blend=1;r.style.color={.13f,.27f,.41f,.63f};
    r.view.viewport={2,3,127,93};r.view.matrix[3]=17;r.view.matrix[7]=23;
    const auto beforeStyle=r.style;const auto beforeView=r.view;
    ofCamera camera;LayerDrawParams params{camera};params.viewport={width,height};params.slotOpacity=opacity;params.time=42;params.beat=.25f;
    layer.draw(params);
    require(r.styles.empty()&&r.views.empty()&&r.matrices.empty(),"draw left graphics stacks unbalanced");
    require(r.style.lineWidth==beforeStyle.lineWidth&&r.style.filled==beforeStyle.filled&&r.style.blend==beforeStyle.blend&&r.style.color.r==beforeStyle.color.r&&r.style.color.g==beforeStyle.color.g&&r.style.color.b==beforeStyle.color.b&&r.style.color.a==beforeStyle.color.a,"draw changed caller graphics style");
    require(r.view.matrix==beforeView.matrix&&r.view.viewport==beforeView.viewport&&r.depth&&r.cull,"draw changed caller view/matrix/depth state");
    return showcase_test::signature();
}
void exportSvg(const std::filesystem::path& path) {
    std::ofstream out(path);out<<"<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 640 360\"><rect width=\"640\" height=\"360\" fill=\"#03060c\"/>\n";
    auto color=[](const showcase_test::Color& c) {return "rgb("+std::to_string(int(std::clamp(c.r,0.f,1.f)*255))+","+std::to_string(int(std::clamp(c.g,0.f,1.f)*255))+","+std::to_string(int(std::clamp(c.b,0.f,1.f)*255))+")";};
    for(const auto& d:showcase_test::recorder.draws){
        const auto& vs=d.vertices;
        if(d.mode==OF_PRIMITIVE_TRIANGLES)for(std::size_t i=0;i+2<vs.size();i+=3){auto a=vs[i],b=vs[i+1],c=vs[i+2];showcase_test::Color col{(a.color.r+b.color.r+c.color.r)/3,(a.color.g+b.color.g+c.color.g)/3,(a.color.b+b.color.b+c.color.b)/3,(a.color.a+b.color.a+c.color.a)/3};out<<"<path d=\"M"<<a.x<<","<<a.y<<" L"<<b.x<<","<<b.y<<" "<<c.x<<","<<c.y<<" Z\" fill=\""<<color(col)<<"\" fill-opacity=\""<<col.a<<"\"/>\n";}
        else if(d.mode==OF_PRIMITIVE_LINES||d.mode==OF_PRIMITIVE_LINE_STRIP)for(std::size_t i=0;i+1<vs.size();i+=(d.mode==OF_PRIMITIVE_LINES?2:1)){const auto&a=vs[i];const auto&b=vs[i+1];out<<"<path d=\"M"<<a.x<<","<<a.y<<" L"<<b.x<<","<<b.y<<"\" fill=\"none\" stroke=\""<<color(a.color)<<"\" stroke-opacity=\""<<a.color.a<<"\" stroke-width=\""<<d.lineWidth<<"\"/>\n";}
    }
    out<<"</svg>\n";
}
void validate(LayerFactory& factory,const std::filesystem::path& path,const std::filesystem::path& output) {
    std::ifstream stream(path);ofJson p;stream>>p;
    const std::string id=p["asset"]["type"];
    const auto* contract=factory.typeContract(id);require(contract!=nullptr,"missing generated registration: "+id);
    const auto& declarations=contract->contract.parameters.parameters;
    require(declarations.size()==p["parameters"].size(),"declaration count mismatch");
    for(const auto& d:declarations){
        const auto found=std::find_if(p["parameters"].begin(),p["parameters"].end(),[&](const ofJson& j){return j["id"]==d.id;});
        require(found!=p["parameters"].end(),"generated parameter missing from manifest: "+d.id);
        const auto& j=*found;
        require(d.id==j["id"]&&d.groupId==j["groupId"]&&d.label==j["label"],"generated parameter identity/group/label mismatch: "+d.id);
        require(parameterValueMatchesJson(d.defaultValue,j["default"]),"generated parameter default mismatch: "+d.id);
        require(d.units==j.value("units",std::string())&&d.description==j.value("description",std::string()),"generated parameter units/description mismatch");
        if(d.range)require(std::abs(d.range->min-j["range"]["min"].get<float>())<1e-4f&&std::abs(d.range->max-j["range"]["max"].get<float>())<1e-4f,"generated parameter range mismatch");
    }
    ofJson defaults=p["asset"].value("defaults",ofJson::object());
    auto layer=prepare(factory,p,defaults);
    draw(*layer.layer);
    require(showcase_test::vertexCount()>0&&showcase_test::alphaSum()>0,"setup produced no first-frame geometry");
    for(const auto& j:p["parameters"]){const std::string local=j["id"];const auto expected=defaults.contains(local)?defaults[local]:j["default"];
        const auto full=std::string(prefix)+"."+local;const auto actual=values(*layer.registry).at(full);
        if(expected.is_number())require(std::abs(actual.get<float>()-expected.get<float>())<1e-4f,"setup/reset lost configured/default float: "+local);
        else require(actual==expected,"setup/reset lost configured/default value: "+local);
    }
    // Exercise every configured parameter through non-default values, including bool/string.
    ofJson configured=ofJson::object();
    for(const auto& j:p["parameters"]){const std::string local=j["id"];const std::string kind=j["kind"];
        if(kind=="float"){float lo=j["range"]["min"],hi=j["range"]["max"];float v=lo+(hi-lo)*.43f;if(local=="seed")v=17;configured[local]=v;}
        else if(kind=="bool")configured[local]=!j["default"].get<bool>();
        else {configured[local]=j["default"];if(j.contains("options")&&!j["options"].empty())configured[local]=j["options"].back()["value"];}
    }
    auto changed=prepare(factory,p,configured);const auto changedValues=values(*changed.registry);
    for(auto it=configured.begin();it!=configured.end();++it){const auto& actual=changedValues.at(std::string(prefix)+"."+it.key());
        if(it.value().is_number())require(std::abs(actual.get<float>()-it.value().get<float>())<1e-4f,"configure/setup lost explicit float: "+it.key());
        else require(actual==it.value(),"configure/setup lost explicit value: "+it.key());
    }
    // Registry writes exercise the same public bound storage targeted by mappings.
    for(const auto& j:p["parameters"])assign(*changed.registry,j["id"],j["default"]);
    for(const auto& j:p["parameters"]){const auto actual=values(*changed.registry).at(std::string(prefix)+"."+j["id"].get<std::string>());if(j["default"].is_number())require(std::abs(actual.get<float>()-j["default"].get<float>())<1e-4f,"registry float transaction failed");else require(actual==j["default"],"registry transaction failed");}
    advance(*layer.layer,240);const auto baseline=draw(*layer.layer);const auto vertices=showcase_test::vertexCount();const auto alpha=showcase_test::alphaSum();
    require(vertices>0&&alpha>0,"default adapter produced no visible geometry");
    exportSvg(output/(path.parent_path().filename().string()+".svg"));
    auto replay=prepare(factory,p,defaults);advance(*replay.layer,240);require(draw(*replay.layer)==baseline,"seeded adapter replay is nondeterministic");
    // Drawing is read-only with respect to model state.
    require(draw(*layer.layer)==baseline,"repeat draw changed geometry");
    advance(*layer.layer,30,0);require(draw(*layer.layer)==baseline,"transport pause advanced model");
    advance(*layer.layer,30,1,0);require(draw(*layer.layer)==baseline,"zero dt advanced model");
    if(layer.registry->findBool(std::string(prefix)+".paused")){
        assign(*layer.registry,"paused",true);advance(*layer.layer,30);require(draw(*layer.layer)==baseline,"local pause advanced model");assign(*layer.registry,"paused",false);
    }
    if(layer.registry->findBool(std::string(prefix)+".reseed")){
        // Model resets may intentionally preserve independent camera spin.
        // Compare synchronized actions rather than inventing a whole-view
        // reset rule; the declared persistent seed must still remain stable.
        auto actionA=prepare(factory,p,defaults),actionB=prepare(factory,p,defaults);
        advance(*actionA.layer,120);advance(*actionB.layer,120);
        assign(*actionA.registry,"reseed",true);assign(*actionB.registry,"reseed",true);
        advance(*actionA.layer,120);advance(*actionB.layer,120);
        require(!*actionA.registry->findBool(std::string(prefix)+".reseed")->value,"reseed action did not reset its trigger");
        require(draw(*actionA.layer)==draw(*actionB.layer),"identical reseed actions produced different geometry");
        if(const auto* seed=actionA.registry->findFloat(std::string(prefix)+".seed"))
            require(std::abs(*seed->value-seed->baseValue)<1e-4f,"reseed changed live seed outside persisted registry transaction");
    }
    draw(*layer.layer,.5f);require(showcase_test::vertexCount()==vertices,"half opacity changed geometry count");require(std::abs(showcase_test::alphaSum()/alpha-.5)<.012,"slot opacity was ignored or double-applied");
    draw(*layer.layer,0);require(showcase_test::alphaSum()==0,"zero slot opacity remained visible");
    draw(*layer.layer,1,0,0);draw(*layer.layer,1,1920,1080);draw(*layer.layer,1,360,640);
    if(layer.registry->findFloat(std::string(prefix)+".seed")){auto seeded=prepare(factory,p,defaults);assign(*seeded.registry,"seed",83.f);advance(*seeded.layer,240);require(draw(*seeded.layer)!=baseline,"changing seed did not change geometry");}
    // Every range endpoint must remain drawable after live edits. Controls use
    // real registry transactions; no private state access or reinitialization.
    std::size_t responding=0;
    for(const auto& j:p["parameters"]){const std::string local=j["id"];if(j["kind"]!="float"||local=="seed")continue;
        auto low=prepare(factory,p,defaults),high=prepare(factory,p,defaults);
        assign(*low.registry,local,j["range"]["min"]);assign(*high.registry,local,j["range"]["max"]);
        advance(*low.layer,90);advance(*high.layer,90);auto lowSig=draw(*low.layer),highSig=draw(*high.layer);if(lowSig!=highSig)++responding;
    }
    require(responding>=3,"fewer than three public float controls change geometry/appearance");
    // Long native evolution, checked periodically. No performance claim is made.
    for(int batch=0;batch<20;++batch){advance(*layer.layer,100);draw(*layer.layer);require(showcase_test::vertexCount()>0,"long-run geometry disappeared");}
    for(const float dt:{-.1f,2.f}){advance(*layer.layer,1,1,dt);draw(*layer.layer);}
    std::cout<<"PASS "<<id<<" parameters="<<declarations.size()<<" vertices="<<vertices<<" respondingControls="<<responding<<" longRunFrames=2000"<<std::endl;
}
}
int main(int argc,char**argv){
    if(argc<3){std::cerr<<"usage: showcase_native OUTPUT_DIR MANIFEST...\n";return 2;}
    std::filesystem::path output=argv[1];std::filesystem::create_directories(output);LayerFactory factory;
    try{synaptome::runtime::registerGeneratedElementPackages(factory);}catch(const std::exception& e){std::cerr<<"FAIL registration: "<<e.what()<<"\n";return 1;}
    int failures=0;
    for(int i=2;i<argc;++i)try{validate(factory,argv[i],output);}catch(const std::exception& e){++failures;std::cerr<<"FAIL "<<argv[i]<<": "<<e.what()<<std::endl;}
    std::cout<<"Portable CPU/native adapter contract: "<<(argc-2-failures)<<" passed, "<<failures<<" failed. GPU draw, Windows build, and renderer performance remain separate gates.\n";
    return failures?1:0;
}
