#include "spell_support.h"
#include <iostream>
void cartridge_set_rumble(unsigned){}
void require(bool ok,const char* text){if(!ok)throw std::runtime_error(text);}
struct Hook{SpellSupport* support;gambatte::GB* core;};
bool observe(void* data,unsigned pc,unsigned,unsigned address,unsigned value){auto& h=*static_cast<Hook*>(data);h.support->observe(*h.core,pc,address,value);return true;}
struct Input:gambatte::InputGetter{unsigned value=0;unsigned operator()()override{return value;}};
int wmain(int argc,wchar_t** argv)try{
    require(argc==3,"ROM and checkpoint required");auto rom=read(argv[1]),state=read(argv[2]);bool third=identifyGame(rom).id==GameId::Gaiden3;
    unsigned roster=third?0xba1:0x1095,work=third?0xb1f:0x1013,primary=third?10:24,mirror=third?0xe52:0xe5e,sumAt=third?0xe04:0xe1c;
    unsigned party=third?0x9af:0xd29,battle=third?0xf3:0x55d,copy=third?0x1fc7:0x2180,load=third?0x1021:0x1175,store=third?0x105a:0x11a3;
    SpellSupport support;support.attach(rom);require(support.alignmentSupported(),"Revision rejected");
    auto core=loadCoreBytes(rom,nullptr);require(!support.setAlignment(*core,0,0),"Boot accepted");require(core->loadState(state.data(),state.size()),"State rejected");
    auto characters=support.characters(*core);require(characters.size()>=6,"Missing characters");std::array<gambatte::uint_least32_t,40000> sound{};
    for(const auto& character:characters)for(unsigned alignment=0;alignment<3;alignment++){
        unsigned slot=character.slot;core->loadState(state.data(),state.size());auto r=static_cast<unsigned char*>(core->rambank0_ptr());auto s=static_cast<unsigned char*>(core->savedata_ptr());
        std::memcpy(r+work,r+roster+slot*128,128);support.resync(*core);r[work+0x35]^=1;
        unsigned at=roster+slot*128,sat=primary+slot*128;
        std::vector<unsigned char> before(r,r+8192),battery(s,s+core->savedata_size());
        require(support.alignment(*core,slot)==r[at+10],"Current alignment wrong");
        require(support.setAlignment(*core,slot,alignment)&&support.alignment(*core,slot)==static_cast<int>(alignment),"Edit failed");
        for(unsigned i=0;i<8192;i++){
            unsigned expected=before[i];
            if(i==at+10||i==work+10)expected=alignment;
            else if(i>=party&&i<party+std::min(unsigned(before[party-1]),6u)&&before[i]!=255&&(before[i]&31)==slot)expected=(before[i]&63)|(alignment<<6);
            require(r[i]==expected,"Other live field/party cache corrupted");
        }
        require(s[sat+10]==alignment&&!std::memcmp(s+sat,s+sat+mirror,128),"Saved attribute/mirror wrong");
        unsigned sum=0;for(unsigned i=0;i<127;i++)sum+=s[sat+i]|(unsigned(s[sat+i+1])<<8);sum&=65535;
        for(unsigned base:{sumAt,sumAt+mirror})require((s[base+slot*2]|(unsigned(s[base+slot*2+1])<<8))==sum,"Checksum wrong");
        for(unsigned i=0;i<battery.size();i++){
            bool allowed=i==sat+10||(i>=sat+mirror&&i<sat+mirror+128)||i==sumAt+slot*2||i==sumAt+slot*2+1||i==sumAt+mirror+slot*2||i==sumAt+mirror+slot*2+1;
            if(!allowed)require(s[i]==battery[i],"Other saved field changed");
        }
        std::vector<char> once(core->stateSize()),twice(core->stateSize());core->saveState(once.data());
        require(support.setAlignment(*core,slot,alignment),"Repeat failed");core->saveState(twice.data());require(once==twice,"Repeat changed state");
        require(!support.setAlignment(*core,20,0)&&!support.setAlignment(*core,slot,3)&&!support.setAlignment(*core,slot,~0u),"Invalid value accepted");
        // Boot the unchanged ROM using only the edited battery data.
        Input input;auto fresh=loadCoreBytes(rom,&input);std::memcpy(fresh->savedata_ptr(),s,battery.size());
        for(unsigned f=0;f<1684;f++){input.value=f>=1200&&f<1208?1:f>=1468&&f<1476?128:f>=1496&&f<1504?1:0;unsigned samples=35112;fresh->runFor(nullptr,160,sound.data(),sound.size(),samples);}
        require(static_cast<const unsigned char*>(fresh->rambank0_ptr())[at+10]==alignment,"Normal boot lost attribute");
        core->loadState(state.data(),state.size());support.resync(*core);require(r[at+10]==before[at+10],"State restore failed");
    }
    // Use the original load/save routines to prove workspace edits survive.
    for(bool other:{false,true})for(unsigned alignment=0;alignment<3;alignment++){
        core->loadState(state.data(),state.size());auto harness=rom;harness[0x100]=char(0xc3);harness[0x101]=0;harness[0x102]=char(0xc0);
        auto live=loadCoreBytes(harness,nullptr);auto r=static_cast<unsigned char*>(live->rambank0_ptr());std::memcpy(r,core->rambank0_ptr(),8192);std::memcpy(live->savedata_ptr(),core->savedata_ptr(),core->savedata_size());
        const unsigned char code[]={0xf3,0x31,0xf0,0xdf,0x3e,1,0xcd,static_cast<unsigned char>(load),static_cast<unsigned char>(load>>8),0x3e,0xa5,0xea,0x80,0xc1,0xfa,0x81,0xc1,0xb7,0x28,0xfa,0x3e,1,0xcd,static_cast<unsigned char>(store),static_cast<unsigned char>(store>>8),0x3e,0x5a,0xea,0x82,0xc1,0x18,0xfe};
        std::memcpy(r,code,sizeof(code));r[0x180]=r[0x181]=r[0x182]=r[battle]=0;support.attach(rom);Hook hook{&support,live.get()};live->setWriteFilter(observe,&hook);
        for(unsigned f=0;f<30&&r[0x180]!=0xa5;f++){unsigned samples=35112;live->runFor(nullptr,160,sound.data(),sound.size(),samples);}require(r[0x180]==0xa5,"Original load stalled");
        r[work+0x35]=3;std::vector<unsigned char> before(r+work,r+work+128);unsigned target=other?0:1;
        require(support.setAlignment(*live,target,alignment),"Workspace edit failed");
        for(unsigned i=0;i<128;i++)require(r[work+i]==(!other&&i==10?alignment:before[i]),"Workspace merge failed");
        r[0x181]=1;for(unsigned f=0;f<30&&r[0x182]!=0x5a;f++){unsigned samples=35112;live->runFor(nullptr,160,sound.data(),sound.size(),samples);}require(r[0x182]==0x5a,"Original save stalled");
        auto s=static_cast<const unsigned char*>(live->savedata_ptr());require(s[primary+target*128+10]==alignment&&s[primary+128+0x35]==3,"Original save lost edit/HP");
    }
    for(unsigned condition=0;condition<5;condition++){
        core->loadState(state.data(),state.size());auto r=static_cast<unsigned char*>(core->rambank0_ptr());
        if(condition==0)r[battle]=0x14;
        if(condition==1){auto regs=core->cpuRegisters();r[regs.sp-0xc000]=copy&255;r[regs.sp-0xc000+1]=copy>>8;}
        if(condition==2)r[roster]=0;
        if(condition==3)r[roster]^=0x40;
        if(condition==4){std::memcpy(r+roster+128,r+roster,128);std::memcpy(r+work,r+roster,128);}
        support.resync(*core);std::vector<char> before(core->stateSize()),after(core->stateSize());core->saveState(before.data());
        require(!support.setAlignment(*core,0,0),"Unsafe edit accepted");core->saveState(after.data());require(before==after,"Rejected edit changed state");
    }
    for(unsigned at:{third?0x21186u:0x20d13u,third?0x17f6u:0x1aa0u,third?0x1816u:0x1ab6u,third?0x4315u:0x40adu,third?0x71du:0x79bu}){
        auto unknown=rom;unknown[at]^=1;SpellSupport rejected;rejected.attach(unknown);require(!rejected.alignmentSupported()&&!rejected.setAlignment(*core,0,0),"Unknown revision accepted");
    }
    std::cout<<"PASS: all registered characters/alignments, party cache, checksums/mirrors, normal boot, native workspace save, guards\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}
