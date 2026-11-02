#include "turbo_runner.h"
#include <iostream>
void cartridge_set_rumble(unsigned){}
void require(bool b,const char* s){if(!b)throw std::runtime_error(s);}
uint64_t bcd(const unsigned char* p){uint64_t n=0;for(unsigned i=0;i<6;i++){require((p[i]>>4)<10&&(p[i]&15)<10,"Invalid BCD");n=n*100+(p[i]>>4)*10+(p[i]&15);}return n;}
void store(unsigned char* p,uint64_t n){for(int i=5;i>=0;i--){unsigned v=unsigned(n%100);p[i]=static_cast<unsigned char>(v/10*16+v%10);n/=100;}}
struct Input:gambatte::InputGetter{unsigned value=0;unsigned operator()()override{return value;}};
int wmain(int argc,wchar_t** argv)try{
 require(argc==3,"ROM and pre-award checkpoint required");auto rom=read(argv[1]),state=read(argv[2]);require(RewardSupport::recognizes(rom),"Profile rejected");bool second=identifyGame(rom).id==GameId::Wizardry2,third=identifyGame(rom).id==GameId::Wizardry3;unsigned slots=second?7:6,base=third?117:(second?66:156),pc=third?0x6f88:(second?0x7122:0x6ce5),caller=third?0x6848:(second?0x69e6:0x6596);auto active=[&](unsigned i){return !second||i!=3;};auto unknown=rom;unknown[pc-3]^=1;require(!RewardSupport::recognizes(unknown),"Unknown revision accepted");
 std::array<gambatte::video_pixel_t,160*144> pixels{};std::array<gambatte::uint_least32_t,40000> sound{};std::array<unsigned char,32768> ordinary{};bool first=true;
 for(unsigned factor:{1u,8u,16u,32u,64u,128u,256u})for(unsigned speed:{2u,3u,4u,6u,8u}){
  Input input,refInput;auto core=loadCoreBytes(rom,&input),reference=loadCoreBytes(rom,&refInput);require(core->loadState(state.data(),state.size())&&reference->loadState(state.data(),state.size()),"State rejected");auto r=static_cast<unsigned char*>(core->rambank0_ptr());std::array<uint64_t,7> initial{};for(unsigned i=0;i<slots;i++)initial[i]=bcd(r+0x10a+110*i+0x17);
  TurboRunner runner;runner.attach(*core,rom,identifyGame(rom));auto& reward=runner.rewards();require(reward.experienceSupported()&&reward.goldSupported(),"Capabilities wrong");require(reward.set(factor,1)&&!reward.set(factor,2)&&reward.experience()==factor,"Settings wrong");runner.speed(speed);
  bool checkedAward=false;
  for(unsigned f=0;f<1200;f++){
   unsigned key=f%64<8?1:0;runner.run(key,input.value,pixels,sound);
   if(factor==1&&speed==2){refInput.value=key;unsigned samples=35112;reference->runFor(pixels.data(),160,sound.data(),sound.size(),samples);}
   if(!checkedAward&&reward.experienceEvents()){
    bool complete=true;for(unsigned i=0;i<slots;i++)if(bcd(r+0x121+110*i)!=initial[i]+(active(i)?base*factor:0))complete=false;
    if(complete){
     if(first)std::copy(r,r+32768,ordinary.begin());
     else for(unsigned i=0;i<slots;i++)for(unsigned j=0;j<110;j++)if(j<0x17||j>=0x1d)require(r[0x10a+110*i+j]==ordinary[0x10a+110*i+j],"XP changed data during native addition");
     checkedAward=true;
    }
   }
  }
  require(checkedAward&&reward.experienceEvents()==1&&reward.baseExperience()==base&&reward.goldEvents()==1,"Battle reward missed or repeated");auto save=static_cast<const unsigned char*>(core->savedata_ptr());
  for(unsigned i=0;i<slots;i++){auto rec=r+0x10a+110*i;require(bcd(rec+0x17)==initial[i]+(active(i)?base*factor:0),"Wrong live gain");require(std::equal(rec,rec+110,save+0x2b+110*i),"Native save differs");}
  if(factor==1&&speed==2){std::vector<char> expected(reference->stateSize());reference->saveState(expected.data());require(runner.saveState()==expected,"1x changed emulation");}
  first=false;auto snapshot=runner.saveState();require(runner.loadState(snapshot)&&reward.experience()==factor,"Restore lost multiplier");
 }
 // Native BCD addition saturates character XP, and status >=4 is excluded.
 {Input input;auto core=loadCoreBytes(rom,&input);core->loadState(state.data(),state.size());auto r=static_cast<unsigned char*>(core->rambank0_ptr());store(r+0x121,999999999998ULL);r[0x10a+110+0x23]=4;uint64_t excluded=bcd(r+0x10a+110+0x17);TurboRunner runner;runner.attach(*core,rom,identifyGame(rom));runner.rewards().set(256,1);for(unsigned f=0;f<1200;f++){runner.run(f%64<8?1:0,input.value,pixels,sound);}require(bcd(r+0x121)==999999999999ULL,"Native addition overflowed");require(bcd(r+0x10a+110+0x17)==excluded,"Eligibility bypassed");}
 auto core=loadCoreBytes(rom,nullptr);auto r=static_cast<unsigned char*>(core->rambank0_ptr());auto h=static_cast<unsigned char*>(core->zeropage_ptr());RewardSupport reward;reward.attach(rom);reward.set(256,1);r[0x49]=1;r[0xff0]=caller&255;r[0xff1]=caller>>8;h[-16]=1;store(h+0x38,999999999999ULL);
 require(!reward.write(*core,pc,0xcff0,0xffbd,0x99)&&bcd(h+0x38)==999999999999ULL,"Share overflowed");
 r[0xff0]=0;require(reward.write(*core,pc,0xcff0,0xffbd,0),"Wrong caller accepted");r[0xff0]=caller&255;h[-16]=2;require(reward.write(*core,pc,0xcff0,0xffbd,0),"Wrong WRAM bank accepted");h[-16]=1;r[0x49]=2;require(reward.write(*core,pc,0xcff0,0xffbd,0),"Wrong ROM bank accepted");r[0x49]=1;require(reward.write(*core,pc,0xcff0,0xffbd,0xfa),"Invalid BCD accepted");
 std::cout<<"PASS: GBC Wizardry I/II/III all factors/speeds, original 1x equality, native live/SRAM, eligibility, BCD saturation and guards\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}
