#include "turbo_runner.h"
#include <iostream>
void cartridge_set_rumble(unsigned){}
void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
int wmain(int argc,wchar_t** argv)try{
    require(argc==2,"Expected ROM");auto rom=read(argv[1]);auto id=identifyGame(rom).id;
    unsigned lookup=id==GameId::Wizardry1?0x25d4:id==GameId::Wizardry2?0x25cc:0x259a;
    std::array<unsigned,4> sites=id==GameId::Wizardry1?std::array<unsigned,4>{0x105ef,0x1283e,0x1a2df,0x1cf33}:id==GameId::Wizardry2?std::array<unsigned,4>{0x105f7,0x128ff,0x1a37a,0x1cf42}:std::array<unsigned,4>{0x105ef,0x12833,0x1a2ec,0x1cf33};
    require(MpSupport::recognizes(rom),"Unknown MP revision");
    std::array<gambatte::video_pixel_t,160*144> pixels{};std::array<gambatte::uint_least32_t,40000> sound{};
    for(unsigned siteIndex=0;siteIndex<4;siteIndex++)for(unsigned slot=0;slot<6;slot++)for(int speed:{2,3,4,6,8}){
        unsigned site=sites[siteIndex];auto testRom=rom;testRom[0x100]=char(0xc3);testRom[0x101]=0;testRom[0x102]=char(0xcb);
        // Test-only return after the original SUB/LD, preserving original PCs.
        testRom[site+3]=char(0xc9);auto core=loadCoreBytes(testRom,nullptr);auto ram=static_cast<unsigned char*>(core->rambank0_ptr());
        std::fill(ram,ram+8192,0);for(unsigned i=0;i<6;i++)ram[0x103+i]=static_cast<unsigned char>(i);
        for(unsigned i=0;i<20*110;i++)ram[0x10a+i]=0x59;
        std::vector<unsigned char> code={0xf3,0x31,0xf0,0xcf,0x3e,static_cast<unsigned char>(site/0x4000),0xea,0,0x21,0xea,0x49,0xc0,0x3e,static_cast<unsigned char>(slot),0xea,0xb0,0xff};
        unsigned wait=code.size();code.insert(code.end(),{0xfa,0x80,0xc0,0xb7,0x28,0xfa,0xaf,0xea,0x80,0xc0});
        for(unsigned level=0;level<14;level++){
            if(siteIndex==2&&level!=8)continue;
            unsigned address=0xc10a+slot*110+0x26+level;
            if(siteIndex==2)code.insert(code.end(),{0x21,static_cast<unsigned char>(address),static_cast<unsigned char>(address>>8),0x7e});
            else code.insert(code.end(),{0x3e,static_cast<unsigned char>(level),0xea,0xba,0xc0,0xcd,static_cast<unsigned char>(lookup),static_cast<unsigned char>(lookup>>8)});
            unsigned entry=(site&0x3fff)+0x4000;code.insert(code.end(),{0xcd,static_cast<unsigned char>(entry),static_cast<unsigned char>(entry>>8)});
        }
        code.insert(code.end(),{0x21,0x81,0xc0,0x34,0xc3,static_cast<unsigned char>(wait),0xcb});require(code.size()<0x300,"Test program overlaps fixture");std::copy(code.begin(),code.end(),ram+0xb00);
        TurboRunner runner;runner.attach(*core,rom,identifyGame(rom));require(!runner.mp().enabled()&&runner.mp().supported(),"Default/support incorrect");runner.speed(speed);unsigned input=0;runner.run(0,input,pixels,sound);
        for(unsigned phase=0;phase<3;phase++){
            runner.mp().enable(phase==1);ram[0x80]=1;
            for(unsigned f=0;f<30&&ram[0x81]!=phase+1;f++)runner.run(0,input,pixels,sound);
            require(ram[0x81]==phase+1,"Original decrement did not finish");
            for(unsigned i=0;i<20*110;i++){
                unsigned field=i%110;bool target=i/110==slot&&field>=0x26&&field<0x34&&(siteIndex!=2||field==0x2e);
                require(ram[0x10a+i]==(target?(phase==2?0x39:0x49):0x59),"MP or unrelated field changed");
            }
            auto state=runner.saveState();require(runner.loadState(state)&&runner.mp().enabled()==(phase==1),"State lost MP setting");
        }
        runner.mp().enable(true);unsigned address=0xc130+slot*110;ram[address-0xc000]=0x59;unsigned pc=(site&0x3fff)+0x4003;
        require(runner.mp().write(*core,pc+1,address,0x49)&&runner.mp().write(*core,pc,address,0x69)&&runner.mp().write(*core,pc,address-1,0x49),"Unrelated write blocked");
        ram[0x49]=0;require(runner.mp().write(*core,pc,address,0x49),"Wrong bank blocked");
    }
    for(unsigned site:sites){auto unknown=rom;unknown[site]^=1;MpSupport mp;mp.attach(unknown);require(!mp.supported()&&!mp.enable(true),"Unknown revision accepted");}
    std::cout<<"PASS: four original decrement sites, six members, 14 MP levels, five speeds, OFF/ON/OFF, state setting, unrelated fields and revision guards\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}
