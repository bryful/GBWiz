#include "turbo_runner.h"
#include <iostream>
void cartridge_set_rumble(unsigned){}
void require(bool ok,const char* text){if(!ok)throw std::runtime_error(text);}
int wmain(int argc,wchar_t** argv)try{
    require(argc==2||argc==3,"Expected ROM and optional town state");auto rom=read(argv[1]);bool gaiden2=identifyGame(rom).id==GameId::Gaiden2,third=identifyGame(rom).id==GameId::Gaiden3;
    require(gaiden2||third||argc==3,"Gaiden 1 needs a town fixture");
    unsigned roster=third?0xba1:gaiden2?0x1095:0x1186,primary=third?0x0a:gaiden2?0x18:0x17,firstMp=third?0xcb60:gaiden2?0xd054:0xd145;
    unsigned bankAddress=third?0xcac7:gaiden2?0xcfbd:0xd0ae,lookup=third?0x0df7:gaiden2?0x0f20:0x10c4,scratch=third?0xcb14:gaiden2?0xd009:0xd0fa;
    unsigned loadRoutine=third?0x1021:gaiden2?0x1175:0x1318,saveRoutine=third?0x105a:gaiden2?0x11a3:0x1346,field=third?0x6656:gaiden2?0x709d:0x6e7a;
    unsigned menu=third?0x506f:gaiden2?0x4f8e:0x5385,counts=third?0x24f4:gaiden2?0x26ae:0x2644;
    require(MpSupport::recognizes(rom),"MP revision not recognized");
    auto source=loadCoreBytes(rom,nullptr);
    if(argc==3){auto fixture=read(argv[2]);require(source->loadState(fixture.data(),fixture.size()),"Town fixture rejected");}
    else{auto ram=static_cast<unsigned char*>(source->rambank0_ptr());for(unsigned slot=0;slot<20;slot++)for(unsigned i=0;i<128;i++)ram[roster+slot*128+i]=static_cast<unsigned char>((slot*17+i)%251+1);}
    std::vector<char> state(source->stateSize());source->saveState(state.data());
    auto privateRom=rom;privateRom[0x100]=char(0xc3);privateRom[0x101]=0;privateRom[0x102]=char(0xc0);
    // Keep the original DEC and its PC; return before unrelated menu/battle UI.
    privateRom[0x8000+(menu-0x4000)+1]=privateRom[third?0x14e7b:0x14d41]=char(0xc9);
    std::array<gambatte::video_pixel_t,160*144> pixels{};std::array<gambatte::uint_least32_t,40000> sound{};
    for(unsigned site=0;site<(third?4u:3u);site++)for(unsigned selected=0;selected<6;selected++)for(int speed:{2,3,4,6,8}){
        auto core=loadCoreBytes(privateRom,nullptr);auto ram=static_cast<unsigned char*>(core->rambank0_ptr());
        std::memcpy(ram,source->rambank0_ptr(),8192);std::memcpy(core->savedata_ptr(),source->savedata_ptr(),8192);
        for(unsigned slot=0;slot<6;slot++){std::fill(ram+roster+128*slot+0x41,ram+roster+128*slot+0x4f,5);if(third)std::fill(ram+roster+128*slot+0x6e,ram+roster+128*slot+0x75,5);}
        auto before=std::vector<unsigned char>(ram+roster,ram+roster+20*128);
        unsigned bank=site==0?(third?6:3):site==1?2:site==2?5:14;
        std::vector<unsigned char> code={0xf3,0x31,0xf0,static_cast<unsigned char>(third?0xdf:0xcf),0x3e,static_cast<unsigned char>(bank),0xea,0,0x20,0xea,static_cast<unsigned char>(bankAddress),static_cast<unsigned char>(bankAddress>>8)};
        unsigned wait=code.size();code.insert(code.end(),{0xfa,0x81,0xc3,0xb7,0x28,0xfa,0xaf,0xea,0x81,0xc3});
        for(unsigned level=0;level<(third?21u:14u);level++){
            if(site==0||site==3){
                const unsigned char call[]={0x3e,static_cast<unsigned char>(level),0xea,static_cast<unsigned char>(scratch),static_cast<unsigned char>(scratch>>8),0x3e,static_cast<unsigned char>(selected),0xcd,static_cast<unsigned char>(site==3?0x5ebc:field),static_cast<unsigned char>((site==3?0x5ebc:field)>>8)};code.insert(code.end(),std::begin(call),std::end(call));
            }else{
                const unsigned char load[]={0x3e,static_cast<unsigned char>(selected),0xcd,static_cast<unsigned char>(loadRoutine),static_cast<unsigned char>(loadRoutine>>8)};code.insert(code.end(),std::begin(load),std::end(load));
                unsigned number=level;
                if(site==2){number=1+(third?(level/7)*0x50:(level>=7?128:0));for(unsigned i=(level/7)*7;i<level;i++)number+=static_cast<unsigned char>(rom[counts+i]);}
                const unsigned char call[]={0x3e,static_cast<unsigned char>(number),0xcd,static_cast<unsigned char>(site==1?(lookup&255):(third?0xa7:0x6d)),static_cast<unsigned char>(site==1?(lookup>>8):(third?0x4e:0x4d)),0xcd,static_cast<unsigned char>(site==1?(menu&255):(third?0x7a:0x40)),static_cast<unsigned char>(site==1?(menu>>8):(third?0x4e:0x4d)),0x3e,static_cast<unsigned char>(selected),0xcd,static_cast<unsigned char>(saveRoutine),static_cast<unsigned char>(saveRoutine>>8)};code.insert(code.end(),std::begin(call),std::end(call));
            }
        }
        code.insert(code.end(),{0x21,0x82,0xc3,0x34,0xc3,static_cast<unsigned char>(wait),0xc0});
        require(code.size()<0x380,"Test program too long");std::copy(code.begin(),code.end(),ram);ram[0x381]=ram[0x382]=0;
        TurboRunner runner;runner.attach(*core,rom,identifyGame(rom));require(!runner.mp().enabled(),"MP support defaults to ON");require(runner.speed(speed),"Speed rejected");
        unsigned input=0;runner.run(0,input,pixels,sound);
        for(unsigned phase=0;phase<3;phase++){
            bool enabled=phase==1;require(runner.mp().enable(enabled),"Global toggle rejected");ram[0x381]=1;
            for(unsigned f=0;f<30&&ram[0x382]!=phase+1;f++)runner.run(0,input,pixels,sound);
            if(ram[0x382]!=phase+1){std::cerr<<"site="<<site<<" selected="<<selected<<" phase="<<phase<<" pc="<<std::hex<<core->cpuRegisters().pc<<"\n";throw std::runtime_error("Original routines did not finish");}
            auto save=static_cast<const unsigned char*>(core->savedata_ptr());unsigned expected=phase<2?4:3;
            for(unsigned slot=0;slot<20;slot++)for(unsigned i=0;i<128;i++){
                bool mp=slot==selected&&((i>=0x41&&i<0x4f)||(third&&i>=0x6e&&i<0x75));
                if(ram[roster+slot*128+i]!=(mp?expected:before[slot*128+i])){std::cerr<<"site="<<site<<" selected="<<selected<<" phase="<<phase<<" slot="<<slot<<" offset="<<std::hex<<i<<" got="<<unsigned(ram[roster+slot*128+i])<<" expected="<<unsigned(mp?expected:before[slot*128+i])<<"\n";throw std::runtime_error("MP or unrelated character field incorrect");}
                if(mp)require(save[primary+slot*128+i]==expected,"Original save lost MP");
            }
            auto checkpoint=runner.saveState();require(runner.mp().enable(!enabled)&&runner.loadState(checkpoint)&&runner.mp().enabled()==!enabled,"State load replaced the global setting");
        }
    }
    // OFF must execute exactly like the original core, not just yield equal MP.
    {
        auto core=loadCoreBytes(rom,nullptr),reference=loadCoreBytes(rom,nullptr);core->loadState(state.data(),state.size());reference->loadState(state.data(),state.size());
        TurboRunner runner;runner.attach(*core,rom,identifyGame(rom));unsigned input=0;
        for(unsigned f=0;f<120;f++){runner.run(0,input,pixels,sound);unsigned count=35112;reference->runFor(pixels.data(),160,sound.data(),sound.size(),count);}
        std::vector<char> expected(reference->stateSize());reference->saveState(expected.data());require(runner.saveState()==expected,"OFF changed original emulation");
        require(runner.mp().enable(true),"Enable failed");auto ram=static_cast<unsigned char*>(core->rambank0_ptr());ram[firstMp-0xc000]=5;ram[bankAddress-0xc000]=2;
        require(runner.mp().write(*core,field+11,firstMp,4),"Wrong bank was intercepted");ram[bankAddress-0xc000]=third?6:3;
        require(runner.mp().write(*core,field+12,firstMp,4)&&runner.mp().write(*core,field+11,firstMp-1,4)&&runner.mp().write(*core,field+11,firstMp,9),"Unrelated/recovery write was intercepted");
    }
    for(unsigned offset:std::vector<unsigned>{lookup,third?0x1a656u:0xc000+(field-0x4000),third?0x904cu:0x8000+(menu-0x4000)-18,third?0x14e6du:0x14d33u,third?0x14ea7u:0x14d6du,third?0x39ebcu:lookup}){auto unknown=rom;unknown[offset]^=1;MpSupport mp;mp.attach(unknown);require(!mp.supported()&&!mp.enable(true)&&mp.enable(false),"Unknown revision enabled");}
    std::cout<<"PASS: global OFF/ON/OFF, six characters, 14/21 levels, three/four original consumption paths, five speeds, original save, current setting on state load, OFF identical, unrelated/recovery writes preserved\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}
