#pragma once
#include "game_support.h"

class PartySupport {
    struct Profile {GameId id;unsigned check,caller,partyLast,alignment,reject;};
    struct ColorProfile {GameId id;unsigned mask,partyLookup,recordLookup,join;};
    const ColorProfile* color_=nullptr;
    static const ColorProfile* colorProfile(const std::vector<char>& rom){
        static const ColorProfile profiles[]={
            {GameId::Wizardry1,0x55d3,0x276b,0x2776,0x5871},
            {GameId::Wizardry2,0x5a1e,0x2763,0x276e,0x5862},
            {GameId::Wizardry3,0x58aa,0x2731,0x273c,0x5871}
        };
        auto id=identifyGame(rom).id;for(const auto& p:profiles)if(p.id==id)return &p;return nullptr;
    }
    static bool recognizesColor(const std::vector<char>& rom){
        const auto* p=colorProfile(rom);if(!p||rom.size()<0xd941)return false;
        const unsigned char mask[]={0xc5,0xd5,0xe5,0xaf,0x57,0x5f,0xfa,0x09,0xc1,0xba,0x28,0x1c,0x7a,0xcd,static_cast<unsigned char>(p->partyLookup),static_cast<unsigned char>(p->partyLookup>>8),0x01,0x0a,0x00,0x09,0x7e,0xe6,0x03,0x28,0x08,0xfe,0x02,0x20,0x08,0xb3,0x5f,0x18,0x04,0x3e,0x01,0xb3,0x5f,0x14,0x18,0xde,0x7b,0xea,0x00,0xc0,0xe1,0xd1,0xc1,0xc9};
        const unsigned char builder[]={0xfa,0x70,0xff,0xf5,0x3e,0x01,0xea,0x70,0xff,0xc5,0xd5,0xe5,0x3e,0x01,0x21,static_cast<unsigned char>(p->mask),static_cast<unsigned char>(p->mask>>8),0xc7,0xfa,0x00,0xc0,0x47,0x0e,0x00,0x1e,0x00,0x79,0xfe,0x14,0x28,0x61,0xcd,static_cast<unsigned char>(p->recordLookup),static_cast<unsigned char>(p->recordLookup>>8),0xc5,0xe5,0x01,0x00,0x00,0x09,0x7e,0xe1,0xc1,0xb7,0x28,0x4f,0xc5,0xe5,0x01,0x58,0x00,0x09,0x7e,0xe1,0xc1,0xb7,0x20,0x43,0x78,0xb7,0x28,0x25,0xc5,0xe5,0x01,0x0a,0x00,0x09,0x7e,0xe1,0xc1,0xe6,0x03,0x28,0x0f,0xfe,0x02,0x20,0x14,0x78,0xfe,0x03,0x28,0x29,0xe6,0x02,0x28,0x25,0x18,0x09,0x78,0xfe,0x03,0x28,0x1e,0xe6,0x01,0x28,0x1a,0xfa,0x09,0xc1,0x57,0x21,0x03,0xc1,0x7a,0xb7,0x28,0x07,0x2a,0xb9,0x28,0x0b,0x15,0x18,0xf5,0x16,0x00,0x21,0xa9,0xd3,0x19,0x71,0x1c,0x0c,0x18,0x9a,0x7b,0xea,0xa8,0xd3,0xe1,0xd1,0xc1,0xf1,0xea,0x70,0xff,0xc9};
        auto matches=[&](size_t at,const auto& bytes){return std::equal(std::begin(bytes),std::end(bytes),reinterpret_cast<const unsigned char*>(rom.data()+at));};
        const unsigned char join[]={0xfa,0x09,0xc1,0x4f,0xaf,0x47,0x21,0x03,0xc1,0x09,0xfa,0xae,0xff,0x77,0xfa,0x09,0xc1,0x3c,0xea,0x09,0xc1};
        return matches(p->mask,mask)&&matches(0xd8b5,builder)&&matches(p->join+0x4000,join);
    }
    gambatte::GB* core_=nullptr;
    bool supported_=false,enabled_=false;
    const Profile* profile_=nullptr;
    static const Profile* profile(const std::vector<char>& rom){
        static const Profile profiles[]={
            {GameId::Gaiden1,0x12e7,0xaa75,0xc955,0xd10e,0x69df},
            {GameId::Gaiden2,0x1144,0xa673,0xcd2e,0xd01d,0x65dd},
            {GameId::Gaiden3,0x0ff0,0xa969,0xc9b4,0xcb29,0x68d3}
        };
        auto id=identifyGame(rom).id;for(const auto& p:profiles)if(p.id==id)return &p;return nullptr;
    }
    static std::string patch(unsigned address,unsigned value,unsigned expected){
        const char* hex="0123456789ABCDEF";std::string code="000-000-000";
        code[0]=hex[value>>4];code[1]=hex[value&15];code[2]=hex[(address>>8)&15];
        code[4]=hex[(address>>4)&15];code[5]=hex[address&15];code[6]=hex[((address>>12)^15)&15];
        unsigned compare=expected^0x45;compare=(((compare<<2)|(compare>>6))^255)&255;
        code[8]=hex[compare>>4];code[10]=hex[compare&15];return code;
    }
public:
    static bool recognizes(const std::vector<char>& rom){
        if(recognizesColor(rom))return true;
        const auto* p=profile(rom);
        if(!p||rom.size()<p->caller+16||rom.size()>0x80000||static_cast<unsigned char>(rom[0x147])!=3)return false;
        const unsigned char check[]={0xb7,0x3d,0xc8,0x3c,0x4f,0x21,static_cast<unsigned char>(p->partyLast),static_cast<unsigned char>(p->partyLast>>8),6,6,0x16,0,0x3a,7,7,0xe6,3,0xfe,3,0x28,9,0xfe,1,0x28,5,0xb9,0x28,2,0x37,0xc9,5,0x20,0xeb,0xb7,0xc9};
        const unsigned char caller[]={0xfa,static_cast<unsigned char>(p->alignment),static_cast<unsigned char>(p->alignment>>8),0x5f,0xcb,0x0b,0xcb,0x0b,0xe5,0xcd,static_cast<unsigned char>(p->check),static_cast<unsigned char>(p->check>>8),0xe1,0xda,static_cast<unsigned char>(p->reject),static_cast<unsigned char>(p->reject>>8)};
        auto matches=[&](size_t at,const auto& bytes){return std::equal(std::begin(bytes),std::end(bytes),reinterpret_cast<const unsigned char*>(rom.data()+at));};
        return matches(p->check,check)&&matches(p->caller,caller);
    }
    void detach(){if(core_&&enabled_&&!color_)core_->clearCheats();core_=nullptr;profile_=nullptr;color_=nullptr;supported_=enabled_=false;}
    void attach(gambatte::GB& core,const std::vector<char>& rom){detach();core_=&core;supported_=recognizes(rom);if(supported_){profile_=profile(rom);color_=colorProfile(rom);}}
    bool supported()const{return supported_;}
    bool enabled()const{return enabled_;}
    bool write(gambatte::GB& core,unsigned pc,unsigned sp,unsigned address,unsigned value){
        if(color_&&!enabled_&&(pc==color_->join+14||pc==color_->join+21)){
            auto ram=static_cast<unsigned char*>(core.rambank0_ptr());unsigned count=ram[0x109];
            // An already open picker can retain candidates built while ON.
            // Reject both native append writes if OFF now disallows the entry.
            // Derive the decision from game data on each write, so loading a
            // state between these instructions needs no extra host state.
            bool append=pc==color_->join+14&&address==0xc103+count;
            bool increment=pc==color_->join+21&&address==0xc109&&value==count+1;
            if(ram[0x49]==2&&count<6&&(append||increment)){
                auto io=static_cast<unsigned char*>(core.zeropage_ptr());unsigned candidate=io[0x2e];
                if(candidate<20){
                    unsigned alignment=ram[0x114+candidate*110]&3,mask=0;
                    for(unsigned i=0;i<count;i++){
                        unsigned slot=ram[0x103+i];if(slot>=20)return true;
                        unsigned a=ram[0x114+slot*110]&3;if(a==0)mask|=1;else if(a==2)mask|=2;
                    }
                    if((alignment==0&&(mask&2))||(alignment==2&&(mask&1)))return false;
                }
            }
        }
        if(!enabled_||!color_||address!=0xc000||pc!=color_->mask+44||sp<0xc000||sp>0xcff4)return true;
        auto ram=static_cast<unsigned char*>(core.rambank0_ptr());
        // Only the bank 3 tavern candidate builder's far call may ignore the
        // alignment mask. Other users of the same bank 1 routine retain it.
        // Six saved register bytes precede RST's return, saved AF and caller.
        auto word=[&](unsigned at){return unsigned(ram[at-0xc000])|(unsigned(ram[at+1-0xc000])<<8);};
        if(ram[0x49]!=1||word(sp+6)!=0x000f||ram[sp+9-0xc000]!=3||word(sp+10)!=0x58c7)return true;
        ram[0]=0;return false;
    }
    bool enable(bool value){
        if(value&&(!supported_||!core_))return false;
        if(value==enabled_)return true;
        if(color_){enabled_=value;return true;}
        // Existing Gambatte Game Genie support provides an undo list. These
        // compare-qualified codes change only the profile's B7 -> AF, 3D -> C9,
        // making the alignment-only check return carry clear. ROM files and
        // character/party data are untouched. This app uses no other cheats.
        if(value)core_->setGameGenie(patch(profile_->check,0xaf,0xb7)+";"+patch(profile_->check+1,0xc9,0x3d));
        else core_->clearCheats();
        enabled_=value;return true;
    }
};
