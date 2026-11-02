#include "game_support.h"
#include <iostream>
void cartridge_set_rumble(unsigned){}
void require(bool ok,const char* text){if(!ok)throw std::runtime_error(text);}
int wmain(int argc,wchar_t** argv)try{
    require(argc==2,"Expected GBC Wizardry ROM");auto rom=read(argv[1]);auto profile=identifyGame(rom);
    for(bool battery:{false,true}){
        auto fast=loadCoreBytes(rom,nullptr),normal=loadCoreBytes(rom,nullptr);
        if(battery){auto ram=static_cast<unsigned char*>(fast->savedata_ptr());for(unsigned i=0;i<fast->savedata_size();i++)ram[i]=static_cast<unsigned char>((i*13+7)&255);std::memcpy(normal->savedata_ptr(),ram,fast->savedata_size());}
        std::array<gambatte::video_pixel_t,160*144> pixels{},reference{};std::array<gambatte::uint_least32_t,40000> sound{};
        auto result=skipBootLogos(fast,profile,rom,nullptr,pixels);
        require(result.skipped&&result.frames<=1200&&wizardryColorTitleReady(*fast),"GBC title not recognized");
        for(unsigned f=0;f<result.frames;f++){unsigned count=35112;normal->runFor(reference.data(),160,sound.data(),sound.size(),count);}
        std::vector<char> a(fast->stateSize()),b(normal->stateSize());fast->saveState(a.data());normal->saveState(b.data());
        require(a==b&&pixels==reference,"Skip differs from normal no-input boot");
        std::cout<<profile.name<<": "<<result.frames<<" frames, normal boot identical\n";
    }
    auto unknown=rom;unknown[0x100]=char(0x18);unknown[0x101]=char(0xfe);auto core=loadCoreBytes(unknown,nullptr);
    std::vector<char> before(core->stateSize());core->saveState(before.data());std::array<gambatte::video_pixel_t,160*144> pixels{};pixels.fill(0x123456);auto oldPixels=pixels;
    require(!skipBootLogos(core,profile,unknown,nullptr,pixels).skipped,"Unknown startup accepted");
    std::vector<char> after(core->stateSize());core->saveState(after.data());require(before==after&&oldPixels==pixels,"Fallback changed running core or display");
    std::cout<<"PASS: title/menu glyph detection, battery/no-battery, normal state/video match, unknown startup fallback\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}
