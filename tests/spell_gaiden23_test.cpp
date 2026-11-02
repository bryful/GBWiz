#include "spell_support.h"
#include <iostream>
void cartridge_set_rumble(unsigned){}
void require(bool b,const char* s){if(!b)throw std::runtime_error(s);}
unsigned checksum(const unsigned char* r){unsigned s=0;for(unsigned i=0;i<127;i++)s+=r[i]|(unsigned(r[i+1])<<8);return s&65535;}
struct Hook{SpellSupport* spells;gambatte::GB* core;};
bool write(void* data,unsigned pc,unsigned,unsigned a,unsigned v){auto& h=*static_cast<Hook*>(data);h.spells->observe(*h.core,pc,a,v);return h.spells->write(*h.core,pc,a,v);}
int wmain(int argc,wchar_t** argv)try{
 require(argc==3,"ROM and checkpoint required");auto rom=read(argv[1]),state=read(argv[2]);bool third=identifyGame(rom).id==GameId::Gaiden3;
 unsigned roster=third?0xba1:0x1095,work=third?0xb1f:0x1013,saveAt=third?0xa:0x18,mirror=third?0xe52:0xe5e,sumAt=third?0xe04:0xe1c;
 unsigned bank=third?0xac7:0xfbd,battle=third?0xf3:0x55d,counts=third?0x24f4:0x26ae,maximum=third?0xb05b:0xacf1,account=third?0x7082:0x6d18;
 unsigned load=third?0x1021:0x1175,store=third?0x105a:0x11a3,mask=third?0x3f:0x7f;
 SpellSupport support;support.attach(rom);require(support.supported()&&support.alignmentSupported(),"Profile rejected");
 for(unsigned a:{counts,maximum,third?0x1ae2au:0xb25u,third?0x1025u:0x1175u}){auto unknown=rom;unknown[a]^=1;require(!SpellSupport::recognizes(unknown),"Unknown revision accepted");}
 auto core=loadCoreBytes(rom,nullptr);require(!support.available(*core),"Boot accepted");require(core->loadState(state.data(),state.size()),"Fixture rejected");
 auto chars=support.characters(*core);require(chars.size()>=6,"Missing characters");std::array<gambatte::uint_least32_t,40000> audio{};
 for(const auto& character:chars){unsigned slot=character.slot;
  core->loadState(state.data(),state.size());auto r=static_cast<unsigned char*>(core->rambank0_ptr());auto s=static_cast<unsigned char*>(core->savedata_ptr());r[work]=0;support.resync(*core);
  auto rec=r+roster+128*slot;auto primary=s+saveAt+128*slot;rec[0x39]=primary[0x39]=static_cast<unsigned char>(~mask);
  std::fill(rec+0x3a,rec+0x41,0);std::fill(primary+0x3a,primary+0x41,0);std::fill(rec+0x41,rec+0x4f,3);std::fill(primary+0x41,primary+0x4f,3);
  if(third){rec[0x6a]=primary[0x6a]=0xf8;std::fill(rec+0x6b,rec+0x6e,0);std::fill(primary+0x6b,primary+0x6e,0);std::fill(rec+0x6e,rec+0x75,2);std::fill(primary+0x6e,primary+0x75,2);}
  std::vector<unsigned char> before(r,r+8192),battery(s,s+core->savedata_size());require(support.learn(*core,slot),"Learn failed");
  require(rec[0x39]==255&&std::all_of(rec+0x3a,rec+0x41,[](unsigned char v){return v==255;}),"Spell coverage wrong");require(std::all_of(rec+0x41,rec+0x4f,[](unsigned char v){return v==9;}),"MP missing");
  if(third)require(rec[0x6a]==255&&std::all_of(rec+0x6b,rec+0x6e,[](unsigned char v){return v==255;})&&std::all_of(rec+0x6e,rec+0x75,[](unsigned char v){return v==9;}),"Alchemy missing");
  for(unsigned i=0;i<8192;i++){unsigned at=roster+slot*128;bool allowed=i>=at+0x39&&i<at+0x4f;allowed|=third&&i>=at+0x6a&&i<at+0x75;if(!allowed)require(r[i]==before[i],"Unrelated live data changed");}
  require(!std::memcmp(rec+0x39,primary+0x39,22)&&!std::memcmp(primary,primary+mirror,128),"Save/backup mismatch");unsigned sum=checksum(primary);
  for(unsigned a:{sumAt+slot*2,sumAt+mirror+slot*2})require((s[a]|(unsigned(s[a+1])<<8))==sum,"Invalid checksum");
  for(unsigned i=0;i<battery.size();i++){unsigned at=saveAt+slot*128;bool allowed=i>=at+0x39&&i<at+0x4f;allowed|=third&&i>=at+0x6a&&i<at+0x75;allowed|=i>=at+mirror&&i<at+mirror+128;allowed|=i==sumAt+slot*2||i==sumAt+slot*2+1||i==sumAt+mirror+slot*2||i==sumAt+mirror+slot*2+1;if(!allowed)require(s[i]==battery[i],"Unrelated saved data changed");}
  std::vector<char> once(core->stateSize()),twice(core->stateSize());core->saveState(once.data());require(support.learn(*core,slot),"Repeat failed");core->saveState(twice.data());require(once==twice,"Non-idempotent");
  // Boot the unchanged game from edited SRAM and pass its mode prompt.
  auto fresh=loadCoreBytes(rom,nullptr);std::memcpy(fresh->savedata_ptr(),s,battery.size());
  struct Input:gambatte::InputGetter{unsigned key=0;unsigned operator()()override{return key;}} input;
  // Use the core input setter through a new core bound to the getter.
  fresh=loadCoreBytes(rom,&input);std::memcpy(fresh->savedata_ptr(),s,battery.size());
  for(unsigned f=0;f<1684;f++){
   input.key=f>=1200&&f<1208?1:f>=1468&&f<1476?128:f>=1496&&f<1504?1:0;
   unsigned n=35112;fresh->runFor(nullptr,160,audio.data(),audio.size(),n);
  }
  auto fr=static_cast<const unsigned char*>(fresh->rambank0_ptr())+roster+slot*128;
  require(!std::memcmp(fr+0x39,rec+0x39,22),"Normal boot lost spell/MP changes");
  if(third)require(!std::memcmp(fr+0x6a,rec+0x6a,11),"Normal boot lost alchemy changes");
  // Run the ROM's own maximum-MP accounting with the product hook.
  auto harness=rom;harness[0x100]=char(0xc3);harness[0x101]=0;harness[0x102]=char(0xc0);auto calc=loadCoreBytes(harness,nullptr);auto rr=static_cast<unsigned char*>(calc->rambank0_ptr());std::memcpy(rr+work,rec,128);
  unsigned bv=bank+0xc000;const unsigned char program[]={0xf3,0x31,0xf0,0xdf,0x3e,2,0xea,0,0x20,0xea,static_cast<unsigned char>(bv),static_cast<unsigned char>(bv>>8),0xcd,static_cast<unsigned char>(account),static_cast<unsigned char>(account>>8),0x3e,0xa5,0xea,0x80,0xc1,0x18,0xfe};std::memcpy(rr,program,sizeof(program));Hook hook{&support,calc.get()};calc->setWriteFilter(write,&hook);
  for(unsigned f=0;f<30&&rr[0x180]!=0xa5;f++){unsigned n=35112;calc->runFor(nullptr,160,audio.data(),audio.size(),n);}require(rr[0x180]==0xa5,"Native accounting stalled");require(std::all_of(rr+work+0x41,rr+work+0x4f,[](unsigned char v){return v==9;}),"Native maximum not 9");if(third)require(std::all_of(rr+work+0x6e,rr+work+0x75,[](unsigned char v){return v==9;}),"Alchemy maximum not 9");
  // Native load/save round trip, with a paused edit to an active workspace.
  auto live=loadCoreBytes(harness,nullptr);auto lr=static_cast<unsigned char*>(live->rambank0_ptr());std::memcpy(lr,before.data(),8192);std::memcpy(live->savedata_ptr(),battery.data(),battery.size());support.attach(rom);
  const unsigned char code[]={0xf3,0x31,0xf0,0xdf,0x3e,static_cast<unsigned char>(slot),0xcd,static_cast<unsigned char>(load),static_cast<unsigned char>(load>>8),0x3e,0xa5,0xea,0x80,0xc1,0xfa,0x81,0xc1,0xb7,0x28,0xfa,0x3e,static_cast<unsigned char>(slot),0xcd,static_cast<unsigned char>(store),static_cast<unsigned char>(store>>8),0x3e,0x5a,0xea,0x82,0xc1,0x18,0xfe};std::memcpy(lr,code,sizeof(code));lr[0x180]=lr[0x181]=lr[0x182]=0;lr[battle]=0;hook.core=live.get();live->setWriteFilter(write,&hook);
  for(unsigned f=0;f<30&&lr[0x180]!=0xa5;f++){unsigned n=35112;live->runFor(nullptr,160,audio.data(),audio.size(),n);}require(lr[0x180]==0xa5,"Native load stalled");require(support.learn(*live,slot),"Active workspace edit failed");lr[work+0x35]=3;lr[0x181]=1;
  for(unsigned f=0;f<30&&lr[0x182]!=0x5a;f++){unsigned n=35112;live->runFor(nullptr,160,audio.data(),audio.size(),n);}require(lr[0x182]==0x5a,"Native save stalled");auto saved=static_cast<const unsigned char*>(live->savedata_ptr())+saveAt+slot*128;require(std::all_of(saved+0x41,saved+0x4f,[](unsigned char v){return v==9;}),"Native save overwrote MP");require(saved[0x35]==3,"Native HP update lost");
 }
 core->loadState(state.data(),state.size());auto r=static_cast<unsigned char*>(core->rambank0_ptr());r[battle]=0x14;require(!support.learn(*core,0),"Battle accepted");r[battle]=0;require(!support.learn(*core,20),"Invalid slot accepted");
 std::cout<<"PASS: Gaiden II/III all characters, all spell schools, current/max MP9, native workspace save, checksums/backups and guards\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}
