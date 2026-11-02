#include "turbo_runner.h"
#include <iostream>
void cartridge_set_rumble(unsigned){}
void require(bool ok,const char* text){if(!ok)throw std::runtime_error(text);}
struct Input:gambatte::InputGetter{unsigned value=0;unsigned operator()()override{return value;}};
struct Session{
    Input input;std::unique_ptr<gambatte::GB> core;TurboRunner runner;
    std::array<gambatte::video_pixel_t,160*144> pixels{};std::array<gambatte::uint_least32_t,40000> sound{};
    Session(const std::vector<char>& rom,const std::vector<char>& state):core(loadCoreBytes(rom,&input)){require(core->loadState(state.data(),state.size()),"Fixture rejected");runner.attach(*core,rom,identifyGame(rom));}
    unsigned char* ram(){return static_cast<unsigned char*>(core->rambank0_ptr());}
    void run(unsigned key,unsigned frames){for(unsigned f=0;f<frames;f++)runner.run(key,input.value,pixels,sound);}
    void press(unsigned key){run(key,4);run(0,120);}
};
int wmain(int argc,wchar_t** argv)try{
    require(argc==3,"Expected ROM and town fixture");auto rom=read(argv[1]),state=read(argv[2]);
    require(PartySupport::recognizes(rom),"Party revision not recognized");
    // Establish original town-save semantics: the game saves characters but
    // starts town with an empty party after reboot, even without mixed parties.
    unsigned originalTownParty=0;
    {
        Session s(rom,state);require(s.runner.spells().setAlignment(*s.core,0,0)&&s.runner.spells().setAlignment(*s.core,1,0),"Control alignments failed");
        s.run(0,60);for(unsigned i=0;i<4;i++)s.press(1);require(s.ram()[0x94f]==2,"Control party formation failed");s.press(2);s.press(2);
        auto fresh=loadCoreBytes(rom,&s.input);std::memcpy(fresh->savedata_ptr(),s.core->savedata_ptr(),s.core->savedata_size());s.input.value=0;
        skipBootLogos(fresh,identifyGame(rom),rom,&s.input,s.pixels);
        for(unsigned f=0;f<128;f++){s.input.value=f<8?1:0;unsigned count=35112;fresh->runFor(s.pixels.data(),160,s.sound.data(),s.sound.size(),count);}
        originalTownParty=static_cast<unsigned char*>(fresh->rambank0_ptr())[0x94f];require(originalTownParty==0,"Unexpected original town reload behavior");
    }
    // Execute the original compatibility routine, at every party position and
    // for good/neutral/evil/empty entries. Toggle without changing RAM or SRAM.
    auto privateRom=rom;privateRom[0x100]=char(0xc3);privateRom[0x101]=0;privateRom[0x102]=char(0xc0);
    for(unsigned position=0;position<6;position++)for(unsigned existing=0;existing<4;existing++)for(unsigned candidate=0;candidate<3;candidate++){
        auto core=loadCoreBytes(privateRom,nullptr);auto ram=static_cast<unsigned char*>(core->rambank0_ptr());
        std::fill(ram+0x950,ram+0x956,0xff);if(existing<3)ram[0x950+position]=static_cast<unsigned char>((existing<<6)|position);
        const unsigned char code[]={0xf3,0x31,0xf0,0xcf,0x3e,static_cast<unsigned char>(candidate),0xcd,0xe7,0x12,0x3e,0,0xce,0,0xea,0x80,0xc3,0x3e,0xa5,0xea,0x81,0xc3,0x18,0xfe};
        std::copy(std::begin(code),std::end(code),ram);ram[0x381]=0;
        std::vector<char> initial(core->stateSize());core->saveState(initial.data());TurboRunner runner;runner.attach(*core,rom,identifyGame(rom));
        auto originalRom=static_cast<const unsigned char*>(core->rombank0_ptr());std::vector<unsigned char> bank(originalRom,originalRom+0x4000);
        unsigned input=0;std::array<gambatte::video_pixel_t,160*144> pixels{};std::array<gambatte::uint_least32_t,40000> sound{};
        for(bool enabled:{false,true,false}){
            require(runner.loadState(initial),"Routine reset failed");auto before=runner.saveState();
            require(runner.party().enable(enabled)&&before==runner.saveState(),"Toggle modified game data");
            for(unsigned i=0;i<0x4000;i++)require(originalRom[i]==(enabled&&i==0x12e7?0xaf:enabled&&i==0x12e8?0xc9:bank[i]),"Patch changed unrelated ROM bytes");
            for(unsigned f=0;f<10&&ram[0x381]!=0xa5;f++)runner.run(0,input,pixels,sound);
            require(ram[0x381]==0xa5,"Compatibility routine did not return");
            bool conflict=existing<3&&existing!=1&&candidate!=1&&existing!=candidate;
            require(ram[0x380]==unsigned(!enabled&&conflict),"Compatibility result incorrect");
        }
        require(runner.party().enable(true),"Enable failed");runner.detach();require(std::equal(bank.begin(),bank.end(),originalRom),"Detach failed to undo patch");
    }
    for(int speed:{2,3,4,6,8}){
        Session s(rom,state);require(!s.runner.party().enabled(),"Default is ON");
        require(s.runner.spells().setAlignment(*s.core,0,0)&&s.runner.spells().setAlignment(*s.core,1,2),"Test alignments rejected");
        auto roster=std::vector<unsigned char>(s.ram()+0x1186,s.ram()+0x1186+20*128);
        require(s.runner.speed(speed),"Speed rejected");s.run(0,60);s.press(1);s.press(1);s.press(1);
        require(s.ram()[0x94f]==1&&s.ram()[0x950]==0,"Good character did not join");
        s.press(1);require(s.ram()[0x94f]==1,"OFF allowed evil character");
        require(s.runner.party().enable(true),"ON failed");s.press(1);s.press(1);
        require(s.ram()[0x94f]==2&&s.ram()[0x950]==0&&s.ram()[0x951]==0x81,"ON failed to form good+evil party");
        auto mixed=s.runner.saveState();require(s.runner.party().enable(false),"OFF failed");
        require(s.ram()[0x94f]==2&&s.ram()[0x951]==0x81,"OFF dismantled existing mixed party");
        s.press(1);require(s.ram()[0x94f]==2,"OFF allowed incompatible addition to mixed party");
        require(s.runner.loadState(mixed)&&!s.runner.party().enabled(),"State load replaced current OFF setting");
        require(s.runner.party().enable(true)&&s.runner.loadState(mixed)&&s.runner.party().enabled(),"State load lost current ON setting");
        s.press(2);s.press(2); // Leave member picker and tavern through original save path.
        require(std::equal(roster.begin(),roster.end(),s.ram()+0x1186),"Party feature changed character attributes/stats");
        auto fresh=loadCoreBytes(rom,&s.input);std::memcpy(fresh->savedata_ptr(),s.core->savedata_ptr(),s.core->savedata_size());
        s.input.value=0;skipBootLogos(fresh,identifyGame(rom),rom,&s.input,s.pixels);
        for(unsigned f=0;f<128;f++){s.input.value=f<8?1:0;unsigned count=35112;fresh->runFor(s.pixels.data(),160,s.sound.data(),s.sound.size(),count);}
        auto restored=static_cast<unsigned char*>(fresh->rambank0_ptr());
        require(restored[0x94f]==originalTownParty,"Mixed-party feature changed original town reload behavior");
        require(restored[0x1186+10]==0&&restored[0x1186+128+10]==2,"Reload changed alignment");
    }
    // Capacity is checked before compatibility and must still reject entry.
    {
        Session s(rom,state);s.ram()[0x94f]=6;for(unsigned i=0;i<6;i++)s.ram()[0x950+i]=static_cast<unsigned char>(i);
        require(s.runner.party().enable(true),"Capacity fixture ON failed");s.run(0,60);s.press(1);s.press(1);
        require(s.ram()[0x94f]==6,"ON bypassed six-member capacity");
    }
    for(unsigned offset:{0x12e7u,0xaa75u}){auto unknown=rom;unknown[offset]^=1;Session s(unknown,state);auto before=s.runner.saveState();require(!s.runner.party().supported()&&!s.runner.party().enable(true)&&before==s.runner.saveState(),"Unknown revision enabled");}
    std::cout<<"PASS: original compatibility truth table, OFF/ON/OFF, exact patch undo, real mixed-party formation, five speeds, attributes/stats and capacity preserved, save reload, current setting on state load\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}
