#include "turbo_runner.h"
#include <iostream>
void cartridge_set_rumble(unsigned){}
void require(bool ok,const char* text){if(!ok)throw std::runtime_error(text);}
int wmain(int argc,wchar_t** argv)try{
    require(argc==2,"Expected Gaiden 2/3 ROM");auto rom=read(argv[1]);auto id=identifyGame(rom).id;
    require(id==GameId::Gaiden2||id==GameId::Gaiden3,"Wrong game");
    unsigned check=id==GameId::Gaiden2?0x1144:0x0ff0,last=id==GameId::Gaiden2?0xd2e:0x9b4,caller=id==GameId::Gaiden2?0xa673:0xa969;
    require(PartySupport::recognizes(rom),"Profile rejected");auto privateRom=rom;privateRom[0x100]=char(0xc3);privateRom[0x101]=0;privateRom[0x102]=char(0xc0);
    std::array<gambatte::video_pixel_t,160*144> pixels{};std::array<gambatte::uint_least32_t,40000> sound{};
    for(unsigned slot=0;slot<6;slot++)for(unsigned existing=0;existing<4;existing++)for(unsigned candidate=0;candidate<3;candidate++){
        auto core=loadCoreBytes(privateRom,nullptr);auto ram=static_cast<unsigned char*>(core->rambank0_ptr());
        std::fill(ram+last-5,ram+last+1,0xff);if(existing<3)ram[last-5+slot]=static_cast<unsigned char>((existing<<6)|slot);
        const unsigned char code[]={0xf3,0x31,0xf0,0xcf,0x3e,static_cast<unsigned char>(candidate),0xcd,static_cast<unsigned char>(check),static_cast<unsigned char>(check>>8),0x3e,0,0xce,0,0xea,0x80,0xc3,0x3e,0xa5,0xea,0x81,0xc3,0x18,0xfe};
        std::copy(std::begin(code),std::end(code),ram);ram[0x381]=0;
        std::vector<char> initial(core->stateSize());core->saveState(initial.data());TurboRunner runner;runner.attach(*core,rom,identifyGame(rom));
        auto bank=static_cast<const unsigned char*>(core->rombank0_ptr());std::vector<unsigned char> original(bank,bank+0x4000);unsigned input=0;
        for(bool enabled:{false,true,false}){
            require(runner.loadState(initial),"Reset failed");auto before=runner.saveState();
            require(runner.party().enable(enabled)&&before==runner.saveState(),"Toggle changed data");
            for(unsigned i=0;i<0x4000;i++)require(bank[i]==(enabled&&i==check?0xaf:enabled&&i==check+1?0xc9:original[i]),"Unrelated ROM byte changed");
            for(unsigned f=0;f<10&&ram[0x381]!=0xa5;f++)runner.run(0,input,pixels,sound);
            require(ram[0x381]==0xa5,"Original check did not return");bool conflict=existing<3&&existing!=1&&candidate!=1&&existing!=candidate;
            require(ram[0x380]==unsigned(!enabled&&conflict),"Wrong original compatibility result");
        }
        require(runner.party().enable(true),"ON failed");runner.detach();require(std::equal(original.begin(),original.end(),bank),"Detach did not restore ROM");
    }
    for(unsigned at:{check,caller}){auto unknown=rom;unknown[at]^=1;require(!PartySupport::recognizes(unknown),"Unrecognized revision accepted");}
    // Enabling the setting leaves full SRAM and WRAM unchanged even on GB3's
    // larger save, so no foreign character/save layout is assumed or edited.
    auto core=loadCoreBytes(rom,nullptr);TurboRunner runner;runner.attach(*core,rom,identifyGame(rom));auto before=runner.saveState();
    require(runner.party().enable(true)&&before==runner.saveState(),"Setting modified save or RAM");
    require(runner.loadState(before)&&runner.party().enabled(),"State load lost current setting");
    std::cout<<"PASS: game-specific original compatibility at all six positions, three alignments, neutral/empty, OFF/ON/OFF, exact patch/undo, SRAM/WRAM preservation, revision guards\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}
