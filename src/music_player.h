#pragma once
#include "game_support.h"
#include <array>
#include <algorithm>
struct SoundDriver {
    bool color=false;
    bool dq=false;
    unsigned music=0x32b3,legacyEffect=0x32d7,timer=0x32de,init=0x3321,effect=0x3a78,update=0x3b51;
    unsigned bank=8,musicMute=0xc8c4,effectMute=0xc8c5,savedBank=0xd0ae,effectState=0xdb86,musicBank=0;
};
// Audio-only execution of the original Gaiden sound drivers. As in Wiiz,
// frontend mailboxes feed original music/SFX commands to a separate 1x core.
class MusicPlayer {
    std::unique_ptr<gambatte::GB> core_;
    bool color_=false;
    bool dq_=false;
    void loadDq(const std::vector<char>& rom,unsigned song){
        dq_=true;auto playerRom=rom;
        playerRom[0x100]=static_cast<char>(0xc3);playerRom[0x101]=0;playerRom[0x102]=static_cast<char>(0xc0);
        core_=loadCoreBytes(playerRom,nullptr);
        // DQ3's original wrappers map a song/effect ID onto its 1-4 channels.
        // All driver work stays in WRAM bank 7; update 3343 runs once per frame.
        std::vector<unsigned char> code={0xf3,0x31,0xf0,0xcf,0xaf,0xe0,0xff,0xea,0,0x31,
            0x3e,29,0xea,0,0x21,0x3e,7,0xe0,0x70,0xcd,0xd9,0x31};
        unsigned loop=0xc000+code.size();
        auto add=[&](std::initializer_list<unsigned char> bytes){code.insert(code.end(),bytes);};
        add({0xf0,0x44,0xfe,0x90,0x30,0xfa,0xf0,0x44,0xfe,0x90,0x38,0xfa});
        add({0xfa,0,0xc1,0xfe,0xff,0x28,8,0xcd,0x16,0x5e,0x3e,0xff,0xea,0,0xc1});
        add({0xfa,1,0xc1,0xfe,0xff,0x28,8,0xcd,0x5c,0x5e,0x3e,0xff,0xea,1,0xc1});
        add({0xcd,0x43,0x33,0xc3,static_cast<unsigned char>(loop),static_cast<unsigned char>(loop>>8)});
        auto ram=static_cast<unsigned char*>(core_->rambank0_ptr());
        std::memcpy(ram,code.data(),code.size());ram[0x100]=static_cast<unsigned char>(song);ram[0x101]=255;
    }
    void loadColor(const std::vector<char>& rom,unsigned song){
        color_=true;auto playerRom=rom;
        playerRom[0x100]=static_cast<char>(0xc3);playerRom[0x101]=0;playerRom[0x102]=static_cast<char>(0xc0);
        core_=loadCoreBytes(playerRom,nullptr);
        // Original GBC sound RAM is in WRAM bank 2. Sound code is duplicated
        // in its data banks; entry 4000 updates all three command slots once.
        std::vector<unsigned char> code={0xf3,0x31,0xf0,0xcf,0xaf,0xe0,0xff,0x3e,2,0xe0,0x70,
            0x3e,30,0xea,0,0x20,0xaf,0xea,0,0x30,0xcd,6,0x40,0xcd,3,0x40};
        const unsigned char initial[]={0x3e,1,0xea,3,0xc1,0xfa,0,0xc1,0xfe,0xff,0x28,0x0d,0xea,8,0xd0,0x3e,1,0xea,7,0xd0,0x3e,4,0xea,0,0xd0};
        code.insert(code.end(),std::begin(initial),std::end(initial));
        unsigned loop=0xc000+code.size();
        const unsigned char tail[]={0xf0,0x44,0xfe,0x90,0x30,0xfa,0xf0,0x44,0xfe,0x90,0x38,0xfa,
            0xcd,0,0x40,0xc3,static_cast<unsigned char>(loop),static_cast<unsigned char>(loop>>8)};
        code.insert(code.end(),std::begin(tail),std::end(tail));
        // Host commands arrive before the bootstrap has initialized sound RAM.
        // Keep that first song in unbanked RAM until initialization completes.
        unsigned char* ram=static_cast<unsigned char*>(core_->rambank0_ptr());
        std::memcpy(ram,code.data(),code.size());ram[0x100]=static_cast<unsigned char>(song);ram[0x103]=0;
        std::fill(ram+0x110,ram+0x11a,0);
    }
public:
    static SoundDriver driver(const std::vector<char>& rom){
        SoundDriver d;auto id=identifyGame(rom).id;
        if(id==GameId::Dq3){d.dq=true;d.bank=29;}
        else if(id==GameId::Wizardry1||id==GameId::Wizardry2||id==GameId::Wizardry3){d.color=true;d.bank=30;}
        else if(id==GameId::Gaiden2){
            d.music+=60;d.legacyEffect+=60;d.timer+=60;d.init+=60;d.effect+=60;d.update+=60;
            d.bank=14;d.musicMute=0xcc99;d.effectMute=0xcc9a;d.savedBank=0xcfbd;d.effectState=0xda95;
        }else if(id==GameId::Gaiden3){
            d.music=0x3426;d.legacyEffect=0x344a;d.timer=0x3451;d.init=0x349d;d.effect=0x3c20;d.update=0x3cf9;
            d.bank=29;d.musicMute=0xc97e;d.effectMute=0xc97f;d.savedBank=0xcac7;d.effectState=0xc0bc;d.musicBank=0xcac6;
        }
        return d;
    }
    static bool supported(const std::vector<char>& rom){
        auto id=identifyGame(rom).id;
        if(id==GameId::Dq3){
            const unsigned char init[]={0xf0,0x70,0xea,0xf9,0xc0,0x3e,7,0xe0,0x70,1,0,0,0xcd,0x36,0x32};
            const unsigned char music[]={0xf3,0xea,0xd1,0xcc,0x3e,1,0xea,0xfa,0xc0,0xf0,0x70,0xea,0xf9,0xc0,0x3e,7,0xe0,0x70,0x21,0x1e,0x62};
            const unsigned char effect[]={0xf3,0xea,0xd1,0xcc,0xcd,0xa4,0x5e};
            return rom.size()>=0x80000&&std::equal(std::begin(init),std::end(init),reinterpret_cast<const unsigned char*>(rom.data()+0x31d9))&&
                std::equal(std::begin(music),std::end(music),reinterpret_cast<const unsigned char*>(rom.data()+0x75e5c))&&
                std::equal(std::begin(effect),std::end(effect),reinterpret_cast<const unsigned char*>(rom.data()+0x75e16));
        }
        if(id==GameId::Wizardry1||id==GameId::Wizardry2||id==GameId::Wizardry3){
            const unsigned char entry[]={0xc3,9,0x40,0xc3,0x1d,0x40,0xc3,0x4c,0x40,0xfa,0x0a,0xd0,0xea,0,0x20,0xaf,0xea,0,0x30};
            return rom.size()>=31*0x4000&&std::equal(std::begin(entry),std::end(entry),reinterpret_cast<const unsigned char*>(rom.data()+30*0x4000));
        }
        if(id!=GameId::Gaiden1&&id!=GameId::Gaiden2&&id!=GameId::Gaiden3)return false;
        auto d=driver(rom);
        const unsigned char start[]={0xc5,0x47,0xfa,static_cast<unsigned char>(d.musicMute),static_cast<unsigned char>(d.musicMute>>8),0xb7,0x28,0x02,0x06,0x00,0x78,0xc1,0xf5};
        const unsigned char effect[]={0xe0,0xee,0xe0,0xed,0xe0,0xef,0xc9};
        const unsigned char sceneEffect[]={0xc5,0x47,0xfa,static_cast<unsigned char>(d.effectMute),static_cast<unsigned char>(d.effectMute>>8),0xb7,0x78,0xc1,0x28,0x06};
        return rom.size()>=(d.bank+1)*0x4000&&std::equal(std::begin(start),std::end(start),reinterpret_cast<const unsigned char*>(rom.data()+d.music))&&
            std::equal(std::begin(effect),std::end(effect),reinterpret_cast<const unsigned char*>(rom.data()+d.legacyEffect))&&
            std::equal(std::begin(sceneEffect),std::end(sceneEffect),reinterpret_cast<const unsigned char*>(rom.data()+d.effect));
    }
    MusicPlayer(const std::vector<char>& rom,unsigned song){
        if(!supported(rom))throw std::runtime_error("Unsupported sound driver revision");
        if(driver(rom).dq){loadDq(rom,song);return;}
        if(driver(rom).color){loadColor(rom,song);return;}
        auto d=driver(rom);
        auto playerRom=rom;playerRom[0x100]=static_cast<char>(0xc3);playerRom[0x101]=0;playerRom[0x102]=static_cast<char>(0xc0);
        // The usual battle/movement SFX driver runs on VBlank; BGM and the
        // legacy effect driver below run on the hardware timer.
        auto vector=[&](unsigned at,unsigned target){playerRom[at]=static_cast<char>(0xcd);playerRom[at+1]=static_cast<char>(target);playerRom[at+2]=static_cast<char>(target>>8);playerRom[at+3]=static_cast<char>(0xd9);};
        vector(0x40,d.update);vector(0x50,d.timer);
        core_=loadCoreBytes(playerRom,nullptr);
        std::vector<unsigned char> code={0xf3,0x31,0xf0,0xdf,0xaf,0xe0,0xff};
        auto add=[&](std::initializer_list<unsigned char> data){code.insert(code.end(),data);};
        auto store=[&](unsigned address){add({0xea,static_cast<unsigned char>(address),static_cast<unsigned char>(address>>8)});};
        auto call=[&](unsigned address){add({0xcd,static_cast<unsigned char>(address),static_cast<unsigned char>(address>>8)});};
        store(d.musicMute);store(d.effectMute);
        // Gaiden 3's VBlank wrapper also checks its play clock/link state.
        // Leave those game tasks disabled in the audio-only core.
        if(d.musicBank)store(0xdd2e);
        add({0x3e,static_cast<unsigned char>(d.bank)});store(0x2000);store(d.savedBank);if(d.musicBank)store(d.musicBank);
        add({0x3e,0xfb,0xe0,0xf4});call(d.init);
        add({0x3e,0xff});store(d.effectState);add({0x3e,1,0xe0,0xff,0xfb});
        unsigned loop=0xc000+code.size();
        add({0xf0,0x44,0xfe,0x90,0x30,0xfa,0xf0,0x44,0xfe,0x90,0x38,0xfa});
        auto mailbox=[&](unsigned index,unsigned target){add({0xfa,static_cast<unsigned char>(index),0xc1,0xfe,0xff,0x28,8});call(target);add({0x3e,0xff,0xea,static_cast<unsigned char>(index),0xc1});};
        mailbox(0,d.music);mailbox(1,d.legacyEffect);mailbox(2,d.effect);
        add({0xc3,static_cast<unsigned char>(loop),static_cast<unsigned char>(loop>>8)});
        std::memcpy(core_->rambank0_ptr(),code.data(),code.size());
        auto ram=static_cast<unsigned char*>(core_->rambank0_ptr());ram[0x100]=song;ram[0x101]=ram[0x102]=0xff;
    }
    void colorCommand(unsigned slot,unsigned operation,unsigned argument){
        if(slot>2)return;
        auto ram=static_cast<unsigned char*>(core_->rambank0_ptr())+0x110;
        ram[1+slot*3]=static_cast<unsigned char>(operation);ram[2+slot*3]=static_cast<unsigned char>(argument);ram[0]|=1u<<slot;
    }
    void command(bool music,unsigned value){if(color_)colorCommand(music?2:0,1,value);else static_cast<unsigned char*>(core_->rambank0_ptr())[music?0x100:0x101]=value;}
    void effect(unsigned value){if(color_)colorCommand(0,1,value);else static_cast<unsigned char*>(core_->rambank0_ptr())[dq_?0x101:0x102]=value;}
    unsigned run(std::array<gambatte::uint_least32_t,40000>& sound){
        if(color_){
            auto ram=static_cast<unsigned char*>(core_->rambank0_ptr());
            if(ram[0x103]){
                for(unsigned slot=0;slot<3;slot++)if(ram[0x110]&(1u<<slot)){
                    ram[0x2001+slot*3]=ram[0x111+slot*3];ram[0x2002+slot*3]=ram[0x112+slot*3];ram[0x2000]|=1u<<slot;
                }
                ram[0x110]=0;
            }
        }
        unsigned samples=35112;core_->runFor(nullptr,160,sound.data(),sound.size(),samples);return samples;
    }
    gambatte::GB& core(){return *core_;}
};
