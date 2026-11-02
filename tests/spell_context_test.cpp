#include "turbo_runner.h"
#include <iostream>
void cartridge_set_rumble(unsigned){}
void require(bool ok,const char* text){if(!ok)throw std::runtime_error(text);}
struct Input:gambatte::InputGetter{unsigned value=0;unsigned operator()()override{return value;}};
bool learned(const unsigned char* record){return (record[0x39]&31)==31&&std::all_of(record+0x3a,record+0x41,[](unsigned char v){return v==255;})&&std::all_of(record+0x41,record+0x4f,[](unsigned char v){return v==9;});}
int wmain(int argc,wchar_t** argv)try{
    require(argc==3,"Expected Gaiden 1 ROM and local build directory");auto rom=read(argv[1]);fs::path root=argv[2];
    std::array<gambatte::video_pixel_t,160*144> pixels{};std::array<gambatte::uint_least32_t,40000> sound{};
    for(const char* path:{"bonus/city/core.bin","bonus/start/core.bin","bonus/name/core.bin","turbo-play/victory-fixed/core.bin","spells/camp/core.bin"}){
        for(bool mp:{false,true})for(unsigned selected=0;selected<6;selected++){
            Input input;auto core=loadCoreBytes(rom,&input);auto state=read(root/path);require(core->loadState(state.data(),state.size()),"Context state rejected");
            TurboRunner runner;runner.attach(*core,rom,identifyGame(rom));
            for(unsigned f=0;f<60&&!runner.spells().available(*core)&&!runner.spells().battle(*core);f++)runner.run(0,input.value,pixels,sound);
            require(runner.spells().available(*core),"Non-battle context unavailable");
            auto ram=static_cast<unsigned char*>(core->rambank0_ptr());auto save=static_cast<unsigned char*>(core->savedata_ptr());
            std::vector<unsigned char> before(ram,ram+8192),saved(save,save+8192);
            if(!(mp?runner.spells().restoreMp(*core,selected):runner.spells().learn(*core,selected))){std::cerr<<path<<" slot="<<selected<<"\n";require(false,"Context learning failed");}
            for(unsigned i=0;i<20;i++){
                unsigned at=0x1186+128*i,primary=0x17+128*i;
                if(i==selected){require(mp?std::all_of(ram+at+0x41,ram+at+0x4f,[](unsigned char v){return v==9;})&&std::all_of(save+primary+0x41,save+primary+0x4f,[](unsigned char v){return v==9;}):learned(ram+at)&&learned(save+primary),"Context edit missing");for(unsigned j=0;j<128;j++)if(mp?(j<0x41||j>=0x4f):(j<0x39||j>=0x4f)){require(ram[at+j]==before[at+j],"Live stats changed");require(save[primary+j]==saved[primary+j],"Saved stats changed");}}
                else require(std::memcmp(ram+at,before.data()+at,128)==0,"Another character changed");
            }
            for(unsigned f=0;f<180;f++)runner.run(0,input.value,pixels,sound);
            require(mp?std::all_of(ram+0x1186+selected*128+0x41,ram+0x1186+selected*128+0x4f,[](unsigned char v){return v==9;}):learned(ram+0x1186+selected*128),"Edit lost after resuming context");
        }
    }
    {
        auto state=read(root/"rewards/before-win/core.bin");auto core=loadCoreBytes(rom,nullptr);core->loadState(state.data(),state.size());TurboRunner runner;runner.attach(*core,rom,identifyGame(rom));
        auto before=runner.saveState();require(runner.spells().battle(*core)&&!runner.spells().available(*core),"Battle was not excluded");
        for(unsigned i=0;i<20;i++)require(!runner.spells().learn(*core,i),"Battle learning accepted");require(before==runner.saveState(),"Rejected battle edit changed state");
    }
    // Execute the ROM's actual load/save routines around a paused live edit.
    for(bool duplicate:{false,true}){
        auto source=loadCoreBytes(rom,nullptr);auto state=read(root/"bonus/city/core.bin");source->loadState(state.data(),state.size());
        auto privateRom=rom;privateRom[0x100]=char(0xc3);privateRom[0x101]=0;privateRom[0x102]=char(0xc0);
        auto core=loadCoreBytes(privateRom,nullptr);auto ram=static_cast<unsigned char*>(core->rambank0_ptr());auto save=static_cast<unsigned char*>(core->savedata_ptr());
        std::memcpy(ram,source->rambank0_ptr(),8192);std::memcpy(save,source->savedata_ptr(),8192);
        unsigned active=duplicate?1:2,target=duplicate?0:2;
        if(duplicate){std::memcpy(ram+0x1186+128,ram+0x1186,128);std::memcpy(save+0x17+128,save+0x17,128);}
        std::vector<unsigned char> code={0xf3,0x31,0xf0,0xcf,0x3e,2,0xea,0xae,0xd0,0xea,0,0x20,0x3e,static_cast<unsigned char>(active),0xcd,0x18,0x13,0x3e,3,0xea,0x39,0xd1,0x3e,0xa5,0xea,0x80,0xc0,0xfa,0x81,0xc0,0xb7,0x28,0xfa,0x3e,static_cast<unsigned char>(active),0xcd,0x46,0x13,0x3e,0x5a,0xea,0x82,0xc0,0x18,0xfe};
        std::copy(code.begin(),code.end(),ram);ram[0x80]=ram[0x81]=ram[0x82]=0;
        TurboRunner runner;runner.attach(*core,rom,identifyGame(rom));unsigned input=0;runner.run(0,input,pixels,sound);require(ram[0x80]==0xa5,"Original character load did not finish");
        auto workBefore=std::vector<unsigned char>(ram+0x1104,ram+0x1184);
        require(runner.spells().learn(*core,target),"Paused workspace edit failed");
        if(duplicate)require(std::equal(workBefore.begin(),workBefore.end(),ram+0x1104),"Identical-name non-target workspace was modified");
        else require(learned(ram+0x1104)&&ram[0x1139]==3,"Target workspace was not merged correctly");
        ram[0x81]=1;runner.run(0,input,pixels,sound);require(ram[0x82]==0x5a,"Original save routine did not finish");
        require(learned(ram+0x1186+128*target)&&learned(save+0x17+128*target),"Original save overwrote the learned spells");
        if(!duplicate)require(ram[0x1186+128*active+0x35]==3,"Original live HP update was lost");
        else require(!learned(ram+0x1186+128*active),"Duplicate-name non-target inherited spells");
    }
    std::cout<<"PASS: town/facility/creation/dungeon/camp, battle refusal, live workspace save, duplicate names, HP preservation and current/max MP9\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}
