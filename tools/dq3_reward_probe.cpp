#include "game_support.h"
#include <iostream>
#include <sstream>
#include <map>
void cartridge_set_rumble(unsigned){}
struct Input:gambatte::InputGetter{unsigned value=0;unsigned operator()()override{return value;}};
struct Trace{unsigned bank=0,wram=0,frame=0,events=0;std::ofstream out;};
bool observe(void* user,unsigned pc,unsigned sp,unsigned address,unsigned value){
    auto& t=*static_cast<Trace*>(user);
    if(address>=0x2000&&address<0x3000)t.bank=(t.bank&256)|value;
    if(address>=0x3000&&address<0x4000)t.bank=(t.bank&255)|((value&1)<<8);
    if(address==0xff70)t.wram=value&7;
    bool watched=(address>=0xd020&&address<=0xd024)||(address>=0xc4dc&&address<=0xc4de);
    for(unsigned i=0;i<4;i++)watched|=address>=0xc562+i*0x48&&address<=0xc564+i*0x48;
    if(watched){t.out<<t.frame<<','<<std::hex<<t.bank<<','<<t.wram<<','<<pc<<','<<sp<<','<<address<<','<<value<<std::dec<<'\n';++t.events;}
    return true;
}
int wmain(int argc,wchar_t** argv)try{
    if(argc!=5)throw std::runtime_error("Expected ROM, field state, script, output directory");
    auto rom=read(argv[1]);Input input;auto core=loadCoreBytes(rom,&input);auto state=read(argv[2]);if(!core->loadState(state.data(),state.size()))throw std::runtime_error("Bad checkpoint");
    fs::path output=argv[4];fs::create_directories(output);Trace trace;trace.out.open(output/"writes.csv");core->setWriteFilter(observe,&trace);
    std::array<gambatte::video_pixel_t,160*144> pixels{};std::array<gambatte::uint_least32_t,40000> sound{};
    std::map<std::string,unsigned> keys={{"A",1},{"B",2},{"LEFT",32},{"WAIT",0}};
    std::ifstream script(argv[3]);std::string line;unsigned capture=0;
    while(std::getline(script,line)){
        std::istringstream row(line);std::string key;unsigned frames=0;row>>key>>frames;if(!frames)continue;input.value=keys.at(key);
        for(unsigned i=0;i<frames;i++){
            auto before=std::vector<char>(core->stateSize());core->saveState(before.data());unsigned oldEvents=trace.events;
            ++trace.frame;unsigned samples=35112;core->runFor(pixels.data(),160,sound.data(),sound.size(),samples);
            if(trace.events!=oldEvents){
                auto name=std::to_string(++capture);std::ofstream pre(output/(name+"-before.bin"),std::ios::binary);pre.write(before.data(),before.size());
                std::vector<char> after(core->stateSize());core->saveState(after.data());std::ofstream post(output/(name+"-after.bin"),std::ios::binary);post.write(after.data(),after.size());
                std::ofstream ram(output/(name+"-ram.bin"),std::ios::binary);ram.write(static_cast<const char*>(core->rambank0_ptr()),0x8000);
                trace.out<<"capture,"<<name<<','<<trace.frame<<'\n';
            }
        }
    }
    std::cout<<"Frames="<<trace.frame<<" events="<<trace.events<<" captures="<<capture<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
