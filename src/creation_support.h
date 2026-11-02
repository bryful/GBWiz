#pragma once
#include "game_support.h"

class CreationSupport {
    struct Profile {GameId id;unsigned roll,random,display,formatter,store,bank,bankValue,bonus,scratch;};
    struct ColorProfile {GameId id;unsigned generator,consumer,cap,effect;};
    const ColorProfile* color_=nullptr;
    static const ColorProfile* colorProfile(const std::vector<char>& rom){
        static const ColorProfile profiles[]={
            {GameId::Wizardry1,0x19b0b,0x19324,0x194a0,0x377e},
            {GameId::Wizardry2,0x19b9c,0x19376,0x194f2,0x3779},
            {GameId::Wizardry3,0x19b02,0x1931b,0x19497,0x3744}
        };
        auto id=identifyGame(rom).id;for(const auto& p:profiles)if(p.id==id)return &p;return nullptr;
    }
    static bool recognizesColor(const std::vector<char>& rom){
        const auto* p=colorProfile(rom);if(!p||rom.size()<p->generator+43)return false;
        const unsigned char generator[]={0xea,0x00,0xc0,0x3e,0x05,0xcd,0x91,0x03,0xc6,0x05,0xea,0x01,0xc0,0x3e,0x14,0xcd,0x91,0x03,0xfe,0x0f,0x20,0x11,0xfa,0x01,0xc0,0xc6,0x0a,0xea,0x01,0xc0,0xfe,0x3c,0x38,0xeb,0x3e,0x3c,0xea,0x01,0xc0,0xfa,0x01,0xc0,0xc9};
        const unsigned char consumer[]={0xfa,0xac,0xc9,0xcd,static_cast<unsigned char>(p->generator),static_cast<unsigned char>(((p->generator&0x3fff)+0x4000)>>8),0xea,0x0c,0xc0,0x3e,0x8d,0xea,0xac,0xff,0x3e,0x03,0x21,0xdb,0x42,0xc7};
        const unsigned char cap[]={0xfa,0x0c,0xc0,0xb7,0xca,static_cast<unsigned char>(p->cap+49),static_cast<unsigned char>(((p->cap&0x3fff)+0x4000+49)>>8),0xfa,0xad,0xff,0x4f,0x06,0x00,0x21,0x06,0xc0,0x09,0x7e,0xc6,0x0a,0x57,0x21,0xad,0xc9,0x09,0x7e,0xba,0xca,static_cast<unsigned char>(p->cap+49),static_cast<unsigned char>(((p->cap&0x3fff)+0x4000+49)>>8),0x3c,0x77,0xfa,0x0c,0xc0,0x3d,0xea,0x0c,0xc0,0xf5,0x3e,0x01,0xcd,static_cast<unsigned char>(p->effect),static_cast<unsigned char>(p->effect>>8),0xf1,0xc3,static_cast<unsigned char>(p->consumer+14),static_cast<unsigned char>(((p->consumer&0x3fff)+0x4000+14)>>8)};
        auto matches=[&](size_t at,const auto& bytes){return std::equal(std::begin(bytes),std::end(bytes),reinterpret_cast<const unsigned char*>(rom.data()+at));};
        return matches(p->generator,generator)&&matches(p->consumer,consumer)&&matches(p->cap,cap);
    }
    const Profile* profile_=nullptr;
    bool maximum_=false;
    unsigned rolls_=0;
    static const Profile* profile(const std::vector<char>& rom){
        static const Profile profiles[]={
            {GameId::Gaiden1,0x16e1b,0x2333,0x16fd3,0x01ef,0x0308,0x10ae,5,0xd0fa,0xd0c0},
            {GameId::Gaiden2,0x20d57,0x232e,0x20f06,0x0202,0x031b,0x0fbd,8,0xd009,0xcfcf},
            {GameId::Gaiden3,0x211ca,0x217e,0x21402,0x0185,0x029e,0x0ac7,8,0xcb14,0xcad9}
        };
        auto id=identifyGame(rom).id;for(const auto& p:profiles)if(p.id==id)return &p;return nullptr;
    }
public:
    static constexpr unsigned maximumBonus=30; // (random % 4) + 7 + 10 + 10.
    static bool recognizes(const std::vector<char>& rom){
        if(recognizesColor(rom))return true;
        const auto* p=profile(rom);if(!p||rom.size()<p->display+5)return false;
        auto lo=[](unsigned n){return static_cast<unsigned char>(n);};
        auto hi=[](unsigned n){return static_cast<unsigned char>(n>>8);};
        const unsigned char roll[]={0x3e,4,0xcd,lo(p->random),hi(p->random),0xc6,7,0x47,0x3e,0x14,0xcd,lo(p->random),hi(p->random),0xfe,0x0a,0x20,4,0x3e,0x0a,0x80,0x47,0x3e,0x14,0xcd,lo(p->random),hi(p->random),0xfe,0x0a,0x20,4,0x3e,0x0a,0x80,0x47,0x78,0xea,lo(p->bonus),hi(p->bonus),0xcd,lo(p->display),hi((p->display&0x3fff)+0x4000)};
        const unsigned char display[]={0x5f,0x16,0,0xcd,lo(p->formatter),hi(p->formatter)};
        const unsigned char store[]={0x7b,0x22,0x7a,0x22,0x79,0x22,0x78,0x77};
        const unsigned char formatter[]={0x01,0,0,0x21,lo(p->scratch),hi(p->scratch),0xcd,lo(p->store),hi(p->store)};
        auto matches=[&](size_t at,const auto& bytes){return at+sizeof(bytes)<=rom.size()&&std::equal(std::begin(bytes),std::end(bytes),reinterpret_cast<const unsigned char*>(rom.data()+at));};
        return matches(p->roll,roll)&&matches(p->display,display)&&matches(p->store,store)&&matches(p->formatter,formatter);
    }
    void attach(const std::vector<char>& rom){bool known=recognizes(rom);profile_=known?profile(rom):nullptr;color_=known?colorProfile(rom):nullptr;maximum_=false;rolls_=0;}
    bool supported()const{return profile_!=nullptr||color_!=nullptr;}
    bool enabled()const{return maximum_;}
    unsigned rolls()const{return rolls_;}
    bool enable(bool value){if(value&&!supported())return false;maximum_=value;return true;}
    bool write(gambatte::GB& core,unsigned pc,unsigned sp,unsigned address,unsigned){
        if(color_){
            // Scale only the consumer's one final store, after the complete
            // native random roll. Native display and allocation read C00C.
            if(core.isCgb()&&pc==(color_->consumer&0x3fff)+0x4000+9&&address==0xc00c){
                auto ram=static_cast<unsigned char*>(core.rambank0_ptr());
                auto io=static_cast<unsigned char*>(core.zeropage_ptr());
                if(ram[0x49]==6&&(io[-0x10]&7)==1){++rolls_;if(maximum_){ram[0x0c]=60;return false;}}
            }
            return true;
        }
        if(!profile_)return true;const auto& p=*profile_;
        auto ram=static_cast<unsigned char*>(core.rambank0_ptr());
        if(ram[p.bank]!=p.bankValue)return true;
        if(pc==(p.roll&0x3fff)+0x4000+38&&address==p.bonus){
            ++rolls_;
            if(maximum_){ram[p.bonus-0xc000]=maximumBonus;return false;}
        }
        // The initial formatter still receives the original roll in A. Correct
        // its scratch too; later redraws use the remaining (spent/refunded) RAM.
        if(maximum_&&pc==p.store+8&&address==p.scratch+3&&sp>=0xc000&&sp<=0xdffc){
            auto stack=ram+sp-0xc000;
            auto word=[](const unsigned char* b){return unsigned(b[0])|(unsigned(b[1])<<8);};
            if(word(stack)==p.formatter+9&&word(stack+2)==(p.display&0x3fff)+0x4000+6){
                ram[p.scratch-0xc000]=ram[p.bonus-0xc000];
                ram[p.scratch-0xc000+1]=ram[p.scratch-0xc000+2]=0;
            }
        }
        return true;
    }
};
