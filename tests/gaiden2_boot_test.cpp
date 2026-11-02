#include "game_support.h"
#include <iostream>
void cartridge_set_rumble(unsigned){}
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
struct Input:gambatte::InputGetter{unsigned value=0;unsigned operator()()override{return value;}};
std::vector<char> state(gambatte::GB& core){std::vector<char> result(core.stateSize());core.saveState(result.data());return result;}
int wmain(int argc,wchar_t** argv)try{
    require(argc==3,"Gaiden II/III ROM and registered-character checkpoint required");auto rom=read(argv[1]),checkpoint=read(argv[2]);auto profile=identifyGame(rom);
    bool third=profile.id==GameId::Gaiden3;require(third||profile.id==GameId::Gaiden2,"Wrong ROM identity");
    auto titleReady=[&](gambatte::GB& core){return third?gaiden3TitleReady(core):gaiden2TitleReady(core);};std::array<gambatte::uint_least32_t,40000> sound{};
    for(unsigned battery=0;battery<3;battery++){
        Input a,b;auto fast=loadCoreBytes(rom,&a),normal=loadCoreBytes(rom,&b);
        if(battery==1){auto s=static_cast<unsigned char*>(fast->savedata_ptr());for(unsigned i=0;i<fast->savedata_size();i++)s[i]=static_cast<unsigned char>(i*13+7);}
        if(battery==2){auto source=loadCoreBytes(rom,nullptr);require(source->loadState(checkpoint.data(),checkpoint.size()),"Checkpoint failed");std::memcpy(fast->savedata_ptr(),source->savedata_ptr(),fast->savedata_size());}
        auto s=static_cast<const char*>(fast->savedata_ptr());std::vector<char> saved(s,s+fast->savedata_size());std::memcpy(normal->savedata_ptr(),s,saved.size());
        std::array<gambatte::video_pixel_t,160*144> pixels{},reference{};auto result=skipBootLogos(fast,profile,rom,&a,pixels);
        require(result.skipped&&result.frames<=1200&&titleReady(*fast),"Title not recognized");
        if(battery==2){
            auto after=static_cast<const unsigned char*>(fast->savedata_ptr());
            for(unsigned i=0;i<saved.size();i++){
                // Gaiden III's ordinary startup clears six bank-3 bytes.
                unsigned expected=third&&i>=0x6078&&i<0x607e?255:static_cast<unsigned char>(saved[i]);
                require(after[i]==expected,"Boot changed persistent save data");
            }
        }
        for(unsigned f=0;f<result.frames;f++){unsigned samples=35112;normal->runFor(reference.data(),160,sound.data(),sound.size(),samples);if(f<(third?600u:800u))require(!titleReady(*normal),"Earlier scene accepted");}
        require(state(*fast)==state(*normal)&&pixels==reference,"Skip differs from ordinary boot");
        // Every part of the title predicate is necessary, including glyphs.
        auto vram=static_cast<unsigned char*>(fast->vram_ptr());auto io=static_cast<unsigned char*>(fast->zeropage_ptr())-0x80;
        for(unsigned at:third?std::array<unsigned,5>{0x18c5,0x1960,0x19a4,0x1510,0x14e0}:std::array<unsigned,5>{0x1924,0x19c0,0x1a04,0x440,0x540}){vram[at]^=1;require(!titleReady(*fast),"Corrupt title accepted");vram[at]^=1;}
        for(unsigned at:{0x40u,0x42u,0x43u,0x47u}){io[at]^=1;require(!titleReady(*fast),"Wrong display state accepted");io[at]^=1;}
        // Press and release A; Gaiden III waits for release before leaving.
        auto title=pixels;
        for(unsigned f=0;f<(third?180u:30u);f++){a.value=b.value=f<(third?30u:8u)?1:0;unsigned samples=35112;fast->runFor(pixels.data(),160,sound.data(),sound.size(),samples);samples=35112;normal->runFor(reference.data(),160,sound.data(),sound.size(),samples);}
        require(state(*fast)==state(*normal)&&pixels==reference&&pixels!=title,"Title input differs or is unresponsive");
        std::cout<<profile.name<<": "<<result.frames<<" frames, battery case "<<battery<<", state/video/input identical\n";
    }
    for(bool stalled:{false,true}){
        auto variant=rom;if(stalled){variant[0x100]=char(0x18);variant[0x101]=char(0xfe);}
        auto core=loadCoreBytes(variant,nullptr);auto before=state(*core);auto identity=core.get();
        std::array<gambatte::video_pixel_t,160*144> pixels{};pixels.fill(0x123456);auto original=pixels;
        require(!skipBootLogos(core,stalled?profile:GameProfile{GameId::Unknown,"Unknown"},variant,nullptr,pixels).skipped,"Unknown startup accepted");
        require(core.get()==identity&&state(*core)==before&&pixels==original,"Fallback changed core/display");
    }
    std::cout<<"PASS: Gaiden II/III title, three battery cases, ordinary state/video/input, predicate guards, exact fallback\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}
