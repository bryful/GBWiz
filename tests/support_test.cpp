#include "game_support.h"
#include <iostream>
void cartridge_set_rumble(unsigned){}
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
int wmain(int argc,wchar_t** argv)try{
    require(argc==3,"Expected GB Gaiden 1 and GBC Wizardry 1 ROMs");
    auto rom=read(argv[1]);auto profile=identifyGame(rom);
    require(profile.id==GameId::Gaiden1,"Gaiden 1 header identity failed");
    auto changed=rom;changed.back()^=1;require(identifyGame(changed).id==profile.id,"Identification depends on full ROM bytes");
    auto fast=loadCoreBytes(rom,nullptr),normal=loadCoreBytes(rom,nullptr);
    std::array<gambatte::video_pixel_t,160*144> pixels{},reference{};
    auto result=skipBootLogos(fast,profile,rom,nullptr,pixels);
    require(result.skipped&&result.frames>0&&result.frames<1200,"Title reveal was not detected");
    require(gaiden1TitleReady(*fast),"Boot skip did not stop at title");
    std::array<gambatte::uint_least32_t,40000> audio{};
    for(unsigned i=0;i<result.frames;i++){unsigned samples=35112;normal->runFor(reference.data(),160,audio.data(),audio.size(),samples);}
    std::vector<char> a(fast->stateSize()),b(normal->stateSize());fast->saveState(a.data());normal->saveState(b.data());
    require(a==b&&pixels==reference,"Boot skip changed game state compared with normal no-input boot");
    auto colorRom=read(argv[2]);auto color=loadCoreBytes(colorRom,nullptr);auto colorProfile=identifyGame(colorRom);
    require(colorProfile.id==GameId::Wizardry1,"GBC Wizardry 1 mistaken for Gaiden 1");
    a.resize(color->stateSize());color->saveState(a.data());
    require(!skipBootLogos(color,{GameId::Unknown,"Unknown"},colorRom,nullptr,pixels).skipped,"Unknown profile boot was altered");
    b.resize(color->stateSize());color->saveState(b.data());require(a==b,"Unsupported game state changed");
    // A dump/variant that never reaches the expected title must fall back to
    // its exact initial state, rather than consuming startup/gameplay time.
    auto variant=rom;variant[0x100]=0x18;variant[0x101]=static_cast<char>(0xfe);
    auto stalled=loadCoreBytes(variant,nullptr);
    auto original=stalled.get();
    auto memory=[](const void* p,size_t n){auto data=static_cast<const char*>(p);return std::vector<char>(data,data+n);};
    auto initialVram=memory(stalled->vram_ptr(),0x2000),initialWram=memory(stalled->rambank0_ptr(),0x2000);
    auto initialIo=memory(stalled->oamram_ptr(),0x200),initialRam=memory(stalled->savedata_ptr(),stalled->savedata_size());
    require(!skipBootLogos(stalled,profile,variant,nullptr,pixels).skipped,"Unknown startup variant was accepted");
    require(stalled.get()==original,"Failed probe replaced running core");
    require(initialVram==memory(stalled->vram_ptr(),0x2000)&&initialWram==memory(stalled->rambank0_ptr(),0x2000)&&
        initialIo==memory(stalled->oamram_ptr(),0x200)&&initialRam==memory(stalled->savedata_ptr(),stalled->savedata_size()),
        "Failed probe changed running memory");
    std::cout<<"PASS: header profiles, Gaiden 1 title skip at "<<result.frames<<" frames, identical normal state, unknown profile untouched\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}
