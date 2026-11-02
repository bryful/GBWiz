#include "turbo_runner.h"
#include <iostream>
void cartridge_set_rumble(unsigned){}
void require(bool ok,const char* text){if(!ok)throw std::runtime_error(text);}
struct Input:gambatte::InputGetter{unsigned value=0;unsigned operator()()override{return value;}};
struct Session{
    Input input;std::unique_ptr<gambatte::GB> core;TurboRunner runner;
    std::array<gambatte::video_pixel_t,160*144> pixels{};std::array<gambatte::uint_least32_t,40000> sound{};
    Session(const std::vector<char>& rom,const std::vector<char>& state,bool maximum,int speed=2):core(loadCoreBytes(rom,&input)){
        require(core->loadState(state.data(),state.size()),"Fixture rejected");runner.attach(*core,rom,identifyGame(rom));require(runner.creation().enable(maximum)&&runner.speed(speed),"Settings rejected");
    }
    unsigned char* ram(){return static_cast<unsigned char*>(core->rambank0_ptr());}
    void run(unsigned key,unsigned frames){for(unsigned f=0;f<frames;f++)runner.run(key,input.value,pixels,sound);}
    void press(unsigned key,unsigned wait=90){run(key,4);run(0,wait);}
    void choose(unsigned race){run(0,60);for(unsigned i=0;i<race;i++)press(128);press(1);press(1,120);}
};
int wmain(int argc,wchar_t** argv)try{
    require(argc==3,"Expected GBC ROM and native race-selection checkpoint");auto rom=read(argv[1]),state=read(argv[2]);auto id=identifyGame(rom).id;
    unsigned consumer=id==GameId::Wizardry2?0x19376:id==GameId::Wizardry3?0x1931b:0x19324;
    unsigned generator=id==GameId::Wizardry2?0x19b9c:id==GameId::Wizardry3?0x19b02:0x19b0b;
    unsigned cap=id==GameId::Wizardry2?0x194f2:id==GameId::Wizardry3?0x19497:0x194a0;
    require(CreationSupport::recognizes(rom),"Creation revision unrecognized");
    for(unsigned race=0;race<5;race++)for(int speed:{2,3,4,6,8}){
        Session normal(rom,state,false,speed),maximum(rom,state,true,speed);normal.choose(race);maximum.choose(race);
        require(maximum.runner.creation().rolls()==1&&maximum.ram()[0x0c]==60,"Maximum bonus not generated exactly once");
        require(normal.ram()[0x0c]>=5&&normal.ram()[0x0c]<=60,"Native bonus range wrong");
        require((maximum.ram()[0x9ac]>>5)==race,"Selected race incorrect");
        require(std::equal(maximum.ram()+0x9ad,maximum.ram()+0x9b3,normal.ram()+0x9ad),"Race base attributes changed");
        require(std::equal(maximum.ram()+0x10a,maximum.ram()+0x9a2,normal.ram()+0x10a),"Existing characters changed");
        std::array<unsigned char,6> base{};std::copy(maximum.ram()+0x9ad,maximum.ram()+0x9b3,base.begin());
        maximum.press(16,30);require(maximum.ram()[0x0c]==59&&maximum.ram()[0x9ad]==base[0]+1,"Spending failed");
        auto spent=maximum.runner.saveState();maximum.press(32,30);require(maximum.ram()[0x0c]==60,"Refund failed");
        require(maximum.runner.loadState(spent)&&maximum.ram()[0x0c]==59,"State restored spent bonus points");maximum.press(32,30);
        require(maximum.runner.creation().enable(false),"OFF failed");maximum.press(16,30);require(maximum.ram()[0x0c]==59,"OFF changed current allocation");
        require(maximum.runner.creation().enable(true),"ON failed");maximum.press(32,30);require(maximum.ram()[0x0c]==60,"ON refilled or changed allocation");
        for(unsigned stat=0;stat<6;stat++){
            for(unsigned point=0;point<11;point++)maximum.press(16,30);
            require(maximum.ram()[0x9ad+stat]==base[stat]+10,"Native base+10 cap changed");
            if(stat<5)maximum.press(128,30);
        }
        require(maximum.ram()[0x0c]==0,"All 60 points were not spent");
        unsigned empty=0;while(empty<20&&maximum.ram()[0x10a+empty*110])++empty;require(empty<20,"Fixture roster full");
        for(unsigned attempt=0;attempt<8&&!maximum.ram()[0x10a+empty*110];attempt++)maximum.press(1,120);
        auto record=maximum.ram()+0x10a+empty*110;require(record[0]!=0,"Native character creation did not complete");
        for(unsigned i=0;i<6;i++)require(record[0x0b+i]==base[i]+10,"Registered attributes changed");
        auto save=static_cast<const unsigned char*>(maximum.core->savedata_ptr())+0x2b+empty*110;
        for(unsigned attempt=0;attempt<4&&!std::equal(record,record+110,save);attempt++)maximum.press(2,120);
        require(std::equal(record,record+110,save),"Native save does not match new character");
    }
    Session off(rom,state,false);Input referenceInput;auto reference=loadCoreBytes(rom,&referenceInput);require(reference->loadState(state.data(),state.size()),"Reference state rejected");
    for(unsigned f=0;f<308;f++){
        unsigned key=(f>=60&&f<64)||(f>=154&&f<158)?1:0;off.run(key,1);referenceInput.value=key;unsigned samples=35112;reference->runFor(off.pixels.data(),160,off.sound.data(),off.sound.size(),samples);
    }
    std::vector<char> expected(reference->stateSize());reference->saveState(expected.data());require(off.runner.saveState()==expected,"OFF changed native emulation");
    for(unsigned at:{generator,consumer,cap}){auto unknown=rom;unknown[at]^=1;require(!CreationSupport::recognizes(unknown),"Unknown revision accepted");}
    std::cout<<"PASS: 5 races x 5 speeds, 60 points, native base+10 caps, spend/refund, mid-allocation toggle, state restore, character registration/SRAM, OFF exact equality and revision rejection\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
