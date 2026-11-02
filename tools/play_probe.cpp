#include "game_support.h"
#include "turbo_runner.h"
#include <iostream>
#include <sstream>
#include <map>
void cartridge_set_rumble(unsigned){}
struct Input:gambatte::InputGetter{unsigned value=0;unsigned operator()()override{return value;}};
struct Trace{unsigned frame=0,bank=1,wram=1;bool rewards=false;std::ofstream output;};
bool trace(void* data,unsigned pc,unsigned,unsigned addr,unsigned value){
    auto& t=*static_cast<Trace*>(data);
    if(addr>=0x2000&&addr<0x3000)t.bank=(t.bank&0x100)|value;
    if(addr>=0x3000&&addr<0x4000)t.bank=(t.bank&255)|((value&1)<<8);
    if(addr==0xff70)t.wram=(value&7)?value&7:1;
    if((t.rewards&&addr>=0xa000&&addr<0xe000)||addr==0xccd1||(addr==0xdb86&&pc==0x3aa0&&value!=0xff)||
        (addr>=0xffed&&addr<=0xfff0&&pc!=0x3339)||addr==0xff06||addr==0xff07||
        (addr>=0xff10&&addr<0xff40)||(t.wram==2&&addr>=0xd000&&addr<=0xd00d))
        t.output<<t.frame<<","<<std::hex<<pc<<","<<addr<<","<<value<<","<<t.bank<<","<<t.wram<<std::dec<<"\n";
    return true;
}
int wmain(int argc,wchar_t** argv)try{
    if(argc<5||argc>7)return 1;auto rom=read(argv[1]);Input input;auto core=loadCoreBytes(rom,&input);restoreBattery(*core,argv[2]);
    std::array<gambatte::video_pixel_t,160*144> pixels{};std::array<gambatte::uint_least32_t,40000> sound{};
    bool rewards=argc>=6&&std::wstring(argv[5])==L"rewards";
    bool traced=argc>=6&&(std::wstring(argv[5])==L"trace"||rewards);
    bool plain=argc>=6&&(std::wstring(argv[5])==L"plain"||traced);
    if(!plain)skipBootLogos(core,identifyGame(rom),rom,&input,pixels);
    if(argc==7){auto state=read(argv[6]);if(!core->loadState(state.data(),state.size()))throw std::runtime_error("Invalid probe checkpoint");}
    fs::path output=argv[4];fs::create_directories(output);
    Trace log;log.rewards=rewards;log.output.open(output/L"commands.csv");
    TurboRunner turbo;bool accelerated=argc>=6&&!plain;
    if(accelerated){turbo.attach(*core,rom,identifyGame(rom));if(!turbo.speed(std::stoi(argv[5])))return 2;}
    else if(!plain||traced)core->setWriteFilter(trace,&log);
    std::ofstream steps(output/L"steps.csv");steps<<"step,ticks,music,effects,samples,scrollX,scrollY,input\n";unsigned long long totalSamples=0;
    std::ofstream forwarded(output/L"forwarded.csv");forwarded<<"tick,song,effects\n";
    std::ifstream script(argv[3]);std::string line;unsigned step=0;
    std::map<std::string,unsigned> keys={{"A",1},{"B",2},{"SELECT",4},{"START",8},{"RIGHT",16},{"LEFT",32},{"UP",64},{"DOWN",128},{"WAIT",0}};
    while(std::getline(script,line)){
        std::istringstream row(line);std::string name;unsigned frames=0;row>>name>>frames;if(!frames)continue;
        if(name=="REWARDS"){
            unsigned gold=1;row>>gold;
            if(!accelerated){turbo.attach(*core,rom,identifyGame(rom));accelerated=true;}
            if(!turbo.rewards().set(frames,gold))throw std::runtime_error("Unsupported reward settings");
            continue;
        }
        if(name=="MAXBONUS"){
            if(!accelerated){turbo.attach(*core,rom,identifyGame(rom));accelerated=true;}
            if(!turbo.creation().enable(frames==1))throw std::runtime_error("Unsupported creation settings");
            continue;
        }
        if(name=="LEARN"){
            if(!accelerated){turbo.attach(*core,rom,identifyGame(rom));accelerated=true;}
            if(!turbo.spells().learn(*core,frames-1))throw std::runtime_error("Spell learning needs a valid character in town");
            continue;
        }
        if(name=="MIXEDPARTY"||name=="ALIGN"){
            if(!accelerated){turbo.attach(*core,rom,identifyGame(rom));accelerated=true;}
            if(name=="MIXEDPARTY"){if(!turbo.party().enable(frames==1))throw std::runtime_error("Unsupported mixed-party revision");}
            else{unsigned alignment=0;row>>alignment;if(!turbo.spells().setAlignment(*core,frames-1,alignment))throw std::runtime_error("Alignment edit rejected");}
            continue;
        }
        if(name=="SPEED"){
            if(!accelerated){turbo.attach(*core,rom,identifyGame(rom));accelerated=true;}
            if(!turbo.speed(frames))return 2;
            continue;
        }
        unsigned held=keys.at(name);input.value=held;
        for(unsigned i=0;i<frames;i++){
            ++log.frame;unsigned samples=35112,commands=turbo.musicCommands(),effects=turbo.effectCommands();
            if(accelerated)samples=turbo.run(held,input.value,pixels,sound);
            else core->runFor(pixels.data(),160,sound.data(),sound.size(),samples);
            totalSamples+=samples;
            if(turbo.musicCommands()!=commands||turbo.effectCommands()!=effects)
                forwarded<<log.frame<<","<<(identifyGame(rom).id==GameId::Dq3?turbo.dqSong():core->isCgb()?static_cast<unsigned>(static_cast<unsigned char*>(core->rambank0_ptr())[0x2008]):static_cast<unsigned>(static_cast<unsigned char*>(core->zeropage_ptr())[0x70]))<<","<<turbo.effectCommands()<<"\n";
        }
        std::array<unsigned char,54> header{};header[0]='B';header[1]='M';header[10]=54;header[14]=40;header[18]=160;header[22]=144;header[26]=1;header[28]=32;
        unsigned size=54+pixels.size()*4;std::memcpy(header.data()+2,&size,4);
        std::ofstream image(output/(std::to_wstring(++step)+L".bmp"),std::ios::binary);image.write(reinterpret_cast<char*>(header.data()),header.size());
        for(int y=143;y>=0;y--)image.write(reinterpret_cast<const char*>(pixels.data()+160*y),640);
        auto io=static_cast<unsigned char*>(core->zeropage_ptr());
        steps<<step<<","<<log.frame<<","<<turbo.musicCommands()<<","<<turbo.effectCommands()<<","<<totalSamples<<","<<unsigned(io[-0x3d])<<","<<unsigned(io[-0x3e])<<","<<input.value<<"\n";
    }
    std::vector<char> state(core->stateSize());core->saveState(state.data());
    std::ofstream dump(output/L"core.bin",std::ios::binary);dump.write(state.data(),state.size());
    std::ofstream ramDump(output/L"wram.bin",std::ios::binary);ramDump.write(static_cast<const char*>(core->rambank0_ptr()),0x2000);
    std::ofstream batteryDump(output/L"battery.bin",std::ios::binary);batteryDump.write(static_cast<const char*>(core->savedata_ptr()),core->savedata_size());
    std::cout<<"Frames="<<log.frame<<" steps="<<step<<" experience rewards="<<turbo.rewards().experienceEvents()<<" gold rewards="<<turbo.rewards().goldEvents()<<"\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}
