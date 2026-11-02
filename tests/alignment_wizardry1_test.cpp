#include "turbo_runner.h"
#include <iostream>
void cartridge_set_rumble(unsigned){}
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
int wmain(int argc,wchar_t** argv)try{
    require(argc==3,"Expected ROM and checkpoint");auto rom=read(argv[1]),state=read(argv[2]);
    auto id=identifyGame(rom).id;bool second=id==GameId::Wizardry2,third=id==GameId::Wizardry3;
    unsigned loader=third?0x2c02:second?0x2c37:0x2c3c;
    auto core=loadCoreBytes(rom,nullptr);SpellSupport support;support.attach(rom);
    require(support.alignmentSupported(),"Revision not recognized");
    require(!support.setAlignment(*core,0,0),"Boot edit accepted");
    require(core->loadState(state.data(),state.size()),"Roster state load failed");
    auto characters=support.characters(*core);require(characters.size()>=6,"Missing test characters");
    std::array<gambatte::uint_least32_t,40000> audio{};
    for(const auto& character:characters)for(unsigned alignment=0;alignment<3;alignment++)for(unsigned upper:{0u,0x5cu,0xfcu}){
        require(core->loadState(state.data(),state.size()),"State load failed");support.resync(*core);
        auto r=static_cast<unsigned char*>(core->rambank0_ptr());auto s=static_cast<unsigned char*>(core->savedata_ptr());
        unsigned selected=character.slot,at=0x10a+selected*110,sat=0x2b+selected*110;
        unsigned original=r[at+10];
        // Independent upper bits must survive in live, saved and working data.
        r[at+10]=static_cast<unsigned char>(upper|1);s[sat+10]=static_cast<unsigned char>((upper^0x40)|1);
        std::memcpy(r+0x9a2,r+at,110);support.resync(*core);r[0x9a2+0x1d]^=1;
        std::vector<unsigned char> before(r,r+8192),battery(s,s+32768);
        require(support.alignment(*core,selected)==1,"Current value wrong");
        require(support.setAlignment(*core,selected,alignment),"Edit failed");
        require(support.alignment(*core,selected)==static_cast<int>(alignment),"Displayed value stale");
        for(unsigned i=0;i<8192;i++)require(r[i]==(i==at+10||i==0x9ac?((before[i]&0xfc)|alignment):before[i]),"Unrelated live field changed");
        for(unsigned i=0;i<32768;i++)if(i!=0xbfe&&i!=0xbff)require(s[i]==(i==sat+10?((battery[i]&0xfc)|alignment):battery[i]),"Unrelated saved field changed");
        unsigned sum=0;for(unsigned i=0;i<0xbfe;i++)sum+=s[i];
        require(s[0xbfe]==(sum&255)&&s[0xbff]==((sum&255)^255),"Checksum wrong");
        std::vector<char> edited(core->stateSize());core->saveState(edited.data());
        require(support.setAlignment(*core,selected,alignment),"Repeat failed");
        std::vector<char> repeated(core->stateSize());core->saveState(repeated.data());require(edited==repeated,"Repeat changed state");
        require(!support.setAlignment(*core,20,0)&&!support.setAlignment(*core,selected,3)&&!support.setAlignment(*core,selected,~0u),"Invalid edit accepted");
        // Execute the original save loader in a disposable core.
        auto harness=rom;harness[0x100]=char(0xc3);harness[0x101]=0;harness[0x102]=char(0xcb);
        auto reload=loadCoreBytes(harness,nullptr);std::memcpy(reload->savedata_ptr(),s,32768);auto rr=static_cast<unsigned char*>(reload->rambank0_ptr());
        const unsigned char code[]={0xf3,0x31,0xf0,0xcf,0xcd,static_cast<unsigned char>(loader),static_cast<unsigned char>(loader>>8),0xea,0x80,0xc0,0x3e,0xa5,0xea,0x81,0xc0,0x18,0xfe};std::memcpy(rr+0xb00,code,sizeof(code));
        for(unsigned f=0;f<120&&rr[0x81]!=0xa5;f++){unsigned samples=35112;reload->runFor(nullptr,160,audio.data(),audio.size(),samples);}
        require(rr[0x81]==0xa5&&rr[0x80]==0&&rr[at+10]==s[sat+10],"Native reload lost edit");
        require(core->loadState(state.data(),state.size()),"Restore failed");support.resync(*core);
        require(r[at+10]==original,"State restore failed");
    }
    // A workspace belonging to a different character remains untouched.
    require(core->loadState(state.data(),state.size()),"Other workspace restore failed");
    auto r=static_cast<unsigned char*>(core->rambank0_ptr());std::memcpy(r+0x9a2,r+0x10a+110,110);support.resync(*core);
    std::vector<unsigned char> work(r+0x9a2,r+0x9a2+110);
    require(support.setAlignment(*core,0,2)&&std::equal(work.begin(),work.end(),r+0x9a2),"Other workspace changed");
    for(unsigned condition=0;condition<5;condition++){
        require(core->loadState(state.data(),state.size()),"Guard restore failed");support.resync(*core);
        auto r=static_cast<unsigned char*>(core->rambank0_ptr());auto regs=core->cpuRegisters();
        if(condition==0)r[0xa11]=3;
        if(condition==1){r[regs.sp-0xc000]=0x4b;r[regs.sp-0xc000+1]=0x0d;}
        if(condition==2)r[0x10a]=0;
        if(condition==3)r[0x10a]^=0x40;
        if(condition==4){std::memcpy(r+0x10a+110,r+0x10a,10);std::memcpy(r+0x9a2,r+0x10a,110);support.resync(*core);}
        std::vector<char> before(core->stateSize());core->saveState(before.data());require(!support.setAlignment(*core,0,0),"Unsafe edit accepted");
        std::vector<char> after(core->stateSize());core->saveState(after.data());require(before==after,"Rejected edit changed state");
    }
    for(unsigned offset:{third?0x192edu:second?0x19348u:0x192f6u,0xd2feu,third?0x1761cu:second?0x176a9u:0x17616u,third?0x58aau:second?0x5a1eu:0x55d3u}){auto unknown=rom;unknown[offset]^=1;SpellSupport other;other.attach(unknown);require(!other.alignmentSupported(),"Unknown revision accepted");}
    std::cout<<"PASS: all registered characters, three alignments, upper bits, workspace HP, other fields, native reload, guards and revisions\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}
