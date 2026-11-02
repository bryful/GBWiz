#include "turbo_runner.h"
#include <iostream>
void cartridge_set_rumble(unsigned){}
void require(bool ok,const char* text){if(!ok)throw std::runtime_error(text);}
struct Input:gambatte::InputGetter{unsigned value=0;unsigned operator()()override{return value;}};
int wmain(int argc,wchar_t** argv)try{
    require(argc==3,"Expected ROM and local build directory");auto rom=read(argv[1]);fs::path root=argv[2];
    require(SpellSupport::recognizesAlignment(rom),"Alignment revision not recognized");
    std::array<gambatte::video_pixel_t,160*144> pixels{};std::array<gambatte::uint_least32_t,40000> sound{};
    for(const char* context:{"bonus/city/core.bin","bonus/start/core.bin","bonus/name/core.bin","turbo-play/victory-fixed/core.bin","spells/camp/core.bin"})
    for(unsigned selected=0;selected<6;selected++)for(unsigned alignment=0;alignment<3;alignment++){
        Input input;auto core=loadCoreBytes(rom,&input);auto state=read(root/context);require(core->loadState(state.data(),state.size()),"Context rejected");
        TurboRunner runner;runner.attach(*core,rom,identifyGame(rom));
        for(unsigned f=0;f<60&&!runner.spells().available(*core);f++)runner.run(0,input.value,pixels,sound);
        auto ram=static_cast<unsigned char*>(core->rambank0_ptr());auto save=static_cast<unsigned char*>(core->savedata_ptr());
        std::vector<unsigned char> before(ram,ram+8192),battery(save,save+8192);
        require(runner.spells().alignment(*core,selected)==ram[0x1186+selected*128+10],"Current alignment read incorrectly");
        require(runner.spells().alignment(*core,20)==-1&&runner.spells().alignment(*core,6)==-1,"Invalid/empty alignment readable");
        require(runner.spells().setAlignment(*core,selected,alignment),"Alignment edit failed");
        require(runner.spells().alignment(*core,selected)==static_cast<int>(alignment),"Displayed alignment stale after edit");
        unsigned at=0x1186+selected*128,primary=0x17+selected*128;
        require(ram[at+10]==alignment&&save[primary+10]==alignment,"Alignment not applied");
        for(unsigned i=0;i<8192;i++){
            bool party=i>=0x950&&i<0x950+std::min(unsigned(before[0x94f]),6u);
            if(party){unsigned old=before[i];unsigned expected=old!=255&&(old&31)==selected?(old&63)|(alignment<<6):old;require(ram[i]==expected,"Party cache corrupt");}
            else if(i!=at+10&&i!=0x110e)require(ram[i]==before[i],"Unrelated live field changed");
            bool allowed=i==primary+10||(i>=primary+0xe4a&&i<primary+0xe4a+128)||i==0xe07+2*selected||i==0xe08+2*selected||i==0x1c51+2*selected||i==0x1c52+2*selected;
            if(!allowed)require(save[i]==battery[i],"Unrelated saved field changed");
        }
        unsigned sum=0;for(unsigned i=0;i<127;i++)sum+=save[primary+i]|(unsigned(save[primary+i+1])<<8);sum&=65535;
        for(unsigned base:{0xe07u,0x1c51u})require((save[base+2*selected]|(unsigned(save[base+2*selected+1])<<8))==sum,"Checksum corrupt");
        require(std::memcmp(save+primary,save+primary+0xe4a,128)==0,"Mirror differs");
        auto edited=runner.saveState();require(runner.spells().setAlignment(*core,selected,alignment)&&edited==runner.saveState(),"Repeated edit changed state");
        for(unsigned f=0;f<180;f++)runner.run(0,input.value,pixels,sound);
        require(ram[at+10]==alignment,"Resuming overwrote alignment");
        auto fresh=loadCoreBytes(rom,&input);std::memcpy(fresh->savedata_ptr(),save,8192);skipBootLogos(fresh,identifyGame(rom),rom,&input,pixels);
        for(unsigned f=0;f<128;f++){input.value=f<8?1:0;unsigned count=35112;fresh->runFor(pixels.data(),160,sound.data(),sound.size(),count);}
        require(static_cast<unsigned char*>(fresh->rambank0_ptr())[at+10]==alignment,"Normal save reload lost alignment");
        auto stable=runner.saveState();for(unsigned bad:{3u,255u,~0u})require(!runner.spells().setAlignment(*core,selected,bad),"Invalid alignment accepted");
        require(!runner.spells().setAlignment(*core,20,0)&&!runner.spells().setAlignment(*core,6,0),"Invalid/empty slot accepted");
        require(stable==runner.saveState(),"Rejected edit changed state");
        require(core->loadState(state.data(),state.size()),"Original checkpoint restore failed");
        require(ram[at+10]==before[at+10],"State restore did not restore alignment");
    }
    // Run the original game's load/save routines with an active workspace,
    // including duplicate identities: only the selected workspace may change.
    for(bool duplicate:{false,true})for(unsigned alignment=0;alignment<3;alignment++){
        auto source=loadCoreBytes(rom,nullptr);auto state=read(root/"bonus/city/core.bin");source->loadState(state.data(),state.size());
        auto privateRom=rom;privateRom[0x100]=char(0xc3);privateRom[0x101]=0;privateRom[0x102]=char(0xc0);
        auto core=loadCoreBytes(privateRom,nullptr);auto ram=static_cast<unsigned char*>(core->rambank0_ptr());auto save=static_cast<unsigned char*>(core->savedata_ptr());
        std::memcpy(ram,source->rambank0_ptr(),8192);std::memcpy(save,source->savedata_ptr(),8192);
        unsigned active=duplicate?1:2,target=duplicate?0:2;
        if(duplicate){std::memcpy(ram+0x1186+128,ram+0x1186,128);std::memcpy(save+0x17+128,save+0x17,128);}
        const unsigned char code[]={0xf3,0x31,0xf0,0xcf,0x3e,2,0xea,0xae,0xd0,0xea,0,0x20,0x3e,static_cast<unsigned char>(active),0xcd,0x18,0x13,0x3e,3,0xea,0x39,0xd1,0x3e,0xa5,0xea,0x80,0xc0,0xfa,0x81,0xc0,0xb7,0x28,0xfa,0x3e,static_cast<unsigned char>(active),0xcd,0x46,0x13,0x3e,0x5a,0xea,0x82,0xc0,0x18,0xfe};
        std::copy(std::begin(code),std::end(code),ram);ram[0x80]=ram[0x81]=ram[0x82]=0;
        TurboRunner runner;runner.attach(*core,rom,identifyGame(rom));unsigned input=0;runner.run(0,input,pixels,sound);require(ram[0x80]==0xa5,"Original load failed");
        std::vector<unsigned char> work(ram+0x1104,ram+0x1184);
        require(runner.spells().setAlignment(*core,target,alignment),"Workspace edit failed");
        for(unsigned i=0;i<128;i++)require(ram[0x1104+i]==(!duplicate&&i==10?alignment:work[i]),"Workspace merge corrupt");
        ram[0x81]=1;runner.run(0,input,pixels,sound);require(ram[0x82]==0x5a,"Original save failed");
        require(ram[0x1186+128*target+10]==alignment&&save[0x17+128*target+10]==alignment,"Original save lost alignment");
        require(ram[0x1186+128*active+0x35]==3,"Live HP update lost");
    }
    auto state=read(root/"rewards/before-win/core.bin");auto core=loadCoreBytes(rom,nullptr);core->loadState(state.data(),state.size());TurboRunner runner;runner.attach(*core,rom,identifyGame(rom));
    auto before=runner.saveState();require(!runner.spells().setAlignment(*core,0,0)&&before==runner.saveState(),"Battle edit changed state");
    for(unsigned offset:{0x16dd7u,0x1b4bu,0x407eu,0x1b61u,0x64du}){
        auto unknown=rom;unknown[offset]^=1;SpellSupport support;support.attach(unknown);
        require(!support.alignmentSupported()&&!support.setAlignment(*core,0,0),"Unknown revision accepted");
    }
    std::cout<<"PASS: all alignments, six characters, five contexts, party cache, checksums/mirror/reload, live save/duplicate identities, invalid/battle/revision refusal\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}
