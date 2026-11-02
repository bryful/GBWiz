#include "turbo_runner.h"
#include <iostream>
void cartridge_set_rumble(unsigned){}
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
struct Input:gambatte::InputGetter{unsigned value=0;unsigned operator()()override{return value;}};
unsigned number(const unsigned char* p){return p[0]|(unsigned(p[1])<<8)|(unsigned(p[2])<<16);}
void number(unsigned char* p,unsigned n){for(unsigned i=0;i<3;i++)p[i]=static_cast<unsigned char>(n>>(8*i));}
struct Session{
    Input input;std::unique_ptr<gambatte::GB> core;TurboRunner runner;
    std::array<gambatte::video_pixel_t,160*144> pixels{};std::array<gambatte::uint_least32_t,40000> sound{};
    Session(const std::vector<char>& loaded,const std::vector<char>& recognized):core(loadCoreBytes(loaded,&input)){runner.attach(*core,recognized,identifyGame(recognized));}
    unsigned char* ram(){return static_cast<unsigned char*>(core->rambank0_ptr());}
    void run(unsigned key,unsigned frames=1){for(unsigned f=0;f<frames;f++)runner.run(key,input.value,pixels,sound);}
};
int wmain(int argc,wchar_t** argv)try{
    require(argc==2||argc==3,"Expected DQ3 ROM and optional before-reward checkpoint");auto rom=read(argv[1]);require(RewardSupport::recognizes(rom),"DQ3 revision unrecognized");
    auto testRom=rom;testRom[0x100]=char(0xc3);testRom[0x101]=0;testRom[0x102]=char(0xcb);
    // Only the test harness skips message rendering. All calculation, division,
    // eligibility, native experience/gold addition and saturation code is original.
    testRom[0x5bcb8]=char(0xc9);testRom[0x5be1e]=char(0xc9);
    unsigned cases=0;
    for(unsigned party:{1u,2u,3u,4u})for(unsigned excluded:{0u,1u})for(unsigned factor:{1u,8u,16u,32u,64u,128u,256u})for(int speed:{2,3,4,6,8})for(unsigned goldFactor:{1u,factor,256u}){
        if(party==1&&excluded)continue;
        Session s(testRom,rom);auto r=s.ram();std::fill(r,r+0x8000,0);
        for(unsigned slot=0;slot<4;slot++){
            r[0x3380+slot*0x20]=slot<party?static_cast<unsigned char>(slot):255;
            r[0x559+slot*0x48]=slot<party?static_cast<unsigned char>(slot):255;
            r[0x561+slot*0x48]=99;
        }
        if(excluded){r[0x338d+(party-1)*0x20]=1;r[0x580+(party-1)*0x48]=1;}
        number(r+0x3020,101);r[0x3023]=37;number(r+0x4dc,500);
        const unsigned char code[]={0xf3,0x31,0xf0,0xcf,0x3e,0x16,0xea,0x00,0x21,0x3e,3,0xe0,0x70,0xcd,0x8f,0x76,0xcd,0x08,0x77,0x3e,0xa5,0xea,0x80,0xca,0x18,0xfe};
        std::copy(std::begin(code),std::end(code),r+0xb00);
        require(s.runner.rewards().set(factor,goldFactor)&&s.runner.speed(speed),"Settings rejected");
        for(unsigned f=0;f<20&&r[0xa80]!=0xa5;f++)s.run(0);
        require(r[0xa80]==0xa5,"Native reward routines did not return");
        unsigned share=101/(party-excluded);
        require(s.runner.rewards().experienceEvents()==1&&s.runner.rewards().baseExperience()==share,"Wrong base experience or duplicate event");
        require(s.runner.rewards().goldEvents()==1&&s.runner.rewards().baseGold()==37,"Wrong base gold or duplicate event");
        for(unsigned slot=0;slot<party;slot++)require(number(r+0x562+slot*0x48)==(excluded&&slot==party-1?0:share*factor),"Native experience award incorrect");
        require(number(r+0x4dc)==500+37*goldFactor,"Native gold award incorrect");++cases;
    }
    // Exercise 24-bit overflow and the native class caps, plus 999999 gold.
    for(unsigned job=0;job<9;job++){
        Session s(testRom,rom);auto r=s.ram();std::fill(r,r+0x8000,0);
        r[0x3380]=0;for(unsigned i=1;i<4;i++)r[0x3380+i*0x20]=255;
        r[0x55e]=static_cast<unsigned char>(job);r[0x561]=99;
        unsigned cap=static_cast<unsigned char>(rom[0x103ad+job*3])|(unsigned(static_cast<unsigned char>(rom[0x103ae+job*3]))<<8)|(unsigned(static_cast<unsigned char>(rom[0x103af+job*3]))<<16);
        number(r+0x562,cap-1);number(r+0x3020,65535);r[0x3023]=255;r[0x3024]=255;number(r+0x4dc,999990);
        const unsigned char code[]={0xf3,0x31,0xf0,0xcf,0x3e,0x16,0xea,0x00,0x21,0x3e,3,0xe0,0x70,0xcd,0x8f,0x76,0xcd,0x08,0x77,0x3e,0xa5,0xea,0x80,0xca,0x18,0xfe};
        std::copy(std::begin(code),std::end(code),r+0xb00);require(s.runner.rewards().set(256,256),"Cap settings failed");
        for(unsigned f=0;f<20&&r[0xa80]!=0xa5;f++)s.run(0);
        require(r[0xa80]==0xa5&&number(r+0x562)==cap,"Native class cap or overflow incorrect");require(number(r+0x4dc)==999999,"Gold cap incorrect");
    }
    // Merchant bonus keeps its native lottery and amount calculation. Force
    // only the lottery success in this private test ROM, then run the original
    // one-eighth-plus-one bonus routine after the ordinary battle gold award.
    for(unsigned factor:{1u,8u,16u,32u,64u,128u,256u}){
        auto merchantRom=testRom;merchantRom[0x087c]=char(0xc9);
        Session s(merchantRom,rom);auto r=s.ram();std::fill(r,r+0x8000,0);
        r[0x3380]=0;for(unsigned i=1;i<4;i++)r[0x3380+i*0x20]=255;
        r[0x55e]=6;r[0x561]=99;r[0x3023]=37;number(r+0x4dc,500);
        const unsigned char code[]={0xf3,0x31,0xf0,0xcf,0x3e,0x16,0xea,0,0x21,0x3e,3,0xe0,0x70,0xcd,8,0x77,0xcd,0x38,0x77,0x3e,0xa5,0xea,0x80,0xca,0x18,0xfe};
        std::copy(std::begin(code),std::end(code),r+0xb00);require(s.runner.rewards().set(1,factor),"Merchant settings failed");
        for(unsigned f=0;f<20&&r[0xa80]!=0xa5;f++)s.run(0);
        require(r[0xa80]==0xa5&&s.runner.rewards().goldEvents()==2&&number(r+0x4dc)==500+42*factor,"Merchant bonus scaled incorrectly");
        require(number(r+0x2a1)==5*factor,"Merchant displayed amount incorrect");
    }
    if(argc==3){
        auto checkpoint=read(argv[2]);
        for(unsigned factor:{1u,8u,16u,32u,64u,128u,256u})for(int speed:{2,3,4,6,8}){
            Session s(rom,rom);require(s.core->loadState(checkpoint.data(),checkpoint.size()),"Battle checkpoint rejected");auto r=s.ram();unsigned gold=number(r+0x4dc),exp=number(r+0x562);r[0x561]=99;
            require(s.runner.rewards().set(factor,factor)&&s.runner.speed(speed),"Battle settings failed");
            Session reference(rom,rom);require(reference.core->loadState(checkpoint.data(),checkpoint.size()),"Reference checkpoint failed");reference.ram()[0x561]=99;reference.core->setWriteFilter(nullptr,nullptr);
            for(unsigned f=0;f<3000&&s.runner.rewards().goldEvents()==0;f++){
                unsigned key=f%32<4?1:0;s.run(key);
                if(factor==1&&speed==2){reference.input.value=key;unsigned samples=35112;reference.core->runFor(reference.pixels.data(),160,reference.sound.data(),reference.sound.size(),samples);}
            }
            if(factor==1&&speed==2){std::vector<char> expected(reference.core->stateSize());reference.core->saveState(expected.data());require(s.runner.saveState()==expected,"1x altered original emulation");}
            auto snapshot=s.runner.saveState();require(s.runner.rewards().set(8,16)&&s.runner.loadState(snapshot)&&s.runner.rewards().experience()==8&&s.runner.rewards().gold()==16,"State load replaced current settings");
            require(s.runner.rewards().experienceEvents()==1&&s.runner.rewards().goldEvents()>=1,"Real battle reward path not reached");
            require(number(r+0x562)==exp+16*factor,"Real battle experience incorrect");require(number(r+0x4dc)==gold+8*factor,"Real battle gold incorrect");
        }
    }
    for(unsigned at:{0x5b68fu,0x10351u,0x1853u,0x42a1eu}){auto unknown=rom;unknown[at]^=1;require(!RewardSupport::recognizes(unknown),"Unknown revision accepted");}
    std::cout<<"PASS: "<<cases<<" original reward cases, all factors/speeds/party sizes, dead exclusion, 9 class caps and 24-bit overflow, gold cap, unknown revision rejection"<<(argc==3?", real victory path":"")<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
