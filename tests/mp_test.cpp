#include "turbo_runner.h"
#include <iostream>
void cartridge_set_rumble(unsigned){}
void require(bool ok,const char* text){if(!ok)throw std::runtime_error(text);}
int wmain(int argc,wchar_t** argv)try{
    require(argc==3,"Expected ROM and town checkpoint");auto rom=read(argv[1]),state=read(argv[2]);
    auto source=loadCoreBytes(rom,nullptr);require(source->loadState(state.data(),state.size()),"Fixture rejected");
    auto privateRom=rom;privateRom[0x100]=char(0xc3);privateRom[0x101]=0;privateRom[0x102]=char(0xc0);
    auto core=loadCoreBytes(privateRom,nullptr);auto ram=static_cast<unsigned char*>(core->rambank0_ptr());
    std::memcpy(ram,source->rambank0_ptr(),8192);std::memcpy(core->savedata_ptr(),source->savedata_ptr(),8192);
    for(unsigned slot=0;slot<6;slot++)std::fill(ram+0x1186+128*slot+0x41,ram+0x1186+128*slot+0x4f,5);
    auto before=std::vector<unsigned char>(ram+0x1186,ram+0x1186+20*128);
    // Run the ROM's real field-spell MP decrement and character save, once
    // for every mage/priest level on both the selected and another character.
    std::vector<unsigned char> code={0xf3,0x31,0xf0,0xcf,0x3e,3,0xea,0,0x20,0xea,0xae,0xd0};
    unsigned wait=code.size();code.insert(code.end(),{0xfa,0x81,0xc3,0xb7,0x28,0xfa,0xaf,0xea,0x81,0xc3});
    for(unsigned slot:{2,3})for(unsigned level=0;level<14;level++){
        const unsigned char call[]={0x3e,static_cast<unsigned char>(level),0xea,0xfa,0xd0,0x3e,static_cast<unsigned char>(slot),0xcd,0x7a,0x6e};
        code.insert(code.end(),std::begin(call),std::end(call));
    }
    code.insert(code.end(),{0x21,0x82,0xc3,0x34,0xc3,static_cast<unsigned char>(wait),0xc0});
    require(code.size()<0x300,"Test program too long");std::copy(code.begin(),code.end(),ram);ram[0x381]=ram[0x382]=0;
    TurboRunner runner;runner.attach(*core,rom,identifyGame(rom));
    std::array<gambatte::video_pixel_t,160*144> pixels{};std::array<gambatte::uint_least32_t,40000> sound{};unsigned input=0;
    runner.run(0,input,pixels,sound);require(runner.spells().restoreMp(*core,2),"MP restore failed");
    require(!runner.spells().restoreMp(*core,20)&&!runner.spells().restoreMp(*core,6),"Empty/out-of-range character accepted");
    for(unsigned i=0;i<20*128;i++)if(i<2*128+0x41||i>=2*128+0x4f)require(before[i]==ram[0x1186+i],"Restore changed another field");
    for(unsigned phase=0;phase<2;phase++){
        ram[0x588]=phase?19:20; // Consumption stays normal in combat too.
        ram[0x381]=1;for(unsigned f=0;f<30&&ram[0x382]!=phase+1;f++)runner.run(0,input,pixels,sound);
        require(ram[0x382]==phase+1,"Original consumption routine did not finish");
        auto save=static_cast<const unsigned char*>(core->savedata_ptr());
        for(unsigned level=0;level<14;level++){
            require(ram[0x1186+2*128+0x41+level]==(phase?7:8),"MP remained fixed after the one-shot action");
            require(ram[0x1186+3*128+0x41+level]==(phase?3:4),"Non-target MP consumption changed");
            require(save[0x17+2*128+0x41+level]==(phase?7:8),"Original save lost target MP");
        }
    }
    ram[0x588]=19;require(runner.spells().restoreMp(*core,2),"Repeated MP restore failed");
    for(unsigned level=0;level<14;level++)require(ram[0x1186+2*128+0x41+level]==9,"Repeated restore did not refill");
    auto saved=static_cast<const unsigned char*>(core->savedata_ptr());
    std::vector<unsigned char> checkpoint(ram,ram+8192),battery(saved,saved+8192);
    require(runner.spells().restoreMp(*core,2)&&std::equal(checkpoint.begin(),checkpoint.end(),ram)&&std::equal(battery.begin(),battery.end(),saved),"Repeated restore was not idempotent");
    ram[0x588]=20;checkpoint.assign(ram,ram+8192);
    require(!runner.spells().restoreMp(*core,2)&&std::equal(checkpoint.begin(),checkpoint.end(),ram)&&std::equal(battery.begin(),battery.end(),saved),"Battle edit accepted or changed data");
    std::cout<<"PASS: selected MP9 refill, normal original consumption at all 14 levels, non-target preservation, repeat, battle refusal\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}
