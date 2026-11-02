#include "spell_support.h"
#include <iostream>
void cartridge_set_rumble(unsigned){}
void require(bool b,const char* message){if(!b)throw std::runtime_error(message);}
int wmain(int argc,wchar_t** argv)try{
 require(argc==3,"ROM and registered-character checkpoint required");auto rom=read(argv[1]),state=read(argv[2]);auto id=identifyGame(rom).id;bool second=id==GameId::Wizardry2,third=id==GameId::Wizardry3;
 unsigned record=third?0x273c:second?0x276e:0x2776,flags=second||third?0xf143:0xf141,counts=second?0xa979:0xa988;
 unsigned mp=second||third?0xf5bf:0xf5bd,save=third?0x2bc9:second?0x2bfe:0x2c03,sumAddress=third?0x2af6:second?0x2b2b:0x2b30;
 unsigned loader=third?0x2c02:second?0x2c37:0x2c3c;SpellSupport spells;spells.attach(rom);
 require(spells.supported()&&spells.learnsMaximumMp()&&spells.alignmentSupported(),"Capabilities wrong");
 for(unsigned address:{record,flags+3,counts+2,mp+6,save,sumAddress,0x8079u}){auto unknown=rom;unknown[address]^=1;require(!SpellSupport::recognizes(unknown),"Unverified revision accepted");}
 const std::array<unsigned,9> copies=third?std::array<unsigned,9>{0x1945b,0x19909,0x1b194,0x1b308,0x1ba2a,0x1c8ec,0x1cf1a,0x22693,0x227db}:
  second?std::array<unsigned,9>{0x194b6,0x199a3,0x1b4ea,0x1b65e,0x1bd80,0x1c8fb,0x1cf29,0x2267b,0x227c3}:
  std::array<unsigned,9>{0x19464,0x19912,0x1b211,0x1b385,0x1baa7,0x1c8ec,0x1cf1a,0x22603,0x2274b};
 for(unsigned offset:copies){auto unknown=rom;unknown[offset]^=1;require(!SpellSupport::recognizes(unknown),"Unknown facility transfer accepted");}
 auto core=loadCoreBytes(rom,nullptr);require(!spells.available(*core)&&!spells.learn(*core,0),"Boot accepted");require(core->loadState(state.data(),state.size()),"Checkpoint failed");auto chars=spells.characters(*core);require(chars.size()>=6,"Missing characters");
 require(chars[0].name==u8"せんし1"&&chars[1].name==u8"せんし2"&&chars[5].name==u8"まほうつかい1","Registered names decoded incorrectly");
 auto namesRam=static_cast<unsigned char*>(core->rambank0_ptr());std::memcpy(namesRam+0x10a+110,namesRam+0x10a,10);
 auto duplicates=spells.characters(*core);require(duplicates[0].name==duplicates[1].name&&duplicates[0].slot!=duplicates[1].slot,"Duplicate names lost registration numbers");
 require(core->loadState(state.data(),state.size()),"Name fixture restore failed");

 for(const auto& character:chars){
  require(core->loadState(state.data(),state.size()),"Restore failed");auto r=static_cast<unsigned char*>(core->rambank0_ptr());auto s=static_cast<unsigned char*>(core->savedata_ptr());unsigned at=0x10a+110*character.slot,sat=0x2b+110*character.slot;
  // Exercise unlearned flags, reserved bits and empty / partially used MP.
  for(unsigned i=0;i<14;i++){unsigned n=unsigned(static_cast<unsigned char>(rom[counts+1+i]))-unsigned(static_cast<unsigned char>(rom[counts+i]));unsigned mask=(1u<<n)-1;r[at+0x34+i]=s[sat+0x34+i]=static_cast<unsigned char>(~mask);r[at+0x26+i]=s[sat+0x26+i]=static_cast<unsigned char>((i%10)*16+(i%10));}
  std::vector<unsigned char> before(r,r+32768),battery(s,s+32768);
  require(!spells.learn(*core,20)&&!spells.restoreMp(*core,character.slot),"Unsupported action accepted");
  require(spells.learn(*core,character.slot),"Learn failed");unsigned learned=0;
  for(unsigned i=0;i<14;i++){unsigned count=static_cast<unsigned char>(rom[counts+1+i])-static_cast<unsigned char>(rom[counts+i]);learned+=count;require(r[at+0x34+i]==255,"Missing spell or reserved bits lost");require(r[at+0x26+i]==0x99,"Current/max MP not 9");}
  require(learned==54,"Wrong native spell count");
  for(unsigned i=0;i<32768;i++){if(i<at+0x26||i>=at+0x42)require(r[i]==before[i],"Unrelated live data changed");if((i<sat+0x26||i>=sat+0x42)&&i!=0xbfe&&i!=0xbff)require(s[i]==battery[i],"Unrelated save data changed");}
  unsigned sum=0;for(unsigned i=0;i<0xbfe;i++)sum+=s[i];require(s[0xbfe]==(sum&255)&&s[0xbff]==((sum&255)^255),"Invalid checksum");
  std::vector<char> once(core->stateSize());core->saveState(once.data());require(spells.learn(*core,character.slot),"Repeated learn failed");std::vector<char> twice(core->stateSize());core->saveState(twice.data());require(once==twice,"Not idempotent");
  // Original ROM loader validates the battery checksum and loads C10A.
  auto harness=rom;harness[0x100]=char(0xc3);harness[0x101]=0;harness[0x102]=char(0xcb);auto reload=loadCoreBytes(harness,nullptr);std::memcpy(reload->savedata_ptr(),s,32768);auto rr=static_cast<unsigned char*>(reload->rambank0_ptr());
  const unsigned char code[]={0xf3,0x31,0xf0,0xcf,0xcd,static_cast<unsigned char>(loader),static_cast<unsigned char>(loader>>8),0xea,0x80,0xc0,0x3e,0xa5,0xea,0x81,0xc0,0x18,0xfe};std::memcpy(rr+0xb00,code,sizeof(code));std::array<gambatte::uint_least32_t,40000> audio{};
  for(unsigned f=0;f<120&&rr[0x81]!=0xa5;f++){unsigned samples=35112;reload->runFor(nullptr,160,audio.data(),audio.size(),samples);}
  require(rr[0x81]==0xa5&&rr[0x80]==0,"Native loader rejected save");require(!std::memcmp(rr+at,r+at,110),"Native reload lost changes");
 }
 require(core->loadState(state.data(),state.size()),"Restore failed");auto r=static_cast<unsigned char*>(core->rambank0_ptr());r[0xa11]=3;require(!spells.learn(*core,0),"Battle accepted");r[0xa11]=2;r[0x49]=1;require(!spells.learn(*core,0),"Wrong context accepted");
 require(core->loadState(state.data(),state.size()),"Restore failed");r=static_cast<unsigned char*>(core->rambank0_ptr());r[0x10a]=0;require(!spells.learn(*core,0),"Empty slot accepted");r[0x10a]=0x7f;require(!spells.learn(*core,0),"Different saved character accepted");

 // A loaded facility workspace must receive the selected fields too, while
 // preserving unsaved HP / other edits. A different workspace stays intact.
 require(core->loadState(state.data(),state.size()),"Workspace restore failed");
 r=static_cast<unsigned char*>(core->rambank0_ptr());auto work=r+0x9a2;
 std::memcpy(work,r+0x10a,110);spells.resync(*core);work[0x1d]^=1;unsigned hp=work[0x1d];
 require(spells.learn(*core,0),"Facility workspace edit failed");
 for(unsigned i=0;i<14;i++)require(work[0x26+i]==0x99,"Workspace MP lost");
 require(work[0x1d]==hp,"Unsaved workspace HP changed");
 std::array<unsigned char,110> workspace;std::copy(work,work+110,workspace.begin());
 require(spells.learn(*core,1)&&std::equal(workspace.begin(),workspace.end(),work),"Other character modified workspace");
 // IRQ stack returning to the shared transfer loop is also busy.
 require(core->loadState(state.data(),state.size()),"Transfer restore failed");
 r=static_cast<unsigned char*>(core->rambank0_ptr());auto regs=core->cpuRegisters();
 r[regs.sp-0xc000]=0x4b;r[regs.sp-0xc000+1]=0x0d;
 require(!spells.learn(*core,0),"Suspended transfer accepted");
 for(unsigned offset:copies){
  require(core->loadState(state.data(),state.size()),"Facility transfer restore failed");
  r=static_cast<unsigned char*>(core->rambank0_ptr());regs=core->cpuRegisters();unsigned pc=(offset&0x3fff)+0x4004;r[0x49]=offset/0x4000;
  r[regs.sp-0xc000]=pc&255;r[regs.sp-0xc000+1]=pc>>8;
  require(!spells.learn(*core,0),"Suspended facility transfer accepted");
 }
 std::cout<<"PASS: all registered characters, 54 spells, current/max MP 9, reserved/unrelated data, idempotence, native checksum/reload and guards\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}

