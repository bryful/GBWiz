#include "turbo_runner.h"
#include <iostream>
void cartridge_set_rumble(unsigned){}
void require(bool b,const char* s){if(!b)throw std::runtime_error(s);}
uint64_t bcd(const unsigned char* p){uint64_t n=0;for(unsigned i=0;i<6;i++){require((p[i]>>4)<10&&(p[i]&15)<10,"Invalid BCD");n=n*100+(p[i]>>4)*10+(p[i]&15);}return n;}
void store(unsigned char* p,uint64_t n){for(int i=5;i>=0;i--){unsigned v=unsigned(n%100);p[i]=static_cast<unsigned char>(v/10*16+v%10);n/=100;}}
struct Input:gambatte::InputGetter{unsigned value=0;unsigned operator()()override{return value;}};
int wmain(int argc,wchar_t** argv)try{
 require(argc==4,"ROM, pre-experience and pre-gold states required");auto rom=read(argv[1]),state=read(argv[2]),goldState=read(argv[3]);require(RewardSupport::recognizes(rom),"Profile rejected");bool second=identifyGame(rom).id==GameId::Wizardry2,third=identifyGame(rom).id==GameId::Wizardry3;unsigned slots=second?7:6,pc=third?0x5f8d:(second?0x6027:0x5f89),battleReturn=third?0x6893:(second?0x6a31:0x65e1);auto active=[&](unsigned i){return !second||i!=3;};auto unknown=rom;unknown[0x14000+pc-1]^=1;require(!RewardSupport::recognizes(unknown),"Unknown gold revision accepted");
 std::array<gambatte::video_pixel_t,160*144> pixels{};std::array<gambatte::uint_least32_t,40000> sound{};
 for(unsigned exp:{1u,256u})for(unsigned speed:{2u,3u,4u,6u,8u}){
  std::array<unsigned char,7*110> ordinary{};uint64_t ordinaryBase=0;
  for(unsigned factor:{1u,8u,16u,32u,64u,128u,256u}){
   Input input;auto core=loadCoreBytes(rom,&input);require(core->loadState(state.data(),state.size()),"State rejected");auto r=static_cast<unsigned char*>(core->rambank0_ptr());std::array<uint64_t,7> initial{};for(unsigned i=0;i<slots;i++)initial[i]=bcd(r+0x10a+110*i+0x11);
   TurboRunner runner;runner.attach(*core,rom,identifyGame(rom));auto& reward=runner.rewards();require(reward.goldSupported()&&reward.set(exp,factor),"Gold setting rejected");require(!reward.set(exp,2)&&reward.gold()==factor,"Invalid setting changed multiplier");runner.speed(speed);
   bool checkedAward=false;
   for(unsigned f=0;f<1200;f++){
    runner.run(f%64<8?(runner.rewards().goldEvents()?2:1):0,input.value,pixels,sound);
    if(!checkedAward&&reward.goldEvents()){
     bool complete=true;for(unsigned i=0;i<slots;i++)if(bcd(r+0x11b+110*i)!=initial[i]+(active(i)?reward.baseGold()*factor:0))complete=false;
     if(complete){
      if(factor==1)std::copy(r+0x10a,r+0x10a+slots*110,ordinary.begin());
      else for(unsigned i=0;i<slots;i++)for(unsigned j=0;j<110;j++)if(j<0x11||j>=0x17)require(r[0x10a+110*i+j]==ordinary[i*110+j],"Gold changed data during native addition");
      checkedAward=true;
     }
    }
   }
   require(checkedAward,"Native award did not complete");
   require(reward.experienceEvents()==1&&reward.goldEvents()==1,"Reward missed or repeated");if(factor==1){ordinaryBase=reward.baseGold();}require(reward.baseGold()==ordinaryBase,"Gold multiplier changed original base");if(exp==1&&speed==2)require(ordinaryBase==(second?9u:15u),"Unexpected baseline");
   auto save=static_cast<const unsigned char*>(core->savedata_ptr());for(unsigned i=0;i<slots;i++){auto rec=r+0x10a+110*i;require(bcd(rec+0x11)==initial[i]+(active(i)?ordinaryBase*factor:0),"Wrong gold gain");require(std::equal(rec,rec+110,save+0x2b+110*i),"Gold not saved");}
   auto snapshot=runner.saveState();require(runner.loadState(snapshot)&&reward.gold()==factor&&reward.experience()==exp,"Restore lost multipliers");
  }
 }
 // Follow the native gold eligibility (battle status <2) and saturating add.
 {Input input;auto core=loadCoreBytes(rom,&input);core->loadState(goldState.data(),goldState.size());auto r=static_cast<unsigned char*>(core->rambank0_ptr());store(r+0x11b,999999999998ULL);r[0x1544]=1;r[0x1545]=2;uint64_t eligible=bcd(r+0x11b+110),excluded=bcd(r+0x11b+220);TurboRunner runner;runner.attach(*core,rom,identifyGame(rom));runner.rewards().set(1,256);for(unsigned f=0;f<1200;f++)runner.run(f%64<8?(runner.rewards().goldEvents()?2:1):0,input.value,pixels,sound);require(runner.rewards().goldEvents()==1,"Eligibility run missed gold");require(bcd(r+0x11b)==999999999999ULL,"Native gold add overflowed");require(bcd(r+0x11b+110)==eligible+runner.rewards().baseGold()*256,"Status 1 incorrectly excluded");require(bcd(r+0x11b+220)==excluded,"Status 2 received gold");auto save=static_cast<const unsigned char*>(core->savedata_ptr());require(bcd(save+0x3c)==999999999999ULL,"Saturated gold not saved");}
 auto core=loadCoreBytes(rom,nullptr);auto r=static_cast<unsigned char*>(core->rambank0_ptr());auto h=static_cast<unsigned char*>(core->zeropage_ptr());RewardSupport reward;reward.attach(rom);reward.set(1,256);r[0x49]=6;h[-16]=1;unsigned sp=0xcfef,stack=sp-0xc000;r[stack+2]=0x0f;r[stack+3]=0;r[stack+5]=1;r[stack+6]=battleReturn&255;r[stack+7]=battleReturn>>8;store(h+0x38,999999999999ULL);
 require(!reward.write(*core,pc,sp,0xffbd,0x99)&&bcd(h+0x38)==999999999999ULL,"Gold share overflowed");unsigned events=reward.goldEvents();
 r[stack+2]=0;require(reward.write(*core,pc,sp,0xffbd,0),"Wrong far-call return accepted");r[stack+2]=0x0f;r[stack+6]=0;require(reward.write(*core,pc,sp,0xffbd,0),"Nonbattle caller accepted");r[stack+6]=battleReturn&255;r[stack+5]=2;require(reward.write(*core,pc,sp,0xffbd,0),"Wrong caller bank accepted");r[stack+5]=1;h[-16]=2;require(reward.write(*core,pc,sp,0xffbd,0),"Wrong WRAM bank accepted");h[-16]=1;r[0x49]=1;require(reward.write(*core,pc,sp,0xffbd,0),"Wrong ROM bank accepted");r[0x49]=6;require(reward.write(*core,pc,sp,0xffbd,0xfa),"Invalid BCD accepted");require(reward.goldEvents()==events,"Rejected write changed reward count");
 std::cout<<"PASS: GBC Wizardry I/II/III gold all factors/speeds, XP combination, native SRAM, eligibility, saturation and guards\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}
