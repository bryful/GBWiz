#include "game.h"
#include "game_support.h"
#include <array>
#include <iostream>
void cartridge_set_rumble(unsigned){}
struct Input:gambatte::InputGetter{unsigned value=0;unsigned operator()()override{return value;}};
uint32_t number(const unsigned char* p){return uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);}
struct Probe{gambatte::GB* core;unsigned factor=1,events=0,base=0,frame=0;};
bool observe(void* data,unsigned pc,unsigned sp,unsigned address,unsigned value){
    auto& p=*static_cast<Probe*>(data);
    if(pc!=0x323||address!=0xcf77||sp<0xc000||sp>=0xdfff)return true;
    auto r=static_cast<unsigned char*>(p.core->rambank0_ptr());unsigned caller=r[sp-0xc000]|(unsigned(r[sp-0xc000+1])<<8);
    if(r[0xfbd]!=4||caller!=0x6afc)return true;
    auto target=r+0xf74;uint64_t base=uint64_t(target[0])|(uint64_t(target[1])<<8)|(uint64_t(target[2])<<16)|(uint64_t(value)<<24);
    p.base=static_cast<unsigned>(base);++p.events;
    std::cout<<"reward frame="<<p.frame<<" pc="<<std::hex<<pc<<" sp="<<sp<<" caller="<<caller<<std::dec<<" base="<<base<<" eligible="<<unsigned(r[0xf73])<<" factor="<<p.factor<<"\n";
    if(p.factor==1)return true;
    uint64_t scaled=std::min<uint64_t>(base*p.factor,UINT32_MAX);for(unsigned i=0;i<4;i++)target[i]=static_cast<unsigned char>(scaled>>(8*i));return false;
}
void dump(const fs::path& path,const void* data,size_t size){std::ofstream f(path,std::ios::binary);f.write(static_cast<const char*>(data),size);if(!f)throw std::runtime_error("Write failed");}
int wmain(int argc,wchar_t** argv)try{
    if(argc<4||argc>5)return 1;Input input;auto rom=read(argv[1]);if(identifyGame(rom).id!=GameId::Gaiden2)throw std::runtime_error("Gaiden II ROM required");
    auto core=loadCoreBytes(rom,&input);auto state=read(argv[2]);if(!core->loadState(state.data(),state.size()))throw std::runtime_error("Invalid checkpoint");
    Probe probe{core.get()};if(argc==5)probe.factor=std::stoul(argv[4]);
    if(probe.factor!=1&&(probe.factor<8||probe.factor>256||(probe.factor&(probe.factor-1))))throw std::runtime_error("Invalid multiplier");
    const unsigned char exp[]={0x21,0x74,0xcf,0xcd,0x1b,3,0x21,0x74,0xcf,0x11,0xcf,0xcf,1,4,0,0xcd,0x80,0x21};
    const unsigned char store[]={0x7b,0x22,0x7a,0x22,0x79,0x22,0x78,0x77,0x2b,0x2b,0x2b,0xc9};
    if(rom.size()<0x12b08||!std::equal(std::begin(exp),std::end(exp),reinterpret_cast<const unsigned char*>(rom.data()+0x12af6))||!std::equal(std::begin(store),std::end(store),reinterpret_cast<const unsigned char*>(rom.data()+0x31b)))throw std::runtime_error("Unknown reward revision");
    core->setWriteFilter(observe,&probe);
    fs::path output=argv[3];fs::create_directories(output);
    auto r=static_cast<const unsigned char*>(core->rambank0_ptr());std::vector<unsigned char> initial(r,r+8192);
    std::array<gambatte::video_pixel_t,160*144> pixels{};std::array<gambatte::uint_least32_t,40000> sound{};std::vector<char> before(core->stateSize());unsigned first=0;
    for(probe.frame=0;probe.frame<20000;probe.frame++){
        if(!probe.events)core->saveState(before.data());
        input.value=probe.frame%128<8?1:0;unsigned samples=35112;core->runFor(pixels.data(),160,sound.data(),sound.size(),samples);
        if(probe.events&&!first){first=probe.frame;dump(output/"before-experience.bin",before.data(),before.size());}
        if(first&&(r[0xfbd]==3||probe.frame>first+2400))break;
    }
    for(unsigned slot=0;slot<20;slot++){auto a=initial.data()+0x1095+slot*128;auto b=r+0x1095+slot*128;if(!a[0])continue;std::cout<<"slot="<<slot<<" status="<<unsigned(a[11])<<" exp="<<number(a+0x2d)<<" -> "<<number(b+0x2d)<<" gold="<<number(a+0x19)<<" -> "<<number(b+0x19)<<"\n";}
    core->saveState(before.data());dump(output/"after.bin",before.data(),before.size());dump(output/"wram.bin",r,8192);dump(output/"battery.bin",core->savedata_ptr(),core->savedata_size());dump(output/"pixels.rgb32",pixels.data(),pixels.size()*4);
    std::cout<<"frames="<<probe.frame<<" events="<<probe.events<<" mode="<<unsigned(r[0xfbd])<<"\n";return probe.events==1?0:2;
}catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}

