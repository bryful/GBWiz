#include "turbo_runner.h"
#include <iostream>
void cartridge_set_rumble(unsigned){}
void require(bool ok,const char* text){if(!ok)throw std::runtime_error(text);}
struct Input:gambatte::InputGetter{unsigned value=0;unsigned operator()()override{return value;}};
int wmain(int argc,wchar_t** argv)try{
    require(argc==3,"Expected Gaiden 1 ROM and town checkpoint");auto rom=read(argv[1]),state=read(argv[2]);
    SpellSupport support;support.attach(rom);require(support.supported(),"Spell revision not recognized");
    auto unknown=rom;unknown[0x2644]^=1;require(!SpellSupport::recognizes(unknown),"Unknown spell table accepted");
    for(unsigned selected=0;selected<6;selected++){
        Input input;auto core=loadCoreBytes(rom,&input);require(core->loadState(state.data(),state.size()),"Town checkpoint rejected");
        require(support.town(*core),"Town not recognized");auto characters=support.characters(*core);require(characters.size()==6,"Wrong roster size");
        require(characters[0].name==u8"せんし1"&&characters[2].name==u8"そうりょ1","Character names decoded incorrectly");
        auto ram=static_cast<unsigned char*>(core->rambank0_ptr());auto save=static_cast<unsigned char*>(core->savedata_ptr());
        std::vector<unsigned char> before(ram,ram+8192),battery(save,save+8192);
        require(!support.learn(*core,20)&&!support.learn(*core,6),"Empty or invalid slot accepted");
        require(std::equal(before.begin(),before.end(),ram)&&std::equal(battery.begin(),battery.end(),save),"Rejected edit changed data");
        require(support.learn(*core,selected),"Learning failed");size_t at=0x1186+selected*128;
        const unsigned char flags[]={0x1f,255,255,255,255,255,255,255};
        require(std::equal(std::begin(flags),std::end(flags),ram+at+0x39),"Spell flags incorrect");
        for(unsigned i=0;i<8192;i++)if(i<at+0x39||i>=at+0x4f)require(ram[i]==before[i],"Learning changed another character or unrelated WRAM");
        require(std::all_of(ram+at+0x41,ram+at+0x4f,[](unsigned char v){return v==9;}),"Current MP not 9");
        size_t primary=0x17+selected*128;unsigned sum=0;for(unsigned i=0;i<127;i++)sum+=ram[at+i]|(unsigned(ram[at+i+1])<<8);sum&=65535;
        for(size_t base:{size_t(0xe07),size_t(0x1c51)})require((save[base+2*selected]|(unsigned(save[base+2*selected+1])<<8))==sum,"Save checksum incorrect");
        require(std::memcmp(save+primary,save+primary+0xe4a,128)==0,"Backup differs");
        for(size_t i=0;i<battery.size();i++){
            bool allowed=(i>=primary+0x39&&i<primary+0x4f)||(i>=primary+0xe4a&&i<primary+0xe4a+128)||i==0xe07+2*selected||i==0xe08+2*selected||i==0x1c51+2*selected||i==0x1c52+2*selected;
            if(!allowed)require(save[i]==battery[i],"Another save record or unrelated save field changed");
        }
        // Ask the ROM's own inn/level MP accounting to count the learned
        // bits in a disposable core. This verifies the bit order and coverage.
        auto checkRom=rom;checkRom[0x100]=char(0xc3);checkRom[0x101]=0;checkRom[0x102]=char(0xc0);
        auto accounting=loadCoreBytes(checkRom,nullptr);auto work=static_cast<unsigned char*>(accounting->rambank0_ptr());
        std::memcpy(work+0x1104,ram+at,128);
        const unsigned char code[]={0xf3,0x31,0xf0,0xcf,0x3e,2,0xea,0,0x20,0xea,0xae,0xd0,0xcd,0x19,0x0d,0x3e,0xa5,0xea,0x80,0xc0,0x18,0xfe};
        std::memcpy(work,code,sizeof(code));
        std::array<gambatte::uint_least32_t,40000> checkSound{};
        for(unsigned f=0;f<20&&work[0x80]!=0xa5;f++){unsigned count=35112;accounting->runFor(nullptr,160,checkSound.data(),checkSound.size(),count);}
        require(work[0x80]==0xa5,"Original spell accounting did not finish");
        for(unsigned level=0;level<14;level++)require(work[0x1145+level]==static_cast<unsigned char>(rom[0x2644+level]),"Original game did not recognize all learned spells");
        // The app's hook raises the original maximum calculation to 9.
        SpellSupport maximum;maximum.attach(rom);
        struct Hook{SpellSupport* spells;gambatte::GB* core;} hook{&maximum,accounting.get()};
        accounting->setWriteFilter([](void* user,unsigned pc,unsigned,unsigned address,unsigned value){auto& h=*static_cast<Hook*>(user);return h.spells->write(*h.core,pc,address,value);},&hook);
        // Restart a fresh harness so the native maximum routine executes again.
        auto maxCore=loadCoreBytes(checkRom,nullptr);auto maxRam=static_cast<unsigned char*>(maxCore->rambank0_ptr());
        std::memcpy(maxRam+0x1104,ram+at,128);std::memcpy(maxRam,code,sizeof(code));
        hook.core=maxCore.get();maxCore->setWriteFilter([](void* user,unsigned pc,unsigned,unsigned address,unsigned value){auto& h=*static_cast<Hook*>(user);return h.spells->write(*h.core,pc,address,value);},&hook);
        for(unsigned f=0;f<20&&maxRam[0x80]!=0xa5;f++){unsigned count=35112;maxCore->runFor(nullptr,160,checkSound.data(),checkSound.size(),count);}
        require(maxRam[0x80]==0xa5&&std::all_of(maxRam+0x1145,maxRam+0x1153,[](unsigned char v){return v==9;}),"Original maximum calculation not raised to 9");
        require(maximum.write(*maxCore,0x7113,0xd145,3),"Other write intercepted");maxRam[0x113d]&=0xfe;
        require(maximum.write(*maxCore,0x7112,0xd145,3),"Partially learned character affected");
        std::vector<char> learned(core->stateSize());core->saveState(learned.data());require(support.learn(*core,selected),"Repeated learning failed");
        std::vector<char> repeated(core->stateSize());core->saveState(repeated.data());require(learned==repeated,"Learning twice changed state");
        auto fresh=loadCoreBytes(rom,&input);std::memcpy(fresh->savedata_ptr(),save,8192);
        std::array<gambatte::video_pixel_t,160*144> pixels{};std::array<gambatte::uint_least32_t,40000> sound{};
        skipBootLogos(fresh,identifyGame(rom),rom,&input,pixels);
        for(unsigned frame=0;frame<128;frame++){input.value=frame<8?1:0;unsigned count=35112;fresh->runFor(pixels.data(),160,sound.data(),sound.size(),count);}
        require(support.town(*fresh),"Fresh game did not return to town");
        auto freshRam=static_cast<unsigned char*>(fresh->rambank0_ptr());require(std::equal(std::begin(flags),std::end(flags),freshRam+at+0x39),"Learning lost after normal save reload");
        require(std::all_of(freshRam+at+0x41,freshRam+at+0x4f,[](unsigned char v){return v==9;}),"MP9 lost after reload");
        require(core->loadState(state.data(),state.size()),"State restore failed");require(std::equal(before.begin(),before.end(),ram),"State did not restore original spells");
    }
    auto core=loadCoreBytes(rom,nullptr);require(!support.town(*core)&&!support.learn(*core,0),"Non-town edit accepted");
    core->loadState(state.data(),state.size());
    auto reserved=static_cast<unsigned char*>(core->rambank0_ptr())+0x1186+0x39;
    auto savedReserved=static_cast<unsigned char*>(core->savedata_ptr())+0x17+0x39;
    *reserved|=0x80;*savedReserved|=0x80;require(support.learn(*core,0)&&*reserved==0x9f,"Reserved spell bits were cleared");
    std::cout<<"PASS: six named characters, all 61 spell flags, MP9/unrelated data unchanged, save checksums/backup/reopen and state restore\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}
