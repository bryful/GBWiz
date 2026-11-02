#include "turbo_runner.h"
#include <iostream>
void cartridge_set_rumble(unsigned){}
void require(bool ok,const char* text){if(!ok)throw std::runtime_error(text);}
int wmain(int argc,wchar_t** argv)try{
    require(argc==2||argc==3,"Expected ROM and optional tavern fixture");auto rom=read(argv[1]);
    require(PartySupport::recognizes(rom),"Revision unrecognized");
    unsigned mask=identifyGame(rom).id==GameId::Wizardry1?0x55d3:identifyGame(rom).id==GameId::Wizardry2?0x5a1e:0x58aa;
    auto privateRom=rom;privateRom[0x100]=char(0xc3);privateRom[0x101]=0;privateRom[0x102]=char(0xcb);
    for(unsigned position=0;position<6;position++)for(unsigned existing=0;existing<5;existing++)for(unsigned candidate=0;candidate<3;candidate++)for(bool direct:{false,true})for(bool away:{false,true}){
        auto core=loadCoreBytes(privateRom,nullptr);auto ram=static_cast<unsigned char*>(core->rambank0_ptr());
        std::fill(ram+0x103,ram+0x10a+20*110,0);
        ram[0x109]=existing==3?0:existing==4?2:position+1;
        // Neutral entries before the tested party position do not affect compatibility.
        for(unsigned i=0;i<6;i++){ram[0x103+i]=static_cast<unsigned char>(i);ram[0x10a+i*110]=1;ram[0x114+i*110]=1;}
        if(existing<3)ram[0x114+position*110]=static_cast<unsigned char>(existing);
        if(existing==4){ram[0x114]=0;ram[0x114+110]=2;}
        ram[0x10a+6*110]=1;ram[0x114+6*110]=static_cast<unsigned char>(candidate);
        ram[0x10a+6*110+0x58]=away?1:0;
        const unsigned char code[]={0xf3,0x31,0xf0,0xcf,0x3e,static_cast<unsigned char>(direct?1:3),0xea,0x49,0xc0,0xea,0x00,0x20,0x3e,1,0xea,0x70,0xff,0xcd,static_cast<unsigned char>(direct?mask:0x58b5),static_cast<unsigned char>(direct?mask>>8:0x58),0x3e,0xa5,0xea,0x80,0xca,0x18,0xfe};
        std::copy(std::begin(code),std::end(code),ram+0xb00);ram[0xa80]=0;
        auto roster=std::vector<unsigned char>(ram+0x103,ram+0x10a+20*110);
        std::vector<char> initial(core->stateSize());core->saveState(initial.data());TurboRunner runner;runner.attach(*core,rom,identifyGame(rom));
        unsigned input=0;std::array<gambatte::video_pixel_t,160*144> pixels{};std::array<gambatte::uint_least32_t,40000> sound{};
        for(bool enabled:{false,true,false}){
            require(runner.loadState(initial),"State reset failed");auto before=runner.saveState();
            require(runner.party().enable(enabled)&&before==runner.saveState(),"Toggle changed game state");
            for(unsigned f=0;f<10&&ram[0xa80]!=0xa5;f++)runner.run(0,input,pixels,sound);
            require(ram[0xa80]==0xa5,"Native routine did not return");
            require(std::equal(roster.begin(),roster.end(),ram+0x103),"Character or party data changed");
            if(direct){require(ram[0]==(existing==0?1:existing==2?2:existing==4?3:0),"Non-tavern alignment mask changed");continue;}
            auto bank1=ram+0x1000;bool found=false;
            for(unsigned i=0;i<bank1[0x3a8];i++)if(bank1[0x3a9+i]==6)found=true;
            bool conflict=(existing==4&&candidate!=1)||(existing<3&&existing!=1&&candidate!=1&&existing!=candidate);
            require(found==(!away&&(!conflict||enabled)),"Native candidate eligibility incorrect");
            for(unsigned i=0;i<bank1[0x3a8];i++)require(bank1[0x3a9+i]>=ram[0x109],"Duplicate party member offered");
        }
    }
    if(argc==3)for(int speed:{2,3,4,6,8})for(bool enabled:{false,true}){
        struct Input:gambatte::InputGetter{unsigned value=0;unsigned operator()()override{return value;}} input;
        auto core=loadCoreBytes(rom,&input);auto state=read(argv[2]);require(core->loadState(state.data(),state.size()),"Tavern fixture rejected");
        auto ram=static_cast<unsigned char*>(core->rambank0_ptr());
        require(ram[0x109]==0,"Fixture party not empty");
        for(unsigned i=2;i<20;i++)ram[0x10a+i*110]=0;
        ram[0x114]&=0xfc;ram[0x114+110]=(ram[0x114+110]&0xfc)|2;
        TurboRunner runner;runner.attach(*core,rom,identifyGame(rom));require(runner.party().enable(enabled)&&runner.speed(speed),"UI toggle or speed failed");
        std::array<gambatte::video_pixel_t,160*144> pixels{};std::array<gambatte::uint_least32_t,40000> sound{};
        auto run=[&](unsigned key,unsigned frames){for(unsigned f=0;f<frames;f++)runner.run(key,input.value,pixels,sound);};
        auto press=[&](unsigned key){run(key,4);run(0,120);};
        run(0,60);press(1);press(1);
        require(ram[0x109]==1&&ram[0x103]==0,"UI first member failed");
        if(enabled){
            auto openPicker=runner.saveState();
            require(runner.party().enable(false),"Mid-picker OFF failed");press(1);
            require(ram[0x109]==1,"Mid-picker OFF allowed incompatible entry");
            require(runner.party().enable(true)&&runner.loadState(openPicker),"Mid-picker ON or restore failed");
        }
        press(1);require(ram[0x109]==(enabled?2:1),"UI mixed party result incorrect");
        require(runner.party().enable(false),"UI OFF failed");require(ram[0x109]==(enabled?2:1),"OFF dismantled party");
        require((ram[0x114]&3)==0&&(ram[0x114+110]&3)==2,"UI changed alignment");
        require(core->loadState(state.data(),state.size()),"Capacity fixture reset failed");
        runner.attach(*core,rom,identifyGame(rom));require(runner.party().enable(true),"Capacity ON failed");
        ram[0x109]=6;for(unsigned i=0;i<6;i++)ram[0x103+i]=static_cast<unsigned char>(i);
        run(0,60);press(1);require(ram[0x109]==6,"Six-member capacity bypassed");
    }
    for(unsigned offset:{mask,0xd8b5u,0xd8fdu,mask==0x5a1e?0x9862u:0x9871u}){auto unknown=rom;unknown[offset]^=1;require(!PartySupport::recognizes(unknown),"Unknown revision accepted");}
    std::cout<<"PASS: native tavern candidate builder, six party positions, all alignments, OFF/ON/OFF, duplicate exclusion, unchanged roster/party, non-tavern mask preserved, revision rejection\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
