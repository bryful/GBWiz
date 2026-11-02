#include "turbo_runner.h"
#include "dungeon_map.h"
#include <iostream>
void cartridge_set_rumble(unsigned){}
void require(bool v,const char* s){if(!v)throw std::runtime_error(s);}
struct Input:gambatte::InputGetter{unsigned value=0;unsigned operator()()override{return value;}};
int wmain(int argc,wchar_t** argv)try{
    require(argc==3,"ROM and build root required");auto rom=read(argv[1]);fs::path root=argv[2];Input input;
    auto core=loadCoreBytes(rom,&input);DungeonMap map;map.attach(rom);require(map.supported(),"Profile rejected");
    std::array<gambatte::video_pixel_t,160*144> pixels{};std::array<gambatte::uint_least32_t,40000> audio{};
    for(auto path:{"bonus/city/core.bin","rewards/before-win/core.bin"}){auto state=read(root/path);require(core->loadState(state.data(),state.size()),"Fixture rejected");require(!map.show(*core),"Non-dungeon accepted");}
    auto camp=read(root/"spells/camp/core.bin");require(core->loadState(camp.data(),camp.size()),"Camp fixture rejected");require(map.show(*core),"Camp unavailable");
    map.stage=2;map.floor=2;require(map.move(*core,[]{return true;}),"Camp relocation failed");
    for(unsigned i=0;i<180;i++){unsigned samples=35112;core->runFor(pixels.data(),160,audio.data(),audio.size(),samples);}
    require(map.available(*core),"Camp relocation did not return to dungeon");require((static_cast<const unsigned char*>(core->zeropage_ptr())[-0x40]&0xff)==0x8b,"Camp relocation failed to restore dungeon LCD");
    require(core->loadState(camp.data(),camp.size()),"Camp restore failed");input.value=2;
    for(unsigned i=0;i<181;i++){unsigned samples=35112;core->runFor(pixels.data(),160,audio.data(),audio.size(),samples);input.value=0;}
    std::vector<char> initial(core->stateSize());core->saveState(initial.data());require(map.show(*core),"Dungeon unavailable");
    require(map.floor==1&&map.x==1&&map.y==15,"Wrong coordinates");
    drawDungeonMap(map,*core,pixels);
    std::ofstream image(root/"map-preview.ppm",std::ios::binary);image<<"P6\n160 144\n255\n";
    for(auto pixel:pixels){char rgb[]={char(pixel>>16),char(pixel>>8),char(pixel)};image.write(rgb,3);}image.close();
    map.key(32);require(map.x==0&&map.floor==1,"Left must change X only");
    map.key(128);require(map.y==14&&map.floor==1,"Down must change Y only");
    map.key(512);require(map.floor==2&&map.x==0&&map.y==14,"PageDown must change floor only");
    map.key(256);require(map.floor==1,"PageUp failed");
    map.key(256);require(map.floor==134,"PageUp wrap failed");
    map.key(512);require(map.floor==1,"PageDown wrap failed");
    map.x=1;map.y=15;require(map.key(1),"A must decide immediately");
    map.key(2);require(!map.open,"B must cancel immediately");
    auto revised=rom;revised[0xc09f]=0;DungeonMap unknown;unknown.attach(revised);require(!unknown.supported(),"Unknown revision accepted");
    for(unsigned speed:{2u,8u})for(unsigned floor:{1u,2u,3u,4u,5u,6u,129u,130u,131u,132u,133u,134u}){
        require(core->loadState(initial.data(),initial.size()),"Restore failed");TurboRunner runner;runner.attach(*core,rom,identifyGame(rom));
        require(runner.speed(speed),"Speed rejected");require(map.show(*core),"Cannot show");map.floor=floor;map.x=1;map.y=15;map.stage=2;
        // Find an open non-event destination for movement/render integration.
        auto table=0x1918f+((floor&128)?12:0)+2*((floor&127)-1);unsigned base=0x14000+static_cast<unsigned char>(rom[table])+(static_cast<unsigned char>(rom[table+1])<<8);
        bool found=false;for(unsigned y=0;y<16&&!found;y++)for(unsigned x=0;x<16&&!found;x++)if(map.destination(floor,x,y)&&(static_cast<unsigned char>(rom[base+(15-y)*32+x*2+1])&31)==0){map.x=x;map.y=y;found=true;}
        require(found,"No destination");auto before=runner.saveState();auto saved=std::vector<char>(static_cast<const char*>(core->savedata_ptr()),static_cast<const char*>(core->savedata_ptr())+core->savedata_size());
        require(!map.move(*core,[]{return false;}),"Failed save allowed move");require(before==runner.saveState(),"Failed save mutated core");
        bool backedUp=false;require(map.move(*core,[&]{require(before==runner.saveState(),"Mutation preceded backup");backedUp=true;return true;}),"Move rejected");require(backedUp&&!map.open,"Missing backup/close");runner.resetInput();
        auto ram=static_cast<unsigned char*>(core->rambank0_ptr());require(ram[0x8a7]==floor&&ram[0x8a5]==map.x&&ram[0x8a6]==map.y,"Position not updated");
        for(unsigned i=0;i<180;i++)runner.run(0,input.value,pixels,audio);
        require(ram[0x8a7]==floor&&ram[0x8a5]==map.x&&ram[0x8a6]==map.y,"Position lost");if(!map.available(*core)){auto reg=core->cpuRegisters();std::cerr<<"floor="<<floor<<" speed="<<speed<<" pc="<<std::hex<<reg.pc<<" sp="<<reg.sp<<" bank="<<unsigned(ram[0x10ae])<<" stack=";for(unsigned at=reg.sp;at<0xe000&&at>=0xc000;at++)std::cerr<<unsigned(ram[at-0xc000])<<" ";std::cerr<<std::dec<<"\n";}require(map.available(*core),"Dungeon did not return to input wait");require(std::memcmp(saved.data()+0x17,static_cast<const char*>(core->savedata_ptr())+0x17,20*128)==0,"Character roster changed during teleport");
        // ROM's reloaded map matches the host's decoder.
        for(unsigned y=0;y<16;y++)for(unsigned x=0;x<16;x++)require(ram[0x590+(15-y)*32+x*2]==map.cell(floor,x,y),"Map decoder differs from live floor");
        require(runner.loadState(before),"Slot1 restore failed");require(map.available(*core),"Backup not resumable");require(runner.speed()==speed,"Backup speed lost");
        std::cout<<"floor "<<floor<<" OK\n";
    }
    std::cout<<"PASS: 12 floors, relocation/reload, map decoder, input, context refusal, backup ordering/failure\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}
