#include "turbo_runner.h"
#include <iostream>
void cartridge_set_rumble(unsigned){}
void require(bool ok,const char* text){if(!ok)throw std::runtime_error(text);}
struct Input:gambatte::InputGetter{unsigned value=0;unsigned operator()()override{return value;}};
struct Session{
    Input input;std::unique_ptr<gambatte::GB> core;TurboRunner turbo;bool third=false;
    std::array<gambatte::video_pixel_t,160*144> pixels{};
    std::array<gambatte::uint_least32_t,40000> sound{};
    Session(const std::vector<char>& rom,const std::vector<char>& state,bool enabled,int speed=2):core(loadCoreBytes(rom,&input)){
        third=identifyGame(rom).id==GameId::Gaiden3;
        require(core->loadState(state.data(),state.size()),"Creation checkpoint rejected");turbo.attach(*core,rom,identifyGame(rom));
        require(turbo.creation().enable(enabled)&&turbo.speed(speed),"Creation settings rejected");
    }
    unsigned char* ram(){return static_cast<unsigned char*>(core->rambank0_ptr());}
    void run(unsigned key,unsigned frames){for(unsigned f=0;f<frames;f++)turbo.run(key,input.value,pixels,sound);}
    void press(unsigned key,unsigned wait=60){run(key,4);run(0,wait);}
    void choose(unsigned race,unsigned sex=0){if(third){if(sex)press(128);press(1,120);}for(unsigned i=0;i<race;i++)press(128);press(1,120);press(1,120);}
};
int wmain(int argc,wchar_t** argv)try{
    require(argc==3,"Expected Gaiden 1/2/3 ROM and race-selection checkpoint");auto rom=read(argv[1]),state=read(argv[2]);
    require(CreationSupport::recognizes(rom),"Creation driver not recognized");
    bool second=identifyGame(rom).id==GameId::Gaiden2,third=identifyGame(rom).id==GameId::Gaiden3;
    unsigned bonus=third?0xb14:second?0x1009:0x10fa,raceAt=third?0xb27:second?0x101b:0x110c,stats=third?0xb2d:second?0x1021:0x1112,roster=third?0xba1:second?0x1095:0x1186,races=third?10:5;
    auto unknown=rom;unknown[third?0x211ca:second?0x20d57:0x16e1b]^=1;require(!CreationSupport::recognizes(unknown),"Unknown creation revision accepted");
    for(unsigned sex=0;sex<(third?2u:1u);sex++)for(unsigned race=0;race<races;race++)for(unsigned delay:{0u,17u,59u}){
        Session normal(rom,state,false),maximum(rom,state,true);normal.run(0,delay);maximum.run(0,delay);normal.choose(race,sex);maximum.choose(race,sex);
        require(maximum.turbo.creation().rolls()==1&&maximum.ram()[bonus]==30,"Maximum bonus was not generated exactly once");
        require(normal.ram()[bonus]>=7&&normal.ram()[bonus]<=30,"Original bonus range changed");
        require(maximum.ram()[raceAt]==race,"Wrong race selected");
        require(std::equal(maximum.ram()+stats,maximum.ram()+stats+6,normal.ram()+stats),"Race base attributes were modified");
        require(std::memcmp(maximum.core->savedata_ptr(),normal.core->savedata_ptr(),normal.core->savedata_size())==0,"Generation modified saved characters");
    }
    for(int speed:{3,4,6,8})for(unsigned sex=0;sex<(third?2u:1u);sex++)for(unsigned race=0;race<races;race++){Session s(rom,state,true,speed);s.choose(race,sex);require(s.ram()[bonus]==30&&s.turbo.creation().rolls()==1,"Turbo creation bonus failed");}
    Session s(rom,state,true);s.choose(0);auto base=std::array<unsigned char,6>{};std::copy(s.ram()+stats,s.ram()+stats+6,base.begin());
    s.press(16);require(s.ram()[bonus]==29&&s.ram()[stats]==base[0]+1,"Spending a bonus point failed");
    auto spent=s.turbo.saveState();s.press(32);require(s.ram()[bonus]==30,"Refunding a bonus point failed");
    require(s.turbo.loadState(spent)&&s.ram()[bonus]==29,"State restore refilled spent bonus points");s.press(32);
    for(unsigned stat=0;stat<6&&s.ram()[bonus];stat++){for(unsigned i=0;i<10&&s.ram()[bonus];i++)s.press(16,30);if(stat<5&&s.ram()[bonus])s.press(128,30);}
    require(s.ram()[bonus]==0,"Could not spend all thirty points");
    unsigned total=0;for(unsigned i=0;i<6;i++){require(s.ram()[stats+i]<=(third?base[i]+10-(i==0?1:0):18),"Original attribute cap was exceeded");total+=s.ram()[stats+i]-base[i];}
    require(total==30,"Allocated attributes do not total thirty bonus points");
    auto final=std::array<unsigned char,6>{};std::copy(s.ram()+stats,s.ram()+stats+6,final.begin());
    unsigned empty=20;for(unsigned i=0;i<20;i++)if(s.ram()[roster+i*128]==0){empty=i;break;}
    require(empty<20,"Roster fixture is full");
    s.press(1,120);s.press(1,120);s.press(1,120);
    auto record=s.ram()+roster+empty*128;
    require(record[0]!=0&&std::equal(final.begin(),final.end(),record+0x0e),"Created character did not retain allocated attributes");
    auto save=static_cast<const unsigned char*>(s.core->savedata_ptr());
    require(std::equal(record,record+128,save+(third?0x0a:second?0x18:0x17)+empty*128),"Created character did not reach native save");
    if(third)for(unsigned sex=0;sex<2;sex++){
        Session cap(rom,state,true);cap.choose(0,sex);unsigned initial=cap.ram()[stats];
        for(unsigned i=0;i<11;i++)cap.press(16,30);
        require(cap.ram()[stats]==initial+(sex?10:9)&&cap.ram()[bonus]==30-(sex?10:9),"Original sex-adjusted strength cap changed");
        for(unsigned i=0;i<11;i++)cap.press(32,30);require(cap.ram()[bonus]==30,"Cap refund changed points");
        for(unsigned i=0;i<3;i++)cap.press(128,30);initial=cap.ram()[stats+3];
        for(unsigned i=0;i<11;i++)cap.press(16,30);
        require(cap.ram()[stats+3]==initial+(sex?9:10),"Original sex-adjusted vitality cap changed");
    }
    // OFF must be identical to the unmodified core, including RNG and timing.
    Session off(rom,state,false);Input input;auto reference=loadCoreBytes(rom,&input);reference->loadState(state.data(),state.size());
    for(unsigned f=0;f<(third?372u:248u);f++){unsigned key=f%124<4?1:0;off.run(key,1);input.value=key;unsigned count=35112;reference->runFor(off.pixels.data(),160,off.sound.data(),off.sound.size(),count);}
    std::vector<char> a(off.core->stateSize()),b(reference->stateSize());off.core->saveState(a.data());reference->saveState(b.data());require(a==b,"OFF changed original creation behavior");
    std::cout<<"PASS: all races/sexes, varied RNG, all speeds, spend/refund, state restore, creation/save and OFF identical to original\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}
