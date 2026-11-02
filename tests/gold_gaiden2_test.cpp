#include "turbo_runner.h"
#include <iostream>
void cartridge_set_rumble(unsigned){}
void require(bool v,const char* message){if(!v)throw std::runtime_error(message);}
uint32_t number(const unsigned char* p){return uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);}
struct Input:gambatte::InputGetter{unsigned value=0;unsigned operator()()override{return value;}};
int wmain(int argc,wchar_t** argv)try{
    require(argc==3,"Gaiden II/III ROM and before-experience checkpoint required");auto rom=read(argv[1]),state=read(argv[2]);
    bool gaiden3=identifyGame(rom).id==GameId::Gaiden3;
    unsigned roster=gaiden3?0xba1:0x1095,primary=gaiden3?0x0a:0x18,bank=gaiden3?0xac7:0xfbd,baseExp=gaiden3?80:125;
    unsigned pc=gaiden3?0x2a6:0x323,last=gaiden3?0xcadc:0xcfd2,caller=gaiden3?0x70ac:0x6c1f,scratch=last-0xc000-3;
    auto changed=rom;changed[gaiden3?0x130a3:0x12c16]^=1;require(!RewardSupport::recognizes(changed),"Unknown gold revision accepted");
    std::array<gambatte::video_pixel_t,160*144> pixels{};std::array<gambatte::uint_least32_t,40000> sound{};
    for(unsigned exp:{1u,256u})for(unsigned speed:{2u,3u,4u,6u,8u}){
        std::array<unsigned char,20*128> ordinary{};unsigned ordinaryGold=0;
        for(unsigned gold:{1u,8u,16u,32u,64u,128u,256u}){
            Input input;auto core=loadCoreBytes(rom,&input);require(core->loadState(state.data(),state.size()),"Checkpoint rejected");
            auto ram=static_cast<unsigned char*>(core->rambank0_ptr());TurboRunner runner;runner.attach(*core,rom,identifyGame(rom));auto& rewards=runner.rewards();
            require(rewards.experienceSupported()&&rewards.goldSupported(),"Gold unavailable");require(rewards.set(exp,gold)&&!rewards.set(exp,2)&&rewards.gold()==gold,"Settings rejected or damaged");require(runner.speed(speed),"Speed rejected");
            for(unsigned frame=0;frame<2000;frame++){runner.run(frame%128<8?1:0,input.value,pixels,sound);if(rewards.goldEvents()&&ram[bank]==3)break;}
            require(rewards.experienceEvents()==1&&rewards.goldEvents()==1&&ram[bank]==3,"Battle rewards missing/repeated");
            if(gold==1){ordinaryGold=rewards.baseGold();std::copy(ram+roster,ram+roster+ordinary.size(),ordinary.begin());}
            require(ordinaryGold>0&&rewards.baseGold()==ordinaryGold,"Gold scaling changed the preceding loot roll");
            auto save=static_cast<const unsigned char*>(core->savedata_ptr());
            for(unsigned slot=0;slot<20;slot++){
                auto r=ram+roster+128*slot;auto reference=ordinary.data()+128*slot;
                uint32_t expected=slot<6?100+ordinaryGold*gold:number(reference+0x19);
                require(number(r+0x19)==expected&&number(save+primary+slot*128+0x19)==expected,"Wrong live/saved gold");
                require(number(r+0x2d)==(slot<6?baseExp*exp:number(reference+0x2d)),"Gold multiplier changed experience");
                for(unsigned b=0;b<128;b++)if(b<0x19||b>=0x1d)require(r[b]==reference[b],"Gold changed another character field");
            }
            auto snapshot=runner.saveState();require(runner.loadState(snapshot)&&rewards.gold()==gold&&rewards.experience()==exp,"Restore lost settings");
            std::cout<<"exp="<<exp<<" gold="<<gold<<" speed="<<speed<<" base="<<ordinaryGold<<" OK\n";
        }
    }
    for(unsigned status:{1u,3u}){
        Input input;auto core=loadCoreBytes(rom,&input);core->loadState(state.data(),state.size());auto ram=static_cast<unsigned char*>(core->rambank0_ptr());ram[roster+11]=status;
        TurboRunner runner;runner.attach(*core,rom,identifyGame(rom));runner.rewards().set(8,8);
        for(unsigned f=0;f<2000;f++){runner.run(f%128<8?1:0,input.value,pixels,sound);if(runner.rewards().goldEvents()&&ram[bank]==3)break;}
        require(number(ram+roster+0x19)==100,"Ineligible member received gold");
        require(number(ram+roster+0x2d)==(status==1?baseExp*8:0),"Gold eligibility changed experience eligibility");
        require(number(ram+roster+128+0x19)==100+runner.rewards().baseGold()*8,"Eligible member missed gold");
    }
    {
        Input input;auto core=loadCoreBytes(rom,&input);core->loadState(state.data(),state.size());auto ram=static_cast<unsigned char*>(core->rambank0_ptr());
        for(unsigned i=0;i<4;i++)ram[roster+0x19+i]=static_cast<unsigned char>((uint32_t(UINT32_MAX-3))>>(i*8));
        TurboRunner runner;runner.attach(*core,rom,identifyGame(rom));runner.rewards().set(1,256);
        for(unsigned f=0;f<2000;f++){runner.run(f%128<8?1:0,input.value,pixels,sound);if(runner.rewards().goldEvents()&&ram[bank]==3)break;}
        require(number(ram+roster+0x19)==UINT32_MAX&&number(static_cast<const unsigned char*>(core->savedata_ptr())+primary+0x19)==UINT32_MAX,"Original addition overflowed");
    }
    auto core=loadCoreBytes(rom,nullptr);auto ram=static_cast<unsigned char*>(core->rambank0_ptr());RewardSupport rewards;rewards.attach(rom);rewards.set(1,256);
    ram[bank]=4;ram[0x1ff0]=caller&255;ram[0x1ff1]=caller>>8;ram[scratch]=ram[scratch+1]=ram[scratch+2]=255;
    require(!rewards.write(*core,pc,0xdff0,last,255)&&number(ram+scratch)==UINT32_MAX,"Scaled scratch overflowed");
    ram[0x1ff0]=0;require(rewards.write(*core,pc,0xdff0,last,1),"Unrelated caller changed");ram[0x1ff0]=caller&255;ram[bank]=3;require(rewards.write(*core,pc,0xdff0,last,1),"Wrong bank changed");
    std::cout<<"PASS: gold factors/speeds, experience combination, SRAM, eligibility, saturation and guards\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}
