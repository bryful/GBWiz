#include "game_support.h"
#include <iostream>
#include <iomanip>
void cartridge_set_rumble(unsigned){}
struct Input:gambatte::InputGetter{unsigned value=0;unsigned operator()()override{return value;}};
struct Trace{gambatte::GB* core;unsigned frame=0,bank=1,lines=0;};
bool observe(void* data,unsigned pc,unsigned sp,unsigned address,unsigned value){
    auto& t=*static_cast<Trace*>(data);
    if(address>=0x2000&&address<0x4000)t.bank=value&31;
    if(address>=0xffed&&address<=0xfff0&&pc!=0x3338)std::cout<<"COMMAND frame="<<std::dec<<t.frame<<" pc="<<std::hex<<pc<<" address="<<address<<" value="<<value<<"\n";
    if(address>=0xff10&&address<0xff40&&t.lines++<800){
        std::cout<<std::dec<<t.frame<<" bank="<<std::hex<<t.bank<<" pc="<<pc<<" addr="<<address<<" val="<<value<<" sp="<<sp<<" stack=";
        for(unsigned i=0;i<16;i++){
            unsigned a=(sp+i)&0xffff;int v=-1;
            if(a>=0xc000&&a<0xe000)v=static_cast<unsigned char*>(t.core->rambank0_ptr())[a-0xc000];
            if(a>=0xff80)v=static_cast<unsigned char*>(t.core->zeropage_ptr())[a-0xff80];
            std::cout<<v<<",";
        }std::cout<<"\n";
    }
    return true;
}
int wmain(int argc,wchar_t** argv){
    if(argc!=2)return 1;auto rom=read(argv[1]);Input input;auto core=loadCoreBytes(rom,&input);
    std::array<gambatte::video_pixel_t,160*144> pixels{};
    skipBootLogos(core,identifyGame(rom),rom,&input,pixels);
    Trace trace{core.get()};core->setWriteFilter(observe,&trace);
    std::array<gambatte::uint_least32_t,40000> audio{};
    for(unsigned frame=0;frame<900;frame++){
        trace.frame=frame;input.value=(frame%120==60||frame%120==61)?1:0;
        unsigned samples=35112;core->runFor(pixels.data(),160,audio.data(),audio.size(),samples);
    }
}
