#include "turbo_runner.h"
#include <iostream>
void cartridge_set_rumble(unsigned){}
void require(bool ok,const char* text){if(!ok)throw std::runtime_error(text);}
struct Input:gambatte::InputGetter{unsigned value=0;unsigned operator()()override{return value;}};
uint32_t number(const unsigned char* p){return uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);}
int wmain(int argc,wchar_t** argv)try{
    require(argc==3,"Expected Gaiden 1 ROM and before-victory checkpoint");auto rom=read(argv[1]),state=read(argv[2]);
    require(RewardSupport::recognizes(rom),"Reward revision not recognized");
    auto changed=rom;changed[0x12aa9]^=1;require(!RewardSupport::recognizes(changed),"Unknown reward revision accepted");
    std::array<unsigned char,0x2000> ordinary{};bool haveOrdinary=false;
    for(unsigned factor:{1u,8u,16u,32u,64u,128u,256u}){
        for(bool experienceOnly:{true,false}){
            Input input;auto core=loadCoreBytes(rom,&input);require(core->loadState(state.data(),state.size()),"Checkpoint rejected");
            auto ram=static_cast<unsigned char*>(core->rambank0_ptr());std::array<unsigned char,0x2000> before{};std::copy(ram,ram+0x2000,before.begin());
            TurboRunner runner;runner.attach(*core,rom,identifyGame(rom));
            Input ordinaryInput;auto reference=loadCoreBytes(rom,&ordinaryInput);require(reference->loadState(state.data(),state.size()),"Reference checkpoint rejected");
            unsigned exp=experienceOnly?factor:1,gold=experienceOnly?1:factor;
            require(runner.rewards().set(exp,gold),"Multiplier rejected");require(!runner.rewards().set(2,1),"Invalid multiplier accepted");
            std::array<gambatte::video_pixel_t,160*144> pixels{};std::array<gambatte::uint_least32_t,40000> sound{};
            for(unsigned f=0;f<1500;f++){
                unsigned key=f>=60&&(f-60)%128<8?1:0;runner.run(key,input.value,pixels,sound);
                if(factor==1){ordinaryInput.value=key;unsigned count=35112;reference->runFor(pixels.data(),160,sound.data(),sound.size(),count);}
            }
            if(factor==1){std::vector<char> actual(core->stateSize()),expected(reference->stateSize());core->saveState(actual.data());reference->saveState(expected.data());require(actual==expected,"1x reward settings changed the original battle");}
            auto& rewards=runner.rewards();require(rewards.experienceEvents()==1&&rewards.goldEvents()==1,"Battle reward was missed or repeated");
            unsigned recipients=0;
            for(unsigned slot=0;slot<20;slot++){
                size_t record=0x1186+slot*128;auto a=before.data()+record,b=ram+record;
                uint32_t oldExp=number(a+0x2d),newExp=number(b+0x2d),oldGold=number(a+0x19),newGold=number(b+0x19);
                if(oldExp==newExp&&oldGold==newGold)continue;
                require(newExp==std::min<uint64_t>(uint64_t(oldExp)+uint64_t(rewards.baseExperience())*exp,UINT32_MAX),"Experience gain has wrong multiplier");
                require(newGold==std::min<uint64_t>(uint64_t(oldGold)+uint64_t(rewards.baseGold())*gold,UINT32_MAX),"Gold gain has wrong multiplier");
                if(haveOrdinary)for(unsigned byte=0;byte<128;byte++)if(!(byte>=0x19&&byte<0x1d)&&!(byte>=0x2d&&byte<0x31))require(ordinary[record+byte]==b[byte],"Reward changed an unrelated character field");
                ++recipients;
            }
            require(recipients==3,"Dead/ineligible party members received rewards");
            if(!haveOrdinary){std::copy(ram,ram+0x2000,ordinary.begin());haveOrdinary=true;}
        }
    }
    // Exercise overflow and caller guards without depending on a huge battle.
    auto core=loadCoreBytes(rom,nullptr);RewardSupport rewards;rewards.attach(rom);rewards.set(256,256);
    auto ram=static_cast<unsigned char*>(core->rambank0_ptr());ram[0x10ae]=4;ram[0x1ff0]=0xaf;ram[0x1ff1]=0x6a;
    ram[0x1068]=ram[0x1069]=ram[0x106a]=255;
    require(!rewards.write(*core,0x310,0xdff0,0xd06b,255)&&number(ram+0x1068)==UINT32_MAX,"Reward overflow wrapped");
    ram[0x1ff0]=0;require(rewards.write(*core,0x310,0xdff0,0xd06b,1),"Unrelated store was modified");
    std::cout<<"PASS: real victory, all reward factors independently, three eligible recipients, unrelated fields and saturation\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}
