#include "turbo_runner.h"
#include <iostream>
void cartridge_set_rumble(unsigned){}
void require(bool ok,const char* text){if(!ok)throw std::runtime_error(text);}
uint32_t number(const unsigned char* p){return uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);}
struct Input:gambatte::InputGetter{unsigned value=0;unsigned operator()()override{return value;}};
int wmain(int argc,wchar_t** argv)try{
    require(argc==3,"Gaiden II/III ROM and before-experience state required");auto rom=read(argv[1]),state=read(argv[2]);require(RewardSupport::recognizes(rom),"Profile rejected");
    bool gaiden3=identifyGame(rom).id==GameId::Gaiden3;
    unsigned roster=gaiden3?0xba1:0x1095,primary=gaiden3?0x0a:0x18,bank=gaiden3?0xac7:0xfbd;
    unsigned base=gaiden3?80:125,pc=gaiden3?0x2a6:0x323,last=gaiden3?0xca78:0xcf77,caller=gaiden3?0x6f89:0x6afc;
    auto changed=rom;changed[gaiden3?0x12f83:0x12af6]^=1;require(!RewardSupport::recognizes(changed),"Unknown revision accepted");
    std::array<unsigned char,8192> ordinary{};bool haveOrdinary=false;
    std::array<gambatte::video_pixel_t,160*144> pixels{};std::array<gambatte::uint_least32_t,40000> sound{};
    for(unsigned factor:{1u,8u,16u,32u,64u,128u,256u})for(unsigned speed:{2u,3u,4u,6u,8u}){
        Input input,referenceInput;auto core=loadCoreBytes(rom,&input),reference=loadCoreBytes(rom,&referenceInput);
        require(core->loadState(state.data(),state.size())&&reference->loadState(state.data(),state.size()),"State rejected");
        TurboRunner runner;runner.attach(*core,rom,identifyGame(rom));auto& rewards=runner.rewards();
        require(rewards.experienceSupported()&&rewards.goldSupported(),"Capabilities incorrect");require(rewards.set(factor,1),"Multiplier rejected");
        require(!rewards.set(factor,2)&&!rewards.set(2,1)&&rewards.experience()==factor&&rewards.gold()==1,"Invalid/unsupported settings changed rewards");
        require(runner.speed(speed),"Speed rejected");auto ram=static_cast<unsigned char*>(core->rambank0_ptr());
        for(unsigned f=0;f<2000;f++){
            unsigned key=f%128<8?1:0;runner.run(key,input.value,pixels,sound);
            if(factor==1&&speed==2){referenceInput.value=key;unsigned samples=35112;reference->runFor(pixels.data(),160,sound.data(),sound.size(),samples);}
            if(rewards.experienceEvents()&&ram[bank]==3)break;
        }
        require(rewards.experienceEvents()==1&&rewards.goldEvents()==1&&rewards.baseExperience()==base,"Reward missed or repeated");
        require(ram[bank]==3,"Battle did not finish");
        auto save=static_cast<const unsigned char*>(core->savedata_ptr());
        for(unsigned slot=0;slot<20;slot++){
            auto record=ram+roster+slot*128;unsigned expected=slot<6?base*factor:number(ordinary.data()+roster+slot*128+0x2d);
            require(number(record+0x2d)==expected,"Wrong live experience");require(number(save+primary+slot*128+0x2d)==expected,"Experience not saved");
            if(haveOrdinary)for(unsigned b=0;b<128;b++)if(!(b>=0x19&&b<0x1d)&&!(b>=0x2d&&b<0x31))require(record[b]==ordinary[roster+slot*128+b],"Unrelated character data changed");
        }
        if(factor==1&&speed==2){std::vector<char> expected(reference->stateSize());reference->saveState(expected.data());require(runner.saveState()==expected,"1x changed original emulation");}
        if(!haveOrdinary){std::copy(ram,ram+8192,ordinary.begin());haveOrdinary=true;}
        auto snapshot=runner.saveState();require(runner.loadState(snapshot)&&rewards.experience()==factor,"State restore lost reward settings");
    }
    // Original eligibility still excludes status >=3, even after scaling.
    {
        Input input;auto core=loadCoreBytes(rom,&input);core->loadState(state.data(),state.size());auto ram=static_cast<unsigned char*>(core->rambank0_ptr());ram[roster+11]=3;
        TurboRunner runner;runner.attach(*core,rom,identifyGame(rom));runner.rewards().set(8,1);
        for(unsigned f=0;f<2000;f++){runner.run(f%128<8?1:0,input.value,pixels,sound);if(runner.rewards().experienceEvents()&&ram[bank]==3)break;}
        require(number(ram+roster+0x2d)==0&&number(ram+roster+128+0x2d)==base*8,"Eligibility check bypassed");
    }
    auto core=loadCoreBytes(rom,nullptr);auto ram=static_cast<unsigned char*>(core->rambank0_ptr());RewardSupport rewards;rewards.attach(rom);rewards.set(256,1);
    ram[bank]=4;ram[0x1ff0]=caller&255;ram[0x1ff1]=caller>>8;unsigned scratch=last-0xc000-3;ram[scratch]=ram[scratch+1]=ram[scratch+2]=255;
    require(!rewards.write(*core,pc,0xdff0,last,255)&&number(ram+scratch)==UINT32_MAX,"Scaled reward overflowed");
    ram[0x1ff0]=0;require(rewards.write(*core,pc,0xdff0,last,1),"Wrong caller intercepted");ram[0x1ff0]=caller&255;ram[bank]=3;require(rewards.write(*core,pc,0xdff0,last,1),"Wrong bank intercepted");
    ram[bank]=4;require(rewards.write(*core,pc,0xdff0,gaiden3?0xcadc:0xcfd2,1),"Gold intercepted");
    std::cout<<"PASS: Gaiden II/III all factors and speeds, live/SRAM gains, 1x original equality, eligibility, guards, overflow, gold capabilities\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}
