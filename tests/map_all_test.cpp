#include "dungeon_map.h"
#include "turbo_runner.h"
#include <iostream>
void cartridge_set_rumble(unsigned){}
void require(bool ok,const char* text){if(!ok)throw std::runtime_error(text);}
struct Input:gambatte::InputGetter{unsigned value=0;unsigned operator()()override{return value;}};
int wmain(int argc,wchar_t** argv)try{
    require(argc==2,"Repository root required");fs::path root=argv[1];
    struct Case{const char* rom;const char* state;unsigned count;};
    const Case cases[]={{"wizardrygaiden2.gb","gaiden2/play/core.bin",12},{"wizardrygaiden3.gb","gaiden3/battle/core.bin",36},{"WIZA1.gbc","wiza1/advance/core.bin",10},{"wiza2.gbc","wiza2/play/core.bin",11},{"wiza3.gbc","wiza3/play/core.bin",10}};
    std::array<gambatte::video_pixel_t,160*144> pixels{};std::array<gambatte::uint_least32_t,40000> sound{};
    for(const auto& c:cases){
        std::cout<<c.rom<<std::endl;Input input;auto rom=read(root/".rom"/c.rom);auto core=loadCoreBytes(rom,&input);auto original=read(root/"build"/c.state);DungeonMap map;map.attach(rom);
        require(map.supported(),"Unsupported profile");require(map.floors().size()==c.count,"Wrong floor count");require(!map.show(*core),"Boot accepted as dungeon");
        require(core->loadState(original.data(),original.size()),"Fixture rejected");require(map.show(*core),"Dungeon unavailable");
        auto id=identifyGame(rom).id;
        auto revision=rom;revision[id==GameId::Gaiden2?0xc08d:id==GameId::Gaiden3?0xc10d:0x1357]=0;
        DungeonMap unknown;unknown.attach(revision);require(!unknown.supported(),"Unknown reload revision accepted");
        const char* battle=id==GameId::Gaiden2?"gaiden2/encounter/core.bin":id==GameId::Gaiden3?"gaiden3/encounter/core.bin":id==GameId::Wizardry2?"wiza2/encounter/core.bin":nullptr;
        if(battle){auto state=read(root/"build"/battle);require(core->loadState(state.data(),state.size()),"Battle fixture rejected");require(!map.show(*core),"Battle accepted");}
        if(map.isColor()){
            require(core->loadState(original.data(),original.size()),"Restore failed");
            auto r=static_cast<unsigned char*>(core->rambank0_ptr());r[0xa11]=3;
            require(!map.show(*core),"Battle mode accepted");
        }
        if(id==GameId::Wizardry1)for(unsigned f:{5u,6u,7u})for(unsigned variant:{1u,2u,3u}){
            require(core->loadState(original.data(),original.size()),"Variant restore failed");
            auto r=static_cast<unsigned char*>(core->rambank0_ptr());unsigned shift=(f-4)*2;
            r[0xa9c]=static_cast<unsigned char>((r[0xa9c]&~(3u<<shift))|(variant<<shift));
            require(map.show(*core),"Variant unavailable");map.floor=f;bool found=false;
            for(unsigned y=1;y<15&&!found;y++)for(unsigned x=1;x<15&&!found;x++)if(map.destination(f,x,y)&&map.event(f,x,y)==0){map.x=x;map.y=y;found=true;}
            require(found&&map.key(1),"Variant destination failed");require(map.move(*core,[]{return true;}),"Variant movement failed");
            for(unsigned frame=0;frame<180;frame++){unsigned samples=35112;core->runFor(pixels.data(),160,sound.data(),sound.size(),samples);}
            require(map.available(*core)&&r[0xa11]==2,"Variant did not resume dungeon");
            for(unsigned y=0;y<16;y++)for(unsigned x=0;x<16;x++){
                unsigned raw=r[0x7100+y*16+x],live=((raw&3)<<6)|((raw&0xc0)>>2)|(raw&0x0c)|((raw&0x30)>>4);
                require(live==map.cell(f,x,y),"Progression variant map mismatch");
            }
        }
        require(core->loadState(original.data(),original.size()),"Camp restore failed");
        // Reach camp through the original controller path, then relocate.
        input.value=2;
        for(unsigned frame=0;frame<188;frame++){unsigned samples=35112;core->runFor(pixels.data(),160,sound.data(),sound.size(),samples);if(frame==7)input.value=0;}
        require(map.show(*core),"Camp input unavailable");
        require(map.key(1),"Camp A failed");require(map.move(*core,[]{return true;}),"Camp movement failed");
        for(unsigned frame=0;frame<180;frame++){unsigned samples=35112;core->runFor(pixels.data(),160,sound.data(),sound.size(),samples);}
        require(map.available(*core),"Camp movement did not return to dungeon");if(map.isColor())require(static_cast<const unsigned char*>(core->rambank0_ptr())[0xa11]==2,"Camp remained open after move");
        for(unsigned speed:{2u,8u})for(unsigned f:map.floors()){
            require(core->loadState(original.data(),original.size()),"Restore failed");TurboRunner runner;runner.attach(*core,rom,identifyGame(rom));require(runner.speed(speed),"Speed unsupported");require(map.show(*core),"Cannot show");map.floor=f;
            bool found=false;
            for(unsigned y=1;y<15&&!found;y++)for(unsigned x=1;x<15&&!found;x++)if(map.destination(f,x,y)&&map.cell(f,x,y)!=0&&map.event(f,x,y)==0){map.x=x;map.y=y;found=true;}
            require(found,"No destination");require(map.key(1),"A did not decide");auto before=runner.saveState();auto bytes=static_cast<const char*>(core->rambank0_ptr());auto ramBefore=std::vector<char>(bytes,bytes+(map.isColor()?32768:8192));auto saved=static_cast<const char*>(core->savedata_ptr());auto batteryBefore=std::vector<char>(saved,saved+core->savedata_size());
            require(!map.move(*core,[]{return false;}),"Failed save allowed movement");require(before==runner.saveState(),"Failed save mutated state");
            require(map.move(*core,[&]{require(before==runner.saveState(),"Mutation preceded backup");return true;}),"Movement failed");runner.resetInput();
            for(unsigned frame=0;frame<180;frame++)runner.run(0,input.value,pixels,sound);
            auto r=static_cast<const unsigned char*>(core->rambank0_ptr());unsigned p=map.positionAddress();
            if(!map.available(*core)||r[p]!=map.x||r[p+1]!=map.y||r[p+(map.isColor()?3:2)]!=f){auto reg=core->cpuRegisters();std::cerr<<"floor="<<f<<" speed="<<speed<<" expected="<<map.x<<","<<map.y<<" actual="<<unsigned(r[p])<<","<<unsigned(r[p+1])<<","<<unsigned(r[p+(map.isColor()?3:2)])<<" pc="<<std::hex<<reg.pc<<" sp="<<reg.sp<<" stack=";for(unsigned at=reg.sp;at<(map.isColor()?0xcfff:0xe000)&&at>=0xc000;at++)std::cerr<<unsigned(r[at-0xc000])<<" ";std::cerr<<std::dec<<"\n";require(false,"Did not return to destination input wait");}
            unsigned mismatch=0;
            for(unsigned y=0;y<16;y++)for(unsigned x=0;x<16;x++){
                unsigned raw=r[map.mapAddress()+(map.isColor()?y*16+x:(15-y)*32+x*2)];
                unsigned live=map.isColor()?((raw&3)<<6)|((raw&0xc0)>>2)|(raw&0x0c)|((raw&0x30)>>4):raw;
                if(live!=map.cell(f,x,y))++mismatch;
            }
            if(mismatch){std::cerr<<"floor="<<f<<" mismatches="<<mismatch<<"\n";require(mismatch==0,"Host decoder does not match live terrain");}
            require(runner.loadState(before),"Backup rejected");require(map.available(*core),"Backup not resumable");require(runner.speed()==speed,"Backup speed lost");require(std::memcmp(ramBefore.data(),core->rambank0_ptr(),ramBefore.size())==0,"Backup lost WRAM");require(std::memcmp(batteryBefore.data(),core->savedata_ptr(),batteryBefore.size())==0,"Backup lost battery data");
            std::cout<<"  floor "<<f<<" speed "<<speed<<" OK\n";
        }
    }
    std::cout<<"PASS: all Wizardry profiles, every floor, 1x/4x, backup failure/order/restore, live terrain\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}

