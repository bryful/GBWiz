#pragma once
#include "game_support.h"

// A single global setting; suppress only validated spell MP decrements.
class MpSupport {
    struct Profile {
        GameId id;unsigned firstMp,bankAddress,level,field,menu,load,save,scratch,menuFlag,random,battleScratch,counts;
    };
    const Profile* profile_=nullptr;
    struct ColorProfile {GameId id;unsigned lookup,party;std::array<unsigned,4> sites;};
    const ColorProfile* color_=nullptr;
    static const ColorProfile* colorProfile(const std::vector<char>& rom){
        static const ColorProfile profiles[]={
            {GameId::Wizardry1,0x25d4,0x276b,{0x105ef,0x1283e,0x1a2df,0x1cf33}},
            {GameId::Wizardry2,0x25cc,0x2763,{0x105f7,0x128ff,0x1a37a,0x1cf42}},
            {GameId::Wizardry3,0x259a,0x2731,{0x105ef,0x12833,0x1a2ec,0x1cf33}}
        };
        for(const auto& p:profiles)if(p.id==identifyGame(rom).id)return &p;return nullptr;
    }
    static bool recognizesColor(const std::vector<char>& rom){
        const auto* p=colorProfile(rom);if(!p||rom.size()<0x20000)return false;
        auto match=[&](unsigned at,std::initializer_list<unsigned char> bytes){return std::equal(bytes.begin(),bytes.end(),reinterpret_cast<const unsigned char*>(rom.data()+at));};
        auto lo=[](unsigned x){return static_cast<unsigned char>(x);};auto hi=[](unsigned x){return static_cast<unsigned char>(x>>8);};
        if(!match(p->lookup,{0xd5,0xfa,0xb0,0xff,0xcd,lo(p->party),hi(p->party),0x11,0x26,0,0x19,0xfa,0xba,0xc0,0x5f,0x16,0,0x19,0x7e,0xd1,0xc9})
            ||!match(p->party,{0xc5,0x4f,6,0,0x21,3,0xc1,9,0x7e,0x18,1,0xc5,0x6f,0x26,0x6e,1,0x0a,0xc1,0xcd,0x88,0x0d}))return false;
        for(unsigned i:{0u,1u,3u})if(!match(p->sites[i]-3,{0xcd,lo(p->lookup),hi(p->lookup),0xd6,0x10,0x77}))return false;
        return match(p->sites[2]-11,{0x21,0x2e,0,9,0x7e,0xe6,0xf0,0xca,lo(p->id==GameId::Wizardry1?0x6216:p->id==GameId::Wizardry2?0x62b1:0x6223),0x62,0x7e,0xd6,0x10,0x77});
    }
    bool third_=false;
    static bool recognizesThird(const std::vector<char>& rom){
        if(identifyGame(rom).id!=GameId::Gaiden3||rom.size()<0x39ecb)return false;
        auto match=[&](unsigned at,std::initializer_list<unsigned char> bytes){return std::equal(bytes.begin(),bytes.end(),reinterpret_cast<const unsigned char*>(rom.data()+at));};
        return match(0x0df7,{0x11,0x60,0xcb,0xfe,7,0x38,0x0e,0x11,0x67,0xcb,0xd6,7,0xfe,7,0x38,5,0x11,0x8d,0xcb,0xd6,7,0x6f,0x26,0,0x19,0xc9})
            &&match(0x1a656,{0xf5,0xcd,0x21,0x10,0xfa,0x14,0xcb,0xcd,0xf7,0x0d,0x35,0xf1,0xc3,0x5a,0x10})
            &&match(0x39ebc,{0xf5,0xcd,0x21,0x10,0xfa,0x14,0xcb,0xcd,0xf7,0x0d,0x35,0xf1,0xc3,0x5a,0x10})
            &&match(0x904c,{0xfe,0x50,0x38,0x0c,0xfe,0xa0,0x38,0x10,0xfa,0xc0,0xc9,0xb7,0x28,0x12,0x18,0x13,0xfa,0xbe,0xc9,0xb7,0x28,0x0a,0x18,0x0b,0xfa,0xbf,0xc9,0xb7,0x28,2,0x18,3,0xf1,0xb7,0xc9,0x35,1,6,0})
            &&match(0x14e6d,{0x21,0xa1,0xca,0x19,0x7e,0xb7,0x20,6,0xf0,0x8e,0xcd,0xa7,0x4e,0x35,0xf1,0xcd,0x5a,0x10})
            &&match(0x14ea7,{0x3d,0x21,0xf4,0x24,0x11,0x60,0xcb,0xfe,0x50,0x38,0x14,0xd6,0x50,0x21,0xfb,0x24,0x11,0x67,0xcb,0xfe,0x50,0x38,8,0xd6,0x50,0x21,2,0x25,0x11,0x8d,0xcb,0x0e,0,0x28,7,0x96,0x23,0x38,3,0x0c,0x18,0xf9,0x69,0x26,0,0x19,0xc9})
            &&match(0x1025,{0xcd,0xe0,0x17,0x11,0x1f,0xcb,1,0x80,0,0xc3,0xc7,0x1f})
            &&match(0x105a,{0xf5,0xcd,0xe0,0x17,0x54,0x5d,0x21,0x1f,0xcb,1,0x80,0,0xcd,0xc7,0x1f});
    }
    bool supported_=false,enabled_=false;
    static const Profile* profile(const std::vector<char>& rom){
        static const Profile profiles[]={
            {GameId::Gaiden1,0xd145,0xd0ae,0x10c4,0xee7a,0x9373,0x1318,0x1346,0xd0fa,0xc960,0x2333,0xd092,0x2644},
            {GameId::Gaiden2,0xd054,0xcfbd,0x0f20,0xf09d,0x8f7c,0x1175,0x11a3,0xd009,0xcd39,0x232e,0xcf9f,0x26ae}
        };
        auto id=identifyGame(rom).id;for(const auto& p:profiles)if(p.id==id)return &p;return nullptr;
    }
public:
    static bool recognizes(const std::vector<char>& rom){
        if(identifyGame(rom).id==GameId::Gaiden3)return recognizesThird(rom);
        if(colorProfile(rom))return recognizesColor(rom);
        const auto* p=profile(rom);if(!p||rom.size()<0x14d90)return false;
        auto lo=[](unsigned n){return static_cast<unsigned char>(n);};auto hi=[](unsigned n){return static_cast<unsigned char>(n>>8);};
        const unsigned char level[]={0x11,lo(p->firstMp),hi(p->firstMp),0xfe,7,0x38,5,0x11,lo(p->firstMp+7),hi(p->firstMp+7),0xd6,7,0x6f,0x26,0,0x19,0xc9};
        const unsigned char field[]={0xf5,0xcd,lo(p->load),hi(p->load),0xfa,lo(p->scratch),hi(p->scratch),0xcd,lo(p->level),hi(p->level),0x35,0xf1,0xc3,lo(p->save),hi(p->save)};
        const unsigned char menu[]={0xfa,lo(p->menuFlag),hi(p->menuFlag),0xb7,0x28,9,0x3e,0x64,0xcd,lo(p->random),hi(p->random),0xfe,0x32,0x30,3,0xf1,0xb7,0xc9,0x35,1,6,0};
        const unsigned char battle[]={0x21,lo(p->battleScratch),hi(p->battleScratch),0x19,0x7e,0xb7,0x20,6,0xf0,0x8e,0xcd,0x6d,0x4d,0x35,0xf1,0xcd,lo(p->save),hi(p->save)};
        const unsigned char spellLevel[]={0x3d,0x21,lo(p->counts),hi(p->counts),0x11,lo(p->firstMp),hi(p->firstMp),0xcb,0x7f,0x28,6,0x21,lo(p->counts+7),hi(p->counts+7),0x11,lo(p->firstMp+7),hi(p->firstMp+7),0x0e,0,0xe6,0x7f,0x28,7,0x96,0x23,0x38,3,0x0c,0x18,0xf9,0x69,0x26,0,0x19,0xc9};
        auto matches=[&](size_t at,const auto& bytes){return std::equal(std::begin(bytes),std::end(bytes),reinterpret_cast<const unsigned char*>(rom.data()+at));};
        return matches(p->level,level)&&matches(p->field,field)&&matches(p->menu,menu)&&matches(0x14d33,battle)&&matches(0x14d6d,spellLevel);
    }
    void attach(const std::vector<char>& rom){supported_=recognizes(rom);profile_=supported_?profile(rom):nullptr;color_=supported_?colorProfile(rom):nullptr;third_=supported_&&identifyGame(rom).id==GameId::Gaiden3;enabled_=false;}
    bool supported()const{return supported_;}
    bool enabled()const{return enabled_;}
    bool enable(bool value){if(value&&!supported_)return false;enabled_=value;return true;}
    bool write(gambatte::GB& core,unsigned pc,unsigned address,unsigned value)const{
        if(enabled_&&third_){
            if(!((address>=0xcb60&&address<0xcb6e)||(address>=0xcb8d&&address<0xcb94)))return true;
            auto ram=static_cast<const unsigned char*>(core.rambank0_ptr());unsigned bank=ram[0xac7];
            if(!((bank==2&&pc==0x5070)||(bank==5&&pc==0x4e7b)||(bank==6&&pc==0x6661)||(bank==14&&pc==0x5ec7)))return true;
            return value!=static_cast<unsigned char>(ram[address-0xc000]-1);
        }
        if(enabled_&&color_){
            bool mp=address>=0xc10a&&address<0xc10a+20*110&&(address-0xc10a)%110>=0x26&&(address-0xc10a)%110<0x34;
            if(!mp)return true;
            auto ram=static_cast<const unsigned char*>(core.rambank0_ptr());
            for(unsigned site:color_->sites)if(ram[0x49]==site/0x4000&&pc==(site&0x3fff)+0x4003)
                return (ram[address-0xc000]>>4)==0||value!=ram[address-0xc000]-0x10;
            return true;
        }
        if(!enabled_||!profile_||address<profile_->firstMp||address>=profile_->firstMp+14)return true;
        auto ram=static_cast<const unsigned char*>(core.rambank0_ptr());unsigned bank=ram[profile_->bankAddress-0xc000];
        unsigned fieldPc=(profile_->field&0x3fff)+0x4000+11,menuPc=(profile_->menu&0x3fff)+0x4000+19;
        if(!((bank==2&&pc==menuPc)||(bank==3&&pc==fieldPc)||(bank==5&&pc==0x4d41)))return true;
        return value!=static_cast<unsigned char>(ram[address-0xc000]-1);
    }
};
